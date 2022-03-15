#include "Ravine.h"

// STD Includes
#include <stdexcept>

// EASTL Includes
#include <EASTL/set.h>

// GLM Includes
#include "RvDataTypes.h"
#include "RvTools.h"
#include "fmt/core.h"
#include "glm/ext/vector_float3.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "vulkan/vulkan_core.h"

// STB Includes
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Ravine Systems Includes
#include "RvConfig.h"
#include "RvDebug.h"
#include "RvTime.h"

// Types dependencies
#include "RvUniformTypes.h"

// GLM includes
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

// FMT Includes
#include <fmt/printf.h>

// OpenFBX Includes
#include "ofbx.h"

Ravine::Ravine() {}

Ravine::~Ravine() = default;

void Ravine::run()
{
	RvTime::initialize();
	initWindow();
	initVulkan();
	mainLoop();
	cleanup();
}

#pragma region Static Methods

// Static method because GLFW doesn't know how to call a member function with the "this" pointer to our Ravine instance.
void Ravine::framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
	auto rvWindowTemp = reinterpret_cast<RvWindow*>(glfwGetWindowUserPointer(window));
	rvWindowTemp->framebufferResized = true;
	rvWindowTemp->extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
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

void Ravine::initWindow()
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	window = new RvWindow(WIDTH, HEIGHT, WINDOW_NAME, false, framebufferResizeCallback);

	stbi_set_flip_vertically_on_load(true);
}

void Ravine::initVulkan()
{
	// Core setup
	createInstance();
#ifdef VALIDATION_LAYERS_ENABLED
	rvDebug::setupDebugCallback(instance);
#endif
	window->CreateSurface(instance);
	pickPhysicalDevice();

	// Load Scene
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

	// Rendering pipeline
	swapChain = new RvSwapChain(*device, window->surface, window->extent.width, window->extent.height, NULL);
	swapChain->createImageViews();
	swapChain->createSyncObjects();
	renderPass = RvRenderPass::defaultRenderPass(*device, *swapChain);
	createDescriptorSetLayout();

	// Shaders Loading
	glslang::InitializeProcess();
	// skinnedTexColCode = rvTools::readFile("../data/shaders/skinned_tex_color.vert");
	// skinnedWireframeCode = rvTools::readFile("../data/shaders/skinned_wireframe.vert");
	staticTexColCode = rvTools::readFile("../data/shaders/static_tex_color.vert");
	staticWireframeCode = rvTools::readFile("../data/shaders/static_wireframe.vert");
	phongTexColCode = rvTools::readFile("../data/shaders/phong_tex_color.frag");
	solidColorCode = rvTools::readFile("../data/shaders/solid_color.frag");

	VkDescriptorSetLayout* descriptorSetLayouts = new VkDescriptorSetLayout[3]{
	    globalDescriptorSetLayout, materialDescriptorSetLayout, modelDescriptorSetLayout};
	// skinnedGraphicsPipeline =
	//     new RvPolygonPipeline(*device, swapChain->extent, device->getMaxUsableSampleCount(),
	//     descriptorSetLayouts,
	// 			  3, renderPass->handle, skinnedTexColCode, phongTexColCode);
	// skinnedWireframeGraphicsPipeline =
	//     new RvWireframePipeline(*device, swapChain->extent, device->getMaxUsableSampleCount(),
	//     descriptorSetLayouts,
	// 			    3, renderPass->handle, skinnedWireframeCode, solidColorCode);
	staticGraphicsPipeline =
	    new RvPolygonPipeline(*device, swapChain->extent, device->getMaxUsableSampleCount(), descriptorSetLayouts,
				  3, renderPass->handle, staticTexColCode, phongTexColCode);
	staticWireframeGraphicsPipeline =
	    new RvWireframePipeline(*device, swapChain->extent, device->getMaxUsableSampleCount(), descriptorSetLayouts,
				    3, renderPass->handle, staticWireframeCode, solidColorCode);
	staticLineGraphicsPipeline =
	    new RvLinePipeline(*device, swapChain->extent, device->getMaxUsableSampleCount(), descriptorSetLayouts, 3,
			       renderPass->handle, staticWireframeCode, solidColorCode);
	gui = new RvGui(device, swapChain, window, renderPass);
	gui->init(device->getMaxUsableSampleCount());

	loadTextureImages();
	createTextureSampler();
	createVertexBuffer();
	createIndexBuffer();
	createUniformBuffers();
	createDescriptorPool();
	createDescriptorSets();
	allocateCommandBuffers();
}

void Ravine::createInstance()
{

	// Initialize Volk
	if (volkInitialize() != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to initialize Volk!\
			Ensure your driver is up to date and supports Vulkan!");
	}

	// Check validation layer support
#ifdef VALIDATION_LAYERS_ENABLED
	if (!rvCfg::checkValidationLayerSupport())
	{
		throw std::runtime_error("Validation layers requested, but not available!");
	}
#endif

	// Application related info
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Ravine Engine";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "Ravine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	// Information for VkInstance creation
	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	// Query for available extensions
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);	     // Query size
	vector<VkExtensionProperties> extensions(extensionCount);			     // Reserve
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data()); // Query data
	fmt::print(stdout, "Vulkan available extensions:\n");
	for (const VkExtensionProperties& extension : extensions)
	{
		fmt::print(stdout, "\t{0}\n", extension.extensionName);
	}

	// GLFW Window Management extensions
	vector<const char*> requiredExtensions = getRequiredInstanceExtensions();
	fmt::print(stdout, "Application required extensions:\n");
	for (const char*& requiredExtension : requiredExtensions)
	{
		bool found = false;
		for (VkExtensionProperties& extension : extensions)
		{
			if (strcmp(requiredExtension, static_cast<const char*>(extension.extensionName)) == 0)
			{
				found = true;
				break;
			}
		}
		fmt::print(stdout, "\t{0} {1}\n", requiredExtension, (found ? "found!" : "NOT found!"));
	}
	createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
	createInfo.ppEnabledExtensionNames = requiredExtensions.data();

	// Add validation layer info
#ifdef VALIDATION_LAYERS_ENABLED
	createInfo.enabledLayerCount = static_cast<uint32_t>(rvCfg::VALIDATION_LAYERS.size());
	createInfo.ppEnabledLayerNames = rvCfg::VALIDATION_LAYERS.data();
	fmt::print(stdout, "!Enabling validation layers!\n");
#else
	createInfo.enabledLayerCount = 0;
#endif

	// Ask for an instance
	if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create instance!");
	}

	// Initialize Entry-points on Volk
	volkLoadInstance(instance);
}

void Ravine::pickPhysicalDevice()
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	if (deviceCount == 0)
	{
		throw std::runtime_error("Failed to find GPUs with Vulkan support!");
	}

	vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	for (const auto& curPhysicalDevice : devices)
	{
		if (isDeviceSuitable(curPhysicalDevice))
		{
			device = new RvDevice(curPhysicalDevice, window->surface);
			break;
		}
	}

	if (device->physicalDevice == VK_NULL_HANDLE)
	{
		throw std::runtime_error("failed to find a suitable GPU!");
	}
}

bool Ravine::isDeviceSuitable(const VkPhysicalDevice device)
{

	// Debug Physical Device Properties
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);
	fmt::print(stdout, "Checking device: {0}\n", deviceProperties.deviceName);

	rvTools::QueueFamilyIndices indices = rvTools::findQueueFamilies(device, window->surface);

	bool extensionsSupported = checkDeviceExtensionSupport(device);

	bool swapChainAdequate = false;
	if (extensionsSupported)
	{
		RvSwapChainSupportDetails swapChainSupport = rvTools::querySupport(device, window->surface);
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	VkPhysicalDeviceFeatures supportedFeatures;
	vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

	bool isSuitable = indices.isComplete() && extensionsSupported && swapChainAdequate &&
			  supportedFeatures.samplerAnisotropy; // Checking for anisotropy support
	if (isSuitable)
	{
		fmt::print(stdout, "{0} is suitable and was selected!\n", deviceProperties.deviceName);
	}

	return isSuitable;
}

