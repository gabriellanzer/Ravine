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
	RvPersistentBuffer vertexBuffer;
	RvPersistentBuffer indexBuffer;

	//TODO: Create command buffers here and do a blitting operation later
	//Today we are using the same CMD buffers to draw models and do UI stuff
	//std::vector<VkCommandBuffer> commandBuffers;
	//void CreateFrameBuffers();

	RvGUI(RvDevice& device, RvSwapChain& swapChain, RvWindow& window);
	~RvGUI();

	void Init(VkSampleCountFlagBits samplesCount);
	void AcquireFrame();
	void SubmitFrame();
	void UpdateBuffers();
	void DrawFrame(VkCommandBuffer commandBuffer);

private:
	void CreateTextureSampler();
	void CreateFontTexture();
	void CreateDescriptorPool();
	void CreateDescriptorSetLayout();
	void CreateDescriptorSet();
	void CreatePushConstants();

};


#endif