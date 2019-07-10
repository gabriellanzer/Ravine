#include "Ravine.h"

//STD Includes
#include <stdexcept>

//EASTL Includes
#include <eastl/set.h>
#include <eastl/string.h>
#include <eastl/sort.h>

//GLM Includes
#include "glm/gtc/matrix_transform.hpp"

//STB Includes
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

//Ravine Systems Includes
#include "RvTime.h"
#include "RvConfig.h"
#include "RvDebug.h"

//Types dependencies
#include "RvUniformTypes.h"

//ShaderC Includes
#include "shaderc/shaderc.h"

//GLM includes
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

//FMT Includes
#include <fmt/printf.h>

//Assimp Includes
#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/postprocess.h>     // Post processing flags

//Specific usages of Assimp library
using Assimp::Importer;

Ravine::Ravine()
{
}

Ravine::~Ravine()
= default;

void Ravine::run()
{
	RvTime::initialize();
	initWindow();
	initVulkan();
	mainLoop();
	cleanup();
}

#pragma region Static Methods

//Static method because GLFW doesn't know how to call a member function with the "this" pointer to our Ravine instance.
void Ravine::framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
	auto rvWindowTemp = reinterpret_cast<RvWindow*>(glfwGetWindowUserPointer(window));
	rvWindowTemp->framebufferResized = true;
	rvWindowTemp->extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
}

vector<const char*> Ravine::getRequiredInstanceExtensions()
{
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

#ifdef VALIDATION_LAYERS_ENABLED
	extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
#endif

	return extensions;
}

#pragma endregion

void Ravine::initWindow() {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	window = new RvWindow(WIDTH, HEIGHT, WINDOW_NAME, false, framebufferResizeCallback);

	stbi_set_flip_vertically_on_load(true);
}

void Ravine::initVulkan() {
	//Core setup
	createInstance();
	rvDebug::setupDebugCallback(instance);
	window->CreateSurface(instance);
	pickPhysicalDevice();

	//Load Scene
	string modelName = "cacimba.fbx";
	if (loadScene("../data/" + modelName))
	{
		fmt::print(stdout, "{0} loaded!\n", modelName.c_str());
	}
	else
	{
		fmt::print(stdout, "File not fount at path: {0}\n", modelName.c_str());
		return;
	}

	//Rendering pipeline
	swapChain = new RvSwapChain(*device, window->surface, window->extent.width, window->extent.height, NULL);
	swapChain->createImageViews();
	swapChain->createRenderPass();
	createDescriptorSetLayout();

	//Shaders Loading
	skinnedTexColCode = rvTools::readFile("../data/shaders/skinned_tex_color.vert");
	skinnedWireframeCode = rvTools::readFile("../data/shaders/skinned_wireframe.vert");
	staticTexColCode = rvTools::readFile("../data/shaders/static_tex_color.vert");
	staticWireframeCode = rvTools::readFile("../data/shaders/static_wireframe.vert");
	phongTexColCode = rvTools::readFile("../data/shaders/phong_tex_color.frag");
	solidColorCode = rvTools::readFile("../data/shaders/solid_color.frag");

	VkDescriptorSetLayout* descriptorSetLayouts = new VkDescriptorSetLayout[3]
	{
		globalDescriptorSetLayout,
		materialDescriptorSetLayout,
		modelDescriptorSetLayout
	};
	skinnedGraphicsPipeline = new RvPolygonPipeline(*device, swapChain->extent, device->getMaxUsableSampleCount(),
		descriptorSetLayouts, 3, swapChain->renderPass, skinnedTexColCode, phongTexColCode);
	skinnedWireframeGraphicsPipeline = new RvWireframePipeline(*device, swapChain->extent, device->getMaxUsableSampleCount(),
		descriptorSetLayouts, 3, swapChain->renderPass, skinnedWireframeCode, solidColorCode);
	staticGraphicsPipeline = new RvPolygonPipeline(*device, swapChain->extent, device->getMaxUsableSampleCount(),
		descriptorSetLayouts, 3, swapChain->renderPass, staticTexColCode, phongTexColCode);
	staticWireframeGraphicsPipeline = new RvWireframePipeline(*device, swapChain->extent, device->getMaxUsableSampleCount(),
		descriptorSetLayouts, 3, swapChain->renderPass, staticWireframeCode, solidColorCode);
	staticLineGraphicsPipeline = new RvLinePipeline(*device, swapChain->extent, device->getMaxUsableSampleCount(),
		descriptorSetLayouts, 3, swapChain->renderPass, staticWireframeCode, solidColorCode);

	createMultiSamplingResources();
	createDepthResources();
	swapChain->createFramebuffers();
	loadTextureImages();
	createTextureSampler();
	createIndexBuffer();
	createVertexBuffer();
	createUniformBuffers();
	createDescriptorPool();
	createDescriptorSets();
	allocateCommandBuffers();
	swapChain->createSyncObjects();

	//GUI Management
	gui = new RvGui(*device, *swapChain, *window);
	gui->init(device->getMaxUsableSampleCount());
}

void Ravine::createInstance() {

	//Initialize Volk
	if (volkInitialize() != VK_SUCCESS) {
		throw std::runtime_error("Failed to initialize Volk!\
			Ensure your driver is up to date and supports Vulkan!");
	}

	//Check validation layer support
#ifdef VALIDATION_LAYERS_ENABLED
	if (!rvCfg::checkValidationLayerSupport()) {
		throw std::runtime_error("Validation layers requested, but not available!");
	}
#endif

	//Application related info
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Ravine Engine";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "Ravine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	//Information for VkInstance creation
	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	//Query for available extensions
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr); //Query size
	vector<VkExtensionProperties> extensions(extensionCount); //Reserve
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data()); //Query data
	fmt::print(stdout, "Vulkan available extensions:\n");
	for (const VkExtensionProperties& extension : extensions) {
		fmt::print(stdout, "\t{0}\n", extension.extensionName);
	}

	//GLFW Window Management extensions
	vector<const char*> requiredExtensions = getRequiredInstanceExtensions();
	fmt::print(stdout, "Application required extensions:\n");
	for (const char*& requiredExtension : requiredExtensions)
	{
		bool found = false;
		for (VkExtensionProperties& extension : extensions)
		{
			if (strcmp(requiredExtension, static_cast<const char*>(extension.extensionName)) == 0) {
				found = true;
				break;
			}
		}
		fmt::print(stdout, "\t{0} {1}\n", requiredExtension, (found ? "found!" : "NOT found!"));
	}
	createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
	createInfo.ppEnabledExtensionNames = requiredExtensions.data();

	//Add validation layer info
#ifdef VALIDATION_LAYERS_ENABLED
	createInfo.enabledLayerCount = static_cast<uint32_t>(rvCfg::VALIDATION_LAYERS.size());
	createInfo.ppEnabledLayerNames = rvCfg::VALIDATION_LAYERS.data();
	fmt::print(stdout, "!Enabling validation layers!\n");
#else
	createInfo.enabledLayerCount = 0;
#endif

	//Ask for an instance
	if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create instance!");
	}

	//Initialize Entry-points on Volk
	volkLoadInstance(instance);
}

void Ravine::pickPhysicalDevice()
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	if (deviceCount == 0) {
		throw std::runtime_error("Failed to find GPUs with Vulkan support!");
	}

	vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	for (const auto& curPhysicalDevice : devices) {
		if (isDeviceSuitable(curPhysicalDevice)) {
			device = new RvDevice(curPhysicalDevice, window->surface);
			break;
		}
	}

	if (device->physicalDevice == VK_NULL_HANDLE) {
		throw std::runtime_error("failed to find a suitable GPU!");
	}
}

bool Ravine::isDeviceSuitable(const VkPhysicalDevice device) {

	//Debug Physical Device Properties
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);
	fmt::print(stdout, "Checking device: {0}\n", deviceProperties.deviceName);

	rvTools::QueueFamilyIndices indices = rvTools::findQueueFamilies(device, window->surface);

	bool extensionsSupported = checkDeviceExtensionSupport(device);

	bool swapChainAdequate = false;
	if (extensionsSupported) {
		RvSwapChainSupportDetails swapChainSupport = rvTools::querySupport(device, window->surface);
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	VkPhysicalDeviceFeatures supportedFeatures;
	vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

	bool isSuitable = indices.isComplete() && extensionsSupported && swapChainAdequate &&
		supportedFeatures.samplerAnisotropy; //Checking for anisotropy support
	if (isSuitable)
	{
		fmt::print(stdout, "{0} is suitable and was selected!\n", deviceProperties.deviceName);
	}

	return isSuitable;
}

