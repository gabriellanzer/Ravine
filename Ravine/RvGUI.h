#ifndef RV_GUI_H
#define RV_GUI_H

//Vulkan Includes
#include <vulkan/vulkan.h>

//STD Includes
#include <vector>

//ImGUI Includes
#include "imgui.h"

//GLM Includes
#include <glm/glm.hpp>

//Ravine Includes
#include "RvDevice.h"
#include "RvWindow.h"
#include "RvSwapChain.h"
#include "RvTexture.h"
#include "RvGUIPipeline.h"


struct RvGUI
{
	//External Parameters
	ImGuiIO* io;
	RvDevice* device;
	RvSwapChain* swapChain;
	RvWindow* window;

	//External parameters state
	uint32_t swapChainImagesCount = 0;

	//Font Attributes
	RvTexture fontTexture;
	VkSampler textureSampler;

	//Pipeline Attributes
	RvGUIPipeline* guiPipeline;
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
	std::vector<RvPersistentBuffer> vertexBuffer;
	std::vector<RvPersistentBuffer> indexBuffer;

	//The GUI CommandBuffers for each frame
	std::vector<VkCommandBuffer> cmdBuffers;

	//The GUI FrameBuffers for each frame
	std::vector<VkFramebuffer> framebuffers;
	std::vector<RvFramebufferAttachment> framebufferAttachments;

	uint32_t lastVtxCrc[RV_MAX_FRAMES_IN_FLIGHT] = { ~uint32_t{ 0 } &uint32_t{ 0xFFFFFFFFuL } };

	//TODO: Create command buffers here and do a blitting operation later
	//Today we are using the same CMD buffers to draw models and do UI stuff
	//std::vector<VkCommandBuffer> commandBuffers;
	//void CreateFrameBuffers();

	RvGUI(RvDevice& device, RvSwapChain& swapChain, RvWindow& window);
	~RvGUI();

	void Init(VkSampleCountFlagBits samplesCount);
	void AcquireFrame();
	void SubmitFrame();
	void UpdateBuffers(uint32_t frameIndex);
	void RecordCmdBuffers(uint32_t frameIndex);

private:
	void CreateFrameBuffers();
	void CreateCmdBuffers();
	void CreateTextureSampler();
	void CreateFontTexture();
	void CreateDescriptorPool();
	void CreateDescriptorSetLayout();
	void CreateDescriptorSet();
	void CreatePushConstants();

};


#endif