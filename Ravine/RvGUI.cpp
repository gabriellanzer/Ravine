#include "RvGUI.h"

//STD Includes
#include <array>

RvGUI::RvGUI(RvDevice& device, RvSwapChain& swapChain, size_t width, size_t height) : device(&device), io(&ImGui::GetIO()), swapChain(&swapChain)
{
	//Color scheme
	ImGuiStyle& style = ImGui::GetStyle();
	style.Colors[ImGuiCol_TitleBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.6f);
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
	style.Colors[ImGuiCol_MenuBarBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
	style.Colors[ImGuiCol_Header] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
	style.Colors[ImGuiCol_CheckMark] = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);

	//Dimensions
	io->DisplaySize = ImVec2(width, height);
	io->DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
}

RvGUI::~RvGUI()
{
	delete guiPipeline;
}

void RvGUI::Init(VkRenderPass & renderPass)
{
	VkExtent2D extent;
	extent.width = width;
	extent.height = height;

	//Load Font Texture
	CreateFontTexture();

	//Setup Sampler
	CreateTextureSampler();

	//Descriptors define how date is acessed by the pipeline
	CreateDescriptorPool(); //They are allocated in a pool by the device
	CreateDescriptorSetLayout(); //The Layout indicate which binding is used in which pipeline stage
	CreateDescriptorSet(); //The descriptor set holds information regarding data location and types

	//Create GUI Pipeline with no Multisampling (SAMPLE_COUNT_1)
	guiPipeline->Init(extent, VK_SAMPLE_COUNT_1_BIT, descriptorSetLayout, renderPass);
}

void RvGUI::CreateTextureSampler()
{
	VkSamplerCreateInfo samplerCreateInfo = {};
	samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

	//Sampler interpolation filters
	samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.minFilter = VK_FILTER_LINEAR;

	//Address mode per axis
	samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

	//Anisotropy filter
	samplerCreateInfo.anisotropyEnable = VK_TRUE;
	samplerCreateInfo.maxAnisotropy = 16;

	//Sampling beyond image with "Clamp to Border"
	samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

	//Coordinates normalization:
	//VK_TRUE: [0,texWidth]/[0,texHeight]
	//VK_FALSE: [0,1]/[0,1]
	samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;

	//Comparison function (used for shadow maps)
	samplerCreateInfo.compareEnable = VK_FALSE;
	samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	//Mipmapping
	samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

	//Creating sampler
	if (vkCreateSampler(device->handle, &samplerCreateInfo, nullptr, &textureSampler) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create texture sampler!");
	}
}

void RvGUI::CreateFontTexture()
{
	//Get font data
	unsigned char* fontData;
	int texWidth, texHeight;
	io->Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);

	//Create texture
	fontTexture = &device->createTexture(fontData, texWidth, texHeight, VK_FORMAT_R8G8B8A8_UNORM);
}

void RvGUI::CreateDescriptorPool()
{
	VkDescriptorPoolSize poolSize = {};
	poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSize.descriptorCount = static_cast<uint32_t>(1); //1 descriptor for the font texture

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(1);
	poolInfo.pPoolSizes = &poolSize;
	poolInfo.maxSets = static_cast<uint32_t>(2); //TODO: Understand why ImGUI uses 2

	if (vkCreateDescriptorPool(device->handle, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create descriptor pool!");
	}
}

void RvGUI::CreateDescriptorSetLayout()
{
	//Texture Sampler layout
	VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
	samplerLayoutBinding.binding = 0;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.pImmutableSamplers = nullptr;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	//Bindings array
	std::array<VkDescriptorSetLayoutBinding, 1> bindings = { samplerLayoutBinding };

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(device->handle, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create descriptor set layout!");
	}
}

void RvGUI::CreateDescriptorSet()
{
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(swapChain->images.size());
	allocInfo.pSetLayouts = &descriptorSetLayout;

	// Setting descriptor sets vector to count of SwapChain images
	if (vkAllocateDescriptorSets(device->handle, &allocInfo, &descriptorSet) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate descriptor sets!");
	}

	// Define image Descriptor information (binds a sampler and image view)
	VkDescriptorImageInfo imageInfo;
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = fontTexture->view;
	imageInfo.sampler = textureSampler;

	// Image Info
	VkWriteDescriptorSet descriptorWrites = {};
	descriptorWrites.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites.dstSet = descriptorSet;
	descriptorWrites.dstBinding = 0;
	descriptorWrites.dstArrayElement = 0;
	descriptorWrites.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites.descriptorCount = 1;
	descriptorWrites.pImageInfo = &imageInfo;

	vkUpdateDescriptorSets(device->handle, static_cast<uint32_t>(1), &descriptorWrites, 0, nullptr);
}
