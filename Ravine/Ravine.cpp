#include "Ravine.h"

//STD Includes
#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>
#include <set>
#include <algorithm>
#include <fstream>
#include <string>

//GLM Includes
#include <glm\gtc\matrix_transform.hpp>

//STB Includes
#define STB_IMAGE_IMPLEMENTATION
#include "SingleFileLibraries\stb_image.h"

//Ravine Systems Includes
#include "Time.h"

Ravine::Ravine()
{
}


Ravine::~Ravine()
{
}

void Ravine::run()
{
	Time::initialize();
	initWindow();
	initVulkan();
	mainLoop();
	cleanup();
}

//Validation layers to be enabled
const std::vector<const char*> validationLayers = {
	"VK_LAYER_LUNARG_standard_validation"
};

//Physical Device required extensions
const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

//MOVE TO: VULKAN APP
#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

//MOVE TO: TOOLS
static void check_vk_result(VkResult err)
{
	if (err == 0) return;
	printf("VkResult %d\n", err);
	if (err < 0)
		abort();
}

//MOVE TO: DEBUG
VkResult CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback) {
	auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pCallback);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

//MOVE TO: DEBUG
void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
	if (func != nullptr) {
		func(instance, callback, pAllocator);
	}
}

#pragma region Static Methods

VKAPI_ATTR VkBool32 VKAPI_CALL Ravine::debugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char* layerPrefix, const char* msg, void* userData) {
	std::cerr << "Validation layer: " << msg << std::endl;

	return VK_FALSE;
}

std::vector<char> Ravine::readFile(const std::string& filename) {
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("Failed to open file at " + filename);
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();

	return buffer;
}

//Static method because GLFW doesn't know how to call a member function with the "this" pointer to our Ravine instance.
void Ravine::framebufferResizeCallback(GLFWwindow * window, int width, int height)
{
	auto app = reinterpret_cast<Ravine*>(glfwGetWindowUserPointer(window));
	app->framebufferResized = true;
}

#pragma endregion

void Ravine::initWindow() {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	//glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	window = glfwCreateWindow(WIDTH, HEIGHT, "Ravine", 
							  nullptr, //glfwGetPrimaryMonitor(), //Fullscreen
							  nullptr);
	//Storing Ravine pointer inside window instance
	glfwSetWindowUserPointer(window, this);
	glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

void Ravine::initVulkan() {
	//Core setup
	createInstance();
	setupDebugCallback();
	createSurface();
	pickPhysicalDevice();
	createLogicalDevice();

	//Rendering pipeline
	swapChain = new RvSwapChain(device, physicalDevice, surface, WIDTH, HEIGHT, NULL);
	swapChain->CreateImageViews();
	createRenderPass();
	createDescriptorSetLayout();
	createGraphicsPipeline();
	createCommandPool();
	createMultiSamplingResources();
	createDepthResources();
	createFramebuffers();
	createTextureImage();
	createTextureImageView();
	createTextureSampler();
	createVertexBuffer();
	createIndexBuffer();
	createUniformBuffers();
	createDescriptorPool();
	createDescriptorSets();
	createCommandBuffers();
	swapChain->CreateSyncObjects();
}

std::vector<const char*> Ravine::getRequiredInstanceExtensions() {
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if (enableValidationLayers) {
		extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	}

	return extensions;
}

void Ravine::createInstance() {

	//Check validation layer support
	if (enableValidationLayers && !checkValidationLayerSupport()) {
		throw std::runtime_error("Validation layers requested, but not available!");
	}

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
	std::vector<VkExtensionProperties> extensions(extensionCount); //Reserve
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data()); //Query data
	std::cout << "Vulkan available extensions:" << std::endl;
	for (const auto& extension : extensions) {
		const char* extensionName = extension.extensionName;
		std::cout << "\t" << extension.extensionName << std::endl;
	}

	//GLFW Window Management extensions
	std::vector<const char*> requiredExtensions = getRequiredInstanceExtensions();
	std::cout << "Application required extensions:" << std::endl;
	for (uint32_t i = 0; i < requiredExtensions.size(); i++) {
		bool found = false;
		for (uint32_t j = 0; j < extensions.size(); j++) {
			if (strcmp(requiredExtensions[i], (const char*)extensions.at(j).extensionName) == 0) {
				found = true;
				break;
			}
		}
		std::cout << "\t" << requiredExtensions[i] << (found ? " found!" : " NOT found!") << std::endl;
	}
	createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
	createInfo.ppEnabledExtensionNames = requiredExtensions.data();

	//Add validation layer info
	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
		std::cout << "!Enabling validation layers!" << std::endl;
	}
	else {
		createInfo.enabledLayerCount = 0;
	}

	//Ask for an instance
	if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create instance!");
	}
}

void Ravine::setupDebugCallback()
{
	if (!enableValidationLayers)
		return;

	VkDebugReportCallbackCreateInfoEXT createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
	createInfo.pfnCallback = debugCallback;

	if (CreateDebugReportCallbackEXT(instance, &createInfo, nullptr, &callback) != VK_SUCCESS) {
		throw std::runtime_error("Failed to set up debug callback!");
	}
}

