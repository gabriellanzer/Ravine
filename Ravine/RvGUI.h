#ifndef RV_GUI_H
#define RV_GUI_H

//Vulkan Includes
#include <vulkan/vulkan.h>

//ImGUI Includes
#include "imgui.h"

//Ravine Includes
#include "RvDevice.h"
#include "RvSwapChain.h"
#include "RvGraphicsPipeline.h"
#include "RvTexture.h"

struct RvGUI
{
	//External Parameters
	ImGuiIO* io;
	RvDevice* device;
	size_t width, height;
	RvSwapChain* swapChain;

	//Font Attributes
	RvTexture* fontTexture;
	VkSampler textureSampler;

	//Pipeline Attributes
	RvGraphicsPipeline* guiPipeline;
	VkDescriptorPool descriptorPool;
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSet descriptorSet;

	RvGUI(RvDevice& device, RvSwapChain& swapChain, size_t width, size_t height);
	~RvGUI();

	void Init(VkRenderPass& renderPass);

private:
	void CreateTextureSampler();
	void CreateFontTexture();
	void CreateDescriptorPool();
	void CreateDescriptorSetLayout();
	void CreateDescriptorSet();

};


#endif