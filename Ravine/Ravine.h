#pragma once

//GLFW Includes
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

//STD includes
#include <vector>
#include <array>

//GLM includes
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

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
	glm::vec2 pos;
	glm::vec3 color;

	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions = {};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, color);

		return attributeDescriptions;
	}

};

class Ravine
{

public:
	Ravine();
	~Ravine();

	void run() {
		initWindow();
		initVulkan();
		mainLoop();
		cleanup();
	}

private:

	const int WIDTH = 1920;
	const int HEIGHT = 1080;
	const int MAX_FRAMES_IN_FLIGHT = 2;


	const std::vector<Vertex> vertices = {
			{ { -0.5f, +0.5f },{ 0.0f, 1.0f, 0.0f } },
			{ { -0.5f, -0.5f },{ 1.0f, 1.0f, 1.0f } },
			{ {	+0.5f, +0.5f },{ 0.0f, 0.0f, 1.0f } },
			{ {	+0.5f, +0.5f },{ 0.0f, 0.0f, 1.0f } },
			{ { -0.5f, -0.5f },{ 0.0f, 0.0f, 1.0f } },
			{ { +0.5f, -0.5f },{ 0.0f, 0.0f, 1.0f } }
	};


#pragma region Attributes
	//Window/Surface related contents
	GLFWwindow * window;
	VkSurfaceKHR surface;

	//Vulkan Instance
	VkInstance instance;

	//Debug Callback Handler
	VkDebugReportCallbackEXT callback;

	//Physical and Logical device handles
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device;

	//Queues
	VkQueue graphicsQueue;
	VkQueue presentQueue;

	//Swap chain related content
	VkSwapchainKHR swapChain;
	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	std::vector<VkImageView> swapChainImageViews;

	//Framebuffers
	std::vector<VkFramebuffer> swapChainFramebuffers;

	//Shader Modules
	VkShaderModule vertShaderModule;
	VkShaderModule fragShaderModule;

	//Pipeline related content
	VkRenderPass renderPass;
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;

	//Commands Buffers and it's Pool
	VkCommandPool commandPool;
	std::vector<VkCommandBuffer> commandBuffers;

	//Queues semaphors
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;

	//Queue fences
	std::vector<VkFence> inFlightFences;

	//Current frame for swap chain and Semaphore access
	size_t currentFrame = 0;
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

	//Build graphics pipeline
	void createGraphicsPipeline();

	//Create framebuffers for drawing
	void createFramebuffers();

	//Defines command buffer pool
	void createCommandPool();

	//Create vertex command buffer
	void createVertexBuffer();

	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

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

	//Partial Cleanup of Swap Chain data
	void cleanupSwapChain();

	//Finalize
	void cleanup();

	//Debugging
	bool checkValidationLayerSupport();



#pragma endregion

#pragma region Static
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char* layerPrefix, const char* msg, void* userData);
	
	static std::vector<char> readFile(const std::string& filename);
#pragma endregion
};