void Ravine::pickPhysicalDevice()
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	if (deviceCount == 0) {
		throw std::runtime_error("Failed to find GPUs with Vulkan support!");
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	for (const auto& device : devices) {
		if (isDeviceSuitable(device)) {
			physicalDevice = device;
			msaaSamples = getMaxUsableSampleCount();
			std::cout << "Choosen samples count: " << msaaSamples << std::endl;
			break;
		}
	}

	if (physicalDevice == VK_NULL_HANDLE) {
		throw std::runtime_error("failed to find a suitable GPU!");
	}
}

bool Ravine::isDeviceSuitable(VkPhysicalDevice device) {

	//Debug Physical Device Properties
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);
	std::cout << "Checking device: " << deviceProperties.deviceName << "\n";

	vkTools::QueueFamilyIndices indices = vkTools::findQueueFamilies(device, surface);

	bool extensionsSupported = checkDeviceExtensionSupport(device);

	bool swapChainAdequate = false;
	if (extensionsSupported) {
		SwapChainSupportDetails swapChainSupport = vkTools::querySupport(device, surface);
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	VkPhysicalDeviceFeatures supportedFeatures;
	vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

	bool isSuitable = indices.isComplete() && extensionsSupported && swapChainAdequate &&
		supportedFeatures.samplerAnisotropy; //Checking for anisotropy support
	if (isSuitable)
		std::cout << deviceProperties.deviceName << " is suitable and was selected!\n";

	return isSuitable;
}

bool Ravine::checkDeviceExtensionSupport(VkPhysicalDevice device) {
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

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
		glfwGetFramebufferSize(window, &width, &height);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(device);

	//Storing handle
	RvSwapChain *oldSwapchain = swapChain;

	swapChain = new RvSwapChain(device, physicalDevice, surface, WIDTH, HEIGHT, oldSwapchain->handle);
	swapChain->CreateImageViews();
	createRenderPass();
	createGraphicsPipeline();
	createMultiSamplingResources();
	createDepthResources();
	createFramebuffers();
	createCommandBuffers();

	//Deleting old swapchain
	delete oldSwapchain;
}


void Ravine::createRenderPass() {
	//Reference: https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Render_passes

	//Color Attachment description
	//Reference: https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Render_passes#page_Attachment_description
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = swapChain->imageFormat;	//Formats should match
	colorAttachment.samples = msaaSamples;

	//What should Vulkan do after loading or storing data to framebuffers
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	//No stencil test
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	//VKImage layout defines what is the image usage after the pass
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;		//We don't care about it's value after the pass
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;	//Multisampled images need to be resolved

	//Subpasses and attachment references
	//Reference: https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Render_passes#page_Subpasses_and_attachment_references
	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	//Multisampled image resolving 
	VkAttachmentDescription colorAttachmentResolve = {};
	colorAttachmentResolve.format = swapChain->imageFormat;
	colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentResolveRef = {};
	colorAttachmentResolveRef.attachment = 2;
	colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	//Depth attachment description
	//Reference: https://vulkan-tutorial.com/Depth_buffering#page_Render_pass
	VkAttachmentDescription depthAttachment = {};
	depthAttachment.format = findDepthFormat();
	depthAttachment.samples = msaaSamples;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	//Subpasses and attachment references
	VkAttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	//Vulkan supports compute subpasses, this is a graphics pipeline
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;
	subpass.pResolveAttachments = &colorAttachmentResolveRef;

	//Subpass Dependencies
	//Reference: https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Rendering_and_presentation#page_Subpass_dependencies
	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;

	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;

	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	//Attachments
	std::array<VkAttachmentDescription, 3> attachments = { colorAttachment, depthAttachment, colorAttachmentResolve };

	//Render Pass
	//Reference: https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Render_passes#page_Render_pass
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create render pass!");
	}

}

void Ravine::createDescriptorPool()
{
	std::array<VkDescriptorPoolSize, 3> poolSizes = {};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = static_cast<uint32_t>(swapChain->images.size());
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = static_cast<uint32_t>(swapChain->images.size());
	poolSizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[2].descriptorCount = static_cast<uint32_t>(swapChain->images.size());

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(swapChain->images.size());

	if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create descriptor pool!");
	}
}

