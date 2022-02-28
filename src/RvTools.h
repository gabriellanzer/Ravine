#ifndef RAVINE_TOOLS_H
#define RAVINE_TOOLS_H

// Vulkan Includes
#include "RvDynamicBuffer.h"
#include "volk.h"

//STD Includes
#include "RvStdDefs.h"

// Ravine Includes
#include "RvDevice.h"
#include "RvFramebufferAttachment.h"
#include "RvTexture.h"

// GLSLang Includes
#include "glslang/Public/ShaderLang.h"

struct RvSwapChainSupportDetails;

namespace rvTools
{

	bool hasStencilComponent(VkFormat format);

	RvTexture createTexture(RvDevice* device, void* pixels, uint32_t width, uint32_t height, VkFormat format);

	void generateMipmap(RvDevice* device, VkImage image, VkFormat imageFormat, uint32_t texWidth,
			    uint32_t texHeight, uint32_t mipLevels = 1);

	// Transfer buffer's data to an image
	void copyBufferToImage(RvDevice* device, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

	VkImageView createImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags,
				    uint32_t mipLevels = 1);

	// Structure for Queue Family query of available queue types
	// TODO: Move to Device
	struct QueueFamilyIndices
	{
		int graphicsFamily = -1;
		int presentFamily = -1;

		// Is this struct complete to be used?
		bool isComplete() const { return graphicsFamily >= 0 && presentFamily >= 0; }
	};

	// TODO: Move to Device
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);

	RvSwapChainSupportDetails querySupport(VkPhysicalDevice device, VkSurfaceKHR surface);

	vector<char> readFile(const string& filename);

	void transitionImageLayout(RvDevice device, VkImage image, VkFormat format, VkImageLayout oldLayout,
				   VkImageLayout newLayout, uint32_t mipLevels);

	void copyBuffer(RvDevice& device, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

	void copyToMemory(RvDevice& device, char* data, VkDeviceSize size, const RvDynamicBuffer& dynamicBuffer);

	vector<char> GLSLtoSPV(const int shaderStageBit, const vector<char>& pshader);

	VkShaderModule createShaderModule(VkDevice device, const vector<char>& code);

	RvFramebufferAttachment createFramebufferAttachment(const RvDevice& device,
							    const RvFramebufferAttachmentCreateInfo& createInfo);

}; // namespace rvTools

#endif