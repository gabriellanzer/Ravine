#ifndef RV_GUI_H
#define RV_GUI_H

//Vulkan Includes
#include "volk.h"

//EASTL Includes
#include <eastl/vector.h>

//ImGUI Includes
#include "imgui.h"

//GLM Includes
#include <glm/glm.hpp>

//Ravine Includes
#include "RvDevice.h"
#include "RvWindow.h"
#include "RvSwapChain.h"
#include "RvTexture.h"
#include "RvRenderPass.h"
#include "RvGuiPipeline.h"

struct RvGui
{
	//External Parameters
	ImGuiIO* io;
	RvDevice* device;
	RvSwapChain* swapChain;
	RvRenderPass* renderPass;
	RvWindow* window;

	//External parameters state
	uint32_t swapChainImagesCount = 0;

	//Font Attributes
	RvTexture fontTexture;
	VkSampler textureSampler;

	//Pipeline Attributes
	RvGuiPipeline* guiPipeline;
	VkDescriptorPool descriptorPool;
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSet descriptorSet;
	
	//Block of push constant information
	struct PushConstBlock {
		glm::vec2 scale;
		glm::vec2 translate;
	} pushConstBlock;

	//Buffer Attributes
	VkPushConstantRange pushConstantRange;
	vector<RvDynamicBuffer> vertexBuffer;
	vector<RvDynamicBuffer> indexBuffer;

	//The GUI CommandBuffers for each frame
	vector<VkCommandBuffer> cmdBuffers;

	//The GUI FrameBuffers for each frame
	vector<VkFramebuffer> framebuffers;
	vector<RvFramebufferAttachment> framebufferAttachments;

	uint32_t lastVtxCrc[RV_MAX_FRAMES_IN_FLIGHT] = { ~uint32_t{ 0 } &uint32_t{ 0xFFFFFFFFuL } };

	RvGui(RvDevice* device, RvSwapChain* swapChain, RvWindow* window, RvRenderPass* renderPass);
	~RvGui();

	void init(VkSampleCountFlagBits samplesCount);
	void acquireFrame();
	void submitFrame();
	void updateBuffers(uint32_t frameIndex);
	void recordCmdBuffers(uint32_t frameIndex);

private:
	void createFrameBuffers();
	void createCmdBuffers();
	void createTextureSampler();
	void createFontTexture();
	void createDescriptorPool();
	void createDescriptorSetLayout();
	void createDescriptorSet();
	void createPushConstants();

};


#endif