void Ravine::createDescriptorSets()
{
	std::vector<VkDescriptorSetLayout> layouts(swapChain->images.size(), descriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(swapChain->images.size());
	allocInfo.pSetLayouts = layouts.data();

	// Setting descriptor sets vector to count of SwapChain images
	descriptorSets.resize(swapChain->images.size());
	if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate descriptor sets!");
	}

	//Configuring descriptor sets
	for (size_t i = 0; i < swapChain->images.size(); i++) {
		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = uniformBuffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(RvUniformBufferObject);

		VkDescriptorImageInfo imageInfo = {};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = textureImageView;
		imageInfo.sampler = textureSampler;

		VkDescriptorBufferInfo materialInfo = {};
		materialInfo.buffer = materialBuffers[i];
		materialInfo.offset = 0;
		materialInfo.range = sizeof(RvMaterialBufferObject);

		std::array<VkWriteDescriptorSet, 3> descriptorWrites = {};

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = descriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		//Buffer Info
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = descriptorSets[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].descriptorCount = 1;
		//Image Info
		descriptorWrites[1].pImageInfo = &imageInfo;

		descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[2].dstSet = descriptorSets[i];
		descriptorWrites[2].dstBinding = 2;
		descriptorWrites[2].dstArrayElement = 0;
		descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[2].descriptorCount = 1;
		//Buffer Info
		descriptorWrites[2].pBufferInfo = &materialInfo;

		vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}

}

void Ravine::createDescriptorSetLayout()
{
	//Uniform layout
	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	//Only relevant for image sampling related descriptors.
	uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

	//Texture Sampler layout
	VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.pImmutableSamplers = nullptr;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	//Uniform layout
	VkDescriptorSetLayoutBinding materialLayoutBinding = {};
	materialLayoutBinding.binding = 2;
	materialLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	materialLayoutBinding.descriptorCount = 1;
	materialLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	//Only relevant for image sampling related descriptors.
	materialLayoutBinding.pImmutableSamplers = nullptr; // Optional

	//Bindings array
	std::array<VkDescriptorSetLayoutBinding, 3> bindings = { uboLayoutBinding, samplerLayoutBinding, materialLayoutBinding };

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create descriptor set layout!");
	};

}

void Ravine::createGraphicsPipeline() {

	std::vector<char> vertShaderCode = readFile("../data/shaders/shader.vert.spv");
	std::vector<char> fragShaderCode = readFile("../data/shaders/shader.frag.spv");

	vertShaderModule = createShaderModule(vertShaderCode);
	fragShaderModule = createShaderModule(fragShaderCode);

	//Shader Stage creation (assign shader modules to vertex or fragment shader stages in the pipeline).
	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	//Vertex Input (describes the format of the vertex data that will be passed to the vertex shader).
	//Reference: https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Fixed_functions#page_Vertex_input
	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	auto bindingDescription = RvVertex::getBindingDescription();
	auto attributeDescriptions = RvVertex::getAttributeDescriptions();
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	//Input Assembly (describes what kind of geometry will be drawn from the vertices and if primitive restart should be enabled).
	//Reference: https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Fixed_functions#page_Input_assembly
	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	//Viewports and Scissors (describes the region of the framebuffer that the output will be rendered to)
	//Reference: https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Fixed_functions#page_Viewports_and_scissors
	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)swapChain->extent.width;
	viewport.height = (float)swapChain->extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = swapChain->extent;

	//Combine both into a Viewport State
	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	//Rasterizer (takes the geometry that is shaped by the vertices at the vertex shader and turns it into fragments)
	//Reference: https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Fixed_functions#page_Rasterizer
	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE; //VK_FALSE discards fragments outside of near/far plane frustum
	rasterizer.rasterizerDiscardEnable = VK_FALSE; //VK_TRUE discards any geometry rendered here
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL; //Could be FILL, LINE or POINT (requires GPU feature enabling)
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_NONE;
	//We're flipping glm's Y axis in the descriptor set, so we need to flip the front face
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	//Used for shadow mapping
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

	//Multisampling (anti-aliasing: combines the fragment shader results of multiple polygons that rasterize to the same pixel)
	//Reference: https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Fixed_functions#page_Multisampling
	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_TRUE; // Enable sample shading in the pipeline
	multisampling.rasterizationSamples = msaaSamples;
	multisampling.minSampleShading = 1.0f; // Min fraction for sample shading; closer to one is smooth
	multisampling.pSampleMask = nullptr; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE; // Optional

	//Depth and stencil testing (configuration of these auxiliar buffers)
	//Reference: https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Fixed_functions#page_Depth_and_stencil_testing
	//NOT UTILIZED IN THIS TUTORIAL

	//Color Blending (combines the fragment's output with the color that is already in the framebuffer)
	//Reference: https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Fixed_functions#page_Color_blending
	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f; // Optional
	colorBlending.blendConstants[1] = 0.0f; // Optional
	colorBlending.blendConstants[2] = 0.0f; // Optional
	colorBlending.blendConstants[3] = 0.0f; // Optional

	//Dynamic State (Defines the states that can be changed without recreating the graphics pipeline)
	//Reference: https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Fixed_functions#page_Dynamic_state
	VkDynamicState dynamicStates[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_LINE_WIDTH
	};

	VkPipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates = dynamicStates;

	//Pipeline Layout (Specify the uniform variables used in the pipeline)
	//Reference: https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Fixed_functions#page_Pipeline_layout
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
	pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

	//Proper Graphics Pipeline Creation
	//Reference: https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Conclusion
	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;

	//Reference all state objects
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = nullptr; // Optional
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = nullptr; // Optional

	//The uniforms layout
	pipelineInfo.layout = pipelineLayout;

	//Reference render pass and define current subpass index
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;

	//Pipeline inheritence
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex = -1; // Optional

	//Depth/Stencil state
	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;	//Depth compare function

	//Creates bounds for depth
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f; // Optional
	depthStencil.maxDepthBounds = 1.0f; // Optional

	//Settin depth/stencil state to pipeline
	pipelineInfo.pDepthStencilState = &depthStencil;

	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create graphics pipeline!");
	}

}

