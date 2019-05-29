#include "Ravine.h"

//STD Includes
#include <stdexcept>

//EASTL Includes
#include <EASTL/functional.h>
#include <EASTL/set.h>
#include <EASTL/algorithm.h>
#include <EASTL/string.h>

//GLM Includes
#include <glm\gtc\matrix_transform.hpp>

//STB Includes
#define STB_IMAGE_IMPLEMENTATION
#include "SingleFileLibraries\stb_image.h"

//Ravine Systems Includes
#include "RvTime.h"
#include "RvConfig.h"
#include "RvDebug.h"

//ShaderC Includes
#include "shaderc/shaderc.h"

//Assimp Includes
#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/postprocess.h>     // Post processing flags

//Specific usages of ASSIMP library
using Assimp::Importer;

Ravine::Ravine()
{
}

Ravine::~Ravine()
{
}

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
	rvWindowTemp->extent = { (uint32_t)width, (uint32_t)height };
}

vector<const char*> Ravine::getRequiredInstanceExtensions()
{
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

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
	string modelName = "guard.fbx";
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
	swapChain->CreateImageViews();
	swapChain->CreateRenderPass();
	createDescriptorSetLayout();


	//Shaders Loading
	skinnedTexColCode = rvTools::readFile("../data/shaders/skinned_tex_color.vert");
	skinnedWireframeCode = rvTools::readFile("../data/shaders/skinned_wireframe.vert");
	staticTexColCode = rvTools::readFile("../data/shaders/static_tex_color.vert");
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

	createMultiSamplingResources();
	createDepthResources();
	swapChain->CreateFramebuffers();
	loadTextureImages();
	createTextureSampler();
	createVertexBuffer();
	createIndexBuffer();
	createUniformBuffers();
	createDescriptorPool();
	createDescriptorSets();
	allocateCommandBuffers();
	swapChain->CreateSyncObjects();

	//GUI Management
	gui = new RvGUI(*device, *swapChain, *window);
	gui->Init(device->getMaxUsableSampleCount());
}

