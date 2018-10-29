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

//GLM includes
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

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

//Structure for Queue Family query of available queue types
struct QueueFamilyIndices {
	int graphicsFamily = -1;
	int presentFamily = -1;

	//Is this struct complete to be used?
	bool isComplete() {
		return graphicsFamily >= 0 && presentFamily >= 0;
	}
};

//Structure for Swap Chain Support query of details
struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

struct Vertex {
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoord;
	glm::vec3 normal;

	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions = {};

		//Position
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		//Color
		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, color);

		//Texture
		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

		//Texture
		attributeDescriptions[3].binding = 0;
		attributeDescriptions[3].location = 3;
		attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[3].offset = offsetof(Vertex, normal);

		return attributeDescriptions;
	}

};

struct MeshData
{
	Vertex*		vertices;
	uint32_t	vertex_count;

	uint32_t*	indices;
	uint32_t	index_count;

	uint32_t*	textureIds;
	uint32_t	textures_count;
};

//REORGANIZE AND DIVIDE
struct UniformBufferObject {
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
	glm::vec4 objectColor;
	glm::vec4 lightColor;
	glm::vec4 camPos;
};

//REORGANIZE AND DIVIDE
struct MaterialBufferObject {
	glm::vec4 customColor;
};

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
	//MOVE TO: SWAPCHAIN
	const int MAX_FRAMES_IN_FLIGHT = 2;

	//Mouse parameters
	//MOVE TO: IO
	double lastMouseX = 0, lastMouseY = 0;
	double mouseX, mouseY;

	//Camera parameters
	//MOVE TO: CAMERA
	float camHorRot = 0, camVerRot = 0;
	glm::vec4 camPos = glm::vec4(0,0,0,0);
	
	//MOVE TO: MESH
	MeshData* meshes;
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

	//Physical and Logical device handles
	//MOVED: DEVICE
	VkPhysicalDevice physicalDevice;
	//MOVED: DEVICE
	VkDevice device;

	//Queues
	//MOVE TO: DEVICE
	VkQueue graphicsQueue;
	VkQueue presentQueue;

	//Swap chain related content
	//MOVE TO: SWAPCHAIN
	VkSwapchainKHR swapChain = VK_NULL_HANDLE;
	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	std::vector<VkImageView> swapChainImageViews;

	//Framebuffers
	//MOVE TO: FRAMEBUFFER
	std::vector<VkFramebuffer> swapChainFramebuffers;

	//Shader Modules
	//MOVE TO: GRAPHICS PIPELINE
	VkShaderModule vertShaderModule;
	VkShaderModule fragShaderModule;

	//Pipeline related content
	//MOVE TO: VULKAN APP
	VkRenderPass renderPass;
	//MOVE TO: GRAPHICS PIPELINE
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;

	//Descriptors related content
	//MOVE TO: DESCRIPTOR
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets; //Automatically freed with descriptol pool

	//Commands Buffers and it's Pool
	//CHANGE NAME TO SOMETHING MORE MEANINGFUL
	//(This one should be "graphicsCommandPool")
	//MOVE TO: DEVICE
	VkCommandPool commandPool;
	//MOVE TO: COMMAND BUFFER
	std::vector<VkCommandBuffer> commandBuffers;

	//Queues semaphors
	//MOVE TO: SWAPCHAIN??
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;

	//Queue fences
	//MOVE TO: SWAPCHAIN??
	std::vector<VkFence> inFlightFences;

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
	//MOVE TO: TEXTURE
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

	//Current frame for swap chain and Semaphore access
	//MOVE TO: VULKAN APP
	size_t currentFrame = 0;

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

	//Properly create the swap chain used by the application
	void createSwapChain();

	//Recreate swap chain
	void recreateSwapChain();

	//Queries swap chain support details info
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

	//Return the ids for family queues supported on the given device
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

	//Choses the best swap chain surface format
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

	//Choses the best swap chain presentation mode (aka. vsync)
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes);

	//Choses the best swap chain image size capabilities
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

	//Create the image handles (views)
	void createImageViews();

	//Inform Vulkan of the framebuffers and their properties
	void createRenderPass();

	//Creating descriptor binding layouts (uniform layouts)
	void createDescriptorSetLayout();

	//Creating pool for descriptor sets (uniforms bindings)
	void createDescriptorPool();

	//Creating descriptors sets (uniforms bindings)
	void createDescriptorSets();

	//Build graphics pipeline
	void createGraphicsPipeline();

	//Create framebuffers for drawing
	void createFramebuffers();

	//Defines command buffer pool
	void createCommandPool();

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

	//Helper to get depth format
	VkFormat findDepthFormat();

	//Get max sampling counts
	//MOVED: DEVICE!
	VkSampleCountFlagBits getMaxUsableSampleCount();

	//Create resources for sampling
	void createMultiSamplingResources();

	//Creates command buffers array
	void createCommandBuffers();

	//Creates rendering semaphore state objects
	void createSyncObjects();

	//Wrap shader code in shader module
	VkShaderModule createShaderModule(const std::vector<char>& code);

	//Create a logical GPU device interface for suitable physical device
	void createLogicalDevice();

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

	//Helper function for creating images
	void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);

	//Helper function for craeting image views
	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);

	//Helper function to get some device info
	//MOVED: DEVICE
	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	//MOVED: DEVICE
	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

	//Checks for stencil flag
	//MOVED: TOOLS
	bool hasStencilComponent(VkFormat format);

	//Single time command buffer helpers
	//MOVE TO: BUFFER
	VkCommandBuffer beginSingleTimeCommands();
	//MOVE TO: BUFFER
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);

	//Set image to correct layout
	//MOVE TO: BUFFER
	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);

	//Transfer buffer's data to an image
	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);


#pragma endregion

#pragma region Static
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char* layerPrefix, const char* msg, void* userData);

	static std::vector<char> readFile(const std::string& filename);

	static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
#pragma endregion
};