void Ravine::createFramebuffers() {

	//Create a framebuffer for each image view in the swapchain
	//Reference: https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Framebuffers
	swapChainFramebuffers.resize(swapChain->imageViews.size());

	for (size_t i = 0; i < swapChain->imageViews.size(); i++) {
		std::array<VkImageView, 3> attachments = {
			msColorImageView,
			depthImageView,	//Same depth image because only one subpass is being ran at a time (due to semaphores)
			swapChain->imageViews[i]
		};

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = swapChain->extent.width;
		framebufferInfo.height = swapChain->extent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create framebuffer!");
		}
	}
}

void Ravine::createCommandPool() {

	vkTools::QueueFamilyIndices queueFamilyIndices = vkTools::findQueueFamilies(physicalDevice, surface);

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
	poolInfo.flags = 0; // Optional

	if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create command pool!");
	}
}

void Ravine::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer & buffer, VkDeviceMemory & bufferMemory)
{
	//Defining buffer creation info
	VkBufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = size;
	bufferCreateInfo.usage = usage;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	//Creating buffer
	if (vkCreateBuffer(device, &bufferCreateInfo, nullptr, &buffer) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create buffer!");
	}

	//Allocating buffer memory
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate buffer memory!");
	}

	//Binding buffer memory
	vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

void Ravine::createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage & image, VkDeviceMemory & imageMemory)
{
	//Defining image creation info
	VkImageCreateInfo imageCreateInfo = {};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.extent.width = width;
	imageCreateInfo.extent.height = height;
	imageCreateInfo.extent.depth = 1;
	imageCreateInfo.mipLevels = mipLevels;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.format = format;
	imageCreateInfo.tiling = tiling;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCreateInfo.usage = usage;
	imageCreateInfo.samples = numSamples;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	//Creating image
	if (vkCreateImage(device, &imageCreateInfo, nullptr, &image) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create image!");
	}

	//Allocating image memory
	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate image memory!");
	}

	//Binding image memory
	vkBindImageMemory(device, image, imageMemory, 0);
}

VkCommandBuffer Ravine::beginSingleTimeCommands()
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;

	//TODO: We might want to have a separate command pool for this kind of short lived command buffer.
	//Reference: https://vulkan-tutorial.com/Vertex_buffers/Staging_buffer#page_Using_a_staging_buffer
	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; //Telling the driver we're only using the buffer once

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void Ravine::endSingleTimeCommands(VkCommandBuffer commandBuffer)
{
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	//The graphics queue implictly has a transfer queue
	vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	//TODO: Could use a fence instead of waiting for the queue.
	//That would allow for scheduling multiple transfers simultaneously and waiting for all to complete.
	vkQueueWaitIdle(graphicsQueue);

	vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void Ravine::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels)
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; //We're not transfering ownership
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; //So we use FAMILY_IGNORED
	barrier.image = image;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = mipLevels;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = 0; // TODO
	barrier.dstAccessMask = 0; // TODO

	//Setting aspect mask
	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		if (hasStencilComponent(format)) {
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	}
	else {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	vkCmdPipelineBarrier(
		commandBuffer,
		0 /* TODO */, 0 /* TODO */,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	//Defining pipeline stage flags
	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	//TODO: We can change that to separated functions for optimization!

	//Transition types -
	//Undefined → Transfer destination: transfer writes that don't need to wait on anything
	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		//Setting to top of pipe because it doesn't have to wait on anything
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	//Transfer destination → shader reading: shader reads should wait on transfer writes, specifically the shader reads in the fragment shader, because that's where we're going to use the texture
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	//Undefined → depth|stencil attachment
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;				//Doesn't have to wait
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;	//Phase where buffer reading occurs
	}
	//Undefined → Layout color attachment
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	}
	else {
		throw std::invalid_argument("Unsupported layout transition!");
	}

	vkCmdPipelineBarrier(
		commandBuffer,
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	endSingleTimeCommands(commandBuffer);
}

void Ravine::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkBufferImageCopy region = {};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = {
		width,
		height,
		1
	};

	vkCmdCopyBufferToImage(
		commandBuffer,
		buffer,
		image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&region
	);

	endSingleTimeCommands(commandBuffer);
}

void Ravine::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{

	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkBufferCopy copyRegion = {};
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	endSingleTimeCommands(commandBuffer);
}