bool Ravine::checkDeviceExtensionSupport(const VkPhysicalDevice device) {
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	eastl::set<eastl::string> requiredExtensions(rvCfg::DEVICE_EXTENSIONS.begin(), rvCfg::DEVICE_EXTENSIONS.end());

	for (const VkExtensionProperties& extension : availableExtensions) {
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

void Ravine::recreateSwapChain() {

	int width = 0, height = 0;
	//If the window is minimized, wait for it to come back to the foreground.
	//TODO: We probably want to handle that another way, which we should probably discuss.
	while (width == 0 || height == 0) {
		width = window->extent.width;
		height = window->extent.height;
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(device->handle);

	//Storing handle
	RvSwapChain* oldSwapchain = swapChain;
	swapChain = new RvSwapChain(*device, window->surface, WIDTH, HEIGHT, oldSwapchain->handle);
	swapChain->createSyncObjects();
	swapChain->createImageViews();
	swapChain->createRenderPass();

	VkDescriptorSetLayout* descriptorSetLayouts = new VkDescriptorSetLayout[3]
	{
		globalDescriptorSetLayout,
		materialDescriptorSetLayout,
		modelDescriptorSetLayout
	};

	createMultiSamplingResources();
	createDepthResources();
	swapChain->createFramebuffers();
	allocateCommandBuffers();
	RvGui* oldGui = gui;
	gui = new RvGui(*device, *swapChain, *window);
	gui->init(device->getMaxUsableSampleCount());
	delete oldGui;
	//Deleting old swapchain
	oldSwapchain->clear();
	delete oldSwapchain;
}

void Ravine::createDescriptorPool()
{
	array<VkDescriptorPoolSize, 5> poolSizes = {};
	//Global Uniforms
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = static_cast<uint32_t>(swapChain->images.size());

	//TODO: Change descriptor count accodingly to materials count instead of meshes count
	//Material Uniforms
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[1].descriptorCount = static_cast<uint32_t>(swapChain->images.size() * meshesCount);
	poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[2].descriptorCount = static_cast<uint32_t>(swapChain->images.size() * meshesCount);

	//Model Uniforms
	poolSizes[3].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[3].descriptorCount = static_cast<uint32_t>(swapChain->images.size() * meshesCount);
	poolSizes[4].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[4].descriptorCount = static_cast<uint32_t>(swapChain->images.size() * meshesCount);

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = swapChain->images.size() * (1 + meshesCount * 2);/*Global, Material (per mesh), Model (per mesh)*/

	if (vkCreateDescriptorPool(device->handle, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create descriptor pool!");
	}
}

void Ravine::createDescriptorSets()
{
	//Descriptor Sets Count
	size_t setsPerFrame = (1 + meshesCount * 2)/*Global, Material (per mesh), Model (per mesh)*/;
	size_t framesCount = swapChain->images.size();
	size_t descriptorSetsCount = framesCount * setsPerFrame;
	vector<VkDescriptorSetLayout> layouts(descriptorSetsCount);
	for (size_t i = 0; i < framesCount; i++)
	{
		layouts[i * setsPerFrame + 0] = globalDescriptorSetLayout;
		for (size_t meshId = 0; meshId < meshesCount; meshId++)
		{
			layouts[i * setsPerFrame + meshId * 2 + 1] = materialDescriptorSetLayout;
			layouts[i * setsPerFrame + meshId * 2 + 2] = modelDescriptorSetLayout;
		}
	}
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(descriptorSetsCount);
	allocInfo.pSetLayouts = layouts.data();

	// Setting descriptor sets vector to count of SwapChain images
	descriptorSets.resize(descriptorSetsCount);
	if (vkAllocateDescriptorSets(device->handle, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate descriptor sets!");
	}

	//For each frame
	for (size_t i = 0; i < framesCount; i++)
	{
		VkDescriptorBufferInfo globalUniformsInfo = {};
		globalUniformsInfo.buffer = uniformBuffers[i].handle;
		globalUniformsInfo.offset = 0;
		globalUniformsInfo.range = sizeof(RvUniformBufferObject);

		//Offset per frame iteration
		size_t frameSetOffset = (i * setsPerFrame);
		size_t writesPerFrame = (1 + meshesCount * 4);
		vector<VkWriteDescriptorSet> descriptorWrites(writesPerFrame);

		//Global Uniform Buffer Info
		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = descriptorSets[frameSetOffset + 0/*Frame Global Uniform Set*/];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &globalUniformsInfo;

		//For each mesh
		VkDescriptorBufferInfo* materialsInfo = new VkDescriptorBufferInfo[meshesCount]{};
		VkDescriptorImageInfo* imageInfo = new VkDescriptorImageInfo[meshesCount]{};
		VkDescriptorBufferInfo* modelsInfo = new VkDescriptorBufferInfo[meshesCount]{};
		VkDescriptorBufferInfo* animationsInfo = new VkDescriptorBufferInfo[meshesCount]{};
		for (size_t meshId = 0; meshId < meshesCount; meshId++)
		{
			//Offset per mesh iteration
			size_t meshSetOffset = meshId * 2;
			size_t meshWritesOffset = meshId * 4;

			//Materials Uniform Buffer Info
			materialsInfo[meshId] = {};
			materialsInfo[meshId].buffer = materialsBuffers[i].handle;
			materialsInfo[meshId].offset = 0;
			materialsInfo[meshId].range = sizeof(RvMaterialBufferObject);

			descriptorWrites[meshWritesOffset + 1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[meshWritesOffset + 1].dstSet = descriptorSets[frameSetOffset + meshSetOffset + 1];
			descriptorWrites[meshWritesOffset + 1].dstBinding = 0;
			descriptorWrites[meshWritesOffset + 1].dstArrayElement = 0;
			descriptorWrites[meshWritesOffset + 1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[meshWritesOffset + 1].descriptorCount = 1;
			descriptorWrites[meshWritesOffset + 1].pBufferInfo = &materialsInfo[meshId];

			//Image Info
			imageInfo[meshId] = {};
			imageInfo[meshId].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			RvSkinnedMeshColored& mesh = meshes[meshId];
			size_t textureId = mesh.textures_count > 0 ? 1 + mesh.textureIds[0] : 0/*Missing Texture (Pink)*/;
			imageInfo[meshId].imageView = textures[textureId].view;
			imageInfo[meshId].sampler = textureSampler;

			descriptorWrites[meshWritesOffset + 2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[meshWritesOffset + 2].dstSet = descriptorSets[frameSetOffset + meshSetOffset + 1];
			descriptorWrites[meshWritesOffset + 2].dstBinding = 1;
			descriptorWrites[meshWritesOffset + 2].dstArrayElement = 0;
			descriptorWrites[meshWritesOffset + 2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[meshWritesOffset + 2].descriptorCount = 1;
			descriptorWrites[meshWritesOffset + 2].pImageInfo = &imageInfo[meshId];

			//Models Uniform Buffer Info
			modelsInfo[meshId] = {};
			modelsInfo[meshId].buffer = modelsBuffers[i * meshesCount + meshId].handle;
			modelsInfo[meshId].offset = 0;
			modelsInfo[meshId].range = sizeof(RvModelBufferObject);

			descriptorWrites[meshWritesOffset + 3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[meshWritesOffset + 3].dstSet = descriptorSets[frameSetOffset + meshSetOffset + 2];
			descriptorWrites[meshWritesOffset + 3].dstBinding = 0;
			descriptorWrites[meshWritesOffset + 3].dstArrayElement = 0;
			descriptorWrites[meshWritesOffset + 3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[meshWritesOffset + 3].descriptorCount = 1;
			descriptorWrites[meshWritesOffset + 3].pBufferInfo = &modelsInfo[meshId];

			//Animations Uniform Buffer Info
			animationsInfo[meshId] = {};
			animationsInfo[meshId].buffer = animationsBuffers[i * meshesCount + meshId].handle;
			animationsInfo[meshId].offset = 0;
			animationsInfo[meshId].range = sizeof(RvBoneBufferObject);

			descriptorWrites[meshWritesOffset + 4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[meshWritesOffset + 4].dstSet = descriptorSets[frameSetOffset + meshSetOffset + 2];
			descriptorWrites[meshWritesOffset + 4].dstBinding = 1;
			descriptorWrites[meshWritesOffset + 4].dstArrayElement = 0;
			descriptorWrites[meshWritesOffset + 4].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[meshWritesOffset + 4].descriptorCount = 1;
			descriptorWrites[meshWritesOffset + 4].pBufferInfo = &animationsInfo[meshId];
		}

		//Update the sets for this frame
		vkUpdateDescriptorSets(device->handle, writesPerFrame, descriptorWrites.data(), 0, nullptr);

		delete[] materialsInfo;
		delete[] imageInfo;
		delete[] modelsInfo;
		delete[] animationsInfo;
	}

}

void Ravine::createDescriptorSetLayout()
{
	//Global Uniforms layout
	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	//Materials layout
	VkDescriptorSetLayoutBinding materialLayoutBinding = {};
	materialLayoutBinding.binding = 0;
	materialLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	materialLayoutBinding.descriptorCount = 1;
	materialLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	//Texture Sampler layout
	VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	samplerLayoutBinding.pImmutableSamplers = nullptr;

	//Models Data layout
	VkDescriptorSetLayoutBinding modelDataLayoutBinding = {};
	modelDataLayoutBinding.binding = 0;
	modelDataLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	modelDataLayoutBinding.descriptorCount = 1;
	modelDataLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	//Animations layout
	VkDescriptorSetLayoutBinding animationLayoutBinding = {};
	animationLayoutBinding.binding = 1;
	animationLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	animationLayoutBinding.descriptorCount = 1;
	animationLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	//Global Descriptor Set Layout
	{
		//Bindings array
		array<VkDescriptorSetLayoutBinding, 1> bindings = { uboLayoutBinding };

		VkDescriptorSetLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();

		if (vkCreateDescriptorSetLayout(device->handle, &layoutInfo, nullptr, &globalDescriptorSetLayout) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create descriptor set layout!");
		};
	}

	//Material Descriptor Set Layout
	{
		//Bindings array
		array<VkDescriptorSetLayoutBinding, 2> bindings = { materialLayoutBinding, samplerLayoutBinding };

		VkDescriptorSetLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();

		if (vkCreateDescriptorSetLayout(device->handle, &layoutInfo, nullptr, &materialDescriptorSetLayout) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create descriptor set layout!");
		};
	}

	//Model Descriptor Set Layout
	{
		//Bindings array
		array<VkDescriptorSetLayoutBinding, 2> bindings = { modelDataLayoutBinding, animationLayoutBinding };

		VkDescriptorSetLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();

		if (vkCreateDescriptorSetLayout(device->handle, &layoutInfo, nullptr, &modelDescriptorSetLayout) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create descriptor set layout!");
		};
	}
}

#pragma region ANIMATION STUFF

bool Ravine::loadScene(const string& filePath)
{
	Importer importer;
	importer.ReadFile(filePath.c_str(), 
		/*aiProcess_CalcTangentSpace |*/ \
		/*aiProcess_GenNormals |*/ \
		/*aiProcess_JoinIdenticalVertices |*/ \
		aiProcess_ImproveCacheLocality | \
		aiProcess_LimitBoneWeights | \
		aiProcess_RemoveRedundantMaterials | \
		aiProcess_Triangulate | \
		aiProcess_GenUVCoords | \
		aiProcess_SortByPType | \
		aiProcess_FindDegenerates | \
		aiProcess_FindInvalidData | \
		aiProcess_FindInstances | \
		aiProcess_ValidateDataStructure | \
		aiProcess_OptimizeMeshes | \
		aiProcess_OptimizeGraph | \
		0);
	scene = importer.GetOrphanedScene();

	// If the import failed, report it
	if (!scene)
	{
		return false;
	}

	//Load mesh
	meshesCount = scene->mNumMeshes;
	meshes = new RvSkinnedMeshColored[scene->mNumMeshes];
	aiMatrix4x4 animGlobalInverseTransform = scene->mRootNode->mTransformation;
	animGlobalInverseTransform.Inverse();

	//Load each mesh
	for (uint32_t i = 0; i < meshesCount; i++)
	{
		//Hold reference
		const aiMesh* mesh = scene->mMeshes[i];

		meshes[i].animGlobalInverseTransform = animGlobalInverseTransform;

		//Allocate data structures
		meshes[i].vertex_count = mesh->mNumVertices;
		meshes[i].vertices = new RvSkinnedVertexColored[mesh->mNumVertices];
		meshes[i].index_count = mesh->mNumFaces * 3;
		meshes[i].indices = new uint32_t[mesh->mNumFaces * 3];

		//Setup vertices
		const aiVector3D* verts = mesh->mVertices;
		bool hasCoords = mesh->HasTextureCoords(0);
		const aiVector3D* uvs = mesh->mTextureCoords[0];
		bool hasColors = mesh->HasVertexColors(0);
		const aiColor4D* cols = mesh->mColors[0];
		bool hasNormals = mesh->HasNormals();
		const aiVector3D* norms = &mesh->mNormals[0];

		//Treat each case for optimal performance
		if (hasCoords && hasColors && hasNormals)
		{
			for (uint32_t j = 0; j < mesh->mNumVertices; j++)
			{
				//Vertices
				meshes[i].vertices[j].pos = { verts[j].x, verts[j].y, verts[j].z };

				//Texture coordinates
				meshes[i].vertices[j].texCoord = { uvs[j].x, uvs[j].y };

				//Vertex colors
				meshes[i].vertices[j].color = { cols[j].r, cols[j].g, cols[j].b };

				//Normals
				meshes[i].vertices[j].normal = { norms[j].x , norms[j].y, norms[j].z };
			}
		}
		else if (hasCoords && hasNormals)
		{
			for (uint32_t j = 0; j < mesh->mNumVertices; j++)
			{
				//Vertices
				meshes[i].vertices[j].pos = { verts[j].x, verts[j].y, verts[j].z };

				//Texture coordinates
				meshes[i].vertices[j].texCoord = { uvs[j].x, uvs[j].y };

				//Vertex colors
				meshes[i].vertices[j].color = { 1, 1, 1 };

				//Normals
				meshes[i].vertices[j].normal = { norms[j].x , norms[j].y, norms[j].z };
			}
		}
		else if (hasNormals)
		{
			for (uint32_t j = 0; j < mesh->mNumVertices; j++)
			{
				//Vertices
				meshes[i].vertices[j].pos = { verts[j].x, verts[j].y, verts[j].z };

				//Texture coordinates
				meshes[i].vertices[j].texCoord = { 0, 0 };

				//Vertex colors
				meshes[i].vertices[j].color = { 1, 1, 1 };

				//Normals
				meshes[i].vertices[j].normal = { norms[j].x , norms[j].y, norms[j].z };
			}
		}
		else if (hasCoords)
		{
			for (uint32_t j = 0; j < mesh->mNumVertices; j++)
			{
				//Vertices
				meshes[i].vertices[j].pos = { verts[j].x, verts[j].y, verts[j].z };

				//Texture coordinates
				meshes[i].vertices[j].texCoord = { uvs[j].x, uvs[j].y };

				//Vertex colors
				meshes[i].vertices[j].color = { 1, 1, 1 };

				//Normals
				meshes[i].vertices[j].normal = { 0, 0, 0 };
			}
		}
		else
		{
			for (uint32_t j = 0; j < mesh->mNumVertices; j++)
			{
				//Vertices
				meshes[i].vertices[j].pos = { verts[j].x, verts[j].y, verts[j].z };

				//Texture coordinates
				meshes[i].vertices[j].texCoord = { 0, 0 };

				//Vertex colors
				meshes[i].vertices[j].color = { 1, 1, 1 };

				//Normals
				meshes[i].vertices[j].normal = { 0, 0, 0 };
			}
		}

		//Setup face indices
		for (uint32_t j = 0; j < mesh->mNumFaces; j++)
		{
			//Make sure it's triangulated
			assert(mesh->mFaces[j].mNumIndices == 3);

			//Copy each index (the mesh was triangulated on import)
			meshes[i].indices[j * 3 + 0] = mesh->mFaces[j].mIndices[0];
			meshes[i].indices[j * 3 + 1] = mesh->mFaces[j].mIndices[1];
			meshes[i].indices[j * 3 + 2] = mesh->mFaces[j].mIndices[2];
		}

		//Register textures for late-loading (and generate texture Ids)
		uint32_t matId = mesh->mMaterialIndex;
		const aiMaterial* mat = scene->mMaterials[matId];

		//Get the number of textures
		uint32_t textureCounts = mat->GetTextureCount(aiTextureType_DIFFUSE);
		meshes[i].textures_count = textureCounts;
		meshes[i].textureIds = new uint32_t[textureCounts];

		//List each texture on the texturesToLoad list and hold texture ids
		aiString aiTexPath;
		for (uint32_t tId = 0; tId < textureCounts; tId++)
		{
			if (mat->GetTexture(aiTextureType_DIFFUSE, tId, &aiTexPath) == AI_SUCCESS)
			{
				int textureId = 0;
				string texPath = aiTexPath.C_Str();

				//Check if the texture is listed and set it's list id
				bool listed = false;
				for (auto it = texturesToLoad.begin(); it != texturesToLoad.end(); it++)
				{
					if ((it->data()) == texPath)
					{
						listed = true;
						break;
					}

					//Make sure to update textureId
					textureId++;
				}

				//Hold textureId
				meshes[i].textureIds[tId] = textureId;

				//List texture if it isn't already
				if (!listed)
				{
					texturesToLoad.push_back(texPath);
				}
			}
		}

		loadBones(mesh, meshes[i]);
	}

	fmt::print(stdout, "Loaded file with {0} animations.\n", scene->mNumAnimations);

	meshes[0].animations.reserve(scene->mNumAnimations);
	if (scene->mNumAnimations > 0)
	{
		//Record animation parameters
		for (uint32_t i = 0; i < scene->mNumAnimations; i++)
		{
			meshes[0].animations.push_back(new RvAnimation({ scene->mAnimations[i] }));
		}

		//Set current animation
		meshes[0].curAnimId = 0;
	}
	meshes[0].rootNode = new aiNode(*scene->mRootNode);

	//Return success
	return true;
}

void Ravine::loadBones(const aiMesh* pMesh, RvSkinnedMeshColored& meshData)
{
	for (uint16_t i = 0; i < pMesh->mNumBones; i++) {
		uint16_t boneIndex = 0;
		string boneName(pMesh->mBones[i]->mName.data);

		if (meshData.boneMapping.find(boneName) == meshData.boneMapping.end()) {
			boneIndex = meshes[0].numBones;
			meshes[0].numBones++;
			RvBoneInfo bi;
			meshes[0].boneInfo.push_back(bi);
		}
		else {
			boneIndex = meshData.boneMapping[boneName];
		}

		meshData.boneMapping[boneName] = boneIndex;
		meshes[0].boneInfo[boneIndex].BoneOffset = pMesh->mBones[i]->mOffsetMatrix;

		for (uint16_t j = 0; j < pMesh->mBones[i]->mNumWeights; j++) {
			uint16_t vertexId = 0 + pMesh->mBones[i]->mWeights[j].mVertexId;
			float weight = pMesh->mBones[i]->mWeights[j].mWeight;
			meshData.vertices[vertexId].AddBoneData(boneIndex, weight);
		}
	}
}

void Ravine::boneTransform(double timeInSeconds, vector<aiMatrix4x4>& transforms)
{
	const aiMatrix4x4 identity;
	uint16_t otherindex = (meshes[0].curAnimId + 1) % meshes[0].animations.size();
	double animDuration = meshes[0].animations[meshes[0].curAnimId]->aiAnim->mDuration;
	double otherDuration = meshes[0].animations[otherindex]->aiAnim->mDuration;
	runTime += RvTime::deltaTime() * (animDuration / otherDuration * animInterpolation + 1.0 * (1.0 - animInterpolation));
	readNodeHierarchy(runTime, animDuration, otherDuration, meshes[0].rootNode, identity);

	transforms.resize(meshes[0].numBones);

	for (uint16_t i = 0; i < meshes[0].numBones; i++) {
		transforms[i] = meshes[0].boneInfo[i].FinalTransformation;
	}
}

void Ravine::readNodeHierarchy(double animationTime, double curDuration, double otherDuration, const aiNode* pNode,
	const aiMatrix4x4& parentTransform)
{
	string nodeName(pNode->mName.data);

	aiMatrix4x4 nodeTransformation(pNode->mTransformation);

	uint16_t otherIndex = (meshes[0].curAnimId + 1) % meshes[0].animations.size();
	const aiNodeAnim* pNodeAnim = findNodeAnim(meshes[0].animations[meshes[0].curAnimId]->aiAnim, nodeName);
	const aiNodeAnim* otherNodeAnim = findNodeAnim(meshes[0].animations[otherIndex]->aiAnim, nodeName);

	double otherAnimTime = animationTime * curDuration / otherDuration;

	double ticksPerSecond = meshes[0].animations[meshes[0].curAnimId]->aiAnim->mTicksPerSecond;
	double TimeInTicks = animationTime * ticksPerSecond;
	double animationTickTime = std::fmod(TimeInTicks, curDuration);

	ticksPerSecond = meshes[0].animations[otherIndex]->aiAnim->mTicksPerSecond;
	double otherTimeInTicks = otherAnimTime * ticksPerSecond;
	double otherAnimationTime = std::fmod(otherTimeInTicks, otherDuration);

	if (pNodeAnim)
	{
		// Get interpolated matrices between current and next frame
		aiMatrix4x4 matScale = interpolateScale(animInterpolation, animationTickTime, otherAnimationTime, pNodeAnim, otherNodeAnim);
		aiMatrix4x4 matRotation = interpolateRotation(animInterpolation, animationTickTime, otherAnimationTime, pNodeAnim, otherNodeAnim);
		aiMatrix4x4 matTranslation = interpolateTranslation(animInterpolation, animationTickTime, otherAnimationTime, pNodeAnim, otherNodeAnim);

		nodeTransformation = matTranslation * matRotation * matScale;
	}

	aiMatrix4x4 globalTransformation = parentTransform * nodeTransformation;

	if (meshes[0].boneMapping.find(nodeName) != meshes[0].boneMapping.end())
	{
		uint32_t BoneIndex = meshes[0].boneMapping[nodeName];
		meshes[0].boneInfo[BoneIndex].FinalTransformation = meshes[0].animGlobalInverseTransform *
			globalTransformation * meshes[0].boneInfo[BoneIndex].BoneOffset;
	}

	for (uint32_t i = 0; i < pNode->mNumChildren; i++)
	{
		readNodeHierarchy(animationTime, curDuration, otherDuration, pNode->mChildren[i], globalTransformation);
	}
}

#pragma endregion

void Ravine::createVertexBuffer()
{
	/*
	The vertex buffer should use "VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT",
	which is a more optimal memory but it's not accessible by CPU.
	That's why we use a staging buffer to transfer vertex data to the vertex buffer.
	*/
	vertexBuffers.reserve(meshesCount);
	for (size_t i = 0; i < meshesCount; i++)
	{
		vertexBuffers.push_back(device->createPersistentBuffer(
			meshes[i].vertices, sizeof(RvSkinnedVertexColored) * meshes[i].vertex_count, sizeof(RvSkinnedVertexColored),
			static_cast<VkBufferUsageFlagBits>(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT),
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		));

		delete[] meshes[i].vertices;
		meshes[i].index_count = 0;
	}
}

struct EdgeContraction
{
	uint32_t even, odd;
	double cost;

	EdgeContraction() : even(UINT32_MAX), odd(UINT32_MAX), cost(FLT_MAX)
	{

	}

	EdgeContraction(const uint32_t& even, const uint32_t& odd, const double& cost) : even(even), odd(odd), cost(cost)
	{

	}
};

void Ravine::createIndexBuffer()
{
	indexBuffers.reserve(meshesCount);
	oddIndexBuffers.reserve(meshesCount);
	for (size_t i = 0; i < meshesCount; i++)
	{
		size_t linesCount = meshes[i].index_count * static_cast<size_t>(2);

		//Calculate vertex quadratic matrix based on each face
		vector<glm::mat4> vertexQuadric(meshes[i].index_count);
		for (size_t j = 0; j < meshes[i].index_count; j += 3) //For each face (3 ids)
		{
			uint32_t& aId = meshes[i].indices[j + 0];
			uint32_t& bId = meshes[i].indices[j + 1];
			uint32_t& cId = meshes[i].indices[j + 2];

			//Calculate plane coefficients
			glm::vec3& a = meshes[i].vertices[aId].pos;
			glm::vec3& b = meshes[i].vertices[bId].pos;
			glm::vec3& c = meshes[i].vertices[cId].pos;
			glm::vec3 ba = b - a;
			glm::vec3 ca = c - a;
			glm::vec3 nor = glm::normalize(glm::cross(ba, ca));
			glm::vec4 plane = glm::vec4(nor, glm::dot(nor, -a)); //ABCD planar coefficients
			//Calculate error quadric influence of this face on each vertex
			glm::mat4 errorQuadric;
			errorQuadric[0][0] = plane[0] * plane[0]; //A²
			errorQuadric[1][0] = errorQuadric[0][1] = plane[0] * plane[1]; //AB
			errorQuadric[2][0] = errorQuadric[0][2] = plane[0] * plane[2]; //AC
			errorQuadric[3][0] = errorQuadric[0][3] = plane[0] * plane[3]; //AD
			errorQuadric[1][1] = plane[1] * plane[1]; //B²
			errorQuadric[1][2] = errorQuadric[2][1] = plane[1] * plane[2]; //BC
			errorQuadric[1][3] = errorQuadric[3][1] = plane[1] * plane[3]; //BD
			errorQuadric[2][2] = plane[2] * plane[2]; //C²
			errorQuadric[2][3] = errorQuadric[3][2] = plane[2] * plane[3]; //CD
			errorQuadric[3][3] = plane[3] * plane[3]; //D²
			//Accumulate the error quadric for each vertex
			vertexQuadric[aId] += errorQuadric;
			vertexQuadric[bId] += errorQuadric;
			vertexQuadric[cId] += errorQuadric;
		}

		//Calculate contraction cost per edge
		vector<EdgeContraction> contractions(meshes[i].index_count);
		vector<vector<uint32_t>> vNeighbors(meshes[i].vertex_count);
		for (size_t j = 0; j < meshes[i].index_count; j += 3) //For each face (3 ids)
		{
			//Calculate triangle equation
			uint32_t aId = meshes[i].indices[j + 0];
			uint32_t bId = meshes[i].indices[j + 1];
			uint32_t cId = meshes[i].indices[j + 2];

			//Hold Neighbors for later
			vNeighbors[aId].push_back(bId);
			vNeighbors[bId].push_back(aId);
			vNeighbors[bId].push_back(cId);
			vNeighbors[cId].push_back(bId);
			vNeighbors[cId].push_back(aId);
			vNeighbors[aId].push_back(cId);

			//Edges are 'ab', 'bc' and 'ca'
			glm::vec4 a = glm::vec4(meshes[i].vertices[aId].pos, 1);
			glm::vec4 b = glm::vec4(meshes[i].vertices[bId].pos, 1);
			glm::vec4 c = glm::vec4(meshes[i].vertices[cId].pos, 1);

			//Cost for 'ab'
			{
				glm::mat4 combinedError = vertexQuadric[aId] + vertexQuadric[bId];
				double costA = glm::dot(a, combinedError * a);
				double costB = glm::dot(b, combinedError * b);
				if (costA < costB) //'a' is even, 'b' is odd
				{
					contractions[j + 0] = { aId, bId, costA };
				}
				else //'b' is even, 'a' is odd
				{
					contractions[j + 0] = { bId, aId, costB };
				}
			}

			//Cost for 'bc'
			{
				glm::mat4 combinedError = vertexQuadric[bId] + vertexQuadric[cId];
				double costB = glm::dot(b, combinedError * b);
				double costC = glm::dot(c, combinedError * c);
				if (costB < costC) //'b' is even, 'c' is odd
				{
					contractions[j + 1] = { bId, cId, costB };
				}
				else //'c' is even, 'b' is odd
				{
					contractions[j + 1] = { cId, bId, costC };
				}
			}

			//Cost for 'ca'
			{
				glm::mat4 combinedError = vertexQuadric[cId] + vertexQuadric[aId];
				double costC = glm::dot(c, combinedError * c);
				double costA = glm::dot(a, combinedError * a);
				if (costC < costA) //'c' is even, 'a' is odd
				{
					contractions[j + 2] = { cId, aId, costC };
				}
				else //'a' is even, 'c' is odd
				{
					contractions[j + 2] = { aId, cId, costA };
				}
			}
		}

		//Order based on smallest costs first
		eastl::sort(contractions.begin(), contractions.end(),
			[](const EdgeContraction& a, const EdgeContraction& b) -> bool
		{
			return a.cost < b.cost;
		});

		unordered_set<uint32_t> oddsList;
		vector<uint32_t> edgesToCollapse;
		//Reserve estimation
		edgesToCollapse.reserve(contractions.size() / 3);
		for (EdgeContraction& contraction : contractions)
		{
			//Check if this is actually an odd and not an even vertex
			bool isOdd = true;
			vector<uint32_t>& neighbors = vNeighbors[contraction.odd];
			if (oddsList.find(contraction.odd) != oddsList.end()) //Check already added
			{
				isOdd = false;
				continue;
			}

			//Check if any neighbor is odd
			for (unsigned int neighbor : neighbors)
			{
				if (oddsList.find(neighbor) != oddsList.end())
				{
					isOdd = false;
					break;
				}
			}

			//Found an odd!
			if (isOdd)
			{
				//Add this contraction to the list
				oddsList.insert(contraction.odd);
				edgesToCollapse.push_back(contraction.odd);
				edgesToCollapse.push_back(contraction.even);
			}
		}

		oddIndexBuffers.push_back(device->createPersistentBuffer(
			edgesToCollapse.data(), sizeof(uint32_t) * edgesToCollapse.size(), sizeof(uint32_t),
			static_cast<VkBufferUsageFlagBits>(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT),
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		));

		indexBuffers.push_back(device->createPersistentBuffer(
			meshes[i].indices, sizeof(uint32_t) * meshes[i].index_count, sizeof(uint32_t),
			static_cast<VkBufferUsageFlagBits>(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT),
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		));
		delete[] meshes[i].indices;
		meshes[i].index_count = 0;
	}
}

void Ravine::createUniformBuffers()
{
	//Setting size of uniform buffers vector to count of SwapChain's images.
	size_t framesCount = swapChain->images.size();
	uniformBuffers.resize(framesCount);
	materialsBuffers.resize(framesCount * meshesCount);
	modelsBuffers.resize(framesCount * meshesCount);
	animationsBuffers.resize(framesCount * meshesCount);

	for (size_t i = 0; i < framesCount; i++) {
		uniformBuffers[i] = device->createDynamicBuffer(sizeof(RvUniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, (VkMemoryPropertyFlagBits)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));
		for (size_t j = 0; j < meshesCount; j++)
		{
			size_t frameOffset = i * meshesCount;
			materialsBuffers[frameOffset + j] = device->createDynamicBuffer(sizeof(RvMaterialBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, (VkMemoryPropertyFlagBits)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));
			modelsBuffers[frameOffset + j] = device->createDynamicBuffer(sizeof(RvModelBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, (VkMemoryPropertyFlagBits)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));
			animationsBuffers[frameOffset + j] = device->createDynamicBuffer(sizeof(RvBoneBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, (VkMemoryPropertyFlagBits)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));
		}
	}
}

void Ravine::loadTextureImages()
{
	//Allocate RvTexture(s)
	texturesSize = 1;
	texturesSize += texturesToLoad.size(); //Normal plus undefined texture
	textures = new RvTexture[texturesSize];

	//Generate Pink 2x2 image for missing texture
	char* pinkTexture = new char[16]; //2x2 = 4 pixels <= 4 * RGBA = 4 * 4 char = 32 char
	for (uint32_t i = 0; i < 4; i++)
	{
		pinkTexture[i * 4 + 0] = 255;	//Red
		pinkTexture[i * 4 + 1] = 0;		//Green
		pinkTexture[i * 4 + 2] = 144;	//Blue
		pinkTexture[i * 4 + 3] = 255;	//Alpha
	}
	textures[0] = device->createTexture(pinkTexture, 2, 2);

	for (uint32_t i = 1; i < texturesSize; i++)
	{
		//Loading image
		int texWidth, texHeight, texChannels;

		fmt::print(stdout, "{0}\n", texturesToLoad[i - 1].c_str());
		stbi_uc* pixels = stbi_load(("../data/" + texturesToLoad[i - 1]).c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

		textures[i] = device->createTexture(pixels, texWidth, texHeight);

		free(pixels);

	}
}

void Ravine::createTextureSampler()
{
	VkSamplerCreateInfo samplerCreateInfo = {};
	samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

	//Sampler interpolation filters
	samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.minFilter = VK_FILTER_LINEAR;

	//Address mode per axis
	samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

	//Anisotropy filter
	samplerCreateInfo.anisotropyEnable = VK_TRUE;
	samplerCreateInfo.maxAnisotropy = 16;

	//Sampling beyond image with "Clamp to Border"
	samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

	//Coordinates normalization:
	//VK_TRUE: [0,texWidth]/[0,texHeight]
	//VK_FALSE: [0,1]/[0,1]
	samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;

	//Comparison function (used for shadow maps)
	samplerCreateInfo.compareEnable = VK_FALSE;
	samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	//Mipmapping
	samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerCreateInfo.mipLodBias = 0.0f;	//Optional
	samplerCreateInfo.minLod = 0.0f;		//Optional
	samplerCreateInfo.maxLod = static_cast<float>(mipLevels);

	//Creating sampler
	if (vkCreateSampler(device->handle, &samplerCreateInfo, nullptr, &textureSampler) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create texture sampler!");
	}
}

void Ravine::createDepthResources()
{
	RvFramebufferAttachmentCreateInfo createInfo = rvDefaultDepthAttachment;
	createInfo.format = device->findDepthFormat();
	createInfo.extent.width = swapChain->extent.width;
	createInfo.extent.height = swapChain->extent.height;

	swapChain->addFramebufferAttachment(createInfo);
}

void Ravine::createMultiSamplingResources()
{
	RvFramebufferAttachmentCreateInfo createInfo = rvDefaultResolveAttachment;
	createInfo.format = swapChain->imageFormat;
	createInfo.extent.width = swapChain->extent.width;
	createInfo.extent.height = swapChain->extent.height;

	swapChain->addFramebufferAttachment(createInfo);
}

void Ravine::allocateCommandBuffers() {

	//Allocate command buffers
	primaryCmdBuffers.resize(swapChain->framebuffers.size());
	secondaryCmdBuffers.resize(swapChain->framebuffers.size());

	//Primary Command Buffers
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = device->commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)primaryCmdBuffers.size();
	if (vkAllocateCommandBuffers(device->handle, &allocInfo, primaryCmdBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate command buffers!");
	}

	//Secondary Command Buffers
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
	if (vkAllocateCommandBuffers(device->handle, &allocInfo, secondaryCmdBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate command buffers!");
	}
}

void Ravine::recordCommandBuffers(uint32_t currentFrame)
{
	//Begining Command Buffer Recording
	//Reference: https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Command_buffers#page_Starting_command_buffer_recording

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

#pragma region Secondary Command Buffers
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;

	//Setup inheritance information to provide access modifiers from RenderPass
	VkCommandBufferInheritanceInfo inheritanceInfo = {};
	inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
	inheritanceInfo.renderPass = swapChain->renderPass;
	//inheritanceInfo.subpass = 0;
	inheritanceInfo.occlusionQueryEnable = VK_FALSE;
	inheritanceInfo.framebuffer = swapChain->framebuffers[currentFrame];
	inheritanceInfo.pipelineStatistics = 0;
	beginInfo.pInheritanceInfo = &inheritanceInfo;

	//Begin recording Command Buffer
	if (vkBeginCommandBuffer(secondaryCmdBuffers[currentFrame], &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("Failed to begin recording command buffer!");
	}

	//Basic Drawing Commands
	//Reference: https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Command_buffers#page_Basic_drawing_commands
	const size_t setsPerFrame = 1 + (meshesCount * 2);
	const VkDeviceSize offsets[] = { 0 };

	if (staticSolidPipelineEnabled)
	{
		//Bind Correct Graphics Pipeline
		vkCmdBindPipeline(secondaryCmdBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, *staticGraphicsPipeline);

		//Global, Material and Model Descriptor Sets
		vkCmdBindDescriptorSets(secondaryCmdBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, staticGraphicsPipeline->layout, 0, 1, &descriptorSets[currentFrame * setsPerFrame], 0, nullptr);

		//Call drawing
		for (size_t meshIndex = 0; meshIndex < meshesCount; meshIndex++)
		{
			const size_t meshSetOffset = meshIndex * 2;
			vkCmdBindDescriptorSets(secondaryCmdBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, staticGraphicsPipeline->layout, 1, 2, &descriptorSets[currentFrame * setsPerFrame + meshSetOffset + 1], 0, nullptr);

			vkCmdBindVertexBuffers(secondaryCmdBuffers[currentFrame], 0, 1, &vertexBuffers[meshIndex].handle, offsets);
			vkCmdBindIndexBuffer(secondaryCmdBuffers[currentFrame], indexBuffers[meshIndex].handle, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(secondaryCmdBuffers[currentFrame], static_cast<uint32_t>(indexBuffers[meshIndex].instancesCount), 1, 0, 0, 0);
		}
	}

	if (staticWiredPipelineEnabled)
	{
		//Bind Correct Graphics Pipeline
		vkCmdBindPipeline(secondaryCmdBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, *staticWireframeGraphicsPipeline);

		//Global, Material and Model Descriptor Sets
		vkCmdBindDescriptorSets(secondaryCmdBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, staticWireframeGraphicsPipeline->layout, 0, 1, &descriptorSets[currentFrame * setsPerFrame], 0, nullptr);

		//Call drawing
		for (size_t meshIndex = 0; meshIndex < meshesCount; meshIndex++)
		{
			const size_t meshSetOffset = meshIndex * 2;
			vkCmdBindDescriptorSets(secondaryCmdBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, staticWireframeGraphicsPipeline->layout, 1, 2, &descriptorSets[currentFrame * setsPerFrame + meshSetOffset + 1], 0, nullptr);

			vkCmdBindVertexBuffers(secondaryCmdBuffers[currentFrame], 0, 1, &vertexBuffers[meshIndex].handle, offsets);
			vkCmdBindIndexBuffer(secondaryCmdBuffers[currentFrame], indexBuffers[meshIndex].handle, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(secondaryCmdBuffers[currentFrame], static_cast<uint32_t>(indexBuffers[meshIndex].instancesCount), 1, 0, 0, 0);
		}
	}

	if (skinnedSolidPipelineEnabled)
	{
		//Bind Correct Graphics Pipeline
		vkCmdBindPipeline(secondaryCmdBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, *skinnedGraphicsPipeline);

		//Global, Material and Model Descriptor Sets
		vkCmdBindDescriptorSets(secondaryCmdBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, skinnedGraphicsPipeline->layout, 0, 1, &descriptorSets[currentFrame * setsPerFrame], 0, nullptr);

		//Call drawing
		for (size_t meshIndex = 0; meshIndex < meshesCount; meshIndex++)
		{
			const size_t meshSetOffset = meshIndex * 2;
			vkCmdBindDescriptorSets(secondaryCmdBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, skinnedGraphicsPipeline->layout, 1, 2, &descriptorSets[currentFrame * setsPerFrame + meshSetOffset + 1], 0, nullptr);

			vkCmdBindVertexBuffers(secondaryCmdBuffers[currentFrame], 0, 1, &vertexBuffers[meshIndex].handle, offsets);
			vkCmdBindIndexBuffer(secondaryCmdBuffers[currentFrame], indexBuffers[meshIndex].handle, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(secondaryCmdBuffers[currentFrame], static_cast<uint32_t>(indexBuffers[meshIndex].instancesCount), 1, 0, 0, 0);
		}
	}


	if (skinnedSolidPipelineEnabled)
	{
		//Bind Correct Graphics Pipeline
		vkCmdBindPipeline(secondaryCmdBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, *skinnedGraphicsPipeline);

		//Global, Material and Model Descriptor Sets
		vkCmdBindDescriptorSets(secondaryCmdBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, skinnedGraphicsPipeline->layout, 0, 1, &descriptorSets[currentFrame * setsPerFrame], 0, nullptr);

		//Call drawing
		for (size_t meshIndex = 0; meshIndex < meshesCount; meshIndex++)
		{
			const size_t meshSetOffset = meshIndex * 2;
			vkCmdBindDescriptorSets(secondaryCmdBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, skinnedGraphicsPipeline->layout, 1, 2, &descriptorSets[currentFrame * setsPerFrame + meshSetOffset + 1], 0, nullptr);

			vkCmdBindVertexBuffers(secondaryCmdBuffers[currentFrame], 0, 1, &vertexBuffers[meshIndex].handle, offsets);
			vkCmdBindIndexBuffer(secondaryCmdBuffers[currentFrame], indexBuffers[meshIndex].handle, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(secondaryCmdBuffers[currentFrame], static_cast<uint32_t>(indexBuffers[meshIndex].instancesCount), 1, 0, 0, 0);
		}
	}

	if (skinnedWiredPipelineEnabled)
	{
		//Perform the same with wireframe rendering
		vkCmdBindPipeline(secondaryCmdBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, *skinnedWireframeGraphicsPipeline);

		//Global, Material and Model Descriptor Sets
		vkCmdBindDescriptorSets(secondaryCmdBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, skinnedWireframeGraphicsPipeline->layout, 0, 1, &descriptorSets[currentFrame * setsPerFrame], 0, nullptr);

		for (size_t meshIndex = 0; meshIndex < meshesCount; meshIndex++)
		{
			const size_t meshSetOffset = meshIndex * 2;
			vkCmdBindDescriptorSets(secondaryCmdBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, skinnedWireframeGraphicsPipeline->layout, 1, 2, &descriptorSets[currentFrame * setsPerFrame + meshSetOffset + 1], 0, nullptr);

			vkCmdBindVertexBuffers(secondaryCmdBuffers[currentFrame], 0, 1, &vertexBuffers[meshIndex].handle, offsets);
			vkCmdBindIndexBuffer(secondaryCmdBuffers[currentFrame], indexBuffers[meshIndex].handle, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(secondaryCmdBuffers[currentFrame], static_cast<uint32_t>(indexBuffers[meshIndex].instancesCount), 1, 0, 0, 0);
		}
	}

	//Perform the same with lines rendering
	vkCmdBindPipeline(secondaryCmdBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, *staticLineGraphicsPipeline);

	//Global, Material and Model Descriptor Sets
	vkCmdBindDescriptorSets(secondaryCmdBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, staticLineGraphicsPipeline->layout, 0, 1, &descriptorSets[currentFrame * setsPerFrame], 0, nullptr);

	for (size_t meshIndex = 0; meshIndex < meshesCount; meshIndex++)
	{
		size_t meshSetOffset = meshIndex * 2;
		vkCmdBindDescriptorSets(secondaryCmdBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, staticLineGraphicsPipeline->layout, 1, 2, &descriptorSets[currentFrame * setsPerFrame + meshSetOffset + 1], 0, nullptr);

		vkCmdBindVertexBuffers(secondaryCmdBuffers[currentFrame], 0, 1, &vertexBuffers[meshIndex].handle, offsets);
		vkCmdBindIndexBuffer(secondaryCmdBuffers[currentFrame], oddIndexBuffers[meshIndex].handle, 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(secondaryCmdBuffers[currentFrame], static_cast<uint32_t>(oddIndexBuffers[meshIndex].instancesCount), 1, 0, 0, 0);
	}

	//Stop recording Command Buffer
	if (vkEndCommandBuffer(secondaryCmdBuffers[currentFrame]) != VK_SUCCESS) {
		throw std::runtime_error("Failed to record command buffer!");
	}
#pragma endregion

#pragma region Primary Command Buffers
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	//Begin recording command buffer
	if (vkBeginCommandBuffer(primaryCmdBuffers[currentFrame], &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("Failed to begin recording command buffer!");
	}

	//Starting a Render Pass
	//Reference: https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Command_buffers#page_Starting_a_render_pass
	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = swapChain->renderPass;
	renderPassInfo.framebuffer = swapChain->framebuffers[currentFrame];
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = swapChain->extent;

	//Clearing values
	array<VkClearValue, 2> clearValues = {};
	clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
	clearValues[1].depthStencil = { 1.0f, 0 };	//Depth goes from [1,0] - being 1 the furthest possible
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(primaryCmdBuffers[currentFrame], &renderPassInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

	//Execute Skinned Mesh Pipeline - Secondary Command Buffer
	vkCmdExecuteCommands(primaryCmdBuffers[currentFrame], 1, &secondaryCmdBuffers[currentFrame]);

	//Execute GUI Pipeline - Secondary Command Buffer
	vkCmdExecuteCommands(primaryCmdBuffers[currentFrame], 1, &gui->cmdBuffers[currentFrame]);

	//Finishing Render Pass
	//Reference: https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Command_buffers#page_Finishing_up
	vkCmdEndRenderPass(primaryCmdBuffers[currentFrame]);

	//Stop recording Command Buffer
	if (vkEndCommandBuffer(primaryCmdBuffers[currentFrame]) != VK_SUCCESS) {
		throw std::runtime_error("Failed to record command buffer!");
	}
#pragma endregion

}

void Ravine::mainLoop() {

	string fpsTitle;

	//Application
	setupFpsCam();

	while (!glfwWindowShouldClose(*window)) {
		RvTime::update();

		glfwSetWindowTitle(*window, "Ravine 1.0a");

		glfwPollEvents();

		drawFrame();
	}

	vkDeviceWaitIdle(device->handle);
}

void Ravine::drawGuiElements()
{
	static bool showPipelinesMenu = false;
	ImGui::BeginMainMenuBar();
	ImGui::TextUnformatted("Ravine Vulkan Prototype - Version 0.1a");
	ImGui::Separator();

	if (ImGui::MenuItem("Configurations Menu", 0, false, !showPipelinesMenu))
	{
		showPipelinesMenu = !showPipelinesMenu;
	}
	ImGui::Separator();

	static double lastUpdateTime = 0;
	lastUpdateTime += RvTime::deltaTime();
	static string lastFps = "0";
	if (lastUpdateTime > 0.1)
	{
		lastUpdateTime = 0;
		lastFps = "FPS - " + eastl::to_string(RvTime::framesPerSecond());
	}
	ImGui::TextUnformatted(lastFps.c_str());
	if (ImGui::MenuItem("Exit Ravine", 0, false))
	{
		glfwSetWindowShouldClose(*window, true);
	}
	ImGui::EndMainMenuBar();

	if (showPipelinesMenu)
	{
		if (ImGui::Begin("Configurations Menu", &showPipelinesMenu, { 400, 300 }, -1, ImGuiWindowFlags_NoCollapse))
		{
			ImGui::TextUnformatted("Graphics Pipelines");
			{
				ImGui::Checkbox("Skinned Opaque Pipeline", &skinnedSolidPipelineEnabled);
				ImGui::Checkbox("Skinned Wireframe Pipeline", &skinnedWiredPipelineEnabled);
				ImGui::Checkbox("Static Opaque Pipeline", &staticSolidPipelineEnabled);
				ImGui::Checkbox("Static Wireframe Pipeline", &staticWiredPipelineEnabled);
				ImGui::Separator();
			}

			ImGui::TextUnformatted("Uniforms");
			{
				ImGui::DragFloat3("Position", value_ptr(uniformPosition), 0.01f);
				ImGui::DragFloat3("Scale", value_ptr(uniformScale), 0.001f, 0.00001f, 1000.0f);
				ImGui::DragFloat3("Rotation", value_ptr(uniformRotation), 1.f, -180.f, 180.f);
				ImGui::Separator();
			}

			ImGui::End();
		}
	}
}

void Ravine::drawFrame()
{
	//Reference: https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Rendering_and_presentation
	//Should perform the following operations:
	//	- Acquire an image from the swap chain
	//	- Execute the command buffer with that image as attachment in the framebuffer
	//	- Return the image to the swap chain for presentation
	//As such operations are performed asynchronously, we must use sync them, either with fences or semaphores:
	//Fences are best fitted to syncronize the application itself with the renderization operations, while
	//semaphores are used to syncronize operations within or across command queues - thus our best fit.

	//Handle resize and such
	if (window->framebufferResized)
	{
		swapChain->framebufferResized = true;
		window->framebufferResized = false;
	}
	uint32_t frameIndex;
	if (!swapChain->acquireNextFrame(frameIndex)) {
		recreateSwapChain();
	}

	//Update bone transforms
	if (!meshes[0].animations.empty()) {
		boneTransform(RvTime::elapsedTime(), meshes[0].boneTransforms);
	}

	//Start GUI recording
	gui->acquireFrame();
	//DRAW THE GUI HERE

	drawGuiElements();

	//BUT NOT AFTER HERE
	gui->submitFrame();

	//Update GUI Buffers
	gui->updateBuffers(frameIndex);

	//Record GUI Draw Commands into CMD Buffers
	gui->recordCmdBuffers(frameIndex);

	//Update the uniforms for the given frame
	updateUniformBuffer(frameIndex);

	//Make sure to record all new Commands
	recordCommandBuffers(frameIndex);

	if (!swapChain->submitNextFrame(primaryCmdBuffers.data(), frameIndex)) {
		recreateSwapChain();
	}
}

void Ravine::setupFpsCam()
{
	//Enable caching of buttons pressed
	glfwSetInputMode(*window, GLFW_STICKY_MOUSE_BUTTONS, GLFW_TRUE);

	//Hide mouse cursor
	//glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	//Update lastMousePos to avoid initial offset not null
	glfwGetCursorPos(*window, &lastMouseX, &lastMouseY);

	//Initial rotations
	camera = new RvCamera(glm::vec3(5.f, 0.f, 0.f), 90.f, 0.f);

}

void Ravine::updateUniformBuffer(uint32_t currentFrame)
{
	/*
	Using a UBO this way is not the most efficient way to pass frequently changing values to the shader.
	A more efficient way to pass a small buffer of data to shaders are push constants.
	Reference: https://vulkan-tutorial.com/Uniform_buffers/Descriptor_layout_and_buffer
	*/

#pragma region Inputs

	glfwGetCursorPos(*window, &mouseX, &mouseY);
	glm::quat lookRot = glm::vec3(0, 0, 0);
	glm::vec4 translation = glm::vec4(0);

	bool imGuiHasKeyCtx = ImGui::GetIO().WantCaptureKeyboard;
	static bool shiftDown = false;
	if ((glfwGetKey(*window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) && !imGuiHasKeyCtx)
	{
		if (!shiftDown)
		{
			shiftDown = true;
			glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		}

		//Delta Mouse Positions
		double deltaX = mouseX - lastMouseX;
		double deltaY = mouseY - lastMouseY;

		//Calculate look rotation update
		camera->horRot -= deltaX * 16.0 * RvTime::deltaTime();
		camera->verRot -= deltaY * 16.0 * RvTime::deltaTime();

		//Limit vertical angle
		camera->verRot = F_MAX(F_MIN(89.9, camera->verRot), -89.9);

		//Define rotation quaternion starting form look rotation
		lookRot = glm::rotate(lookRot, glm::radians(camera->horRot), glm::vec3(0, 1, 0));
		lookRot = glm::rotate(lookRot, glm::radians(camera->verRot), glm::vec3(1, 0, 0));

		//Calculate translation
		if (glfwGetKey(*window, GLFW_KEY_W) == GLFW_PRESS)
			translation.z -= 2.0 * RvTime::deltaTime();

		if (glfwGetKey(*window, GLFW_KEY_A) == GLFW_PRESS)
			translation.x -= 2.0 * RvTime::deltaTime();

		if (glfwGetKey(*window, GLFW_KEY_S) == GLFW_PRESS)
			translation.z += 2.0 * RvTime::deltaTime();

		if (glfwGetKey(*window, GLFW_KEY_D) == GLFW_PRESS)
			translation.x += 2.0 * RvTime::deltaTime();

		if (glfwGetKey(*window, GLFW_KEY_Q) || glfwGetKey(*window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
			translation.y -= 2.0 * RvTime::deltaTime();

		if (glfwGetKey(*window, GLFW_KEY_E) || glfwGetKey(*window, GLFW_KEY_SPACE) == GLFW_PRESS)
			translation.y += 2.0 * RvTime::deltaTime();
	}
	else if (shiftDown)
	{
		shiftDown = false;
		glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}

	//Update Last Mouse coordinates
	lastMouseX = mouseX;
	lastMouseY = mouseY;

	// ANIM INTERPOL
	if (glfwGetKey(*window, GLFW_KEY_UP) == GLFW_PRESS) {
		animInterpolation += 0.001f;
		if (animInterpolation > 1.0f) {
			animInterpolation = 1.0f;
		}
		fmt::print(stdout, "{0}\n", animInterpolation);
	}
	if (glfwGetKey(*window, GLFW_KEY_DOWN) == GLFW_PRESS) {
		animInterpolation -= 0.001f;
		if (animInterpolation < 0.0f) {
			animInterpolation = 0.0f;
		}
		fmt::print(stdout, "{0}\n", animInterpolation);
	}
	// SWAP ANIMATIONS
	if (glfwGetKey(*window, GLFW_KEY_RIGHT) == GLFW_PRESS && !keyUpPressed) {
		keyUpPressed = true;
		meshes[0].curAnimId = (meshes[0].curAnimId + 1) % meshes[0].animations.size();
	}
	if (glfwGetKey(*window, GLFW_KEY_RIGHT) == GLFW_RELEASE) {
		keyUpPressed = false;
	}
	if (glfwGetKey(*window, GLFW_KEY_LEFT) == GLFW_PRESS && !keyDownPressed) {
		keyDownPressed = true;
		meshes[0].curAnimId = (meshes[0].curAnimId - 1) % meshes[0].animations.size();
	}
	if (glfwGetKey(*window, GLFW_KEY_LEFT) == GLFW_RELEASE) {
		keyDownPressed = false;
	}

	camera->Translate(lookRot * translation);

#pragma endregion

#pragma region OddEdges Parameters
	if (glfwGetKey(*window, GLFW_KEY_PAGE_UP) == GLFW_PRESS)
	{
		edgesSelected += 2;
		fmt::print(stdout, "Edges Select Count: {0}\n", edgesSelected);
	}
	else if (glfwGetKey(*window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS)
	{
		edgesSelected = eastl::max(edgesSelected - 2, static_cast<size_t>(2));
		fmt::print(stdout, "Edges Select Count: {0}\n", edgesSelected);
	}

	if (glfwGetKey(*window, GLFW_KEY_HOME) == GLFW_PRESS)
	{
		edgesOffset += 2;
		fmt::print(stdout, "Edges Offset Count: {0}\n", edgesOffset);
	}
	else if (glfwGetKey(*window, GLFW_KEY_END) == GLFW_PRESS)
	{
		edgesOffset = eastl::max(edgesOffset - 2, static_cast<size_t>(0));
		fmt::print(stdout, "Edges Offset Count: {0}\n", edgesOffset);
	}
#pragma endregion

#pragma region Global Uniforms
	RvUniformBufferObject ubo = {};

	//Make the view matrix
	ubo.view = camera->GetViewMatrix();

	//Projection matrix with FOV of 45 degrees
	ubo.proj = glm::perspective(glm::radians(45.0f), swapChain->extent.width / (float)swapChain->extent.height, 0.1f, 200.0f);

	ubo.camPos = camera->pos;
	ubo.lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

	//Flipping coordinates (because glm was designed for openGL, with fliped Y coordinate)
	ubo.proj[1][1] *= -1;

	//Transfering uniform data to uniform buffer
	void* data;
	vkMapMemory(device->handle, uniformBuffers[currentFrame].memory, 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(device->handle, uniformBuffers[currentFrame].memory);
#pragma endregion

	//TODO: Change this to a per-material basis instead of per-mesh
	for (size_t meshId = 0; meshId < meshesCount; meshId++)
	{
#pragma region Materials
		RvMaterialBufferObject materialsUbo = {};

		materialsUbo.customColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

		void* materialsData;
		vkMapMemory(device->handle, materialsBuffers[currentFrame * meshesCount + meshId].memory, 0, sizeof(materialsUbo), 0, &materialsData);
		memcpy(materialsData, &materialsUbo, sizeof(materialsUbo));
		vkUnmapMemory(device->handle, materialsBuffers[currentFrame * meshesCount + meshId].memory);
#pragma endregion

#pragma region Models
		RvModelBufferObject modelsUbo = {};

		//Model matrix updates
		modelsUbo.model = glm::translate(glm::mat4(1.0f), uniformPosition);
		modelsUbo.model = glm::rotate(modelsUbo.model, glm::radians(uniformRotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
		modelsUbo.model = glm::rotate(modelsUbo.model, glm::radians(uniformRotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		modelsUbo.model = glm::rotate(modelsUbo.model, glm::radians(uniformRotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		modelsUbo.model = glm::scale(modelsUbo.model, uniformScale);

		//Transfering model data to gpu buffer
		void* modelData;
		vkMapMemory(device->handle, modelsBuffers[currentFrame * meshesCount + meshId].memory, 0, sizeof(modelsUbo), 0, &modelData);
		memcpy(modelData, &modelsUbo, sizeof(modelsUbo));
		vkUnmapMemory(device->handle, modelsBuffers[currentFrame * meshesCount + meshId].memory);
#pragma endregion

#pragma region Animations
		RvBoneBufferObject bonesUbo = {};

		for (size_t i = 0; i < meshes[0].boneTransforms.size(); i++)
		{
			bonesUbo.transformMatrixes[i] = glm::transpose(glm::make_mat4(&meshes[0].boneTransforms[i].a1));
		}

		void* bonesData;
		vkMapMemory(device->handle, animationsBuffers[currentFrame * meshesCount + meshId].memory, 0, sizeof(bonesUbo), 0, &bonesData);
		memcpy(bonesData, &bonesUbo, sizeof(bonesUbo));
		vkUnmapMemory(device->handle, animationsBuffers[currentFrame * meshesCount + meshId].memory);
#pragma endregion
	}

}

void Ravine::cleanupSwapChain() {

	vkFreeCommandBuffers(device->handle, device->commandPool, static_cast<uint32_t>(primaryCmdBuffers.size()), primaryCmdBuffers.data());
	vkFreeCommandBuffers(device->handle, device->commandPool, static_cast<uint32_t>(secondaryCmdBuffers.size()), secondaryCmdBuffers.data());

	//Destroy Graphics Pipeline and all it's components
	vkDestroyPipeline(device->handle, *skinnedGraphicsPipeline, nullptr);
	vkDestroyPipelineLayout(device->handle, skinnedGraphicsPipeline->layout, nullptr);
	vkDestroyPipeline(device->handle, *skinnedWireframeGraphicsPipeline, nullptr);
	vkDestroyPipelineLayout(device->handle, skinnedWireframeGraphicsPipeline->layout, nullptr);
	vkDestroyPipeline(device->handle, *staticGraphicsPipeline, nullptr);
	vkDestroyPipelineLayout(device->handle, staticGraphicsPipeline->layout, nullptr);
	vkDestroyPipeline(device->handle, *staticWireframeGraphicsPipeline, nullptr);
	vkDestroyPipelineLayout(device->handle, staticWireframeGraphicsPipeline->layout, nullptr);
	vkDestroyPipeline(device->handle, *staticLineGraphicsPipeline, nullptr);
	vkDestroyPipelineLayout(device->handle, staticLineGraphicsPipeline->layout, nullptr);

	//Destroy swap chain and all it's images
	swapChain->clear();
	delete swapChain;
}

void Ravine::cleanup()
{
	//Cleanup RvGui data
	delete gui;

	//Hold number of swapchain images
	uint32_t swapImagesCount = swapChain->images.size();

	//Cleanup swap-chain related data
	cleanupSwapChain();

	//Cleaning up texture related objects
	vkDestroySampler(device->handle, textureSampler, nullptr);
	for (uint32_t i = 0; i < texturesSize; i++)
	{
		textures[i].Free();
	}
	delete[] textures;
	texturesSize = 0;

	//Destroy descriptor pool
	vkDestroyDescriptorPool(device->handle, descriptorPool, nullptr);

	//Destroying uniforms buffers
	for (size_t i = 0; i < swapImagesCount; i++) {
		vkDestroyBuffer(device->handle, uniformBuffers[i].handle, nullptr);
		vkFreeMemory(device->handle, uniformBuffers[i].memory, nullptr);
	}

	//Destroying materials buffers
	for (size_t i = 0; i < swapImagesCount; i++) {
		vkDestroyBuffer(device->handle, materialsBuffers[i].handle, nullptr);
		vkFreeMemory(device->handle, materialsBuffers[i].memory, nullptr);
	}

	//Destroying models buffers
	for (size_t i = 0; i < swapImagesCount; i++) {
		vkDestroyBuffer(device->handle, modelsBuffers[i].handle, nullptr);
		vkFreeMemory(device->handle, modelsBuffers[i].memory, nullptr);
	}

	//Destroying animations buffers
	for (size_t i = 0; i < swapImagesCount; i++) {
		vkDestroyBuffer(device->handle, animationsBuffers[i].handle, nullptr);
		vkFreeMemory(device->handle, animationsBuffers[i].memory, nullptr);
	}

	//Destroy descriptor set layout (uniform bind)
	vkDestroyDescriptorSetLayout(device->handle, globalDescriptorSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(device->handle, materialDescriptorSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(device->handle, modelDescriptorSetLayout, nullptr);

	//TODO: FIX HERE!
	for (size_t meshIndex = 0; meshIndex < meshesCount; meshIndex++)
	{
		//Destroy Vertex and Index Buffer Objects
		vkDestroyBuffer(device->handle, vertexBuffers[meshIndex].handle, nullptr);
		vkDestroyBuffer(device->handle, indexBuffers[meshIndex].handle, nullptr);

		//Free device memory for Vertex and Index Buffers
		vkFreeMemory(device->handle, vertexBuffers[meshIndex].memory, nullptr);
		vkFreeMemory(device->handle, indexBuffers[meshIndex].memory, nullptr);
	}

	//Destroy graphics pipeline
	delete skinnedGraphicsPipeline;
	delete skinnedWireframeGraphicsPipeline;
	delete staticGraphicsPipeline;

	//Destroy vulkan logical device and validation layer
	device->clear();
	delete device;

#ifdef VALIDATION_LAYERS_ENABLED
	rvDebug::destroyDebugReportCallbackExt(instance, rvDebug::callback, nullptr);
#endif

	//Destroy VK surface and instance
	delete window;
	vkDestroyInstance(instance, nullptr);

	//Finish GLFW
	glfwTerminate();
	}