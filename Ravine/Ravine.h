#pragma once

//GLFW Includes
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

//STD includes
#include <vector>
#include <array>
#include <unordered_set>

//Vulkan Include
#include <vulkan\vulkan.h>

//Vulkan Tools
#include "VulkanTools.h"

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
#include "RvDevice.h"
#include "RvSwapChain.h"
#include "RvGraphicsPipeline.h"

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

	//MOVE TO: WINDOW
	const int WIDTH = 1280;
	const int HEIGHT = 720;

	//Ravine objects
	//MOVE TO: VULKAN APP
	RvDevice* device;
	RvSwapChain* swapChain;
	RvGraphicsPipeline* graphicsPipeline;

	//Mouse parameters
	//MOVE TO: Input
	double lastMouseX = 0, lastMouseY = 0;
	double mouseX, mouseY;

	//Camera parameters
	//MOVE TO: CAMERA
	float camHorRot = 0, camVerRot = 0;
	glm::vec4 camPos = glm::vec4(0,0,0,0);
	
	//MOVE TO: MESH
	RvMeshData* meshes;
	uint32_t meshesCount;
	vector<string> texturesToLoad;

#pragma region Attributes
	//Window/Surface related contents
	//MOVE TO: WINDOW
	GLFWwindow * window;
	//MOVE TO: WINDOW
	VkSurfaceKHR surface;

	//Vulkan Instance
	//MOVE TO: VULKAN APP
	VkInstance instance;

	//Debug Callback Handler
	//MOVE TO: DEBUG
	VkDebugReportCallbackEXT callback;

	//Descriptors related content
	//MOVE TO: DESCRIPTOR
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets; //Automatically freed with descriptol pool

	//Commands Buffers and it's Pool
	//MOVE TO: COMMAND BUFFER
	std::vector<VkCommandBuffer> commandBuffers;

	//TODO: Optimally we should use a single buffer with offsets for vertices and indices
	//Reference: https://developer.nvidia.com/vulkan-memory-management

	//Verter buffer
	//MOVE TO: BUFFER
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;

	//Index buffer
	//MOVE TO: BUFFER
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

	//Uniform buffers (per swap chain)
	//MOVE TO: UNIFORM
	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;

	//MOVE TO: UNIFORM
	std::vector<VkBuffer> materialBuffers;
	std::vector<VkDeviceMemory> materialBuffersMemory;

	//Texture related objects
	//MOVE TO: TEXTURE (inherits from framebuffer?)
	uint32_t mipLevels;
	VkImage textureImage;
	VkDeviceMemory textureImageMemory;
	VkImageView textureImageView;
	VkSampler textureSampler;

	//Depth related objects
	//MOVE TO: FRAMEBUFFER
	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;

	//MSAA related objects
	//MOVE TO: RENDER PASS
	VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
	//MOVE TO: FRAMEBUFFER
	VkImage msColorImage;
	VkDeviceMemory msColorImageMemory;
	VkImageView msColorImageView;

	//Helper variable for changes on framebuffer
	//MOVE TO: WINDOW
	bool framebufferResized = false;
#pragma endregion

#pragma region Methods

	void initWindow();

	//Setup Vulkan Pipeline
	void initVulkan();

	//Returns a list of extensions required by the Vulkan Instance
	std::vector<const char*> getRequiredInstanceExtensions();

	//Create Vulkan Instance for the start of the application
	void createInstance();

	//Create DebugReport callback handler and check validation layer support
	void setupDebugCallback();

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

	//Helper function to copy a buffer's content into another
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

	//Load scene file and populates meshes vector
	bool loadScene(const std::string& filePath);

	//Create vertex buffer
	void createVertexBuffer();

	//Create index buffer
	void createIndexBuffer();

	//Create uniform buffers
	void createUniformBuffers();

	//Load image and upload into Vulkan Image Object
	void createTextureImage();

	//Creating image view to hold a texture object
	void createTextureImageView();

	//Create texture sampler - interface for extracting colors from a texture
	void createTextureSampler();

	void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);

	//Create resources for depth testing
	void createDepthResources();

	//Create resources for sampling
	void createMultiSamplingResources();

	//Creates command buffers array
	void createCommandBuffers();

	//Create a Win32 Surface handler
	void createSurface();

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

	//Debugging
	bool checkValidationLayerSupport();

#pragma endregion

#pragma region Helpers

	//Helper function for creating buffers
	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

	//Transfer buffer's data to an image
	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);


#pragma endregion

#pragma region Static
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char* layerPrefix, const char* msg, void* userData);


	static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
#pragma endregion
};