bool Ravine::loadScene(const std::string& filePath)
{
	Importer importer;
	const aiScene* scene = importer.ReadFile(filePath, aiProcess_CalcTangentSpace | \
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

	// If the import failed, report it
	if (!scene)
	{
		return false;
	}

	//Load mesh
	meshesCount = scene->mNumMeshes;
	meshes = new RvMeshData[scene->mNumMeshes];

	//Load each mesh
	for (uint32_t i = 0; i < meshesCount; i++)
	{
		//Hold reference
		const aiMesh* mesh = scene->mMeshes[i];

		//Allocate data structures
		meshes[i].vertex_count = mesh->mNumVertices;
		meshes[i].vertices = new RvVertex[mesh->mNumVertices];
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
				int textureId = -1;
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
	}

	//Return success
	return true;
}

void Ravine::createVertexBuffer()
{
	//Assimp test
	if (loadScene("../data/cube.fbx"))
	{
		std::cout << "cube.fbx loaded!\n";
	}
	else
	{
		std::cout << "file not found at path " << "cube.fbx" << std::endl;
	}

	VkDeviceSize bufferSize = sizeof(RvVertex) * meshes[0].vertex_count;

	//for (size_t i = 0; i < meshes[0].vertex_count; i++)
	//{
	//	std::cout << "Vertex[" << i << "] = "
	//		<< "{ " << meshes[0].vertices[i].pos.x << " "
	//		<< ", " << meshes[0].vertices[i].pos.y << " "
	//		<< ", " << meshes[0].vertices[i].pos.z << " }"
	//		<< std::endl;
	//}

	/*
	The vertex buffer should use "VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT",
	which is a more optimal memory but it's not accessible by CPU.
	That's why we use a staging buffer to transfer vertex data to the vertex buffer.
	*/

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	//Staging buffer is being used as the source in an a memory transfer operation
	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	//Fetching verteices data
	void* data;
	vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, (void*)meshes[0].vertices, (size_t)bufferSize);
	vkUnmapMemory(device, stagingBufferMemory);

	//Verter buffer is being used as the destination for such memory transfer operation
	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

	//Transfering vertices to index buffer
	copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

	//Clearing staging buffer
	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void Ravine::createIndexBuffer()
{
	VkDeviceSize bufferSize = sizeof(uint32_t) * meshes[0].index_count;

	//for (size_t i = 0; i < meshes[0].index_count; i++)
	//{
	//	std::cout << "Vertex Id" << i << " = " << meshes[0].indices[i] << std::endl;
	//}

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	//Creating staging buffer
	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	//Feching indices data
	void* data;
	vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, (void*)meshes[0].indices, (size_t)bufferSize);
	vkUnmapMemory(device, stagingBufferMemory);

	//Creating index buffer
	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

	//Transfering indices to index buffer
	copyBuffer(stagingBuffer, indexBuffer, bufferSize);

	//Clearing staging buffer
	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void Ravine::createUniformBuffers()
{
	VkDeviceSize bufferSize = sizeof(RvUniformBufferObject);
	VkDeviceSize materialSize = sizeof(RvMaterialBufferObject);

	//Setting size of uniform buffers vector to count of SwapChain's images.
	uniformBuffers.resize(swapChain->images.size());
	uniformBuffersMemory.resize(swapChain->images.size());

	materialBuffers.resize(swapChain->images.size());
	materialBuffersMemory.resize(swapChain->images.size());

	for (size_t i = 0; i < swapChain->images.size(); i++) {
		createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);
		createBuffer(materialSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, materialBuffers[i], materialBuffersMemory[i]);
	}
}

void Ravine::createTextureImage()
{
	//Loading image
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load("../data/textures/statue.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	VkDeviceSize imageSize = texWidth * texHeight * 4;

	//Mip levels calculation
	/*
	Max - chooses biggest dimenstion
	Log2 - get how many times it can be divided by 2(two)
	Floor - handles case where the dimension is not power of 2
	+1 - Add a mip level for the original level
	*/
	mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

	if (!pixels) {
		throw std::runtime_error("Failed to load texture image!");
	}

	//Staging buffer
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	//Transfering image data
	void* data;
	vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, pixels, static_cast<size_t>(imageSize));
	vkUnmapMemory(device, stagingBufferMemory);

	stbi_image_free(pixels);

	//Creating new image
	createImage(texWidth, texHeight, mipLevels,
				VK_SAMPLE_COUNT_1_BIT,
				VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				textureImage, textureImageMemory);

	//Setting image layout for transfering to image object
	transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);

	//Transfering buffer data to image object
	copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

	//Clearing staging buffer
	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);

	//Transitioned to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL while generating mipmaps
	generateMipmaps(textureImage, VK_FORMAT_R8G8B8A8_UNORM, texWidth, texHeight, mipLevels);
}

