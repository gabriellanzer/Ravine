#include "RvGUIPipeline.h"

// Ravine Includes
#include "RvTools.h"

// ImGUI Includes
#include "imgui.h"

// STD Include
#include <stdexcept>

RvGuiPipeline::RvGuiPipeline(RvDevice& device, VkExtent2D extent, VkSampleCountFlagBits sampleCount,
			     VkDescriptorSetLayout descriptorSetLayout, VkPushConstantRange* pushConstantRange,
			     VkRenderPass renderPass)
    : device(&device)
{
	// Create Pipeline cache
	VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
	pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	if (vkCreatePipelineCache(device.handle, &pipelineCacheCreateInfo, nullptr, &pipelineCache) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create Pipeline Cache!");
	}

	// Load Shaders
	vector<char> vertShaderCode = rvTools::readFile("../data/shaders/gui.vert");
	vector<char> fragShaderCode = rvTools::readFile("../data/shaders/gui.frag");
	vector<char> vertexShader = rvTools::GLSLtoSPV(VK_SHADER_STAGE_VERTEX_BIT, vertShaderCode);
	vertModule = rvTools::createShaderModule(device.handle, vertexShader);
	vector<char> fragmentShader = rvTools::GLSLtoSPV(VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderCode);
	fragModule = rvTools::createShaderModule(device.handle, fragmentShader);

	// Shader Stage creation (assign shader modules to vertex or fragment shader stages in the pipeline).
	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

	// Vertex Input (describes the format of the vertex data that will be passed to the vertex shader).
	// Reference:
	// https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Fixed_functions#page_Vertex_input
	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	VkVertexInputBindingDescription bindingDescription{};
	bindingDescription.binding = 0;
	bindingDescription.stride = sizeof(ImDrawVert);
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	VkVertexInputAttributeDescription attributeDescriptions[3];
	// Location 0: Position
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[0].offset = offsetof(ImDrawVert, pos);
	// Location 1: UV
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(ImDrawVert, uv);
	// Location 2: Color
	attributeDescriptions[2].location = 2;
	attributeDescriptions[2].binding = 0;
	attributeDescriptions[2].format = VK_FORMAT_R8G8B8A8_UNORM;
	attributeDescriptions[2].offset = offsetof(ImDrawVert, col);

	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(3);
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions;

	// Input Assembly (describes what kind of geometry will be drawn from the vertices and if primitive restart
	// should be enabled). Reference:
	// https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Fixed_functions#page_Input_assembly
	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	// Viewports and Scissors (describes the region of the framebuffer that the output will be rendered to)
	// Reference:
	// https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Fixed_functions#page_Viewports_and_scissors
	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)extent.width;
	viewport.height = (float)extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = {0, 0};
	scissor.extent = extent;

	// Combine both into a Viewport State
	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	// Dynamic State (Defines the states that can be changed without recreating the graphics pipeline)
	// Reference:
	// https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Fixed_functions#page_Dynamic_state
	VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

	VkPipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = 2, dynamicState.pDynamicStates = dynamicStates;

	// Rasterizer (takes the geometry that is shaped by the vertices at the vertex shader and turns it into
	// fragments) Reference:
	// https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Fixed_functions#page_Rasterizer
	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;	       // VK_FALSE discards fragments outside of near/far plane frustum
	rasterizer.rasterizerDiscardEnable = VK_FALSE; // VK_TRUE discards any geometry rendered here
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL; // Could be FILL, LINE or POINT (requires GPU feature enabling)
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_NONE;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

	// Multisampling (anti-aliasing: combines the fragment shader results of multiple polygons that rasterize to the
	// same pixel) Reference:
	// https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Fixed_functions#page_Multisampling
	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_TRUE; // Enable sample shading in the pipeline
	multisampling.rasterizationSamples = sampleCount;
	multisampling.minSampleShading = 1.0f;		// Min fraction for sample shading; closer to one is smooth
	multisampling.pSampleMask = nullptr;		// Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE;	// Optional

	// Color Blending (combines the fragment's output with the color that is already in the framebuffer)
	// Reference:
	// https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Fixed_functions#page_Color_blending
	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.blendEnable = VK_TRUE;
	colorBlendAttachment.colorWriteMask =
	    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	// TODO: Understand why ImGUI doesn't use those values
	// colorBlending.logicOpEnable = VK_FALSE;
	// colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
	// colorBlending.blendConstants[0] = 0.0f; // Optional
	// colorBlending.blendConstants[1] = 0.0f; // Optional
	// colorBlending.blendConstants[2] = 0.0f; // Optional
	// colorBlending.blendConstants[3] = 0.0f; // Optional

	// Depth Stencil Attachment
	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_FALSE;
	depthStencil.depthWriteEnable = VK_FALSE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	depthStencil.front = depthStencil.back;
	depthStencil.back.compareOp = VK_COMPARE_OP_ALWAYS;

	// Pipeline Layout (Specify the uniform variables and push constants used in the pipeline)
	// Reference:
	// https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Fixed_functions#page_Pipeline_layout
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = pushConstantRange;

	if (vkCreatePipelineLayout(device.handle, &pipelineLayoutInfo, nullptr, &layout) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create pipeline layout!");
	}

	// Proper Graphics Pipeline Creation
	// Reference: https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Conclusion
	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;

	// Reference all state objects
	// Vertex and Rasterizer
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	// Viewpor and scissors
	pipelineInfo.pViewportState = &viewportState;
	// Attachments
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDepthStencilState = &depthStencil;

	// The uniforms layout
	pipelineInfo.layout = layout;

	// Reference render pass and define current subpass index
	pipelineInfo.renderPass = renderPass;

	// Pipeline inheritence
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex = -1;		  // Optional

	// Setup dynamic state
	pipelineInfo.pDynamicState = &dynamicState;

	if (vkCreateGraphicsPipelines(device.handle, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &handle) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create graphics pipeline!");
	}
}

RvGuiPipeline::~RvGuiPipeline()
{
	vkDestroyShaderModule(device->handle, fragModule, nullptr);
	vkDestroyShaderModule(device->handle, vertModule, nullptr);
	vkDestroyPipelineCache(device->handle, pipelineCache, nullptr);
	vkDestroyPipelineLayout(device->handle, layout, nullptr);
	vkDestroyPipeline(device->handle, handle, nullptr);
}

RvGuiPipeline::operator VkPipeline() const { return handle; }