void Ravine::createInstance() {

	//Check validation layer support
#ifdef VALIDATION_LAYERS_ENABLED
	if (!rvCfg::CheckValidationLayerSupport()) {
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
	for (uint32_t i = 0; i < requiredExtensions.size(); i++) {
		bool found = false;
		for (uint32_t j = 0; j < extensions.size(); j++) {
			if (strcmp(requiredExtensions[i], (const char*)extensions.at(j).extensionName) == 0) {
				found = true;
				break;
			}
		}
		fmt::print(stdout, "\t{0} {1}\n", requiredExtensions[i], (found ? "found!" : "NOT found!"));
	}
	createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
	createInfo.ppEnabledExtensionNames = requiredExtensions.data();

	//Add validation layer info
#ifdef VALIDATION_LAYERS_ENABLED
	createInfo.enabledLayerCount = static_cast<uint32_t>(rvCfg::ValidationLayers.size());
	createInfo.ppEnabledLayerNames = rvCfg::ValidationLayers.data();
	fmt::print(stdout, "!Enabling validation layers!\n");
#else
	createInfo.enabledLayerCount = 0;
#endif

	//Ask for an instance
	if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create instance!");
	}
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

bool Ravine::isDeviceSuitable(VkPhysicalDevice device) {

	//Debug Physical Device Properties
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);
	fmt::print(stdout, "Checking device: {0}\n", deviceProperties.deviceName);

	rvTools::QueueFamilyIndices indices = rvTools::findQueueFamilies(device, window->surface);

	bool extensionsSupported = checkDeviceExtensionSupport(device);

	bool swapChainAdequate = false;
	if (extensionsSupported) {
		SwapChainSupportDetails swapChainSupport = rvTools::querySupport(device, window->surface);
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

bool Ravine::checkDeviceExtensionSupport(VkPhysicalDevice device) {
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	eastl::set<eastl::string> requiredExtensions(rvCfg::DeviceExtensions.begin(), rvCfg::DeviceExtensions.end());

	for (const auto& extension : availableExtensions) {
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
	swapChain->CreateSyncObjects();
	swapChain->CreateImageViews();
	swapChain->CreateRenderPass();

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

	createMultiSamplingResources();
	createDepthResources();
	swapChain->CreateFramebuffers();
	allocateCommandBuffers();
	RvGUI* oldGui = gui;
	gui = new RvGUI(*device, *swapChain, *window);
	gui->Init(device->getMaxUsableSampleCount());
	delete oldGui;
	//Deleting old swapchain
	oldSwapchain->Clear();
	delete oldSwapchain;
}

void Ravine::createDescriptorPool()
{
	array<VkDescriptorPoolSize, 4> poolSizes = {};
	//Global Uniform
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = static_cast<uint32_t>(swapChain->images.size());

	//Material Uniform
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[1].descriptorCount = static_cast<uint32_t>(swapChain->images.size());
	poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[2].descriptorCount = static_cast<uint32_t>(swapChain->images.size() * RV_IMAGES_COUNT);

	//Model Uniform
	poolSizes[3].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[3].descriptorCount = static_cast<uint32_t>(swapChain->images.size());

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(swapChain->images.size() * 3/*Global, Material, Model*/);

	if (vkCreateDescriptorPool(device->handle, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create descriptor pool!");
	}
}

void Ravine::createDescriptorSets()
{
	//Descriptor Sets Count
	int descriptorSetsCount = swapChain->images.size() * 3 /*Global, Material, Model*/;
	vector<VkDescriptorSetLayout> layouts(descriptorSetsCount);
	for (int i = 0; i < descriptorSetsCount / 3; i++)
	{
		layouts[i * 3 + 0] = globalDescriptorSetLayout;
		layouts[i * 3 + 1] = materialDescriptorSetLayout;
		layouts[i * 3 + 2] = modelDescriptorSetLayout;
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

	//Configuring descriptor sets
	for (size_t i = 0; i < descriptorSetsCount / 3; i++) {
		VkDescriptorBufferInfo globalUniformsInfo = {};
		globalUniformsInfo.buffer = uniformBuffers[i].buffer;
		globalUniformsInfo.offset = 0;
		globalUniformsInfo.range = sizeof(RvUniformBufferObject);

		VkDescriptorBufferInfo materialsInfo = {};
		materialsInfo.buffer = materialsBuffers[i].buffer;
		materialsInfo.offset = 0;
		materialsInfo.range = sizeof(RvMaterialBufferObject);

		VkDescriptorImageInfo imageInfo[RV_IMAGES_COUNT];
		for (uint32_t j = 0; j < RV_IMAGES_COUNT; j++)
		{
			imageInfo[j].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo[j].imageView = textures[j % texturesSize].view; //TODO Fix this later!
			imageInfo[j].sampler = textureSampler;
		}

		VkDescriptorBufferInfo modelsInfo = {};
		modelsInfo.buffer = modelsBuffers[i].buffer;
		modelsInfo.offset = 0;
		modelsInfo.range = sizeof(RvModelBufferObject);

		VkDescriptorBufferInfo animationsInfo = {};
		animationsInfo.buffer = animationsBuffers[i].buffer;
		animationsInfo.offset = 0;
		animationsInfo.range = sizeof(RvBoneBufferObject);

		array<VkWriteDescriptorSet, 5> descriptorWrites = {};

		//Global Uniform Buffer Info
		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = descriptorSets[i * 3 + 0];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &globalUniformsInfo;

		//Materials Uniform Buffer Info
		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = descriptorSets[i * 3 + 1];
		descriptorWrites[1].dstBinding = 0;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pBufferInfo = &materialsInfo;

		//Image Info
		descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[2].dstSet = descriptorSets[i * 3 + 1];
		descriptorWrites[2].dstBinding = 1;
		descriptorWrites[2].dstArrayElement = 0;
		descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[2].descriptorCount = RV_IMAGES_COUNT;
		descriptorWrites[2].pImageInfo = imageInfo;

		//Models Uniform Buffer Info
		descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[3].dstSet = descriptorSets[i * 3 + 2];
		descriptorWrites[3].dstBinding = 0;
		descriptorWrites[3].dstArrayElement = 0;
		descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[3].descriptorCount = 1;
		descriptorWrites[3].pBufferInfo = &modelsInfo;

		//Animations Uniform Buffer Info
		descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[4].dstSet = descriptorSets[i * 3 + 2];
		descriptorWrites[4].dstBinding = 1;
		descriptorWrites[4].dstArrayElement = 0;
		descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[4].descriptorCount = 1;
		descriptorWrites[4].pBufferInfo = &animationsInfo;

		vkUpdateDescriptorSets(device->handle, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
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
	samplerLayoutBinding.descriptorCount = RV_IMAGES_COUNT;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.pImmutableSamplers = nullptr;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

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
	importer.ReadFile(filePath.c_str(), aiProcess_CalcTangentSpace | \
		aiProcess_GenSmoothNormals | \
		aiProcess_JoinIdenticalVertices | \
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

		const aiVector3D* norms = &mesh->mNormals[0];

		//Treat each case for optimal performance
		if (hasCoords && hasColors)
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
				meshes[i].vertices[j].normal = { norms[j].x , norms[j].y, norms[j].z };
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
				meshes[i].vertices[j].normal = { norms[j].x , norms[j].y, norms[j].z };
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
		uint16_t BoneIndex = 0;
		string BoneName(pMesh->mBones[i]->mName.data);

		if (meshData.boneMapping.find(BoneName) == meshData.boneMapping.end()) {
			BoneIndex = meshes[0].numBones;
			meshes[0].numBones++;
			RvBoneInfo bi;
			meshes[0].boneInfo.push_back(bi);
		}
		else {
			BoneIndex = meshData.boneMapping[BoneName];
		}

		meshData.boneMapping[BoneName] = BoneIndex;
		meshes[0].boneInfo[BoneIndex].BoneOffset = pMesh->mBones[i]->mOffsetMatrix;

		for (uint16_t j = 0; j < pMesh->mBones[i]->mNumWeights; j++) {
			uint16_t VertexID = 0 + pMesh->mBones[i]->mWeights[j].mVertexId;
			float Weight = pMesh->mBones[i]->mWeights[j].mWeight;
			meshData.vertices[VertexID].AddBoneData(BoneIndex, Weight);
		}
	}
}

void Ravine::BoneTransform(double TimeInSeconds, vector<aiMatrix4x4>& Transforms)
{
	const aiMatrix4x4 identity;
	uint16_t otherindex = (meshes[0].curAnimId + 1) % meshes[0].animations.size();
	double animDuration = meshes[0].animations[meshes[0].curAnimId]->aiAnim->mDuration;
	double otherDuration = meshes[0].animations[otherindex]->aiAnim->mDuration;
	runTime += RvTime::deltaTime() * (animDuration / otherDuration * animInterpolation + 1.0f * (1.0f - animInterpolation));
	ReadNodeHeirarchy(runTime, animDuration, otherDuration, meshes[0].rootNode, identity);

	Transforms.resize(meshes[0].numBones);

	for (uint16_t i = 0; i < meshes[0].numBones; i++) {
		Transforms[i] = meshes[0].boneInfo[i].FinalTransformation;
	}
}

void Ravine::ReadNodeHeirarchy(double AnimationTime, double curDuration, double otherDuration, const aiNode* pNode,
	const aiMatrix4x4& ParentTransform)
{
	string NodeName(pNode->mName.data);

	aiMatrix4x4 NodeTransformation(pNode->mTransformation);

	uint16_t otherindex = (meshes[0].curAnimId + 1) % meshes[0].animations.size();
	const aiNodeAnim* pNodeAnim = findNodeAnim(meshes[0].animations[meshes[0].curAnimId]->aiAnim, NodeName);
	const aiNodeAnim* otherNodeAnim = findNodeAnim(meshes[0].animations[otherindex]->aiAnim, NodeName);

	float otherAnimTime = AnimationTime * curDuration / otherDuration;

	double ticksPerSecond = meshes[0].animations[meshes[0].curAnimId]->aiAnim->mTicksPerSecond;
	float TimeInTicks = AnimationTime * ticksPerSecond;
	float animationTime = std::fmod(TimeInTicks, curDuration);

	ticksPerSecond = meshes[0].animations[otherindex]->aiAnim->mTicksPerSecond;
	float otherTimeInTicks = otherAnimTime * ticksPerSecond;
	float otherAnimationTime = std::fmod(otherTimeInTicks, otherDuration);

	if (pNodeAnim)
	{
		// Get interpolated matrices between current and next frame
		aiMatrix4x4 matScale = interpolateScale(animInterpolation, animationTime, otherAnimationTime, pNodeAnim, otherNodeAnim);
		aiMatrix4x4 matRotation = interpolateRotation(animInterpolation, animationTime, otherAnimationTime, pNodeAnim, otherNodeAnim);
		aiMatrix4x4 matTranslation = interpolateTranslation(animInterpolation, animationTime, otherAnimationTime, pNodeAnim, otherNodeAnim);

		NodeTransformation = matTranslation * matRotation * matScale;
	}

	aiMatrix4x4 GlobalTransformation = ParentTransform * NodeTransformation;

	if (meshes[0].boneMapping.find(NodeName) != meshes[0].boneMapping.end())
	{
		uint32_t BoneIndex = meshes[0].boneMapping[NodeName];
		meshes[0].boneInfo[BoneIndex].FinalTransformation = meshes[0].animGlobalInverseTransform *
			GlobalTransformation * meshes[0].boneInfo[BoneIndex].BoneOffset;
	}

	for (uint32_t i = 0; i < pNode->mNumChildren; i++)
	{
		ReadNodeHeirarchy(AnimationTime, curDuration, otherDuration, pNode->mChildren[i], GlobalTransformation);
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
		vertexBuffers.push_back(device->createPersistentBuffer(meshes[i].vertices, sizeof(RvSkinnedVertexColored) * meshes[i].vertex_count, sizeof(RvSkinnedVertexColored),
			(VkBufferUsageFlagBits)(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT), VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
		);
		delete[] meshes[i].vertices;
		meshes[i].vertex_count = 0;
	}

}

void Ravine::createIndexBuffer()
{
	indexBuffers.reserve(meshesCount);
	for (size_t i = 0; i < meshesCount; i++)
	{
		indexBuffers.push_back(device->createPersistentBuffer(meshes[i].indices, sizeof(uint32_t) * meshes[i].index_count, sizeof(uint32_t),
			(VkBufferUsageFlagBits)(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT), VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
		);
		delete[] meshes[i].indices;
		meshes[i].index_count = 0;
	}
}

void Ravine::createUniformBuffers()
{
	//Setting size of uniform buffers vector to count of SwapChain's images.
	size_t framesCount = swapChain->images.size();
	uniformBuffers.resize(framesCount);
	materialsBuffers.resize(framesCount);
	modelsBuffers.resize(framesCount);
	animationsBuffers.resize(framesCount);

	for (size_t i = 0; i < framesCount; i++) {
		uniformBuffers[i] = device->createDynamicBuffer(sizeof(RvUniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, (VkMemoryPropertyFlagBits)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));
		materialsBuffers[i] = device->createDynamicBuffer(sizeof(RvMaterialBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, (VkMemoryPropertyFlagBits)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));
		modelsBuffers[i] = device->createDynamicBuffer(sizeof(RvModelBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, (VkMemoryPropertyFlagBits)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));
		animationsBuffers[i] = device->createDynamicBuffer(sizeof(RvBoneBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, (VkMemoryPropertyFlagBits)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));
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

		stbi_image_free(pixels);
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
	VkFormat depthFormat = device->findDepthFormat();

	RvFramebufferAttachmentCreateInfo createInfo = {};
	createInfo.layerCount = 1;
	createInfo.mipLevels = 1;
	createInfo.format = depthFormat;
	createInfo.tilling = VK_IMAGE_TILING_OPTIMAL;
	createInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	createInfo.memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	createInfo.aspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT;
	createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	createInfo.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	swapChain->AddFramebufferAttachment(createInfo);
}

void Ravine::createMultiSamplingResources()
{
	VkFormat colorFormat = swapChain->imageFormat;

	RvFramebufferAttachmentCreateInfo createInfo = {};
	createInfo.layerCount = 1;
	createInfo.mipLevels = 1;
	createInfo.format = colorFormat;
	createInfo.tilling = VK_IMAGE_TILING_OPTIMAL;
	createInfo.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	createInfo.memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	createInfo.aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
	createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	createInfo.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	swapChain->AddFramebufferAttachment(createInfo);
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
	vkCmdBindPipeline(secondaryCmdBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, *skinnedGraphicsPipeline);

	//Global, Material and Model Descriptor Sets
	vkCmdBindDescriptorSets(secondaryCmdBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, skinnedGraphicsPipeline->layout, 0, 3, &descriptorSets[currentFrame * 3 + 0], 0, nullptr);

	//Set Vertex Buffer for drawing
	VkDeviceSize offsets[] = { 0 };

	//Binding descriptor sets (uniforms)
	//Call drawing
	for (size_t meshIndex = 0; meshIndex < meshesCount; meshIndex++)
	{
		vkCmdBindVertexBuffers(secondaryCmdBuffers[currentFrame], 0, 1, &vertexBuffers[meshIndex].handle, offsets);
		vkCmdBindIndexBuffer(secondaryCmdBuffers[currentFrame], indexBuffers[meshIndex].handle, 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(secondaryCmdBuffers[currentFrame], static_cast<uint32_t>(indexBuffers[meshIndex].instancesCount), 1, 0, 0, 0);
	}

	//Perform the same with lines rendering
	vkCmdBindPipeline(secondaryCmdBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, *skinnedWireframeGraphicsPipeline);

	//Global, Material and Model Descriptor Sets
	vkCmdBindDescriptorSets(secondaryCmdBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, skinnedWireframeGraphicsPipeline->layout, 0, 3, &descriptorSets[currentFrame * 3 + 0], 0, nullptr);

	for (size_t meshIndex = 0; meshIndex < meshesCount; meshIndex++)
	{
		vkCmdBindVertexBuffers(secondaryCmdBuffers[currentFrame], 0, 1, &vertexBuffers[meshIndex].handle, offsets);
		vkCmdBindIndexBuffer(secondaryCmdBuffers[currentFrame], indexBuffers[meshIndex].handle, 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(secondaryCmdBuffers[currentFrame], static_cast<uint32_t>(indexBuffers[meshIndex].instancesCount), 1, 0, 0, 0);
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

	string fpsTitle = "";

	//Application
	setupFPSCam();

	while (!glfwWindowShouldClose(*window)) {
		RvTime::update();

		fpsTitle = "Ravine - Milisseconds " + eastl::to_string(RvTime::deltaTime() * 1000.0);
		glfwSetWindowTitle(*window, fpsTitle.c_str());

		glfwPollEvents();

		drawFrame();
	}

	vkDeviceWaitIdle(device->handle);
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
	uint32_t frameIndex;
	if (!swapChain->AcquireNextFrame(frameIndex)) {
		recreateSwapChain();
	}

	//Update bone transforms
	if (meshes[0].animations.size() > 0) {
		BoneTransform(RvTime::elapsedTime(), meshes[0].boneTransforms);
	}

	//Update the uniforms for the given frame
	updateUniformBuffer(frameIndex);

	//Start GUI recording
	gui->AcquireFrame();
	//DRAW THE GUI HERE

	ImGui::ShowDemoWindow();

	//BUT NOT AFTER HERE
	gui->SubmitFrame();

	//Update GUI Buffers
	gui->UpdateBuffers(frameIndex);

	//Record GUI Draw Commands into CMD Buffers
	gui->RecordCmdBuffers(frameIndex);

	//Make sure to record all new Commands
	recordCommandBuffers(frameIndex);

	if (!swapChain->SubmitNextFrame(primaryCmdBuffers.data(), frameIndex)) {
		recreateSwapChain();
	}
}

void Ravine::setupFPSCam()
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

void Ravine::updateUniformBuffer(uint32_t currentImage)
{
	/*
	Using a UBO this way is not the most efficient way to pass frequently changing values to the shader.
	A more efficient way to pass a small buffer of data to shaders are push constants.
	Reference: https://vulkan-tutorial.com/Uniform_buffers/Descriptor_layout_and_buffer
	*/

#pragma region Inputs

	glfwGetCursorPos(*window, &mouseX, &mouseY);
	glm::quat lookRot = glm::vec3(0, 0, 0);

	if (glfwGetMouseButton(*window, GLFW_MOUSE_BUTTON_1))
	{
		//Delta Mouse Positions
		double deltaX = mouseX - lastMouseX;
		double deltaY = mouseY - lastMouseY;

		//Calculate look rotation update
		camera->horRot -= deltaX * 30.0 * RvTime::deltaTime();
		camera->verRot -= deltaY * 30.0 * RvTime::deltaTime();

		//Limit vertical angle
		camera->verRot = f_max(f_min(89.9, camera->verRot), -89.9);

		//Define rotation quaternion starting form look rotation
		lookRot = glm::rotate(lookRot, glm::radians(camera->horRot), glm::vec3(0, 1, 0));
		lookRot = glm::rotate(lookRot, glm::radians(camera->verRot), glm::vec3(1, 0, 0));
	}

	//Update Last Mouse coordinates
	lastMouseX = mouseX;
	lastMouseY = mouseY;

	//Calculate translation
	glm::vec4 translation = glm::vec4(0);
	if (glfwGetKey(*window, GLFW_KEY_W) == GLFW_PRESS)
		translation.z -= 2.0 * RvTime::deltaTime();

	if (glfwGetKey(*window, GLFW_KEY_A) == GLFW_PRESS)
		translation.x -= 2.0 * RvTime::deltaTime();

	if (glfwGetKey(*window, GLFW_KEY_S) == GLFW_PRESS)
		translation.z += 2.0 * RvTime::deltaTime();

	if (glfwGetKey(*window, GLFW_KEY_D) == GLFW_PRESS)
		translation.x += 2.0 * RvTime::deltaTime();

	if (glfwGetKey(*window, GLFW_KEY_Q) || glfwGetKey(*window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
		translation.y -= 2.0 * RvTime::deltaTime();

	if (glfwGetKey(*window, GLFW_KEY_E) || glfwGetKey(*window, GLFW_KEY_SPACE) == GLFW_PRESS)
		translation.y += 2.0 * RvTime::deltaTime();

	if (glfwGetKey(*window, GLFW_KEY_R) == GLFW_PRESS) {
		camera->horRot = 90.0f;
		camera->verRot = 0.0f;
	}

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
	if (glfwGetKey(*window, GLFW_KEY_RIGHT) == GLFW_PRESS && keyUpPressed == false) {
		keyUpPressed = true;
		meshes[0].curAnimId = (meshes[0].curAnimId + 1) % meshes[0].animations.size();
	}
	if (glfwGetKey(*window, GLFW_KEY_RIGHT) == GLFW_RELEASE) {
		keyUpPressed = false;
	}
	if (glfwGetKey(*window, GLFW_KEY_LEFT) == GLFW_PRESS && keyDownPressed == false) {
		keyDownPressed = true;
		meshes[0].curAnimId = (meshes[0].curAnimId - 1) % meshes[0].animations.size();
	}
	if (glfwGetKey(*window, GLFW_KEY_LEFT) == GLFW_RELEASE) {
		keyDownPressed = false;
	}

	if (glfwGetKey(*window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(*window, true);

	camera->Translate(lookRot * translation);

#pragma endregion

	//TODO: Proper, per-object uniform with different DescriptorSets or PushConstant ids for indirect access
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
	vkMapMemory(device->handle, uniformBuffers[currentImage].memory, 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(device->handle, uniformBuffers[currentImage].memory);
#pragma endregion

#pragma region Materials
	RvMaterialBufferObject materialsUbo = {};

	materialsUbo.customColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

	void* materialsData;
	vkMapMemory(device->handle, materialsBuffers[currentImage].memory, 0, sizeof(materialsUbo), 0, &materialsData);
	memcpy(materialsData, &materialsUbo, sizeof(materialsUbo));
	vkUnmapMemory(device->handle, materialsBuffers[currentImage].memory);
#pragma endregion

#pragma region Models
	RvModelBufferObject modelsUbo = {};

	//Rotating object 90 degrees per second
	//modelsUbo.model = glm::rotate(glm::mat4(1.0f), /*(float)Time::elapsedTime() * */glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	modelsUbo.model = glm::scale(modelsUbo.model, glm::vec3(0.025f, 0.025f, 0.025f));

	//Transfering model data to gpu buffer
	void* modelData;
	vkMapMemory(device->handle, modelsBuffers[currentImage].memory, 0, sizeof(modelsUbo), 0, &modelData);
	memcpy(modelData, &modelsUbo, sizeof(modelsUbo));
	vkUnmapMemory(device->handle, modelsBuffers[currentImage].memory);
#pragma endregion

#pragma region Animations
	RvBoneBufferObject bonesUbo = {};

	for (size_t i = 0; i < meshes[0].boneTransforms.size(); i++)
	{
		bonesUbo.transformMatrixes[i] = glm::transpose(glm::make_mat4(&meshes[0].boneTransforms[i].a1));
	}

	void* bonesData;
	vkMapMemory(device->handle, animationsBuffers[currentImage].memory, 0, sizeof(bonesUbo), 0, &bonesData);
	memcpy(bonesData, &bonesUbo, sizeof(bonesUbo));
	vkUnmapMemory(device->handle, animationsBuffers[currentImage].memory);
#pragma endregion

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

	//Destroy swap chain and all it's images
	swapChain->Clear();
	delete swapChain;
}

void Ravine::cleanup()
{
	//Cleanup RvGUI data
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
		vkDestroyBuffer(device->handle, uniformBuffers[i].buffer, nullptr);
		vkFreeMemory(device->handle, uniformBuffers[i].memory, nullptr);
	}

	//Destroying materials buffers
	for (size_t i = 0; i < swapImagesCount; i++) {
		vkDestroyBuffer(device->handle, materialsBuffers[i].buffer, nullptr);
		vkFreeMemory(device->handle, materialsBuffers[i].memory, nullptr);
	}

	//Destroying models buffers
	for (size_t i = 0; i < swapImagesCount; i++) {
		vkDestroyBuffer(device->handle, modelsBuffers[i].buffer, nullptr);
		vkFreeMemory(device->handle, modelsBuffers[i].memory, nullptr);
	}

	//Destroying animations buffers
	for (size_t i = 0; i < swapImagesCount; i++) {
		vkDestroyBuffer(device->handle, animationsBuffers[i].buffer, nullptr);
		vkFreeMemory(device->handle, animationsBuffers[i].memory, nullptr);
	}

	//Destroy descriptor set layout (uniform bind)
	vkDestroyDescriptorSetLayout(device->handle, globalDescriptorSetLayout, nullptr);

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

	//Destroy vulkan logical device and validation layer
	device->Clear();
	delete device;

#ifdef VALIDATION_LAYERS_ENABLED
	rvDebug::DestroyDebugReportCallbackEXT(instance, rvDebug::callback, nullptr);
#endif

	//Destroy VK surface and instance
	delete window;
	vkDestroyInstance(instance, nullptr);

	//Finish GLFW
	glfwTerminate();
}