void Ravine::createTextureImageView()
{
	textureImageView = vkTools::createImageView(device, textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
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
	if (vkCreateSampler(device, &samplerCreateInfo, nullptr, &textureSampler) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create texture sampler!");
	}
}

void Ravine::generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels)
{
	//Checking device support for linear blitting
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat, &formatProperties);

	if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
		throw std::runtime_error("Texture image format does not support linear blitting!");
	}

	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = image;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;

	int32_t mipWidth = texWidth;
	int32_t mipHeight = texHeight;

	//Starting at 1 because the level 0 is the texture image itself
	for (uint32_t i = 1; i < mipLevels; i++) {
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		//Transition waits for level (i - 1) to be filled (from Blitting or vkCmdCopyBufferToImage)
		vkCmdPipelineBarrier(commandBuffer,
							 VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
							 0, nullptr,
							 0, nullptr,
							 1, &barrier);

		//Image blit object
		VkImageBlit blit = {};
		//Blit Source
		blit.srcOffsets[0] = { 0, 0, 0 };
		blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = i - 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;
		//Blit Dest
		blit.dstOffsets[0] = { 0, 0, 0 };
		blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;

		//Blit command
		vkCmdBlitImage(commandBuffer,
					   image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					   image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					   1, &blit,
					   VK_FILTER_LINEAR);	//Filter must be the same from the sampler.
				   //TODO: Change sampler filtering to variable

		if (mipWidth > 1) mipWidth /= 2;
		if (mipHeight > 1) mipHeight /= 2;
	}

	//Changing layout here because last mipLevel is never blitted from
	barrier.subresourceRange.baseMipLevel = mipLevels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(commandBuffer,
						 VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
						 0, nullptr,
						 0, nullptr,
						 1, &barrier);

	endSingleTimeCommands(commandBuffer);
}

void Ravine::createDepthResources()
{
	VkFormat depthFormat = findDepthFormat();
	//Creating image for storing depth
	createImage(
		swapChain->extent.width, swapChain->extent.height, 1,
		msaaSamples,
		depthFormat,
		VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		depthImage, depthImageMemory);
	depthImageView = vkTools::createImageView(device, depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

	transitionImageLayout(depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);
}

VkFormat Ravine::findDepthFormat()
{
	return findSupportedFormat(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

VkSampleCountFlagBits Ravine::getMaxUsableSampleCount()
{
	VkPhysicalDeviceProperties physicalDeviceProperties;
	vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

	VkSampleCountFlags counts = std::min(physicalDeviceProperties.limits.framebufferColorSampleCounts, physicalDeviceProperties.limits.framebufferDepthSampleCounts);
	if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
	if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
	if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
	if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
	if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
	if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

	return VK_SAMPLE_COUNT_1_BIT;
}

void Ravine::createMultiSamplingResources()
{
	VkFormat colorFormat = swapChain->imageFormat;

	createImage(swapChain->extent.width, swapChain->extent.height, 1, msaaSamples, colorFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, msColorImage, msColorImageMemory);
	msColorImageView = vkTools::createImageView(device, msColorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);

	transitionImageLayout(msColorImage, colorFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1);
}

uint32_t Ravine::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type!");
}

VkFormat Ravine::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	for (VkFormat format : candidates) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
			return format;
		}
	}

	throw std::runtime_error("Failed to find supported format!");
}

bool Ravine::hasStencilComponent(VkFormat format)
{
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void Ravine::createCommandBuffers() {

	commandBuffers.resize(swapChainFramebuffers.size());

	//Allocate command buffers
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

	if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate command buffers!");
	}

	//Begining Command Buffer Recording
	//Reference: https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Command_buffers#page_Starting_command_buffer_recording
	for (size_t i = 0; i < commandBuffers.size(); i++) {
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		beginInfo.pInheritanceInfo = nullptr; // Optional

		if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("Failed to begin recording command buffer!");
		}

		//Starting a Render Pass
		//Reference: https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Command_buffers#page_Starting_a_render_pass
		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass;
		renderPassInfo.framebuffer = swapChainFramebuffers[i];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = swapChain->extent;

		//Clearing values
		std::array<VkClearValue, 2> clearValues = {};
		clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };	//Depth goes from [1,0] - being 1 the furthest possible
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		//Basic Drawing Commands
		//Reference: https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Command_buffers#page_Basic_drawing_commands
		vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

		//Set Vertex Buffer for drawing
		VkBuffer vertexBuffers[] = { vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT32);

		//Binding descriptor sets (uniforms)
		vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[i], 0, nullptr);
		//Call drawing
		vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(meshes[0].index_count), 1, 0, 0, 0);

		//Finishing Render Pass
		//Reference: https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Command_buffers#page_Finishing_up
		vkCmdEndRenderPass(commandBuffers[i]);
		if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to record command buffer!");
		}

	}

}

VkShaderModule Ravine::createShaderModule(const std::vector<char>& code) {

	//Create shader module with data pointer (uint32_t ptr) and size (in bytes)
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	//Proper creation
	VkShaderModule shaderModule;
	if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create shader module!");
	}

	return shaderModule;
}