bool Ravine::checkDeviceExtensionSupport(const VkPhysicalDevice device)
{
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	eastl::set<string> requiredExtensions(rvCfg::DEVICE_EXTENSIONS.begin(), rvCfg::DEVICE_EXTENSIONS.end());

	for (const VkExtensionProperties& extension : availableExtensions)
	{
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

void Ravine::recreateSwapChain()
{

	int width = 0, height = 0;
	// If the window is minimized, wait for it to come back to the foreground.
	// TODO: We probably want to handle that another way, which we should probably discuss.
	while (width == 0 || height == 0)
	{
		width = window->extent.width;
		height = window->extent.height;
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(device->handle);

	// Storing handle
	RvSwapChain* oldSwapchain = swapChain;
	swapChain = new RvSwapChain(*device, window->surface, WIDTH, HEIGHT, oldSwapchain->handle);
	swapChain->createSyncObjects();
	swapChain->createImageViews();
	const VkExtent3D extent = {swapChain->extent.width, swapChain->extent.height, 1};
	renderPass->resizeAttachments(static_cast<uint32_t>(swapChain->imageViews.size()), extent,
				      swapChain->imageViews.data());

	// swapChain->createFramebuffers();
	allocateCommandBuffers();

	// Deleting old swapchain
	// oldSwapchain->renderPass = VK_NULL_HANDLE;
	oldSwapchain->clear();
	delete oldSwapchain;
}

void Ravine::createDescriptorPool()
{
	array<VkDescriptorPoolSize, 5> poolSizes = {};
	// Global Uniforms
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = static_cast<uint32_t>(swapChain->images.size());

	// TODO: Change descriptor count accordingly to materials count instead of meshes count
	// Material Uniforms
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[1].descriptorCount = static_cast<uint32_t>(swapChain->images.size() * meshesCount);
	// Image Uniforms
	poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[2].descriptorCount = static_cast<uint32_t>(swapChain->images.size() * meshesCount);

	// Model Uniforms
	poolSizes[3].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[3].descriptorCount = static_cast<uint32_t>(swapChain->images.size() * meshesCount);
	// Animation Uniforms
	poolSizes[4].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[4].descriptorCount = static_cast<uint32_t>(swapChain->images.size() * meshesCount);

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	/*Global, Material (per mesh), Model Matrix (per mesh)*/
	poolInfo.maxSets = static_cast<uint32_t>(swapChain->images.size()) * (1 + meshesCount * 4);

	if (vkCreateDescriptorPool(device->handle, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create descriptor pool!");
	}
}

void Ravine::createDescriptorSets()
{
	// Descriptor Sets Count
	size_t setsPerFrame = (1 + meshesCount * 2) /*Global, Material (per mesh), Model (per mesh)*/;
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
	if (vkAllocateDescriptorSets(device->handle, &allocInfo, descriptorSets.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate descriptor sets!");
	}

	// For each frame
	for (size_t i = 0; i < framesCount; i++)
	{
		VkDescriptorBufferInfo globalUniformsInfo = {};
		globalUniformsInfo.buffer = globalBuffers[i].handle;
		globalUniformsInfo.offset = 0;
		globalUniformsInfo.range = sizeof(RvGlobalBufferObject);

		// Offset per frame iteration
		size_t frameSetOffset = (i * setsPerFrame);
		size_t writesPerFrame = (1 + meshesCount * 4);
		vector<VkWriteDescriptorSet> descriptorWrites(writesPerFrame);

		// Global Uniform Buffer Info
		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = descriptorSets[frameSetOffset + 0 /*Frame Global Uniform Set*/];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &globalUniformsInfo;

		// For each mesh
		VkDescriptorBufferInfo* materialsInfo = new VkDescriptorBufferInfo[meshesCount]{};
		VkDescriptorImageInfo* imageInfo = new VkDescriptorImageInfo[meshesCount]{};
		VkDescriptorBufferInfo* modelsInfo = new VkDescriptorBufferInfo[meshesCount]{};
		VkDescriptorBufferInfo* animationsInfo = new VkDescriptorBufferInfo[meshesCount]{};
		for (size_t meshId = 0; meshId < meshesCount; meshId++)
		{
			// Offset per mesh iteration
			size_t meshSetOffset = meshId * 2;
			size_t meshWritesOffset = meshId * 4;

			// Materials Uniform Buffer Info
			materialsInfo[meshId] = {};
			materialsInfo[meshId].buffer = materialsBuffers[i].handle;
			materialsInfo[meshId].offset = 0;
			materialsInfo[meshId].range = sizeof(RvMaterialBufferObject);

			descriptorWrites[meshWritesOffset + 1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[meshWritesOffset + 1].dstSet =
			    descriptorSets[frameSetOffset + meshSetOffset + 1];
			descriptorWrites[meshWritesOffset + 1].dstBinding = 0;
			descriptorWrites[meshWritesOffset + 1].dstArrayElement = 0;
			descriptorWrites[meshWritesOffset + 1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[meshWritesOffset + 1].descriptorCount = 1;
			descriptorWrites[meshWritesOffset + 1].pBufferInfo = &materialsInfo[meshId];

			// Image Info
			imageInfo[meshId] = {};
			imageInfo[meshId].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			RvMeshColored& mesh = meshes[meshId];
			uint32_t textureId =
			    mesh.texturesCount > 0 ? 1 + mesh.textureIds[0] : 0 /*Missing Texture (Pink)*/;
			imageInfo[meshId].imageView = textures[textureId].view;
			imageInfo[meshId].sampler = textureSampler;

			descriptorWrites[meshWritesOffset + 2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[meshWritesOffset + 2].dstSet =
			    descriptorSets[frameSetOffset + meshSetOffset + 1];
			descriptorWrites[meshWritesOffset + 2].dstBinding = 1;
			descriptorWrites[meshWritesOffset + 2].dstArrayElement = 0;
			descriptorWrites[meshWritesOffset + 2].descriptorType =
			    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[meshWritesOffset + 2].descriptorCount = 1;
			descriptorWrites[meshWritesOffset + 2].pImageInfo = &imageInfo[meshId];

			// Models Uniform Buffer Info
			modelsInfo[meshId] = {};
			modelsInfo[meshId].buffer = modelsBuffers[i * meshesCount + meshId].handle;
			modelsInfo[meshId].offset = 0;
			modelsInfo[meshId].range = sizeof(RvModelBufferObject);

			descriptorWrites[meshWritesOffset + 3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[meshWritesOffset + 3].dstSet =
			    descriptorSets[frameSetOffset + meshSetOffset + 2];
			descriptorWrites[meshWritesOffset + 3].dstBinding = 0;
			descriptorWrites[meshWritesOffset + 3].dstArrayElement = 0;
			descriptorWrites[meshWritesOffset + 3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[meshWritesOffset + 3].descriptorCount = 1;
			descriptorWrites[meshWritesOffset + 3].pBufferInfo = &modelsInfo[meshId];

			// Animations Uniform Buffer Info
			animationsInfo[meshId] = {};
			animationsInfo[meshId].buffer = animationsBuffers[i * meshesCount + meshId].handle;
			animationsInfo[meshId].offset = 0;
			animationsInfo[meshId].range = sizeof(RvBoneBufferObject);

			descriptorWrites[meshWritesOffset + 4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[meshWritesOffset + 4].dstSet =
			    descriptorSets[frameSetOffset + meshSetOffset + 2];
			descriptorWrites[meshWritesOffset + 4].dstBinding = 1;
			descriptorWrites[meshWritesOffset + 4].dstArrayElement = 0;
			descriptorWrites[meshWritesOffset + 4].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[meshWritesOffset + 4].descriptorCount = 1;
			descriptorWrites[meshWritesOffset + 4].pBufferInfo = &animationsInfo[meshId];
		}

		// Update the sets for this frame
		vkUpdateDescriptorSets(device->handle, static_cast<uint32_t>(writesPerFrame), descriptorWrites.data(),
				       0, nullptr);

		delete[] materialsInfo;
		delete[] imageInfo;
		delete[] modelsInfo;
		delete[] animationsInfo;
	}
}

void Ravine::createDescriptorSetLayout()
{
	// Global Uniforms layout
	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	// Materials layout
	VkDescriptorSetLayoutBinding materialLayoutBinding = {};
	materialLayoutBinding.binding = 0;
	materialLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	materialLayoutBinding.descriptorCount = 1;
	materialLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	// Texture Sampler layout
	VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	samplerLayoutBinding.pImmutableSamplers = nullptr;

	// Models Data layout
	VkDescriptorSetLayoutBinding modelDataLayoutBinding = {};
	modelDataLayoutBinding.binding = 0;
	modelDataLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	modelDataLayoutBinding.descriptorCount = 1;
	modelDataLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	// Animations layout
	VkDescriptorSetLayoutBinding animationLayoutBinding = {};
	animationLayoutBinding.binding = 1;
	animationLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	animationLayoutBinding.descriptorCount = 1;
	animationLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	// Global Descriptor Set Layout
	{
		// Bindings array
		array<VkDescriptorSetLayoutBinding, 1> bindings = {uboLayoutBinding};

		VkDescriptorSetLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();

		if (vkCreateDescriptorSetLayout(device->handle, &layoutInfo, nullptr, &globalDescriptorSetLayout) !=
		    VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create descriptor set layout!");
		};
	}

	// Material Descriptor Set Layout
	{
		// Bindings array
		array<VkDescriptorSetLayoutBinding, 2> bindings = {materialLayoutBinding, samplerLayoutBinding};

		VkDescriptorSetLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();

		if (vkCreateDescriptorSetLayout(device->handle, &layoutInfo, nullptr, &materialDescriptorSetLayout) !=
		    VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create descriptor set layout!");
		};
	}

	// Model Descriptor Set Layout
	{
		// Bindings array
		array<VkDescriptorSetLayoutBinding, 2> bindings = {modelDataLayoutBinding, animationLayoutBinding};

		VkDescriptorSetLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();

		if (vkCreateDescriptorSetLayout(device->handle, &layoutInfo, nullptr, &modelDescriptorSetLayout) !=
		    VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create descriptor set layout!");
		};
	}
}

#pragma region FBX_STUFF

bool Ravine::loadScene(const string& filePath)
{
	auto buffer = rvTools::readFile(filePath);
	scene = ofbx::load((ofbx::u8*)buffer.data(), static_cast<int>(buffer.size()),
			   (ofbx::u64)ofbx::LoadFlags::TRIANGULATE);
	if (scene == nullptr)
	{
		return false;
	}

	// Load mesh
	meshesCount = scene->getMeshCount();
	meshes = new RvMeshColored[meshesCount];

	// Load each mesh
	for (uint32_t i = 0; i < meshesCount; i++)
	{
		// Default Initialize
		meshes[i] = {};

		// Hold reference
		const ofbx::Mesh* mesh = scene->getMesh(i);
		const ofbx::Geometry* geom = mesh->getGeometry();

		// 	meshes[i].animGlobalInverseTransform = animGlobalInverseTransform;

		// Allocate data structures
		int vertexCount = geom->getVertexCount();
		meshes[i].vertexCount = vertexCount;
		meshes[i].vertices = new RvVertexColored[vertexCount];
		int indexCount = geom->getIndexCount();
		meshes[i].indexCount = indexCount;
		meshes[i].indices = new uint32_t[indexCount];

		// Setup vertices
		const ofbx::Vec3* verts = geom->getVertices();
		const ofbx::Vec2* uvs = geom->getUVs();
		bool hasCoords = uvs != nullptr;
		const ofbx::Vec4* cols = geom->getColors();
		bool hasColors = cols != nullptr;
		const ofbx::Vec3* norms = geom->getNormals();
		bool hasNormals = norms != nullptr;

		// Treat each case for optimal performance
		if (hasCoords && hasColors && hasNormals)
		{
			for (int j = 0; j < vertexCount; j++)
			{
				// Vertices
				meshes[i].vertices[j].pos = {verts[j].x, verts[j].y, verts[j].z};

				// Texture coordinates
				meshes[i].vertices[j].texCoord = {uvs[j].x, uvs[j].y};

				// Vertex colors
				meshes[i].vertices[j].color = {cols[j].x, cols[j].y, cols[j].z};

				// Normals
				meshes[i].vertices[j].normal = {norms[j].x, norms[j].y, norms[j].z};
			}
		}
		else if (hasCoords && hasNormals)
		{
			for (int j = 0; j < vertexCount; j++)
			{
				// Vertices
				meshes[i].vertices[j].pos = {verts[j].x, verts[j].y, verts[j].z};

				// Texture coordinates
				meshes[i].vertices[j].texCoord = {uvs[j].x, uvs[j].y};

				// Vertex colors
				meshes[i].vertices[j].color = {1, 1, 1};

				// Normals
				meshes[i].vertices[j].normal = {norms[j].x, norms[j].y, norms[j].z};
			}
		}
		else if (hasNormals)
		{
			for (int j = 0; j < vertexCount; j++)
			{
				// Vertices
				meshes[i].vertices[j].pos = {verts[j].x, verts[j].y, verts[j].z};

				// Texture coordinates
				meshes[i].vertices[j].texCoord = {0, 0};

				// Vertex colors
				meshes[i].vertices[j].color = {1, 1, 1};

				// Normals
				meshes[i].vertices[j].normal = {norms[j].x, norms[j].y, norms[j].z};
			}
		}
		else if (hasCoords)
		{
			for (int j = 0; j < vertexCount; j++)
			{
				// Vertices
				meshes[i].vertices[j].pos = {verts[j].x, verts[j].y, verts[j].z};

				// Texture coordinates
				meshes[i].vertices[j].texCoord = {uvs[j].x, uvs[j].y};

				// Vertex colors
				meshes[i].vertices[j].color = {1, 1, 1};

				// Normals
				meshes[i].vertices[j].normal = {0, 0, 0};
			}
		}
		else
		{
			for (int j = 0; j < vertexCount; j++)
			{
				// Vertices
				meshes[i].vertices[j].pos = {verts[j].x, verts[j].y, verts[j].z};

				// Texture coordinates
				meshes[i].vertices[j].texCoord = {0, 0};

				// Vertex colors
				meshes[i].vertices[j].color = {1, 1, 1};

				// Normals
				meshes[i].vertices[j].normal = {0, 0, 0};
			}
		}

		// Setup face indices
		const int* indices = geom->getFaceIndices();
		for (int j = 0; j < indexCount; j++)
		{
			// Copy each index (the mesh was triangulated on import)
			meshes[i].indices[j] = (indices[j] > 0) ? indices[j] : ~indices[j];
		}

		// Register textures for late-loading (and generate texture Ids)
		const ofbx::Material* mat = mesh->getMaterial(0);

		// TODO: Change this for proper texture fetch logic
		// Get the number of textures
		uint32_t textureCounts = 1; // mat->GetTextureCount(aiTextureType_DIFFUSE);
		meshes[i].texturesCount = textureCounts;
		meshes[i].textureIds = new uint32_t[textureCounts];

		// List each texture on the texturesToLoad list and hold texture ids
		for (uint32_t tId = 0; tId < textureCounts; tId++)
		{
			const ofbx::Texture* tex = mat->getTexture((ofbx::Texture::TextureType)tId);
			if (tex == nullptr)
				continue;

			int textureId = 0;
			char texPath[128];
			tex->getRelativeFileName().toString(texPath);

			// Check if the texture is listed and set it's list id
			bool listed = false;
			for (auto it = texturesToLoad.begin(); it != texturesToLoad.end(); it++)
			{
				if (strcmp(it->data(), texPath) == 0)
				{
					listed = true;
					break;
				}

				// Make sure to update textureId
				textureId++;
			}

			// Hold textureId
			meshes[i].textureIds[tId] = textureId;

			// List texture if it isn't already
			if (!listed)
			{
				texturesToLoad.push_back(texPath);
			}
		}

		loadBones(mesh, meshes[i]);
	}

	// fmt::print(stdout, "Loaded file with {0} animations.\n", scene->mNumAnimations);

	// meshes[0].animations.reserve(scene->mNumAnimations);
	// if (scene->mNumAnimations > 0)
	// {
	// 	//Record animation parameters
	// 	for (uint32_t i = 0; i < scene->mNumAnimations; i++)
	// 	{
	// 		meshes[0].animations.push_back(new RvAnimation({ scene->mAnimations[i] }));
	// 	}

	// 	//Set current animation
	// 	meshes[0].curAnimId = 0;
	// }
	// meshes[0].rootNode = new aiNode(*scene->mRootNode);

	// Return success
	return true;
}

void Ravine::loadBones(const ofbx::Mesh* mesh, RvSkinnedMeshColored& meshData)
{
	const auto geometry = mesh->getGeometry();
	const ofbx::Skin* skin = geometry->getSkin();

	if (skin == nullptr || IsMeshInvalid(mesh))
		continue;

	for (int clusterIndex = 0, c = skin->getClusterCount(); clusterIndex < c; clusterIndex++)
	{
		const ofbx::Cluster* cluster = skin->getCluster(clusterIndex);

		if (cluster->getIndicesCount() == 0)
			continue;

		const auto link = cluster->getLink();
		ASSERT(link != nullptr);

		// Create bone if missing
		int32 boneIndex = data.FindBone(link);
		if (boneIndex == -1)
		{
			// Find the node where the bone is mapped
			int32 nodeIndex = data.FindNode(link);
			if (nodeIndex == -1)
			{
				nodeIndex = data.FindNode(String(link->name), StringSearchCase::IgnoreCase);
				if (nodeIndex == -1)
				{
					LOG(Warning, "Invalid mesh bone linkage. Mesh: {0}, bone: {1}. Skipping...",
					    String(mesh->name), String(link->name));
					continue;
				}
			}

			// Add bone
			boneIndex = data.Bones.Count();
			data.Bones.EnsureCapacity(Math::Max(128, boneIndex + 16));
			data.Bones.Resize(boneIndex + 1);
			auto& bone = data.Bones[boneIndex];

			// Setup bone
			bone.NodeIndex = nodeIndex;
			bone.ParentBoneIndex = -1;
			bone.FbxObj = link;
			bone.OffsetMatrix = GetOffsetMatrix(data, aMesh, link);
			bone.OffsetMatrix.Invert();

			// Mirror offset matrices (RH to LH)
			if (data.ConvertRH)
			{
				auto& m = bone.OffsetMatrix;
				m.M13 = -m.M13;
				m.M23 = -m.M23;
				m.M43 = -m.M43;
				m.M31 = -m.M31;
				m.M32 = -m.M32;
				m.M34 = -m.M34;
			}
		}
	}

	/*
		

		// 

		// Sample Animation Data
		{
			// Skip if animation is not ready to use
			if (anim == nullptr || !anim->IsLoaded())
				return Value::Null;
			PROFILE_CPU_ASSET(anim);
			const float oldTimePos = prevTimePos;

			// Calculate actual time position within the animation node (defined by length and loop mode)
			const float pos = GetAnimPos(newTimePos, startTimePos, loop, length);
			const float prevPos = GetAnimPos(prevTimePos, startTimePos, loop, length);

			// Get animation position (animation track position for channels sampling)
			const float animPos = GetAnimSamplePos(length, anim, pos, speed);
			const float animPrevPos = GetAnimSamplePos(length, anim, prevPos, speed);

			// Sample the animation
			const auto nodes = node->GetNodes(this);
			nodes->RootMotion = RootMotionData::Identity;
			nodes->Position = pos;
			nodes->Length = length;
			const auto mapping = anim->GetMapping(_graph.BaseModel);
			const auto emptyNodes = GetEmptyNodes();
			for (int32 i = 0; i < nodes->Nodes.Count(); i++)
			{
				const int32 nodeToChannel = mapping->At(i);
				nodes->Nodes[i] = emptyNodes->Nodes[i];
				if (nodeToChannel != -1)
				{
					// Calculate the animated node transformation
					anim->Data.Channels[nodeToChannel].Evaluate(animPos, &nodes->Nodes[i], false);
				}
			}
		}
		
		// Sample Animation Data Blended 2 Clips
		Variant AnimGraphExecutor::SampleAnimationsWithBlend(AnimGraphNode* node, bool loop, float length, float startTimePos, float prevTimePos, float& newTimePos, Animation* animA, Animation* animB, float speedA, float speedB, float alpha)
		{
			// Skip if any animation is not ready to use
			if (animA == nullptr || !animA->IsLoaded() ||
				animB == nullptr || !animB->IsLoaded())
				return Value::Null;

			// Calculate actual time position within the animation node (defined by length and loop mode)
			const float pos = GetAnimPos(newTimePos, startTimePos, loop, length);
			const float prevPos = GetAnimPos(prevTimePos, startTimePos, loop, length);

			// Get animation position (animation track position for channels sampling)
			const float animPosA = GetAnimSamplePos(length, animA, pos, speedA);
			const float animPrevPosA = GetAnimSamplePos(length, animA, prevPos, speedA);
			const float animPosB = GetAnimSamplePos(length, animB, pos, speedB);
			const float animPrevPosB = GetAnimSamplePos(length, animB, prevPos, speedB);

			// Sample the animations with blending
			const auto nodes = node->GetNodes(this);
			nodes->RootMotion = RootMotionData::Identity;
			nodes->Position = pos;
			nodes->Length = length;
			const auto mappingA = animA->GetMapping(_graph.BaseModel);
			const auto mappingB = animB->GetMapping(_graph.BaseModel);
			const auto emptyNodes = GetEmptyNodes();
			RootMotionData rootMotionA, rootMotionB;
			int32 rootNodeIndexA = -1, rootNodeIndexB = -1;
			if (_rootMotionMode != RootMotionMode::NoExtraction)
			{
				rootMotionA = rootMotionB = RootMotionData::Identity;
				if (animA->Data.EnableRootMotion)
					rootNodeIndexA = GetRootNodeIndex(animA);
				if (animB->Data.EnableRootMotion)
					rootNodeIndexB = GetRootNodeIndex(animB);
			}
			for (int32 i = 0; i < nodes->Nodes.Count(); i++)
			{
				const int32 nodeToChannelA = mappingA->At(i);
				const int32 nodeToChannelB = mappingB->At(i);
				Transform nodeA = emptyNodes->Nodes[i];
				Transform nodeB = nodeA;

				// Calculate the animated node transformations
				if (nodeToChannelA != -1)
				{
					animA->Data.Channels[nodeToChannelA].Evaluate(animPosA, &nodeA, false);
					if (rootNodeIndexA == i)
						ExtractRootMotion(mappingA, rootNodeIndexA, animA, animPosA, animPrevPosA, nodeA, rootMotionA);
				}
				if (nodeToChannelB != -1)
				{
					animB->Data.Channels[nodeToChannelB].Evaluate(animPosB, &nodeB, false);
					if (rootNodeIndexB == i)
						ExtractRootMotion(mappingB, rootNodeIndexB, animB, animPosB, animPrevPosB, nodeB, rootMotionB);
				}

				// Blend
				Transform::Lerp(nodeA, nodeB, alpha, nodes->Nodes[i]);
			}

			// Handle root motion
			if (_rootMotionMode != RootMotionMode::NoExtraction)
			{
				RootMotionData::Lerp(rootMotionA, rootMotionB, alpha, nodes->RootMotion);
			}

			return nodes;
		}

		// Setup GPU Data
		void AnimatedModel::PreInitSkinningData()
		{
			ASSERT(SkinnedModel && SkinnedModel->IsLoaded());

			ScopeLock lock(SkinnedModel->Locker);

			SetupSkinningData();
			auto& skeleton = SkinnedModel->Skeleton;
			const int32 bonesCount = skeleton.Bones.Count();
			const int32 nodesCount = skeleton.Nodes.Count();

			// Get nodes global transformations for the initial pose
			GraphInstance.NodesPose.Resize(nodesCount, false);
			for (int32 nodeIndex = 0; nodeIndex < nodesCount; nodeIndex++)
			{
				Matrix localTransform;
				skeleton.Nodes[nodeIndex].LocalTransform.GetWorld(localTransform);
				const int32 parentIndex = skeleton.Nodes[nodeIndex].ParentIndex;
				if (parentIndex != -1)
					GraphInstance.NodesPose[nodeIndex] = localTransform * GraphInstance.NodesPose[parentIndex];
				else
					GraphInstance.NodesPose[nodeIndex] = localTransform;
			}
			GraphInstance.Invalidate();
			GraphInstance.RootTransform = skeleton.Nodes[0].LocalTransform;

			// Setup bones transformations including bone offset matrix
			Array<Matrix> identityMatrices; // TODO: use shared memory?
			identityMatrices.Resize(bonesCount, false);
			for (int32 boneIndex = 0; boneIndex < bonesCount; boneIndex++)
			{
				auto& bone = skeleton.Bones[boneIndex];
				identityMatrices[boneIndex] = bone.OffsetMatrix * GraphInstance.NodesPose[bone.NodeIndex];
			}
			_skinningData.SetData(identityMatrices.Get(), true);

			UpdateBounds();
			UpdateSockets();
		}

		// Update data to GPU
		void AnimatedModel::OnAnimationUpdated_Async()
		{
			// Update asynchronous stuff
			auto& skeleton = SkinnedModel->Skeleton;

			// Copy pose from the master
			if (_masterPose && _masterPose->SkinnedModel->Skeleton.Nodes.Count() == skeleton.Nodes.Count())
			{
				ANIM_GRAPH_PROFILE_EVENT("Copy Master Pose");
				const auto& masterInstance = _masterPose->GraphInstance;
				GraphInstance.NodesPose = masterInstance.NodesPose;
				GraphInstance.RootTransform = masterInstance.RootTransform;
				GraphInstance.RootMotion = masterInstance.RootMotion;
			}

			// Calculate the final bones transformations and update skinning
			{
				ANIM_GRAPH_PROFILE_EVENT("Final Pose");
				const int32 bonesCount = skeleton.Bones.Count();
				Matrix3x4* output = (Matrix3x4*)_skinningData.Data.Get();
				ASSERT(_skinningData.Data.Count() == bonesCount * sizeof(Matrix3x4));
				for (int32 boneIndex = 0; boneIndex < bonesCount; boneIndex++)
				{
					auto& bone = skeleton.Bones[boneIndex];
					Matrix matrix = bone.OffsetMatrix * GraphInstance.NodesPose[bone.NodeIndex];
					output[boneIndex].SetMatrixTranspose(matrix);
				}
				_skinningData.OnDataChanged(!PerBoneMotionBlur);
			}

			UpdateBounds();
			_blendShapes.Update(SkinnedModel.Get());
		}
	*/
}

// void Ravine::boneTransform(double timeInSeconds, vector<aiMatrix4x4>& transforms)
// {
// 	const aiMatrix4x4 identity;
// 	uint16_t otherindex = (meshes[0].curAnimId + 1) % meshes[0].animations.size();
// 	double animDuration = meshes[0].animations[meshes[0].curAnimId]->aiAnim->mDuration;
// 	double otherDuration = meshes[0].animations[otherindex]->aiAnim->mDuration;
// 	runTime += RvTime::deltaTime() * (animDuration / otherDuration * animInterpolation + 1.0 * (1.0 -
// animInterpolation)); 	readNodeHierarchy(runTime, animDuration, otherDuration, meshes[0].rootNode, identity);

// 	transforms.resize(meshes[0].numBones);

// 	for (uint16_t i = 0; i < meshes[0].numBones; i++) {
// 		transforms[i] = meshes[0].boneInfo[i].FinalTransformation;
// 	}
// }

// void Ravine::readNodeHierarchy(double animationTime, double curDuration, double otherDuration, const aiNode* pNode,
// 	const aiMatrix4x4& parentTransform)
// {
// 	string nodeName(pNode->mName.data);

// 	aiMatrix4x4 nodeTransformation(pNode->mTransformation);

// 	uint16_t otherIndex = (meshes[0].curAnimId + 1) % meshes[0].animations.size();
// 	const aiNodeAnim* pNodeAnim = findNodeAnim(meshes[0].animations[meshes[0].curAnimId]->aiAnim, nodeName);
// 	const aiNodeAnim* otherNodeAnim = findNodeAnim(meshes[0].animations[otherIndex]->aiAnim, nodeName);

// 	double otherAnimTime = animationTime * curDuration / otherDuration;

// 	double ticksPerSecond = meshes[0].animations[meshes[0].curAnimId]->aiAnim->mTicksPerSecond;
// 	double TimeInTicks = animationTime * ticksPerSecond;
// 	double animationTickTime = std::fmod(TimeInTicks, curDuration);

// 	ticksPerSecond = meshes[0].animations[otherIndex]->aiAnim->mTicksPerSecond;
// 	double otherTimeInTicks = otherAnimTime * ticksPerSecond;
// 	double otherAnimationTime = std::fmod(otherTimeInTicks, otherDuration);

// 	if (pNodeAnim)
// 	{
// 		// Get interpolated matrices between current and next frame
// 		aiMatrix4x4 matScale = interpolateScale(animInterpolation, animationTickTime, otherAnimationTime,
// pNodeAnim, otherNodeAnim); 		aiMatrix4x4 matRotation = interpolateRotation(animInterpolation,
// animationTickTime, otherAnimationTime, pNodeAnim, otherNodeAnim); 		aiMatrix4x4 matTranslation =
// interpolateTranslation(animInterpolation, animationTickTime, otherAnimationTime, pNodeAnim, otherNodeAnim);

// 		nodeTransformation = matTranslation * matRotation * matScale;
// 	}

// 	aiMatrix4x4 globalTransformation = parentTransform * nodeTransformation;

// 	if (meshes[0].boneMapping.find(nodeName) != meshes[0].boneMapping.end())
// 	{
// 		uint32_t BoneIndex = meshes[0].boneMapping[nodeName];
// 		meshes[0].boneInfo[BoneIndex].FinalTransformation = meshes[0].animGlobalInverseTransform *
// 			globalTransformation * meshes[0].boneInfo[BoneIndex].BoneOffset;
// 	}

// 	for (uint32_t i = 0; i < pNode->mNumChildren; i++)
// 	{
// 		readNodeHierarchy(animationTime, curDuration, otherDuration, pNode->mChildren[i], globalTransformation);
// 	}
// }

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
		    meshes[i].vertices, sizeof(RvMeshColored) * meshes[i].vertexCount, sizeof(RvMeshColored),
		    (VkBufferUsageFlagBits)(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT),
		    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));
		delete[] meshes[i].vertices;
		meshes[i].vertexCount = 0;
	}
}

void Ravine::createIndexBuffer()
{
	indexBuffers.reserve(meshesCount);
	for (size_t i = 0; i < meshesCount; i++)
	{
		indexBuffers.push_back(device->createPersistentBuffer(
		    meshes[i].indices, sizeof(uint32_t) * meshes[i].indexCount, sizeof(uint32_t),
		    (VkBufferUsageFlagBits)(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT),
		    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));
		delete[] meshes[i].indices;
		meshes[i].indexCount = 0;
	}
}

void Ravine::createUniformBuffers()
{
	// Setting size of uniform buffers vector to count of SwapChain's images.
	size_t framesCount = swapChain->images.size();
	globalBuffers.resize(framesCount);
	materialsBuffers.resize(framesCount * meshesCount);
	modelsBuffers.resize(framesCount * meshesCount);
	animationsBuffers.resize(framesCount * meshesCount);

	for (size_t i = 0; i < framesCount; i++)
	{
		globalBuffers[i] =
		    device->createDynamicBuffer(sizeof(RvGlobalBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
						(VkMemoryPropertyFlagBits)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
									   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));
		for (size_t j = 0; j < meshesCount; j++)
		{
			size_t frameOffset = i * meshesCount;
			materialsBuffers[frameOffset + j] = device->createDynamicBuffer(
			    sizeof(RvMaterialBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			    (VkMemoryPropertyFlagBits)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
						       VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));
			modelsBuffers[frameOffset + j] = device->createDynamicBuffer(
			    sizeof(RvModelBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			    (VkMemoryPropertyFlagBits)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
						       VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));
			animationsBuffers[frameOffset + j] = device->createDynamicBuffer(
			    sizeof(RvBoneBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			    (VkMemoryPropertyFlagBits)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
						       VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));
		}
	}
}

void Ravine::loadTextureImages()
{
	// Allocate RvTexture(s)
	texturesSize = 1;
	texturesSize += texturesToLoad.size(); // Normal plus undefined texture
	textures = new RvTexture[texturesSize];

	// Generate Pink 2x2 image for missing texture
	unsigned char* pinkTexture = new unsigned char[16]; // 2x2 = 4 pixels <= 4 * RGBA = 4 * 4 char = 32 char
	for (uint32_t i = 0; i < 4; i++)
	{
		pinkTexture[i * 4 + 0] = 255; // Red
		pinkTexture[i * 4 + 1] = 0;   // Green
		pinkTexture[i * 4 + 2] = 144; // Blue
		pinkTexture[i * 4 + 3] = 255; // Alpha
	}
	textures[0] = device->createTexture(pinkTexture, 2, 2);

	for (uint32_t i = 1; i < texturesSize; i++)
	{
		// Loading image
		int texWidth, texHeight, texChannels;

		fmt::print(stdout, "{0}\n", texturesToLoad[i - 1].c_str());
		stbi_uc* pixels = stbi_load(("../data/" + texturesToLoad[i - 1]).c_str(), &texWidth, &texHeight,
					    &texChannels, STBI_rgb_alpha);

		textures[i] = device->createTexture(pixels, texWidth, texHeight);

		stbi_image_free(pixels);
	}
}

void Ravine::createTextureSampler()
{
	VkSamplerCreateInfo samplerCreateInfo = {};
	samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

	// Sampler interpolation filters
	samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.minFilter = VK_FILTER_LINEAR;

	// Address mode per axis
	samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

	// Anisotropy filter
	samplerCreateInfo.anisotropyEnable = VK_TRUE;
	samplerCreateInfo.maxAnisotropy = 16;

	// Sampling beyond image with "Clamp to Border"
	samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

	// Coordinates normalization:
	// VK_TRUE: [0,texWidth]/[0,texHeight]
	// VK_FALSE: [0,1]/[0,1]
	samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;

	// Comparison function (used for shadow maps)
	samplerCreateInfo.compareEnable = VK_FALSE;
	samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	// Mipmapping
	samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerCreateInfo.mipLodBias = 0.0f; // Optional
	samplerCreateInfo.minLod = 0.0f;     // Optional
	samplerCreateInfo.maxLod = static_cast<float>(mipLevels);

	// Creating sampler
	if (vkCreateSampler(device->handle, &samplerCreateInfo, nullptr, &textureSampler) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create texture sampler!");
	}
}

void Ravine::allocateCommandBuffers()
{

	// Allocate command buffers
	primaryCmdBuffers.resize(renderPass->framebuffers.size());
	secondaryCmdBuffers.resize(renderPass->framebuffers.size());

	// Primary Command Buffers
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = device->commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)primaryCmdBuffers.size();
	if (vkAllocateCommandBuffers(device->handle, &allocInfo, primaryCmdBuffers.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate command buffers!");
	}

	// Secondary Command Buffers
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
	if (vkAllocateCommandBuffers(device->handle, &allocInfo, secondaryCmdBuffers.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate command buffers!");
	}
}

void Ravine::recordCommandBuffers(const uint32_t currentFrame)
{

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

#pragma region Secondary Command Buffers
	beginInfo.flags =
	    VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT | VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	// Setup inheritance information to provide access modifiers from RenderPass
	VkCommandBufferInheritanceInfo inheritanceInfo = {};
	inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
	inheritanceInfo.renderPass = renderPass->handle;
	// inheritanceInfo.subpass = 0;
	inheritanceInfo.occlusionQueryEnable = VK_FALSE;
	inheritanceInfo.framebuffer = renderPass->framebuffers[currentFrame];
	inheritanceInfo.pipelineStatistics = 0;
	beginInfo.pInheritanceInfo = &inheritanceInfo;

	// Begin recording Command Buffer
	if (vkBeginCommandBuffer(secondaryCmdBuffers[currentFrame], &beginInfo) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to begin recording command buffer!");
	}

	// Basic Drawing Commands
	// Reference: https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Command_buffers#page_Basic_drawing_commands
	const size_t setsPerFrame = 1 + (meshesCount * 2);
	const VkDeviceSize offsets[] = {0};

	if (staticSolidPipelineEnabled)
	{
		// Bind Correct Graphics Pipeline
		vkCmdBindPipeline(secondaryCmdBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS,
				  *staticGraphicsPipeline);

		// Global, Material and Model Descriptor Sets
		vkCmdBindDescriptorSets(secondaryCmdBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS,
					staticGraphicsPipeline->layout, 0, 1,
					&descriptorSets[currentFrame * setsPerFrame], 0, nullptr);

		// Call drawing
		for (size_t meshIndex = 0; meshIndex < meshesCount; meshIndex++)
		{
			const size_t meshSetOffset = meshIndex * 2;
			vkCmdBindDescriptorSets(secondaryCmdBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS,
						staticGraphicsPipeline->layout, 1, 2,
						&descriptorSets[currentFrame * setsPerFrame + meshSetOffset + 1], 0,
						nullptr);

			vkCmdBindVertexBuffers(secondaryCmdBuffers[currentFrame], 0, 1,
					       &vertexBuffers[meshIndex].handle, offsets);
			vkCmdBindIndexBuffer(secondaryCmdBuffers[currentFrame], indexBuffers[meshIndex].handle, 0,
					     VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(secondaryCmdBuffers[currentFrame],
					 static_cast<uint32_t>(indexBuffers[meshIndex].instancesCount), 1, 0, 0, 0);
		}
	}

	if (staticWiredPipelineEnabled)
	{
		// Bind Correct Graphics Pipeline
		vkCmdBindPipeline(secondaryCmdBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS,
				  *staticWireframeGraphicsPipeline);

		// Global, Material and Model Descriptor Sets
		vkCmdBindDescriptorSets(secondaryCmdBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS,
					staticWireframeGraphicsPipeline->layout, 0, 1,
					&descriptorSets[currentFrame * setsPerFrame], 0, nullptr);

		// Call drawing
		for (size_t meshIndex = 0; meshIndex < meshesCount; meshIndex++)
		{
			const size_t meshSetOffset = meshIndex * 2;
			vkCmdBindDescriptorSets(secondaryCmdBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS,
						staticWireframeGraphicsPipeline->layout, 1, 2,
						&descriptorSets[currentFrame * setsPerFrame + meshSetOffset + 1], 0,
						nullptr);

			vkCmdBindVertexBuffers(secondaryCmdBuffers[currentFrame], 0, 1,
					       &vertexBuffers[meshIndex].handle, offsets);
			vkCmdBindIndexBuffer(secondaryCmdBuffers[currentFrame], indexBuffers[meshIndex].handle, 0,
					     VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(secondaryCmdBuffers[currentFrame],
					 static_cast<uint32_t>(indexBuffers[meshIndex].instancesCount), 1, 0, 0, 0);
		}
	}

	if (skinnedSolidPipelineEnabled)
	{
		// // Bind Correct Graphics Pipeline
		// vkCmdBindPipeline(secondaryCmdBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS,
		// 		  *skinnedGraphicsPipeline);

		// // Global, Material and Model Descriptor Sets
		// vkCmdBindDescriptorSets(secondaryCmdBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS,
		// 			skinnedGraphicsPipeline->layout, 0, 1,
		// 			&descriptorSets[currentFrame * setsPerFrame], 0, nullptr);

		// // Call drawing
		// for (size_t meshIndex = 0; meshIndex < meshesCount; meshIndex++)
		// {
		// 	const size_t meshSetOffset = meshIndex * 2;
		// 	vkCmdBindDescriptorSets(secondaryCmdBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS,
		// 				skinnedGraphicsPipeline->layout, 1, 2,
		// 				&descriptorSets[currentFrame * setsPerFrame + meshSetOffset + 1], 0,
		// 				nullptr);

		// 	vkCmdBindVertexBuffers(secondaryCmdBuffers[currentFrame], 0, 1,
		// 			       &vertexBuffers[meshIndex].handle, offsets);
		// 	vkCmdBindIndexBuffer(secondaryCmdBuffers[currentFrame], indexBuffers[meshIndex].handle, 0,
		// 			     VK_INDEX_TYPE_UINT32);
		// 	vkCmdDrawIndexed(secondaryCmdBuffers[currentFrame],
		// 			 static_cast<uint32_t>(indexBuffers[meshIndex].instancesCount), 1, 0, 0, 0);
		// }
	}

	if (skinnedSolidPipelineEnabled)
	{
		// // Bind Correct Graphics Pipeline
		// vkCmdBindPipeline(secondaryCmdBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS,
		// 		  *skinnedGraphicsPipeline);

		// // Global, Material and Model Descriptor Sets
		// vkCmdBindDescriptorSets(secondaryCmdBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS,
		// 			skinnedGraphicsPipeline->layout, 0, 1,
		// 			&descriptorSets[currentFrame * setsPerFrame], 0, nullptr);

		// // Call drawing
		// for (size_t meshIndex = 0; meshIndex < meshesCount; meshIndex++)
		// {
		// 	const size_t meshSetOffset = meshIndex * 2;
		// 	vkCmdBindDescriptorSets(secondaryCmdBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS,
		// 				skinnedGraphicsPipeline->layout, 1, 2,
		// 				&descriptorSets[currentFrame * setsPerFrame + meshSetOffset + 1], 0,
		// 				nullptr);

		// 	vkCmdBindVertexBuffers(secondaryCmdBuffers[currentFrame], 0, 1,
		// 			       &vertexBuffers[meshIndex].handle, offsets);
		// 	vkCmdBindIndexBuffer(secondaryCmdBuffers[currentFrame], indexBuffers[meshIndex].handle, 0,
		// 			     VK_INDEX_TYPE_UINT32);
		// 	vkCmdDrawIndexed(secondaryCmdBuffers[currentFrame],
		// 			 static_cast<uint32_t>(indexBuffers[meshIndex].instancesCount), 1, 0, 0, 0);
		// }
	}

	if (skinnedWiredPipelineEnabled)
	{
		// // Perform the same with wireframe rendering
		// vkCmdBindPipeline(secondaryCmdBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS,
		// 		  *skinnedWireframeGraphicsPipeline);

		// // Global, Material and Model Descriptor Sets
		// vkCmdBindDescriptorSets(secondaryCmdBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS,
		// 			skinnedWireframeGraphicsPipeline->layout, 0, 1,
		// 			&descriptorSets[currentFrame * setsPerFrame], 0, nullptr);

		// for (size_t meshIndex = 0; meshIndex < meshesCount; meshIndex++)
		// {
		// 	const size_t meshSetOffset = meshIndex * 2;
		// 	vkCmdBindDescriptorSets(secondaryCmdBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS,
		// 				skinnedWireframeGraphicsPipeline->layout, 1, 2,
		// 				&descriptorSets[currentFrame * setsPerFrame + meshSetOffset + 1], 0,
		// 				nullptr);

		// 	vkCmdBindVertexBuffers(secondaryCmdBuffers[currentFrame], 0, 1,
		// 			       &vertexBuffers[meshIndex].handle, offsets);
		// 	vkCmdBindIndexBuffer(secondaryCmdBuffers[currentFrame], indexBuffers[meshIndex].handle, 0,
		// 			     VK_INDEX_TYPE_UINT32);
		// 	vkCmdDrawIndexed(secondaryCmdBuffers[currentFrame],
		// 			 static_cast<uint32_t>(indexBuffers[meshIndex].instancesCount), 1, 0, 0, 0);
		// }
	}

	// Stop recording Command Buffer
	if (vkEndCommandBuffer(secondaryCmdBuffers[currentFrame]) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to record command buffer!");
	}
#pragma endregion

#pragma region Primary Command Buffers
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	// Begin recording command buffer
	if (vkBeginCommandBuffer(primaryCmdBuffers[currentFrame], &beginInfo) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to begin recording command buffer!");
	}

	// Starting a Render Pass
	// Reference: https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Command_buffers#page_Starting_a_render_pass
	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPass->handle;
	renderPassInfo.framebuffer = renderPass->framebuffers[currentFrame];
	renderPassInfo.renderArea.offset = {0, 0};
	renderPassInfo.renderArea.extent = swapChain->extent;

	// Clearing values
	array<VkClearValue, 2> clearValues = {};
	clearValues[0].color = {0.0f, 0.0f, 0.3f, 1.0f};
	clearValues[1].depthStencil = {1.0f, 0}; // Depth goes from [1,0] - being 1 the furthest possible
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(primaryCmdBuffers[currentFrame], &renderPassInfo,
			     VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

	// Execute Skinned Mesh Pipeline - Secondary Command Buffer
	vkCmdExecuteCommands(primaryCmdBuffers[currentFrame], 1, &secondaryCmdBuffers[currentFrame]);

	// Execute GUI Pipeline - Secondary Command Buffer
	vkCmdExecuteCommands(primaryCmdBuffers[currentFrame], 1, &gui->cmdBuffers[currentFrame]);

	// Finishing Render Pass
	vkCmdEndRenderPass(primaryCmdBuffers[currentFrame]);

	// Stop recording Command Buffer
	if (vkEndCommandBuffer(primaryCmdBuffers[currentFrame]) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to record command buffer!");
	}
#pragma endregion
}

void Ravine::mainLoop()
{

	string fpsTitle;

	// Application
	setupFpsCam();

	while (!glfwWindowShouldClose(*window))
	{
		RvTime::update();

		glfwSetWindowTitle(*window, "Ravine 1.0a");

		glfwPollEvents();

		drawFrame();
	}

	vkDeviceWaitIdle(device->handle);
}

template <int N>
static void toString(ofbx::DataView view, char (&out)[N])
{
	int len = int(view.end - view.begin);
	if (len > sizeof(out) - 1)
		len = sizeof(out) - 1;
	strncpy_s(out, (const char*)view.begin, len);
	out[len] = 0;
}

template <int N>
static void catProperty(char (&out)[N], const ofbx::IElementProperty* prop)
{
	char tmp[128];
	switch (prop->getType())
	{
	case ofbx::IElementProperty::DOUBLE:
		sprintf_s(tmp, "%f", prop->getValue().toDouble());
		break;
	case ofbx::IElementProperty::LONG:
		sprintf_s(tmp, "%" PRId64, prop->getValue().toU64());
		break;
	case ofbx::IElementProperty::INTEGER:
		sprintf_s(tmp, "%d", prop->getValue().toInt());
		break;
	case ofbx::IElementProperty::STRING:
		prop->getValue().toString(tmp);
		break;
	default:
		sprintf_s(tmp, "Type: %c", (char)prop->getType());
		break;
	}
	strcat_s(out, tmp);
}

template <typename T>
static void showArray(const char* label, const char* format, ofbx::IElementProperty* prop)
{
	if (!ImGui::CollapsingHeader(label))
		return;

	int count = prop->getCount();
	ImGui::Text("Count: %d", count);
	std::vector<T> tmp;
	tmp.resize(count);
	prop->getValues(&tmp[0], int(sizeof(tmp[0]) * tmp.size()));
	for (T v : tmp)
	{
		ImGui::Text(format, v);
	}
}

void Ravine::showFbxGUI(ofbx::IElementProperty* prop)
{
	ImGui::PushID((void*)prop);
	char tmp[256];
	switch (prop->getType())
	{
	case ofbx::IElementProperty::LONG:
		ImGui::Text("Long: %" PRId64, prop->getValue().toU64());
		break;
	case ofbx::IElementProperty::FLOAT:
		ImGui::Text("Float: %f", prop->getValue().toFloat());
		break;
	case ofbx::IElementProperty::DOUBLE:
		ImGui::Text("Double: %f", prop->getValue().toDouble());
		break;
	case ofbx::IElementProperty::INTEGER:
		ImGui::Text("Integer: %d", prop->getValue().toInt());
		break;
	case ofbx::IElementProperty::ARRAY_FLOAT:
		showArray<float>("float array", "%f", prop);
		break;
	case ofbx::IElementProperty::ARRAY_DOUBLE:
		showArray<double>("double array", "%f", prop);
		break;
	case ofbx::IElementProperty::ARRAY_INT:
		showArray<int>("int array", "%d", prop);
		break;
	case ofbx::IElementProperty::ARRAY_LONG:
		showArray<ofbx::u64>("long array", "%" PRId64, prop);
		break;
	case ofbx::IElementProperty::STRING:
		toString(prop->getValue(), tmp);
		ImGui::Text("String: %s", tmp);
		break;
	default:
		ImGui::Text("Other: %c", (char)prop->getType());
		break;
	}

	ImGui::PopID();
	if (prop->getNext())
		showFbxGUI(prop->getNext());
}

void Ravine::showFbxGUI(const ofbx::IElement* parent)
{
	for (const ofbx::IElement* element = parent->getFirstChild(); element; element = element->getSibling())
	{
		auto id = element->getID();
		char label[128];
		id.toString(label);
		strcat_s(label, " (");
		ofbx::IElementProperty* prop = element->getFirstProperty();
		bool first = true;
		while (prop)
		{
			if (!first)
				strcat_s(label, ", ");
			first = false;
			catProperty(label, prop);
			prop = prop->getNext();
		}
		strcat_s(label, ")");

		ImGui::PushID((const void*)id.begin);
		ImGuiTreeNodeFlags flags = selectedElement == element ? ImGuiTreeNodeFlags_Selected : 0;
		if (!element->getFirstChild())
			flags |= ImGuiTreeNodeFlags_Leaf;
		if (ImGui::TreeNodeEx(label, flags))
		{
			if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0))
				selectedElement = element;
			if (element->getFirstChild())
				showFbxGUI(element);
			ImGui::TreePop();
		}
		else
		{
			if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0))
				selectedElement = element;
		}
		ImGui::PopID();
	}
}

static void showCurveGUI(const ofbx::Object* object)
{
	const ofbx::AnimationCurve* curve = static_cast<const ofbx::AnimationCurve*>(object);

	const int c = curve->getKeyCount();
	for (int i = 0; i < c; ++i)
	{
		const float t = (float)ofbx::fbxTimeToSeconds(curve->getKeyTime()[i]);
		const float v = curve->getKeyValue()[i];
		ImGui::Text("%fs: %f ", t, v);
	}
}

void Ravine::showObjectGUI(const ofbx::Object* object)
{
	const char* label;
	switch (object->getType())
	{
	case ofbx::Object::Type::GEOMETRY:
		label = "geometry";
		break;
	case ofbx::Object::Type::MESH:
		label = "mesh";
		break;
	case ofbx::Object::Type::MATERIAL:
		label = "material";
		break;
	case ofbx::Object::Type::ROOT:
		label = "root";
		break;
	case ofbx::Object::Type::TEXTURE:
		label = "texture";
		break;
	case ofbx::Object::Type::NULL_NODE:
		label = "null";
		break;
	case ofbx::Object::Type::LIMB_NODE:
		label = "limb node";
		break;
	case ofbx::Object::Type::NODE_ATTRIBUTE:
		label = "node attribute";
		break;
	case ofbx::Object::Type::CLUSTER:
		label = "cluster";
		break;
	case ofbx::Object::Type::SKIN:
		label = "skin";
		break;
	case ofbx::Object::Type::ANIMATION_STACK:
		label = "animation stack";
		break;
	case ofbx::Object::Type::ANIMATION_LAYER:
		label = "animation layer";
		break;
	case ofbx::Object::Type::ANIMATION_CURVE:
		label = "animation curve";
		break;
	case ofbx::Object::Type::ANIMATION_CURVE_NODE:
		label = "animation curve node";
		break;
	default:
		assert(false);
		break;
	}

	ImGuiTreeNodeFlags flags = selectedObject == object ? ImGuiTreeNodeFlags_Selected : 0;
	char tmp[128];
	sprintf_s(tmp, "%" PRId64 " %s (%s)", object->id, object->name, label);
	if (ImGui::TreeNodeEx(tmp, flags))
	{
		if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0))
			selectedObject = object;
		int i = 0;
		while (ofbx::Object* child = object->resolveObjectLink(i))
		{
			showObjectGUI(child);
			++i;
		}
		if (object->getType() == ofbx::Object::Type::ANIMATION_CURVE)
		{
			showCurveGUI(object);
		}

		ImGui::TreePop();
	}
	else
	{
		if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0))
			selectedObject = object;
	}
}

void Ravine::showObjectsGUI(const ofbx::IScene* scene)
{
	if (!ImGui::Begin("Objects"))
	{
		ImGui::End();
		return;
	}
	const ofbx::Object* root = scene->getRoot();
	if (root)
		showObjectGUI(root);

	int count = scene->getAnimationStackCount();
	for (int i = 0; i < count; ++i)
	{
		const ofbx::Object* stack = scene->getAnimationStack(i);
		showObjectGUI(stack);
	}

	ImGui::End();
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
		lastFps = "FPS - " + to_string(RvTime::framesPerSecond());
	}
	ImGui::TextUnformatted(lastFps.c_str());
	if (ImGui::MenuItem("Exit Ravine", 0, false))
	{
		glfwSetWindowShouldClose(*window, true);
	}
	ImGui::EndMainMenuBar();

	if (showPipelinesMenu)
	{
		if (ImGui::Begin("Configurations Menu", &showPipelinesMenu, {400, 300}, -1,
				 ImGuiWindowFlags_NoCollapse))
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

	if (scene)
	{
		if (ImGui::Begin("Elements"))
		{
			const ofbx::IElement* root = scene->getRootElement();
			if (root && root->getFirstChild())
				showFbxGUI(root);
		}
		ImGui::End();

		if (ImGui::Begin("Properties") && selectedElement)
		{
			ofbx::IElementProperty* prop = selectedElement->getFirstProperty();
			if (prop)
				showFbxGUI(prop);
		}
		ImGui::End();

		showObjectsGUI(scene);
	}
}

void Ravine::drawFrame()
{
	// Reference: https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Rendering_and_presentation
	// Should perform the following operations:
	//	- Acquire an image from the swap chain
	//	- Execute the command buffer with that image as attachment in the framebuffer
	//	- Return the image to the swap chain for presentation
	// As such operations are performed asynchronously, we must use sync them, either with fences or semaphores:
	// Fences are best fitted to syncronize the application itself with the renderization operations, while
	// semaphores are used to syncronize operations within or across command queues - thus our best fit.

	// Handle resize and such
	if (window->framebufferResized)
	{
		swapChain->framebufferResized = true;
		window->framebufferResized = false;
	}
	uint32_t frameIndex;
	if (!swapChain->acquireNextFrame(frameIndex))
	{
		recreateSwapChain();
	}

	// //Update bone transforms
	// if (!meshes[0].animations.empty()) {
	// 	boneTransform(RvTime::elapsedTime(), meshes[0].boneTransforms);
	// }

	// Start GUI recording
	gui->acquireFrame();
	// DRAW THE GUI HERE

	drawGuiElements();

	// BUT NOT AFTER HERE
	gui->submitFrame();

	// Update GUI Buffers
	gui->updateBuffers(frameIndex);

	// Record GUI Draw Commands into CMD Buffers
	gui->recordCmdBuffers(frameIndex);

	// Update the uniforms for the given frame
	updateUniformBuffer(frameIndex);

	// Make sure to record all new Commands
	recordCommandBuffers(frameIndex);

	if (!swapChain->submitNextFrame(primaryCmdBuffers.data(), frameIndex))
	{
		recreateSwapChain();
	}
}

void Ravine::setupFpsCam()
{
	// Enable caching of buttons pressed
	glfwSetInputMode(*window, GLFW_STICKY_MOUSE_BUTTONS, GLFW_TRUE);

	// Update lastMousePos to avoid initial offset not null
	glfwGetCursorPos(*window, &lastMouseX, &lastMouseY);

	// Initial rotations
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

		// Delta Mouse Positions
		float deltaX = static_cast<float>(mouseX - lastMouseX);
		float deltaY = static_cast<float>(mouseY - lastMouseY);

		float deltaTime = static_cast<float>(RvTime::deltaTime());
		// Calculate look rotation update
		camera->horRot -= deltaX * 32.0f * deltaTime;
		camera->verRot -= deltaY * 32.0f * deltaTime;

		// Limit vertical angle
		camera->verRot = F_MAX(F_MIN(89.9f, camera->verRot), -89.9f);

		// Define rotation quaternion starting form look rotation
		lookRot = glm::rotate(lookRot, glm::radians(camera->horRot), glm::vec3(0, 1, 0));
		lookRot = glm::rotate(lookRot, glm::radians(camera->verRot), glm::vec3(1, 0, 0));

		// Calculate translation
		if (glfwGetKey(*window, GLFW_KEY_W) == GLFW_PRESS)
			translation.z -= 2.0f * deltaTime;

		if (glfwGetKey(*window, GLFW_KEY_A) == GLFW_PRESS)
			translation.x -= 2.0f * deltaTime;

		if (glfwGetKey(*window, GLFW_KEY_S) == GLFW_PRESS)
			translation.z += 2.0f * deltaTime;

		if (glfwGetKey(*window, GLFW_KEY_D) == GLFW_PRESS)
			translation.x += 2.0f * deltaTime;

		if (glfwGetKey(*window, GLFW_KEY_Q) || glfwGetKey(*window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
			translation.y -= 2.0f * deltaTime;

		if (glfwGetKey(*window, GLFW_KEY_E) || glfwGetKey(*window, GLFW_KEY_SPACE) == GLFW_PRESS)
			translation.y += 2.0f * deltaTime;
	}
	else if (shiftDown)
	{
		shiftDown = false;
		glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}

	// Update Last Mouse coordinates
	lastMouseX = mouseX;
	lastMouseY = mouseY;

	// ANIM INTERPOL
	if (glfwGetKey(*window, GLFW_KEY_UP) == GLFW_PRESS)
	{
		animInterpolation += 0.001f;
		if (animInterpolation > 1.0f)
		{
			animInterpolation = 1.0f;
		}
		fmt::print(stdout, "{0}\n", animInterpolation);
	}
	if (glfwGetKey(*window, GLFW_KEY_DOWN) == GLFW_PRESS)
	{
		animInterpolation -= 0.001f;
		if (animInterpolation < 0.0f)
		{
			animInterpolation = 0.0f;
		}
		fmt::print(stdout, "{0}\n", animInterpolation);
	}
	// SWAP ANIMATIONS
	if (glfwGetKey(*window, GLFW_KEY_RIGHT) == GLFW_PRESS && !keyUpPressed)
	{
		keyUpPressed = true;
		// meshes[0].curAnimId = (meshes[0].curAnimId + 1) % meshes[0].animations.size();
	}
	if (glfwGetKey(*window, GLFW_KEY_RIGHT) == GLFW_RELEASE)
	{
		keyUpPressed = false;
	}
	if (glfwGetKey(*window, GLFW_KEY_LEFT) == GLFW_PRESS && !keyDownPressed)
	{
		keyDownPressed = true;
		// meshes[0].curAnimId = (meshes[0].curAnimId - 1) % meshes[0].animations.size();
	}
	if (glfwGetKey(*window, GLFW_KEY_LEFT) == GLFW_RELEASE)
	{
		keyDownPressed = false;
	}

	camera->Translate(lookRot * translation);

#pragma endregion

#pragma region Global Uniforms
	RvGlobalBufferObject ubo = {};

	// Make the view matrix
	ubo.view = camera->GetViewMatrix();

	// Projection matrix with FOV of 45 degrees
	ubo.proj = glm::perspective(glm::radians(45.0f), swapChain->extent.width / (float)swapChain->extent.height,
				    0.1f, 200.0f);

	ubo.camPos = camera->pos;
	ubo.lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

	// Flipping coordinates (because glm was designed for openGL, with fliped Y coordinate)
	ubo.proj[1][1] *= -1;

	// Transfering uniform data to uniform buffer
	void* data;
	vkMapMemory(device->handle, globalBuffers[currentFrame].memory, 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(device->handle, globalBuffers[currentFrame].memory);
#pragma endregion

	// TODO: Change this to a per-material basis instead of per-mesh
	for (size_t meshId = 0; meshId < meshesCount; meshId++)
	{
#pragma region Materials
		RvMaterialBufferObject materialsUbo = {};

		materialsUbo.customColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

		void* materialsData;
		vkMapMemory(device->handle, materialsBuffers[currentFrame * meshesCount + meshId].memory, 0,
			    sizeof(materialsUbo), 0, &materialsData);
		memcpy(materialsData, &materialsUbo, sizeof(materialsUbo));
		vkUnmapMemory(device->handle, materialsBuffers[currentFrame * meshesCount + meshId].memory);
#pragma endregion

#pragma region Models
		RvModelBufferObject modelsUbo = {};

		// Model matrix updates
		modelsUbo.model = glm::translate(glm::mat4(1.0f), uniformPosition);
		modelsUbo.model =
		    glm::rotate(modelsUbo.model, glm::radians(uniformRotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
		modelsUbo.model =
		    glm::rotate(modelsUbo.model, glm::radians(uniformRotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		modelsUbo.model =
		    glm::rotate(modelsUbo.model, glm::radians(uniformRotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		modelsUbo.model = glm::scale(modelsUbo.model, uniformScale);

		// Transfering model data to gpu buffer
		void* modelData;
		vkMapMemory(device->handle, modelsBuffers[currentFrame * meshesCount + meshId].memory, 0,
			    sizeof(modelsUbo), 0, &modelData);
		memcpy(modelData, &modelsUbo, sizeof(modelsUbo));
		vkUnmapMemory(device->handle, modelsBuffers[currentFrame * meshesCount + meshId].memory);
#pragma endregion

#pragma region Animations
		RvBoneBufferObject bonesUbo = {};

		// for (size_t i = 0; i < meshes[0].boneTransforms.size(); i++)
		// {
		// 	bonesUbo.transformMatrixes[i] = glm::transpose(glm::make_mat4(&meshes[0].boneTransforms[i].a1));
		// }

		void* bonesData;
		vkMapMemory(device->handle, animationsBuffers[currentFrame * meshesCount + meshId].memory, 0,
			    sizeof(bonesUbo), 0, &bonesData);
		memcpy(bonesData, &bonesUbo, sizeof(bonesUbo));
		vkUnmapMemory(device->handle, animationsBuffers[currentFrame * meshesCount + meshId].memory);
#pragma endregion
	}
}

void Ravine::cleanup()
{
	// Cleanup RvGui data
	delete gui;

	// Hold number of swapchain images
	const size_t swapImagesCount = swapChain->images.size();

	// Cleanup render-pass
	renderPass->clear();
	delete renderPass;

	// Cleanup swap-chain related data
	swapChain->clear();
	delete swapChain;

	// Cleaning up texture related objects
	vkDestroySampler(device->handle, textureSampler, nullptr);
	for (uint32_t i = 0; i < texturesSize; i++)
	{
		textures[i].free();
	}
	delete[] textures;
	texturesSize = 0;

	// Destroy descriptor pool
	vkDestroyDescriptorPool(device->handle, descriptorPool, nullptr);

	for (size_t framesIt = 0; framesIt < swapImagesCount; framesIt++)
	{
		// Destroying global buffers
		vkDestroyBuffer(device->handle, globalBuffers[framesIt].handle, nullptr);
		vkFreeMemory(device->handle, globalBuffers[framesIt].memory, nullptr);

		for (uint32_t meshesIt = 0; meshesIt < meshesCount; meshesIt++)
		{
			const size_t instanceId = framesIt * meshesCount + meshesIt;

			// Destroying materials buffers
			vkDestroyBuffer(device->handle, materialsBuffers[instanceId].handle, nullptr);
			vkFreeMemory(device->handle, materialsBuffers[instanceId].memory, nullptr);

			// Destroying models buffers
			vkDestroyBuffer(device->handle, modelsBuffers[instanceId].handle, nullptr);
			vkFreeMemory(device->handle, modelsBuffers[instanceId].memory, nullptr);

			// Destroying animations buffers
			vkDestroyBuffer(device->handle, animationsBuffers[instanceId].handle, nullptr);
			vkFreeMemory(device->handle, animationsBuffers[instanceId].memory, nullptr);
		}
	}

	// Destroy descriptor set layout (uniform bind)
	vkDestroyDescriptorSetLayout(device->handle, globalDescriptorSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(device->handle, materialDescriptorSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(device->handle, modelDescriptorSetLayout, nullptr);

	// TODO: FIX HERE!
	for (uint32_t meshIndex = 0; meshIndex < meshesCount; meshIndex++)
	{
		// Destroy Vertex and Index Buffer Objects
		vkDestroyBuffer(device->handle, vertexBuffers[meshIndex].handle, nullptr);
		vkDestroyBuffer(device->handle, indexBuffers[meshIndex].handle, nullptr);

		// Free device memory for Vertex and Index Buffers
		vkFreeMemory(device->handle, vertexBuffers[meshIndex].memory, nullptr);
		vkFreeMemory(device->handle, indexBuffers[meshIndex].memory, nullptr);
	}

	// Destroy pipelines
	if (skinnedGraphicsPipeline)
		delete skinnedGraphicsPipeline;
	if (skinnedWireframeGraphicsPipeline)
		delete skinnedWireframeGraphicsPipeline;
	if (staticGraphicsPipeline)
		delete staticGraphicsPipeline;
	if (staticWireframeGraphicsPipeline)
		delete staticWireframeGraphicsPipeline;
	if (staticLineGraphicsPipeline)
		delete staticLineGraphicsPipeline;

	// Stop GLSLang
	glslang::FinalizeProcess();

	// Destroy vulkan logical device and validation layer
	device->clear();
	delete device;

#ifdef VALIDATION_LAYERS_ENABLED
	rvDebug::destroyDebugCallback(instance);
#endif

	if (scene)
		scene->destroy();

	// Destroy VK surface and instance
	delete window;
	vkDestroyInstance(instance, nullptr);

	// Finish GLFW
	glfwTerminate();
}