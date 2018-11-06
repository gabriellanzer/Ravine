#pragma once

//GLFW Includes
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

//STD includes
#include <vector>
#include <array>
#include <unordered_set>
#include <map>

//Vulkan Include
#include <vulkan\vulkan.h>


//GLM includes
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

//Types dependencies
#include "RvDataTypes.h"
#include "RvUniformTypes.h"

//VK Wrappers
#include "RvTools.h"
#include "RvDevice.h"
#include "RvSwapChain.h"
#include "RvGraphicsPipeline.h"
#include "RvWindow.h"
#include "RvTexture.h"
#include "RvCamera.h"

//Math defines
#define f_max(a,b)            (((a) > (b)) ? (a) : (b))
#define f_min(a,b)            (((a) < (b)) ? (a) : (b))

//Assimp Includes
#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags

//Specific usages of STD library
using std::string;
using std::vector;

//Specific usages of ASSIMP library
using Assimp::Importer;

class Ravine
{

public:
	Ravine();
	~Ravine();

	void run();

private:

	//Todo: Move to Window
	const int WIDTH = 1280;
	const int HEIGHT = 720;
	const std::string WINDOW_NAME = "Ravine Engine";

	//Ravine objects
	RvWindow* window;
	//Todo: Move to VULKAN APP
	RvDevice* device;
	RvSwapChain* swapChain;
	RvGraphicsPipeline* graphicsPipeline;

	//Mouse parameters
	//Todo: Move to INPUT
	double lastMouseX = 0, lastMouseY = 0;
	double mouseX, mouseY;

	//Camera
	RvCamera* camera;

	const aiScene* scene;
	//Todo: Move to MESH
	RvMeshData* meshes;
	uint32_t meshesCount;
	vector<string> texturesToLoad;
	//ANIMATION STUFF
	double ticksPerSecond;
	double animationDuration;
	aiMatrix4x4 animGlobalInverseTransform;
	uint16_t numBones;
	std::map<std::string, uint16_t> boneMapping;
	std::vector<BoneInfo> boneInfos;
	aiAnimation* animation;
	aiNode* rootNode;
	std::vector<glm::mat4x4> boneTransforms;

#pragma region Attributes

	//Vulkan Instance
	//TODO: Move to VULKAN APP
	VkInstance instance;

	//Descriptors related content
	//TODO: Move to DESCRIPTOR
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets; //Automatically freed with descriptol pool

	//Commands Buffers and it's Pool
	//TODO: Move to COMMAND BUFFER
	std::vector<VkCommandBuffer> commandBuffers;

	//TODO: Optimally we should use a single buffer with offsets for vertices and indices
	//Reference: https://developer.nvidia.com/vulkan-memory-management

	//TODO: Move vertex and index buffer in MESH class
	//Verter buffer
	std::vector<RvPersistentBuffer> vertexBuffers;
	//Index buffer
	std::vector<RvPersistentBuffer> indexBuffers;

	//Uniform buffers (per swap chain image)
	//TODO: Move to UNIFORM
	std::vector<RvDynamicBuffer> uniformBuffers;

	//TODO: Move to UNIFORM
	std::vector<RvDynamicBuffer> materialBuffers;

	//Texture related objects
	uint32_t mipLevels;
	RvTexture texture;
	VkSampler textureSampler;

#pragma endregion

#pragma region Methods

	void initWindow();

	//Setup Vulkan Pipeline
	void initVulkan();

	//Returns a list of extensions required by the Vulkan Instance
	std::vector<const char*> getRequiredInstanceExtensions();

	//Create Vulkan Instance for the start of the application
	void createInstance();

	//Get a suitable device and set it up
	void pickPhysicalDevice();

	//Is this device suitable to this application needs
	bool isDeviceSuitable(VkPhysicalDevice device);

	//Checks 
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);

	//Recreate swap chain
	void recreateSwapChain();

	//Creating descriptor binding layouts (uniform layouts)
	void createDescriptorSetLayout();

	//Creating pool for descriptor sets (uniforms bindings)
	void createDescriptorPool();

	//Creating descriptors sets (uniforms bindings)
	void createDescriptorSets();

	//Load scene file and populates meshes vector
	bool loadScene(const std::string& filePath);
	void loadBones(uint16_t MeshIndex, const aiMesh* pMesh, RvMeshData& meshData);
	void BoneTransform(double TimeInSeconds, vector<glm::mat4x4>& Transforms);
	void ReadNodeHeirarchy(double AnimationTime, const aiNode* pNode, const aiMatrix4x4& ParentTransform);
	void CalcInterpolatedRotation(aiQuaternion& Out, double AnimationTime, const aiNodeAnim* pNodeAnim);
	void CalcInterpolatedScale(aiVector3D& Out, double AnimationTime, const aiNodeAnim* pNodeAnim);
	void CalcInterpolatedPosition(aiVector3D& Out, double AnimationTime, const aiNodeAnim* pNodeAnim);
	uint16_t FindRotation(double AnimationTime, const aiNodeAnim* pNodeAnim);
	uint16_t FindScale(double AnimationTime, const aiNodeAnim* pNodeAnim);
	uint16_t FindPosition(double AnimationTime, const aiNodeAnim* pNodeAnim);

	//Create vertex buffer
	void createVertexBuffer();

	//Create index buffer
	void createIndexBuffer();

	//Create uniform buffers
	void createUniformBuffers();

	//Load image and upload into Vulkan Image Object
	void createTextureImage();

	//Create texture sampler - interface for extracting colors from a texture
	void createTextureSampler();

	//Create resources for depth testing
	void createDepthResources();

	//Create resources for sampling
	void createMultiSamplingResources();

	//Creates command buffers array
	void createCommandBuffers();

	//Main application loop
	void mainLoop();

	//Acquires an image from the swap chain, execute command buffer, returns the image for presentation
	void drawFrame();

	//First person camera setup
	void setupFPSCam();

	//Updates uniform buffer for given image
	void updateUniformBuffer(uint32_t currentImage);

	//Partial Cleanup of Swap Chain data
	void cleanupSwapChain();

	//Finalize
	void cleanup();

#pragma endregion

#pragma region Static
	static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
#pragma endregion
};

