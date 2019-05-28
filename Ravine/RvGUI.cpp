#include "RvGUI.h"

//STD Includes
#include <array>
#include <iostream>

//Hash Includes
#include "crc32.h"

//Ravine Include
#include "RvTime.h"

//GLFW Includes
#include <glfw/glfw3.h>

RvGUI::RvGUI(RvDevice& device, RvSwapChain& swapChain, RvWindow& window) : device(&device), swapChain(&swapChain), window(&window), swapChainImagesCount(swapChain.images.size())
{
	ImGui::CreateContext();
	io = &ImGui::GetIO();
}

RvGUI::~RvGUI()
{
	//Freeup Pipeline
	delete guiPipeline;

	//Freeup Texture Samplers
	vkDestroySampler(device->handle, textureSampler, nullptr);
	fontTexture.Free();

	//Destroy Descriptor pool with descriptor sets allocation
	vkDestroyDescriptorPool(device->handle, descriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(device->handle, descriptorSetLayout, nullptr);

	//Clear Buffers
	for (size_t i = 0; i < swapChainImagesCount; i++)
	{
		//Destroy Vertex Buffers
		vkDestroyBuffer(device->handle, vertexBuffer[i].handle, nullptr);
		vkFreeMemory(device->handle, vertexBuffer[i].memory, nullptr);

		//Destroy Index Buffers
		vkDestroyBuffer(device->handle, indexBuffer[i].handle, nullptr);
		vkFreeMemory(device->handle, indexBuffer[i].memory, nullptr);
	}
	vertexBuffer.clear();
	indexBuffer.clear();

	//Cleanup Pointers
	device = nullptr;
	swapChain = nullptr;
	window = nullptr;
	io = nullptr;
	guiPipeline = nullptr;
}

void RvGUI::Init(VkSampleCountFlagBits samplesCount)
{
	//Color scheme
	ImGuiStyle& style = ImGui::GetStyle();
	style.Colors[ImGuiCol_TitleBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.6f);
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
	style.Colors[ImGuiCol_MenuBarBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
	style.Colors[ImGuiCol_Header] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
	style.Colors[ImGuiCol_CheckMark] = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);

	//Dimensions
	io->DisplaySize = ImVec2(swapChain->WIDTH, swapChain->HEIGHT);
	io->DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

	//Initialize Vulkan Resources
	VkExtent2D extent;
	extent.width = swapChain->WIDTH;
	extent.height = swapChain->HEIGHT;

	//Load Font Texture
	CreateFontTexture();

	//Setup Sampler
	CreateTextureSampler();

	//Descriptors define how date is acessed by the pipeline
	CreateDescriptorPool(); //They are allocated in a pool by the device
	CreateDescriptorSetLayout(); //The Layout indicate which binding is used in which pipeline stage
	CreateDescriptorSet(); //The descriptor set holds information regarding data location and types
	CreatePushConstants(); //Create the structure that defines the push constant range

	//Reset Buffers
	swapChainImagesCount = swapChain->images.size();
	vertexBuffer.resize(swapChainImagesCount);
	indexBuffer.resize(swapChainImagesCount);

	//Create Cmd Buffers for drwaing
	CreateCmdBuffers();

	//Create GUI Graphics Pipeline
	guiPipeline = new RvGUIPipeline(*device, extent, samplesCount, descriptorSetLayout, &pushConstantRange, swapChain->renderPass);
}

void RvGUI::AcquireFrame()
{
	double mouseX, mouseY;
	glfwGetCursorPos(*window, &mouseX, &mouseY);
	io->DisplaySize = ImVec2((float)swapChain->WIDTH, (float)swapChain->HEIGHT);
	io->DeltaTime = RvTime::deltaTime();

	io->MousePos = ImVec2(mouseX, mouseY);
	io->MouseDown[0] = glfwGetMouseButton(*window, GLFW_MOUSE_BUTTON_LEFT);
	io->MouseDown[1] = glfwGetMouseButton(*window, GLFW_MOUSE_BUTTON_RIGHT);

	ImGui::NewFrame();
}

void RvGUI::SubmitFrame()
{
	//Render generates draw buffers
	ImGui::Render();
}

void RvGUI::CreateFrameBuffers()
{
	//Resize FrameBuffer data arrays
	framebuffers.resize(swapChainImagesCount);
	framebufferAttachments.resize(swapChainImagesCount);

	//Create Framebuffer Images, Views and Memory regions
	for (uint32_t i = 0; i < swapChainImagesCount; i++)
	{
		device->createImage(swapChain->WIDTH, swapChain->HEIGHT, 0,
							VK_SAMPLE_COUNT_1_BIT,
							swapChain->imageFormat, VK_IMAGE_TILING_OPTIMAL,
							VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
							VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT,
							framebufferAttachments[i].image, framebufferAttachments[i].memory);

		framebufferAttachments[i].imageView = rvTools::createImageView(
			device->handle,framebufferAttachments[i].image, swapChain->imageFormat, VK_IMAGE_ASPECT_COLOR_BIT
		);
	}
}

void RvGUI::CreateCmdBuffers()
{
	//Resize according to swapChain size
	cmdBuffers.resize(swapChainImagesCount);

	//Allocate Command Buffers into Command Pool
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = device->commandPool; //TODO: Use a local pool to enable multi-threading later
	allocInfo.commandBufferCount = (uint32_t)cmdBuffers.size();

	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
	if (vkAllocateCommandBuffers(device->handle, &allocInfo, cmdBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate command buffers!");
	}
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
	fontTexture = device->createTexture(fontData, texWidth, texHeight, VK_FORMAT_R8G8B8A8_UNORM);
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
	array<VkDescriptorSetLayoutBinding, 1> bindings = { samplerLayoutBinding };

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
	allocInfo.descriptorSetCount = static_cast<uint32_t>(1);
	allocInfo.pSetLayouts = &descriptorSetLayout;

	// Setting descriptor sets vector to count of SwapChain images
	if (vkAllocateDescriptorSets(device->handle, &allocInfo, &descriptorSet) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate descriptor sets!");
	}

	// Define image Descriptor information (binds a sampler and image view)
	VkDescriptorImageInfo imageInfo;
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = fontTexture.view;
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

void RvGUI::CreatePushConstants()
{
	pushConstantRange = {};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	pushConstantRange.size = sizeof(PushConstBlock);
	pushConstantRange.offset = 0;
}

void RvGUI::UpdateBuffers(uint32_t frameIndex)
{
	ImDrawData* imDrawData = ImGui::GetDrawData();

	// Note: Alignment is done inside buffer creation
	VkDeviceSize vertexBufferSize = imDrawData->TotalVtxCount * sizeof(ImDrawVert);
	VkDeviceSize indexBufferSize = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);

	//Avoid drawing empty buffers
	if ((vertexBufferSize == 0) || (indexBufferSize == 0)) {
		return;
	}

	//Calculate CRC for changes detection
	uint32_t vtxCrc = ~uint32_t{ 0 } &uint32_t{ 0xFFFFFFFFuL };
	static const unsigned long long guiVtxToCharSize = sizeof(ImDrawVert) / sizeof(char);
	for (int n = 0; n < imDrawData->CmdListsCount; n++) {
		const ImDrawList* cmd_list = imDrawData->CmdLists[n];
		vtxCrc = crc(reinterpret_cast<char*>(cmd_list->VtxBuffer.Data), cmd_list->VtxBuffer.Size*guiVtxToCharSize, vtxCrc);
	}

	//Compare CRC to detect changes and avoid useless updates
	if (vtxCrc == lastVtxCrc[frameIndex])
	{
		//std::cout << "ImGUI Vtx CRC not equal!" << std::endl;
		return;
	}

	lastVtxCrc[frameIndex] = vtxCrc;

#pragma region Vertex Buffer
	//Cleanup from old buffer
	vkDestroyBuffer(device->handle, vertexBuffer[frameIndex].handle, nullptr);
	vkFreeMemory(device->handle, vertexBuffer[frameIndex].memory, nullptr);
	vertexBuffer[frameIndex].~RvPersistentBuffer();

	//Allocate new one
	ImDrawVert* vtxRsc = new ImDrawVert[imDrawData->TotalVtxCount];
	ImDrawVert* vtxItt = vtxRsc;
	for (int n = 0; n < imDrawData->CmdListsCount; n++) {
		const ImDrawList* cmd_list = imDrawData->CmdLists[n];
		memcpy(vtxItt, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
		vtxItt += cmd_list->VtxBuffer.Size;
	}

	//Create buffer on GPU
	vertexBuffer[frameIndex] = device->createPersistentBuffer(vtxRsc, vertexBufferSize, sizeof(ImDrawVert),
		(VkBufferUsageFlagBits)(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT), VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	//Free-up memory
	delete vtxRsc;
#pragma endregion

#pragma region Index Buffer
	//Cleanup from old buffer
	vkDestroyBuffer(device->handle, indexBuffer[frameIndex].handle, nullptr);
	vkFreeMemory(device->handle, indexBuffer[frameIndex].memory, nullptr);
	indexBuffer[frameIndex].~RvPersistentBuffer();

	//Allocate new one
	ImDrawIdx* idxRsc = new ImDrawIdx[imDrawData->TotalIdxCount];
	ImDrawIdx* idxItt = idxRsc;
	for (int n = 0; n < imDrawData->CmdListsCount; n++) {
		const ImDrawList* cmd_list = imDrawData->CmdLists[n];
		memcpy(idxItt, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
		idxItt += cmd_list->IdxBuffer.Size;
	}

	//Create buffer on GPU
	indexBuffer[frameIndex] = device->createPersistentBuffer(idxRsc, indexBufferSize, sizeof(ImDrawIdx),
		(VkBufferUsageFlagBits)(VK_BUFFER_USAGE_INDEX_BUFFER_BIT), VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	//Free-up memory
	delete idxRsc;
#pragma endregion

}

void RvGUI::RecordCmdBuffers(uint32_t frameIndex)
{
	ImGuiIO& io = ImGui::GetIO();

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;

	//Setup inheritance information to provide access modifiers from RenderPass
	VkCommandBufferInheritanceInfo inheritanceInfo = {};
	inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
	inheritanceInfo.renderPass = swapChain->renderPass;
	//inheritanceInfo.subpass = 0; TODO: USE SUBPASS FOR DEPENDENCIES (BLITTING)
	inheritanceInfo.occlusionQueryEnable = VK_FALSE;
	inheritanceInfo.framebuffer = swapChain->framebuffers[frameIndex]; //TODO: USE INTERNAL FRAMEBUFFER
	inheritanceInfo.pipelineStatistics = 0;
	beginInfo.pInheritanceInfo = &inheritanceInfo;

	//Begin recording Command Buffer
	if (vkBeginCommandBuffer(cmdBuffers[frameIndex], &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("Failed to begin recording command buffer!");
	}

	vkCmdBindDescriptorSets(cmdBuffers[frameIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, guiPipeline->layout, 0, 1, &descriptorSet, 0, nullptr);
	vkCmdBindPipeline(cmdBuffers[frameIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, guiPipeline->handle);

	VkViewport viewport = {};
	viewport.width = io.DisplaySize.x;
	viewport.height = io.DisplaySize.y;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(cmdBuffers[frameIndex], 0, 1, &viewport);

	//UI scale and translate via push constants
	pushConstBlock.scale = glm::vec2(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y);
	pushConstBlock.translate = glm::vec2(-1.0f);
	vkCmdPushConstants(cmdBuffers[frameIndex], guiPipeline->layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstBlock), &pushConstBlock);

	//Render commands
	ImDrawData* imDrawData = ImGui::GetDrawData();
	int32_t vertexOffset = 0;
	int32_t indexOffset = 0;

	if (imDrawData->CmdListsCount > 0) {

		VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(cmdBuffers[frameIndex], 0, 1, &vertexBuffer[frameIndex].handle, offsets);
		vkCmdBindIndexBuffer(cmdBuffers[frameIndex], indexBuffer[frameIndex].handle, 0, VK_INDEX_TYPE_UINT16);

		for (int32_t i = 0; i < imDrawData->CmdListsCount; i++)
		{
			const ImDrawList* cmd_list = imDrawData->CmdLists[i];
			for (int32_t j = 0; j < cmd_list->CmdBuffer.Size; j++)
			{
				const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[j];
				VkRect2D scissorRect;
				scissorRect.offset.x = eastl::max((int32_t)(pcmd->ClipRect.x), 0);
				scissorRect.offset.y = eastl::max((int32_t)(pcmd->ClipRect.y), 0);
				scissorRect.extent.width = (uint32_t)(pcmd->ClipRect.z - pcmd->ClipRect.x);
				scissorRect.extent.height = (uint32_t)(pcmd->ClipRect.w - pcmd->ClipRect.y);
				vkCmdSetScissor(cmdBuffers[frameIndex], 0, 1, &scissorRect);
				vkCmdDrawIndexed(cmdBuffers[frameIndex], pcmd->ElemCount, 1, indexOffset, vertexOffset, 0);
				indexOffset += pcmd->ElemCount;
			}
			vertexOffset += cmd_list->VtxBuffer.Size;
		}
	}

	//Stop recording Command Buffer
	if (vkEndCommandBuffer(cmdBuffers[frameIndex]) != VK_SUCCESS) {
		throw std::runtime_error("Failed to record command buffer!");
	}
}