void Ravine::createLogicalDevice()
{
	vkTools::QueueFamilyIndices indices = vkTools::findQueueFamilies(physicalDevice, surface);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<int> uniqueQueueFamilies = { indices.graphicsFamily, indices.presentFamily };

	float queuePriority = 1.0f;
	for (int queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.samplerAnisotropy = VK_TRUE; //Enabling anisotropy
	deviceFeatures.sampleRateShading = VK_TRUE; //Enable sample shading feature for the device

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();

	createInfo.pEnabledFeatures = &deviceFeatures;

	createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();

	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else {
		createInfo.enabledLayerCount = 0;
	}

	if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create logical device!");
	}

	vkGetDeviceQueue(device, indices.graphicsFamily, 0, &graphicsQueue);
	vkGetDeviceQueue(device, indices.presentFamily, 0, &presentQueue);
}

void Ravine::createSurface() {
	if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create window surface!");
	}
}

void Ravine::mainLoop() {

	std::string fpsTitle = "";

	//Application
	setupFPSCam();

	while (!glfwWindowShouldClose(window)) {
		Time::update();
		fpsTitle = "Ravine - Milisseconds " + std::to_string(Time::deltaTime() * 1000.0);
		glfwSetWindowTitle(window, fpsTitle.c_str());
		glfwPollEvents();
		drawFrame();
	}

	vkDeviceWaitIdle(device);
}


