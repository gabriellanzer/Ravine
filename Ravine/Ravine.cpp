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
	rvWindowTemp->extent = { (uint32_t)width, (uint32_t)height };
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
	std::string modelName = "turtle.fbx";
	if (loadScene("../data/" + modelName))
	{
		std::cout << modelName << " loaded!\n";
	}
	else
	{
		std::cout << "file not found at path " << modelName << std::endl;
		return;
	}

	//Rendering pipeline
	swapChain = new RvSwapChain(*device, window->surface, window->extent.width, window->extent.height, NULL);
	swapChain->CreateImageViews();
	swapChain->CreateRenderPass();
	createDescriptorSetLayout();
	graphicsPipeline = new RvGraphicsPipeline(*device, swapChain->extent, device->getMaxUsableSampleCount(), descriptorSetLayout, swapChain->renderPass);
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

	vkDeviceWaitIdle(device->handle);

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
	poolSizes[1].descriptorCount = static_cast<uint32_t>(swapChain->images.size()*RV_IMAGES_COUNT);
	poolSizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[2].descriptorCount = static_cast<uint32_t>(swapChain->images.size());

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(swapChain->images.size());

	if (vkCreateDescriptorPool(device->handle, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
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
	if (vkAllocateDescriptorSets(device->handle, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate descriptor sets!");
	}

	//Configuring descriptor sets
	for (size_t i = 0; i < swapChain->images.size(); i++) {
		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = uniformBuffers[i].buffer;
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(RvUniformBufferObject);

		VkDescriptorImageInfo imageInfo[RV_IMAGES_COUNT];
		for (uint32_t j = 0; j < RV_IMAGES_COUNT; j++)
		{
			imageInfo[j].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo[j].imageView = textures[j%texturesSize].view; //TODO Fix this later!
			imageInfo[j].sampler = textureSampler;
		}

		VkDescriptorBufferInfo materialInfo = {};
		materialInfo.buffer = materialBuffers[i].buffer;
		materialInfo.offset = 0;
		materialInfo.range = sizeof(RvBoneBufferObject);

		std::array<VkWriteDescriptorSet, 3> descriptorWrites = {};

		//Buffer Info
		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = descriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		//Image Info
		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = descriptorSets[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].descriptorCount = RV_IMAGES_COUNT;
		descriptorWrites[1].pImageInfo = imageInfo;

		//Buffer Info
		descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[2].dstSet = descriptorSets[i];
		descriptorWrites[2].dstBinding = 2;
		descriptorWrites[2].dstArrayElement = 0;
		descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[2].descriptorCount = 1;
		descriptorWrites[2].pBufferInfo = &materialInfo;

		vkUpdateDescriptorSets(device->handle, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
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
	samplerLayoutBinding.descriptorCount = RV_IMAGES_COUNT;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.pImmutableSamplers = nullptr;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	//Uniform layout
	VkDescriptorSetLayoutBinding materialLayoutBinding = {};
	materialLayoutBinding.binding = 2;
	materialLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	materialLayoutBinding.descriptorCount = 1;
	materialLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	//Only relevant for image sampling related descriptors.
	materialLayoutBinding.pImmutableSamplers = nullptr; // Optional

	//Bindings array
	std::array<VkDescriptorSetLayoutBinding, 3> bindings = { uboLayoutBinding, samplerLayoutBinding, materialLayoutBinding };

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(device->handle, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create descriptor set layout!");
	};

}


static glm::mat4 AiToGLMMat4(aiMatrix4x4& in_mat)
{
	glm::mat4 tmp;
	tmp[0][0] = in_mat.a1;
	tmp[1][0] = in_mat.b1;
	tmp[2][0] = in_mat.c1;
	tmp[3][0] = in_mat.d1;

	tmp[0][1] = in_mat.a2;
	tmp[1][1] = in_mat.b2;
	tmp[2][1] = in_mat.c2;
	tmp[3][1] = in_mat.d2;

	tmp[0][2] = in_mat.a3;
	tmp[1][2] = in_mat.b3;
	tmp[2][2] = in_mat.c3;
	tmp[3][2] = in_mat.d3;

	tmp[0][3] = in_mat.a4;
	tmp[1][3] = in_mat.b4;
	tmp[2][3] = in_mat.c4;
	tmp[3][3] = in_mat.d4;
	tmp = glm::transpose(tmp);
	return tmp;
}

bool Ravine::loadScene(const std::string& filePath)
{
	Importer importer;
	importer.ReadFile(filePath, aiProcess_CalcTangentSpace | \
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
	meshes = new RvMeshData[scene->mNumMeshes];
	animGlobalInverseTransform = AiToGLMMat4(scene->mRootNode->mTransformation);
	animGlobalInverseTransform = glm::inverse(animGlobalInverseTransform);

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

	if (scene->mNumAnimations > 0) {
		ticksPerSecond = scene->mAnimations[0]->mTicksPerSecond != 0 ?
			scene->mAnimations[0]->mTicksPerSecond : 25.0f;
		animationDuration = scene->mAnimations[0]->mDuration;
		animation = scene->mAnimations[0];
	}
	rootNode = new aiNode(*scene->mRootNode);

	//Return success
	return true;
}

void Ravine::loadBones(const aiMesh* pMesh, RvMeshData& meshData)
{
	for (uint16_t i = 0; i < pMesh->mNumBones; i++) {
		uint16_t BoneIndex = 0;
		std::string BoneName(pMesh->mBones[i]->mName.data);

		if (boneMapping.find(BoneName) == boneMapping.end()) {
			BoneIndex = numBones;
			numBones++;
			BoneInfo bi;
			boneInfos.push_back(bi);
		}
		else {
			BoneIndex = boneMapping[BoneName];
		}

		boneMapping[BoneName] = BoneIndex;
		boneInfos[BoneIndex].BoneOffset = AiToGLMMat4(pMesh->mBones[i]->mOffsetMatrix);

		for (uint16_t j = 0; j < pMesh->mBones[i]->mNumWeights; j++) {
			//m_Entries[MeshIndex].BaseVertex
			uint16_t VertexID = 0 + pMesh->mBones[i]->mWeights[j].mVertexId;
			float Weight = pMesh->mBones[i]->mWeights[j].mWeight;
			meshData.vertices[VertexID].AddBoneData(BoneIndex, Weight);
		}
	}
}

void Ravine::BoneTransform(double TimeInSeconds, vector<glm::mat4x4>& Transforms)
{
	float TimeInTicks = TimeInSeconds * ticksPerSecond;
	float AnimationTime = std::fmod(TimeInTicks, animationDuration);
	const glm::mat4x4 identity;
	ReadNodeHeirarchy(AnimationTime, rootNode, identity);

	Transforms.resize(numBones);

	for (uint16_t i = 0; i < numBones; i++) {
		Transforms[i] = boneInfos[i].FinalTransformation;
	}
}

void Ravine::ReadNodeHeirarchy(double AnimationTime, const aiNode * pNode, const glm::mat4x4& ParentTransform)
{
	std::string NodeName(pNode->mName.data);
	aiString aiNodeName(pNode->mName.data);

	aiMatrix4x4 NodeTransAiMat = pNode->mTransformation;
	glm::mat4x4 NodeTransformation = AiToGLMMat4(NodeTransAiMat);
	aiNodeAnim* pNodeAnim = nullptr;
	for (size_t i = 0; i < animation->mNumChannels; i++)
	{
		if (animation->mChannels[i]->mNodeName == aiNodeName) {
			pNodeAnim = animation->mChannels[i];
			break;
		}
	}

	if (pNodeAnim) {
		// Interpolate scaling and generate scaling transformation matrix
		glm::vec3 Scaling;
		CalcInterpolatedScale(Scaling, AnimationTime, pNodeAnim);
		glm::mat4x4 ScalingM;
		ScalingM = glm::scale(ScalingM, Scaling);

		// Interpolate rotation and generate rotation transformation matrix
		glm::quat RotationQ;
		CalcInterpolatedRotation(RotationQ, AnimationTime, pNodeAnim);
		glm::mat4x4 RotationM = glm::toMat4(RotationQ);

		// Interpolate translation and generate translation transformation matrix
		glm::vec3 Translation;
		CalcInterpolatedPosition(Translation, AnimationTime, pNodeAnim);
		glm::mat4x4 TranslationM;
		TranslationM = glm::translate(TranslationM, Translation);

		// Combine the above transformations
		NodeTransformation = TranslationM * RotationM * ScalingM;
	}

	glm::mat4x4 GlobalTransformation = ParentTransform * NodeTransformation;

	if (boneMapping.find(NodeName) != boneMapping.end()) {
		uint16_t BoneIndex = boneMapping[NodeName];
		boneInfos[BoneIndex].FinalTransformation = animGlobalInverseTransform * GlobalTransformation *
			boneInfos[BoneIndex].BoneOffset;
	}

	for (uint16_t i = 0; i < pNode->mNumChildren; i++) {
		ReadNodeHeirarchy(AnimationTime, pNode->mChildren[i], GlobalTransformation);
	}
}

void Ravine::CalcInterpolatedRotation(glm::quat & Out, double AnimationTime, const aiNodeAnim * pNodeAnim)
{
	// we need at least two values to interpolate...
	if (pNodeAnim->mNumRotationKeys == 1) {
		aiQuaternion assimp_val = pNodeAnim->mRotationKeys[0].mValue;
		glm::quat val(assimp_val.w, assimp_val.x, assimp_val.y, assimp_val.z);
		Out = val;
		return;
	}

	uint16_t RotationIndex = FindRotation(AnimationTime, pNodeAnim);
	uint16_t NextRotationIndex = (RotationIndex + 1);
	assert(NextRotationIndex < pNodeAnim->mNumRotationKeys);
	float DeltaTime = pNodeAnim->mRotationKeys[NextRotationIndex].mTime - pNodeAnim->mRotationKeys[RotationIndex].mTime;
	float Factor = (AnimationTime - (float)pNodeAnim->mRotationKeys[RotationIndex].mTime) / DeltaTime;
	assert(Factor >= 0.0f && Factor <= 1.0f);
	const aiQuaternion& StartRotationQ = pNodeAnim->mRotationKeys[RotationIndex].mValue;
	const aiQuaternion& EndRotationQ = pNodeAnim->mRotationKeys[NextRotationIndex].mValue;
	glm::quat r1(StartRotationQ.w, StartRotationQ.x, StartRotationQ.y, StartRotationQ.z);
	glm::quat r2(EndRotationQ.w, EndRotationQ.x, EndRotationQ.y, EndRotationQ.z);
	glm::quat val = glm::slerp(r1, r2, Factor);
	Out = val;
}

void Ravine::CalcInterpolatedScale(glm::vec3 & Out, double AnimationTime, const aiNodeAnim * pNodeAnim)
{
	if (pNodeAnim->mNumScalingKeys == 1) {
		aiVector3D assimp_val = pNodeAnim->mScalingKeys[0].mValue;
		glm::vec3 val(assimp_val.x, assimp_val.y, assimp_val.z);
		Out = val;
		return;
	}

	uint16_t ScaleIndex = FindScale(AnimationTime, pNodeAnim);
	uint16_t NextScaleIndex = (ScaleIndex + 1);
	assert(NextScaleIndex < pNodeAnim->mNumScalingKeys);
	float DeltaTime = pNodeAnim->mScalingKeys[NextScaleIndex].mTime - pNodeAnim->mScalingKeys[ScaleIndex].mTime;
	float Factor = (AnimationTime - (float)pNodeAnim->mScalingKeys[ScaleIndex].mTime) / DeltaTime;
	assert(Factor >= 0.0f && Factor <= 1.0f);
	const aiVector3D& StartScale = pNodeAnim->mScalingKeys[ScaleIndex].mValue;
	const aiVector3D& EndScale = pNodeAnim->mScalingKeys[NextScaleIndex].mValue;
	glm::vec3 p1(StartScale.x, StartScale.y, StartScale.z);
	glm::vec3 p2(EndScale.x, EndScale.y, EndScale.z);
	glm::vec3 val = glm::mix(p1, p2, Factor);
	Out = val;
}

void Ravine::CalcInterpolatedPosition(glm::vec3 & Out, double AnimationTime, const aiNodeAnim * pNodeAnim)
{
	if (pNodeAnim->mNumPositionKeys == 1) {
		aiVector3D assimp_val = pNodeAnim->mPositionKeys[0].mValue;
		glm::vec3 val(assimp_val.x, assimp_val.y, assimp_val.z);
		Out = val;
		return;
	}

	uint16_t PosIndex = FindPosition(AnimationTime, pNodeAnim);
	uint16_t NextPosIndex = (PosIndex + 1);
	assert(NextPosIndex < pNodeAnim->mNumPositionKeys);
	float DeltaTime = pNodeAnim->mPositionKeys[NextPosIndex].mTime - pNodeAnim->mPositionKeys[PosIndex].mTime;
	float Factor = (AnimationTime - (float)pNodeAnim->mPositionKeys[PosIndex].mTime) / DeltaTime;
	assert(Factor >= 0.0f && Factor <= 1.0f);
	const aiVector3D& StartPos = pNodeAnim->mPositionKeys[PosIndex].mValue;
	const aiVector3D& EndPos = pNodeAnim->mPositionKeys[NextPosIndex].mValue;
	glm::vec3 p1(StartPos.x, StartPos.y, StartPos.z);
	glm::vec3 p2(EndPos.x, EndPos.y, EndPos.z);
	glm::vec3 val = glm::mix(p1, p2, Factor);
	Out = val;
}

uint16_t Ravine::FindRotation(double AnimationTime, const aiNodeAnim * pNodeAnim)
{
	for (uint16_t i = 0; i < pNodeAnim->mNumRotationKeys - 1; i++) {
		if (AnimationTime < (float)pNodeAnim->mRotationKeys[i + 1].mTime) {
			return i;
		}
	}
}

uint16_t Ravine::FindScale(double AnimationTime, const aiNodeAnim * pNodeAnim)
{
	for (uint16_t i = 0; i < pNodeAnim->mNumScalingKeys - 1; i++) {
		if (AnimationTime < (float)pNodeAnim->mScalingKeys[i + 1].mTime) {
			return i;
		}
	}
}

uint16_t Ravine::FindPosition(double AnimationTime, const aiNodeAnim * pNodeAnim)
{
	for (uint16_t i = 0; i < pNodeAnim->mNumPositionKeys - 1; i++) {
		if (AnimationTime < (float)pNodeAnim->mPositionKeys[i + 1].mTime) {
			return i;
		}
	}
}

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
		vertexBuffers.push_back(device->createPersistentBuffer(meshes[i].vertices, sizeof(RvVertex) * meshes[i].vertex_count, sizeof(RvVertex),
			(VkBufferUsageFlagBits)(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT), VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
		);
	}

}

void Ravine::createIndexBuffer()
{
	indexBuffers.reserve(meshesCount);
	for (size_t i = 0; i < meshesCount; i++)
	{
		indexBuffers.push_back(device->createPersistentBuffer(meshes[i].indices, sizeof(uint32_t) * meshes[i].index_count, sizeof(uint32_t),
			(VkBufferUsageFlagBits)(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT), VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
		);
	}
}

void Ravine::createUniformBuffers()
{
	//Setting size of uniform buffers vector to count of SwapChain's images.
	uniformBuffers.resize(swapChain->images.size());
	materialBuffers.resize(swapChain->images.size());

	for (size_t i = 0; i < swapChain->images.size(); i++) {
		uniformBuffers[i] = device->createDynamicBuffer(sizeof(RvUniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, (VkMemoryPropertyFlagBits)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));
		materialBuffers[i] = device->createDynamicBuffer(sizeof(RvBoneBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, (VkMemoryPropertyFlagBits)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));
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
		std::cout << texturesToLoad[i] << std::endl;
		stbi_uc* pixels = stbi_load(("../data/"+texturesToLoad[i]).c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

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


void Ravine::createCommandBuffers() {

	//Allocate command buffers
	primaryCmdBuffers.resize(swapChain->framebuffers.size());
	secondayCmdBuffers.resize(swapChain->framebuffers.size());

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
	if (vkAllocateCommandBuffers(device->handle, &allocInfo, secondayCmdBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate command buffers!");
	}

	//Begining Command Buffer Recording
	//Reference: https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Command_buffers#page_Starting_command_buffer_recording
	for (size_t i = 0; i < secondayCmdBuffers.size(); i++) {

	#pragma region Secondary Command Buffers
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		beginInfo.pInheritanceInfo = nullptr; // Optional

		//Begin recording Command Buffer
		if (vkBeginCommandBuffer(secondayCmdBuffers[i], &beginInfo) != VK_SUCCESS) {
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

		vkCmdBeginRenderPass(secondayCmdBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		//Basic Drawing Commands
		//Reference: https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Command_buffers#page_Basic_drawing_commands
		vkCmdBindPipeline(secondayCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, *graphicsPipeline);

		vkCmdBindDescriptorSets(secondayCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline->pipelineLayout, 0, 1, &descriptorSets[i], 0, nullptr);
		//Set Vertex Buffer for drawing
		//VkBuffer auxVertexBuffers[] = { vertexBuffer.handle };
		VkDeviceSize offsets[] = { 0 };

		//Binding descriptor sets (uniforms)
		//Call drawing
		for (size_t meshIndex = 0; meshIndex < meshesCount; meshIndex++)
		{
			vkCmdBindVertexBuffers(secondayCmdBuffers[i], 0, 1, &vertexBuffers[meshIndex].handle, offsets);
			vkCmdBindIndexBuffer(secondayCmdBuffers[i], indexBuffers[meshIndex].handle, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(secondayCmdBuffers[i], static_cast<uint32_t>(meshes[meshIndex].index_count), 1, 0, 0, 0);
		}

		//Finishing Render Pass
		//Reference: https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Command_buffers#page_Finishing_up
		vkCmdEndRenderPass(secondayCmdBuffers[i]);

		//Stop recording Command Buffer
		if (vkEndCommandBuffer(secondayCmdBuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to record command buffer!");
		}
	#pragma endregion

	#pragma region Primary Command Buffers

		//Begin recording command buffer
		if (vkBeginCommandBuffer(primaryCmdBuffers[i], &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("Failed to begin recording command buffer!");
		}

		//Execute Secondary Command Buffer
		vkCmdExecuteCommands(primaryCmdBuffers[i], 1, &secondayCmdBuffers[i]);

		//Stop recording Command Buffer
		if (vkEndCommandBuffer(primaryCmdBuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to record command buffer!");
		}
	#pragma endregion

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

	uint32_t imageIndex;
	if (!swapChain->AcquireNextFrame(imageIndex)) {
		recreateSwapChain();
	}
	if (animation != nullptr) {
		BoneTransform(Time::elapsedTime(), boneTransforms);
	}
	updateUniformBuffer(imageIndex);

	if (!swapChain->SubmitNextFrame(primaryCmdBuffers.data(), imageIndex)) {
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
	camera = new RvCamera(glm::vec3(5.f, 0.f, 0.f), 90.f, 0.f);

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
	camera->horRot -= deltaX * 30.0 * Time::deltaTime();
	camera->verRot -= deltaY * 30.0 * Time::deltaTime();

	//Limit vertical angle
	camera->verRot = f_max(f_min(89.9, camera->verRot), -89.9);

	//Define rotation quaternion starting form look rotation
	glm::quat lookRot = glm::vec3(0, 0, 0);
	lookRot = glm::rotate(lookRot, glm::radians(camera->horRot), glm::vec3(0, 1, 0));
	lookRot = glm::rotate(lookRot, glm::radians(camera->verRot), glm::vec3(1, 0, 0));

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

	if (glfwGetKey(*window, GLFW_KEY_R) == GLFW_PRESS) {
		camera->horRot = 90.0f;
		camera->verRot = 0.0f;
	}

	if (glfwGetKey(*window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(*window, true);

	camera->Translate(lookRot * translation);

#pragma endregion

	//Rotating object 90 degrees per second
	ubo.model = glm::rotate(glm::mat4(1.0f), /*(float)Time::elapsedTime() * */glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.model = glm::scale(ubo.model, glm::vec3(0.1f, 0.1f, 0.1f));

	//Make the view matrix
	ubo.view = camera->GetViewMatrix();

	//Projection matrix with FOV of 45 degrees
	ubo.proj = glm::perspective(glm::radians(45.0f), swapChain->extent.width / (float)swapChain->extent.height, 0.1f, 200.0f);

	ubo.camPos = camera->pos;
	ubo.objectColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	ubo.lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

	//Flipping coordinates (because glm was designed for openGL, with fliped Y coordinate)
	ubo.proj[1][1] *= -1;

	//Transfering uniform data to uniform buffer
	void* data;
	vkMapMemory(device->handle, uniformBuffers[currentImage].memory, 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(device->handle, uniformBuffers[currentImage].memory);

#pragma region Bones
	RvBoneBufferObject bonesUbo = {};

	for (size_t i = 0; i < boneTransforms.size(); i++)
	{
		bonesUbo.transformMatrixes[i] = boneTransforms[i];
	}

	void* bonesData;
	vkMapMemory(device->handle, materialBuffers[currentImage].memory, 0, sizeof(bonesUbo), 0, &bonesData);
	memcpy(bonesData, &bonesUbo, sizeof(bonesUbo));
	vkUnmapMemory(device->handle, materialBuffers[currentImage].memory);
#pragma endregion

	std::cout << camera->horRot << "; " << camera->verRot << std::endl;
}

void Ravine::cleanupSwapChain() {

	vkFreeCommandBuffers(device->handle, device->commandPool, static_cast<uint32_t>(primaryCmdBuffers.size()), primaryCmdBuffers.data());
	vkFreeCommandBuffers(device->handle, device->commandPool, static_cast<uint32_t>(secondayCmdBuffers.size()), secondayCmdBuffers.data());

	//Destroy Graphics Pipeline and all it's components
	vkDestroyPipeline(device->handle, *graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(device->handle, graphicsPipeline->pipelineLayout, nullptr);

	//Destroy swap chain and all it's images
	swapChain->Clear();
	delete swapChain;
}


void Ravine::cleanup()
{
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

	//Destroying uniform buffers
	for (size_t i = 0; i < swapChain->images.size(); i++) {
		vkDestroyBuffer(device->handle, uniformBuffers[i].buffer, nullptr);
		vkFreeMemory(device->handle, uniformBuffers[i].memory, nullptr);
	}

	for (size_t i = 0; i < swapChain->images.size(); i++) {
		vkDestroyBuffer(device->handle, materialBuffers[i].buffer, nullptr);
		vkFreeMemory(device->handle, materialBuffers[i].memory, nullptr);
	}

	//Destroy descriptor set layout (uniform bind)
	vkDestroyDescriptorSetLayout(device->handle, descriptorSetLayout, nullptr);

	//TODO: FIX HERE!
	for (size_t meshIndex = 0; meshIndex < meshesCount; meshIndex++)
	{
		//Destroy Vertex Buffer Object
		vkDestroyBuffer(device->handle, vertexBuffers[meshIndex].handle, nullptr);
		//Free device memory from Vertex Buffer
		vkFreeMemory(device->handle, vertexBuffers[meshIndex].memory, nullptr);
	}

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