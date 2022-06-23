#ifndef RAVINE_H
#define RAVINE_H

// GLFW Includes
#define VK_NO_PROTOTYPES
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

// Vulkan Includes
#include "volk.h"

// STD Includes
#include "RvStdDefs.h"

// OpenFBX Includes
#include "ofbx.h"

// Ravine Includes
#include "RvAnimationTools.h"
#include "RvCamera.h"
#include "RvDataTypes.h"
#include "RvDevice.h"
#include "RvGUI.h"
#include "RvLinePipeline.h"
#include "RvPolygonPipeline.h"
#include "RvRenderPass.h"
#include "RvSwapChain.h"
#include "RvTexture.h"
#include "RvWindow.h"
#include "RvWireframePipeline.h"


// Math defines
#define F_MAX(a, b) (((a) > (b)) ? (a) : (b))
#define F_MIN(a, b) (((a) < (b)) ? (a) : (b))

// Specific usages of Ravine library
// using namespace rvTools::animation;

class Ravine
{
  public:
	Ravine();
	~Ravine();

	void run();

  private:
	// Todo: Move to Window
	const int WIDTH = 1920;
	const int HEIGHT = 1080;
	const string WINDOW_NAME = "Ravine Engine";

	// Ravine objects
	RvWindow* window = nullptr;
	// Todo: Move to VULKAN APP
	RvDevice* device = nullptr;
	RvSwapChain* swapChain = nullptr;
	RvRenderPass* renderPass = nullptr;

	// TODO: Fix Creation flow with shaders integration
	vector<char> skinnedTexColCode;
	vector<char> skinnedWireframeCode;
	vector<char> staticTexColCode;
	vector<char> staticWireframeCode;
	vector<char> phongTexColCode;
	vector<char> solidColorCode;
	RvPolygonPipeline* skinnedGraphicsPipeline = nullptr;
	RvWireframePipeline* skinnedWireframeGraphicsPipeline = nullptr;
	RvPolygonPipeline* staticGraphicsPipeline = nullptr;
	RvWireframePipeline* staticWireframeGraphicsPipeline = nullptr;
	RvLinePipeline* staticLineGraphicsPipeline = nullptr;

	// Mouse parameters
	// Todo: Move to INPUT
	double lastMouseX = 0, lastMouseY = 0;
	double mouseX, mouseY;

	// Camera
	RvCamera* camera = nullptr;

	// GUI
	RvGui* gui = nullptr;

	// PROTOTYPE PRESENTATION STUFF
	bool staticSolidPipelineEnabled = true;
	bool staticWiredPipelineEnabled = false;
	bool skinnedSolidPipelineEnabled = false;
	bool skinnedWiredPipelineEnabled = false;
	glm::vec3 uniformPosition = glm::vec3(0);
	glm::vec3 uniformScale = glm::vec3(0.01f, 0.01f, 0.01f);
	glm::vec3 uniformRotation = glm::vec3(0, 0, 0);
	// PROTOTYPE PRESENTATION STUFF

	ofbx::IScene* scene = nullptr;
	const ofbx::IElement* selectedElement = nullptr;
	const ofbx::Object* selectedObject = nullptr;

	// Todo: Move to MESH
	RvMeshColored* meshes = nullptr;
	uint32_t meshesCount = 0;
	vector<string> texturesToLoad;

	// Animation interpolation helper
	float animInterpolation = 0.0f;
	float runTime = 0.0f;

	// Helper for keyboard input
	bool keyUpPressed = false;
	bool keyDownPressed = false;

#pragma region Attributes

	// Vulkan Instance
	// TODO: Move to VULKAN APP
	VkInstance instance;

	// Descriptors related content
	VkDescriptorSetLayout globalDescriptorSetLayout;
	VkDescriptorSetLayout materialDescriptorSetLayout;
	VkDescriptorSetLayout modelDescriptorSetLayout;
	VkDescriptorPool descriptorPool;
	vector<VkDescriptorSet> descriptorSets; // Automatically freed with descriptor pool

	// Commands Buffers and it's Pool
	// TODO: Move to COMMAND BUFFER
	vector<VkCommandBuffer> primaryCmdBuffers;
	vector<VkCommandBuffer> secondaryCmdBuffers;

	// TODO: Optimally we should use a single buffer with offsets for vertices and indices
	// Reference: https://developer.nvidia.com/vulkan-memory-management

	// TODO: Move vertex and index buffer in MESH class
	// Verter buffer
	vector<RvPersistentBuffer> vertexBuffers;
	// Index buffer
	vector<RvPersistentBuffer> indexBuffers;

	// Uniform buffers (per swap chain image)
	// TODO: Move to UNIFORM
	vector<RvDynamicBuffer> globalBuffers;
	vector<RvDynamicBuffer> materialsBuffers;
	vector<RvDynamicBuffer> modelsBuffers;
	vector<RvDynamicBuffer> animationsBuffers;

	// Texture related objects
	uint32_t mipLevels;
	RvTexture* textures;
#define RV_MAX_IMAGES_COUNT 32
	size_t texturesSize;
	VkSampler textureSampler;

#pragma endregion

#pragma region Methods

	void initWindow();

	// Setup Vulkan Pipeline
	void initVulkan();

	// Create Vulkan Instance for the start of the application
	void createInstance();

	// Get a suitable device and set it up
	void pickPhysicalDevice();

	// Is this device suitable to this application needs
	bool isDeviceSuitable(VkPhysicalDevice device);

	// Checks
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);

	// Recreate swap chain
	void recreateSwapChain();

	// Creating descriptor binding layouts (uniform layouts)
	void createDescriptorSetLayout();

	// Creating pool for descriptor sets (uniforms bindings)
	void createDescriptorPool();

	// Creating descriptors sets (uniforms bindings)
	void createDescriptorSets();

	// Load scene file and populates meshes vector
	bool loadScene(const string& filePath);
	void loadBones(const ofbx::Mesh* mesh, struct RvSkinnedMeshColored& meshData);
	// //TODO: Move to Blend-tree
	// void boneTransform(double timeInSeconds, vector<aiMatrix4x4>& transforms);
	// void readNodeHierarchy(double animationTime, double curDuration, double otherDuration, const aiNode* pNode,
	// const aiMatrix4x4& parentTransform);

	// Create vertex buffer
	void createVertexBuffer();

	// Create index buffer
	void createIndexBuffer();

	// Create uniform buffers
	void createUniformBuffers();

	// Load image and upload into Vulkan Image Object
	void loadTextureImages();

	// Create texture sampler - interface for extracting colors from a texture
	void createTextureSampler();

	// Creates command buffers array
	void allocateCommandBuffers();

	// Records new draw commands
	void recordCommandBuffers(uint32_t currentFrame);

	// Main application loop
	void mainLoop();

	// Gui Calls
	void showObjectGUI(const ofbx::Object* object);
	void showObjectsGUI(const ofbx::IScene* scene);
	void showFbxGUI(ofbx::IElementProperty* prop);
	void showFbxGUI(const ofbx::IElement* parent);
	void drawGuiElements();

	// Acquires an image from the swap chain, execute command buffer, returns the image for presentation
	void drawFrame();

	// First person camera setup
	void setupFpsCam();

	// Updates uniform buffer for given image
	void updateUniformBuffer(uint32_t currentFrame);

	// Finalize
	void cleanup();

#pragma endregion

#pragma region Static

	// Returns a list of extensions required by the Vulkan Instance
	static vector<const char*> getRequiredInstanceExtensions();

	static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

#pragma endregion
};

#endif