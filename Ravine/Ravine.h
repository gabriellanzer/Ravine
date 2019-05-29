#ifndef RAVINE_H
#define RAVINE_H

//GLFW Includes
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

//EASTL includes
#include <EASTL/allocator.h>
#include <EASTL/vector.h>
#include <EASTL/array.h>
#include <EASTL/unordered_set.h>
#include <EASTL/map.h>
#include <EASTL/string.h>

using eastl::string;
using eastl::vector;
using eastl::array;

//Vulkan Include
#include <vulkan\vulkan.h>

//FMT Includes
#include <fmt/printf.h>

//GLM includes
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

//Types dependencies
#include "RvDataTypes.h"
#include "RvUniformTypes.h"

//VK Wrappers
#include "RvTools.h"
#include "RvAnimationTools.h"
#include "RvDevice.h"
#include "RvSwapChain.h"
#include "RvPolygonPipeline.h"
#include "RvWireframePipeline.h"
#include "RvWindow.h"
#include "RvTexture.h"
#include "RvCamera.h"

//GUI Includes
#include "RvGUI.h"

//Math defines
#define f_max(a,b)            (((a) > (b)) ? (a) : (b))
#define f_min(a,b)            (((a) < (b)) ? (a) : (b))

//Assimp Includes
#include <assimp/scene.h>           // Output data structure

//Specific usages of Ravine library
using namespace rvTools::animation;

class Ravine
{

public:
	Ravine();
	~Ravine();

	void run();

private:

	//Todo: Move to Window
	const int WIDTH = 1920;
	const int HEIGHT = 1080;
	const string WINDOW_NAME = "Ravine Engine";

	//Ravine objects
	RvWindow* window;
	//Todo: Move to VULKAN APP
	RvDevice* device;
	RvSwapChain* swapChain;

	//TODO: Fix Creation flow with shaders integration
	vector<char> skinnedTexColCode;
	vector<char> skinnedWireframeCode;
	vector<char> staticTexColCode;
	vector<char> phongTexColCode;
	vector<char> solidColorCode;
	RvPolygonPipeline* skinnedGraphicsPipeline;
	RvWireframePipeline* skinnedWireframeGraphicsPipeline;
	RvPolygonPipeline* staticGraphicsPipeline;

	//Mouse parameters
	//Todo: Move to INPUT
	double lastMouseX = 0, lastMouseY = 0;
	double mouseX, mouseY;

	//Camera
	RvCamera* camera;

	//GUI
	RvGUI* gui;

	const aiScene* scene;
	//Todo: Move to MESH
	RvSkinnedMeshColored* meshes;
	uint32_t meshesCount;
	vector<string> texturesToLoad;

	// Animation interpolation helper
	float animInterpolation = 0.0f;
	float runTime = 0.0f;

	// Helper for keyboard input
	bool keyUpPressed = false;
	bool keyDownPressed = false;

#pragma region Attributes

	//Vulkan Instance
	//TODO: Move to VULKAN APP
	VkInstance instance;

	//Descriptors related content
	VkDescriptorSetLayout globalDescriptorSetLayout;
	VkDescriptorSetLayout materialDescriptorSetLayout;
	VkDescriptorSetLayout modelDescriptorSetLayout;
	VkDescriptorPool descriptorPool;
	vector<VkDescriptorSet> descriptorSets; //Automatically freed with descriptol pool

	//Commands Buffers and it's Pool
	//TODO: Move to COMMAND BUFFER
	vector<VkCommandBuffer> primaryCmdBuffers;
	vector<VkCommandBuffer> secondaryCmdBuffers;

	//TODO: Optimally we should use a single buffer with offsets for vertices and indices
	//Reference: https://developer.nvidia.com/vulkan-memory-management

	//TODO: Move vertex and index buffer in MESH class
	//Verter buffer
	vector<RvPersistentBuffer> vertexBuffers;
	//Index buffer
	vector<RvPersistentBuffer> indexBuffers;

	//Uniform buffers (per swap chain image)
	//TODO: Move to UNIFORM
	vector<RvDynamicBuffer> uniformBuffers;
	vector<RvDynamicBuffer> materialsBuffers;
	vector<RvDynamicBuffer> animationsBuffers;

	//Texture related objects
	uint32_t mipLevels;
	RvTexture *textures;
#define RV_IMAGES_COUNT 32
	uint32_t texturesSize;
	VkSampler textureSampler;

#pragma endregion

#pragma region Methods

	void initWindow();

	//Setup Vulkan Pipeline
	void initVulkan();

	//Returns a list of extensions required by the Vulkan Instance
	vector<const char*> getRequiredInstanceExtensions();

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
	bool loadScene(const string& filePath);
	void loadBones(const aiMesh* pMesh, RvSkinnedMeshColored& meshData);
	//TODO: Move to Blend-tree
	void BoneTransform(double TimeInSeconds, vector<aiMatrix4x4>& Transforms);
	void ReadNodeHeirarchy(double AnimationTime, double curDuration, double otherDuration, const aiNode* pNode, const aiMatrix4x4& ParentTransform);

	//Create vertex buffer
	void createVertexBuffer();

	//Create index buffer
	void createIndexBuffer();

	//Create uniform buffers
	void createUniformBuffers();

	//Load image and upload into Vulkan Image Object
	void loadTextureImages();

	//Create texture sampler - interface for extracting colors from a texture
	void createTextureSampler();

	//Create resources for depth testing
	void createDepthResources();

	//Create resources for sampling
	void createMultiSamplingResources();

	//Creates command buffers array
	void allocateCommandBuffers();

	//Records new draw commands
	void recordCommandBuffers(uint32_t currentFrame);

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

#endif