void Ravine::drawFrame()
{
	//TODO: Move this to Swap Chain

	//Reference: https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Rendering_and_presentation
	//Should perform the following operations:
	//	- Acquire an image from the swap chain
	//	- Execute the command buffer with that image as attachment in the framebuffer
	//	- Return the image to the swap chain for presentation
	//As such operations are performed asynchronously, we must use sync them, either with fences or semaphores:
	//Fences are best fitted to syncronize the application itself with the renderization operations, while
	//semaphores are used to syncronize operations within or across command queues - thus our best fit.

	//Wait for in-flight fences
	vkWaitForFences(device, 1, &swapChain->inFlightFences[currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());
	vkResetFences(device, 1, &swapChain->inFlightFences[currentFrame]);

	//Acquiring an image from the swap chain
	//Reference: https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Rendering_and_presentation#page_Acquiring_an_image_from_the_swap_chain
	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(device, *swapChain, std::numeric_limits<uint64_t>::max(), swapChain->imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		recreateSwapChain();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("Failed to acquire swap chain image!");
	}


	//TODO: Receive this from above function
	updateUniformBuffer(imageIndex);

	//TODO: Move this to Swap Chain

	//Submitting the command queue
	//Reference: https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Rendering_and_presentation#page_Submitting_the_command_buffer
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { swapChain->imageAvailableSemaphores[currentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

	VkSemaphore signalSemaphores[] = { swapChain->renderFinishedSemaphores[currentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, swapChain->inFlightFences[currentFrame]) != VK_SUCCESS) {
		throw std::runtime_error("Failed to submit draw command buffer!");
	}

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { *swapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;

	presentInfo.pResults = nullptr; // Optional

	result = vkQueuePresentKHR(presentQueue, &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
		framebufferResized = false;
		recreateSwapChain();
	}
	else if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to present swap chain image!");
	}

	currentFrame = (currentFrame + 1) % swapChain->MAX_FRAMES_IN_FLIGHT;
}

void Ravine::setupFPSCam()
{
	//Enable caching of buttons pressed
	glfwSetInputMode(window, GLFW_STICKY_MOUSE_BUTTONS, GLFW_TRUE);

	//Hide mouse cursor
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	//Update lastMousePos to avoid initial offset not null
	glfwGetCursorPos(window, &lastMouseX, &lastMouseY);

	//Initial rotations
	camHorRot = 0;
	camVerRot = 0;

}


void Ravine::updateUniformBuffer(uint32_t currentImage)
{
	//Comment on constantly changed uniforms.
	/*
	Using a UBO this way is not the most efficient way to pass frequently changing values to the shader.
	A more efficient way to pass a small buffer of data to shaders are push constants.
	Reference: https://vulkan-tutorial.com/Uniform_buffers/Descriptor_layout_and_buffer
	*/
	RvUniformBufferObject ubo = {};

#pragma region Inputs

	//Delta Mouse Positions
	glfwGetCursorPos(window, &mouseX, &mouseY);
	double deltaX = mouseX - lastMouseX;
	double deltaY = mouseY - lastMouseY;
	lastMouseX = mouseX;
	lastMouseY = mouseY;

	//Calculate look rotation update
	camHorRot -= deltaX * 30.0 * Time::deltaTime();
	camVerRot -= deltaY * 30.0 * Time::deltaTime();

	//Limit vertical angle
	camVerRot = f_max(f_min(89.9, camVerRot), -89.9);

	//Define rotation quaternion starting form look rotation
	glm::quat lookRot = glm::vec3(0, 0, 0);
	lookRot = glm::rotate(lookRot, glm::radians(camHorRot), glm::vec3(0, 1, 0));
	lookRot = glm::rotate(lookRot, glm::radians(camVerRot), glm::vec3(1, 0, 0));

	//Calculate translation
	glm::vec4 translation = glm::vec4(0);
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		translation.z -= 2.0 * Time::deltaTime();

	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		translation.x -= 2.0 * Time::deltaTime();

	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		translation.z += 2.0 * Time::deltaTime();

	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		translation.x += 2.0 * Time::deltaTime();

	if (glfwGetKey(window, GLFW_KEY_Q) || glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
		translation.y -= 2.0 * Time::deltaTime();

	if (glfwGetKey(window, GLFW_KEY_E) || glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
		translation.y += 2.0 * Time::deltaTime();

	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	camPos += lookRot * translation;

#pragma endregion

	//Rotating object 90 degrees per second
	ubo.model = glm::rotate(glm::mat4(1.0f), /*(float)Time::elapsedTime() * */glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

	//Make the view matrix
	glm::mat4 viewMatrix = glm::mat4() * glm::lookAt(glm::vec3(camPos), 
					glm::vec3(camPos) + lookRot * glm::vec3(0, 0, -1),
					glm::vec3(0, 1, 0));
	ubo.view = viewMatrix;

	//Projection matrix with FOV of 45 degrees
	ubo.proj = glm::perspective(glm::radians(45.0f), swapChain->extent.width / (float)swapChain->extent.height, 0.1f, 10.0f);

	ubo.camPos = camPos;
	ubo.objectColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	ubo.lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

	//Flipping coordinates (because glm was designed for openGL, with fliped Y coordinate)
	ubo.proj[1][1] *= -1;

	//Transfering uniform data to uniform buffer
	void* data;
	vkMapMemory(device, uniformBuffersMemory[currentImage], 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(device, uniformBuffersMemory[currentImage]);

	//Material
	RvMaterialBufferObject materialUbo = {};

	materialUbo.customColor = glm::vec4(0.23f, 0.37f, 0.89f, 1.0f);

	void* materialData;
	vkMapMemory(device, materialBuffersMemory[currentImage], 0, sizeof(materialUbo), 0, &materialData);
	memcpy(materialData, &materialUbo, sizeof(materialUbo));
	vkUnmapMemory(device, materialBuffersMemory[currentImage]);
}

void Ravine::cleanupSwapChain() {

	//Destroy multi sampling related objects
	vkDestroyImageView(device, msColorImageView, nullptr);
	vkDestroyImage(device, msColorImage, nullptr);
	vkFreeMemory(device, msColorImageMemory, nullptr);

	//Destroy depth related objects
	vkDestroyImageView(device, depthImageView, nullptr);
	vkDestroyImage(device, depthImage, nullptr);
	vkFreeMemory(device, depthImageMemory, nullptr);

	//Destroy FrameBuffers
	for (VkFramebuffer framebuffer : swapChainFramebuffers) {
		vkDestroyFramebuffer(device, framebuffer, nullptr);
	}

	vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

	//Destroy Graphics Pipeline and all it's components
	vkDestroyPipeline(device, graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
	vkDestroyRenderPass(device, renderPass, nullptr);

	//Destroy swap chain and all it's images
	swapChain->Clear();
}


void Ravine::cleanup()
{
	cleanupSwapChain();

	//Cleaning up texture related objects
	vkDestroySampler(device, textureSampler, nullptr);
	vkDestroyImageView(device, textureImageView, nullptr);
	vkDestroyImage(device, textureImage, nullptr);
	vkFreeMemory(device, textureImageMemory, nullptr);

	//Destroy descriptor pool
	vkDestroyDescriptorPool(device, descriptorPool, nullptr);

	//Destroying uniform buffers
	for (size_t i = 0; i < swapChain->images.size(); i++) {
		vkDestroyBuffer(device, uniformBuffers[i], nullptr);
		vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
	}

	//Destroy descriptor set layout (uniform bind)
	vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

	//Destroy Vertex Buffer Object
	vkDestroyBuffer(device, vertexBuffer, nullptr);

	//Free device memory from Vertex Buffer
	vkFreeMemory(device, vertexBufferMemory, nullptr);

	//Destroy Command Buffer Pool
	vkDestroyCommandPool(device, commandPool, nullptr);

	//Shader State Modules
	vkDestroyShaderModule(device, fragShaderModule, nullptr);
	vkDestroyShaderModule(device, vertShaderModule, nullptr);

	//Destroy vulkan logical device and validation layer
	vkDestroyDevice(device, nullptr);
	if (enableValidationLayers) {
		DestroyDebugReportCallbackEXT(instance, callback, nullptr);
	}

	//Destroy VK surface and instance
	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyInstance(instance, nullptr);

	//Finish GLFW
	glfwDestroyWindow(window);
	glfwTerminate();
}


bool Ravine::checkValidationLayerSupport() {
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : validationLayers) {
		bool layerFound = false;

		for (const VkLayerProperties& layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}

		if (!layerFound) {
			return false;
		}
	}

	return true;
}
