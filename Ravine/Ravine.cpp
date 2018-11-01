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
#include "RvConfig.h"
#include "RvDebug.h"

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


#pragma region Static Methods



//Static method because GLFW doesn't know how to call a member function with the "this" pointer to our Ravine instance.
void Ravine::framebufferResizeCallback(GLFWwindow * window, int width, int height)
{
	auto rvWindowTemp = reinterpret_cast<RvWindow*>(glfwGetWindowUserPointer(window));
	rvWindowTemp->framebufferResized = true;
	rvWindowTemp->extent = {(uint32_t)width, (uint32_t)height};
}

#pragma endregion

void Ravine::initWindow() {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	window = new RvWindow(WIDTH, HEIGHT, "Ravine Engine", false, framebufferResizeCallback);
}

void Ravine::initVulkan() {
	//Core setup
	createInstance();
	rvDebug::setupDebugCallback(instance);
	window->CreateSurface(instance);
	pickPhysicalDevice();

	//Rendering pipeline
	swapChain = new RvSwapChain(*device, window->surface, window->extent.width, window->extent.height, NULL);
	swapChain->CreateImageViews();
	swapChain->CreateRenderPass();
	createDescriptorSetLayout();
	graphicsPipeline = new RvGraphicsPipeline(*device, swapChain->extent, device->getMaxUsableSampleCount(), descriptorSetLayout, swapChain->renderPass);
	createMultiSamplingResources();
	createDepthResources();
	swapChain->CreateFramebuffers();
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

	#ifdef VALIDATION_LAYERS_ENABLED
		extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	#endif

	return extensions;
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
	#ifdef VALIDATION_LAYERS_ENABLED
		createInfo.enabledLayerCount = static_cast<uint32_t>(rvCfg::ValidationLayers.size());
		createInfo.ppEnabledLayerNames = rvCfg::ValidationLayers.data();
		std::cout << "!Enabling validation layers!" << std::endl;
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

	std::vector<VkPhysicalDevice> devices(deviceCount);
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
	std::cout << "Checking device: " << deviceProperties.deviceName << "\n";

	vkTools::QueueFamilyIndices indices = vkTools::findQueueFamilies(device, window->surface);

	bool extensionsSupported = checkDeviceExtensionSupport(device);

	bool swapChainAdequate = false;
	if (extensionsSupported) {
		SwapChainSupportDetails swapChainSupport = vkTools::querySupport(device, window->surface);
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

	std::set<std::string> requiredExtensions(rvCfg::DeviceExtensions.begin(), rvCfg::DeviceExtensions.end());

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

	vkDeviceWaitIdle(*device);

	//Storing handle
	RvSwapChain *oldSwapchain = swapChain;

	swapChain = new RvSwapChain(*device, window->surface, WIDTH, HEIGHT, oldSwapchain->handle);
	swapChain->CreateImageViews();
	swapChain->CreateRenderPass();
	graphicsPipeline = new RvGraphicsPipeline(*device, swapChain->extent, device->getMaxUsableSampleCount(), descriptorSetLayout, swapChain->renderPass);
	createMultiSamplingResources();
	createDepthResources();
	swapChain->CreateFramebuffers();
	createCommandBuffers();

	//Deleting old swapchain
	delete oldSwapchain;
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

	if (vkCreateDescriptorPool(*device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
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
	if (vkAllocateDescriptorSets(*device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate descriptor sets!");
	}

	//Configuring descriptor sets
	for (size_t i = 0; i < swapChain->images.size(); i++) {
		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = uniformBuffers[0];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(RvUniformBufferObject);

		VkDescriptorImageInfo imageInfo = {};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = textureImageView;
		imageInfo.sampler = textureSampler;

		VkDescriptorBufferInfo materialInfo = {};
		materialInfo.buffer = materialBuffers[0];
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

		vkUpdateDescriptorSets(*device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
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

	if (vkCreateDescriptorSetLayout(*device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create descriptor set layout!");
	};

}

void Ravine::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
	VkCommandBuffer commandBuffer = device->beginSingleTimeCommands();

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

	device->endSingleTimeCommands(commandBuffer);
}

void Ravine::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{

	VkCommandBuffer commandBuffer = device->beginSingleTimeCommands();

	VkBufferCopy copyRegion = {};
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	device->endSingleTimeCommands(commandBuffer);
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
	vkMapMemory(*device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, (void*)meshes[0].indices, (size_t)bufferSize);
	vkUnmapMemory(*device, stagingBufferMemory);

	//Creating index buffer
	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

	//Transfering indices to index buffer
	copyBuffer(stagingBuffer, indexBuffer, bufferSize);

	//Clearing staging buffer
	vkDestroyBuffer(*device, stagingBuffer, nullptr);
	vkFreeMemory(*device, stagingBufferMemory, nullptr);
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
	vkMapMemory(*device, stagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, pixels, static_cast<size_t>(imageSize));
	vkUnmapMemory(*device, stagingBufferMemory);

	stbi_image_free(pixels);

	//Creating new image
	device->createImage(texWidth, texHeight, mipLevels,
		VK_SAMPLE_COUNT_1_BIT,
		VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		textureImage, textureImageMemory);

	//Setting image layout for transfering to image object
	vkTools::transitionImageLayout(*device, textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);

	//Transfering buffer data to image object
	copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

	//Clearing staging buffer
	vkDestroyBuffer(*device, stagingBuffer, nullptr);
	vkFreeMemory(*device, stagingBufferMemory, nullptr);

	//Transitioned to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL while generating mipmaps
	generateMipmaps(textureImage, VK_FORMAT_R8G8B8A8_UNORM, texWidth, texHeight, mipLevels);
}

void Ravine::createTextureImageView()
{
	textureImageView = vkTools::createImageView(*device, textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
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
	if (vkCreateSampler(*device, &samplerCreateInfo, nullptr, &textureSampler) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create texture sampler!");
	}
}

void Ravine::generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels)
{
	//Checking device support for linear blitting
	VkFormatProperties formatProperties = device->getFormatProperties(imageFormat);

	if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
		throw std::runtime_error("Texture image format does not support linear blitting!");
	}

	VkCommandBuffer commandBuffer = device->beginSingleTimeCommands();

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

	device->endSingleTimeCommands(commandBuffer);
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


void Ravine::createCommandBuffers() {

	commandBuffers.resize(swapChain->framebuffers.size());

	//Allocate command buffers
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = device->commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

	if (vkAllocateCommandBuffers(*device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
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
		renderPassInfo.renderPass = swapChain->renderPass;
		renderPassInfo.framebuffer = swapChain->framebuffers[i];
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
		vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, *graphicsPipeline);

		//Set Vertex Buffer for drawing
		VkBuffer vertexBuffers[] = { vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT32);

		//Binding descriptor sets (uniforms)
		vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline->pipelineLayout, 0, 1, &descriptorSets[i], 0, nullptr);
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

void Ravine::mainLoop() {

	std::string fpsTitle = "";

	//Application
	setupFPSCam();

	while (!glfwWindowShouldClose(*window)) {
		Time::update();
		fpsTitle = "Ravine - Milisseconds " + std::to_string(Time::deltaTime() * 1000.0);
		glfwSetWindowTitle(*window, fpsTitle.c_str());
		glfwPollEvents();
		drawFrame();
	}

	vkDeviceWaitIdle(*device);
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

	uint32_t imageIndex;
	if (!swapChain->AcquireNextFrame(imageIndex)) {
		recreateSwapChain();
	}
	updateUniformBuffer(imageIndex);

	if (!swapChain->SubmitNextFrame(commandBuffers.data(), imageIndex)) {
		recreateSwapChain();
	}
}

void Ravine::setupFPSCam()
{
	//Enable caching of buttons pressed
	glfwSetInputMode(*window, GLFW_STICKY_MOUSE_BUTTONS, GLFW_TRUE);

	//Hide mouse cursor
	glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	//Update lastMousePos to avoid initial offset not null
	glfwGetCursorPos(*window, &lastMouseX, &lastMouseY);

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
	glfwGetCursorPos(*window, &mouseX, &mouseY);
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
	if (glfwGetKey(*window, GLFW_KEY_W) == GLFW_PRESS)
		translation.z -= 2.0 * Time::deltaTime();

	if (glfwGetKey(*window, GLFW_KEY_A) == GLFW_PRESS)
		translation.x -= 2.0 * Time::deltaTime();

	if (glfwGetKey(*window, GLFW_KEY_S) == GLFW_PRESS)
		translation.z += 2.0 * Time::deltaTime();

	if (glfwGetKey(*window, GLFW_KEY_D) == GLFW_PRESS)
		translation.x += 2.0 * Time::deltaTime();

	if (glfwGetKey(*window, GLFW_KEY_Q) || glfwGetKey(*window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
		translation.y -= 2.0 * Time::deltaTime();

	if (glfwGetKey(*window, GLFW_KEY_E) || glfwGetKey(*window, GLFW_KEY_SPACE) == GLFW_PRESS)
		translation.y += 2.0 * Time::deltaTime();

	if (glfwGetKey(*window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(*window, true);

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
	vkMapMemory(*device, uniformBuffersMemory[currentImage], 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(*device, uniformBuffersMemory[currentImage]);

	//Material
	RvMaterialBufferObject materialUbo = {};

	materialUbo.customColor = glm::vec4(0.23f, 0.37f, 0.89f, 1.0f);

	void* materialData;
	vkMapMemory(*device, materialBuffersMemory[currentImage], 0, sizeof(materialUbo), 0, &materialData);
	memcpy(materialData, &materialUbo, sizeof(materialUbo));
	vkUnmapMemory(*device, materialBuffersMemory[currentImage]);
}

void Ravine::cleanupSwapChain() {

	vkFreeCommandBuffers(*device, device->commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

	//Destroy Graphics Pipeline and all it's components
	vkDestroyPipeline(*device, *graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(*device, graphicsPipeline->pipelineLayout, nullptr);

	//Destroy swap chain and all it's images
	swapChain->Clear();
	delete swapChain;
}


void Ravine::cleanup()
{
	cleanupSwapChain();

	//Cleaning up texture related objects
	vkDestroySampler(*device, textureSampler, nullptr);
	vkDestroyImageView(*device, textureImageView, nullptr);
	vkDestroyImage(*device, textureImage, nullptr);
	vkFreeMemory(*device, textureImageMemory, nullptr);

	//Destroy descriptor pool
	vkDestroyDescriptorPool(*device, descriptorPool, nullptr);

	//Destroying uniform buffers
	for (size_t i = 0; i < swapChain->images.size(); i++) {
		vkDestroyBuffer(*device, uniformBuffers[i], nullptr);
		vkFreeMemory(*device, uniformBuffersMemory[i], nullptr);
	}

	//Destroy descriptor set layout (uniform bind)
	vkDestroyDescriptorSetLayout(*device, descriptorSetLayout, nullptr);

	//Destroy Vertex Buffer Object
	vkDestroyBuffer(*device, vertexBuffer, nullptr);

	//Free device memory from Vertex Buffer
	vkFreeMemory(*device, vertexBufferMemory, nullptr);

	//Destroy Command Buffer Pool
	//TODO: Destroy with device?
	vkDestroyCommandPool(*device, device->commandPool, nullptr);

	//Destroy graphics pipeline
	delete graphicsPipeline;

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