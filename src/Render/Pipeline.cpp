#include "WindowRenderer.hpp"
#include "UiVertexSpirv.hpp"
#include "UiFragmentSpirv.hpp"
#include <cstddef>

namespace Yukiri::Detail
{
Result<void> WindowRenderer::CreatePipeline()
{
	auto vertexInfo = VkInfo<VkShaderModuleCreateInfo>(VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO);
	vertexInfo.codeSize = sizeof(UiVertexSpirv);
	vertexInfo.pCode = UiVertexSpirv;
	VkShaderModule vertex = VK_NULL_HANDLE;
	VkShaderModule fragment = VK_NULL_HANDLE;
	auto result = vkCreateShaderModule(m_Device.m_Device, &vertexInfo, nullptr, &vertex);
	if (result != VK_SUCCESS) return GraphicsError("Vertex shader", result);
	vertexInfo.codeSize = sizeof(UiFragmentSpirv);
	vertexInfo.pCode = UiFragmentSpirv;
	result = vkCreateShaderModule(m_Device.m_Device, &vertexInfo, nullptr, &fragment);
	if (result != VK_SUCCESS)
	{
		vkDestroyShaderModule(m_Device.m_Device, vertex, nullptr);
		return GraphicsError("Fragment shader", result);
	}
	std::array<VkPipelineShaderStageCreateInfo, 2> stages;
	for (auto& stage : stages) stage = VkInfo<VkPipelineShaderStageCreateInfo>(VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO);
	stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	stages[0].module = vertex;
	stages[0].pName = "main";
	stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	stages[1].module = fragment;
	stages[1].pName = "main";
	VkVertexInputBindingDescription binding{0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX};
	const VkVertexInputAttributeDescription attributes[] = {
		{0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, m_Position)},
		{1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, m_Local)},
		{2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, m_Size)},
		{3, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, m_Color)},
		{4, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, m_Radius)},
		{5, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, m_Uv)}};
	auto input = VkInfo<VkPipelineVertexInputStateCreateInfo>(VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO);
	input.vertexBindingDescriptionCount = 1;
	input.pVertexBindingDescriptions = &binding;
	input.vertexAttributeDescriptionCount = 6;
	input.pVertexAttributeDescriptions = attributes;
	auto assembly = VkInfo<VkPipelineInputAssemblyStateCreateInfo>(VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO);
	assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	auto viewport = VkInfo<VkPipelineViewportStateCreateInfo>(VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO);
	viewport.viewportCount = viewport.scissorCount = 1;
	auto raster = VkInfo<VkPipelineRasterizationStateCreateInfo>(VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO);
	raster.polygonMode = VK_POLYGON_MODE_FILL;
	raster.cullMode = VK_CULL_MODE_NONE;
	raster.lineWidth = 1;
	auto multisample = VkInfo<VkPipelineMultisampleStateCreateInfo>(VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO);
	multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	VkPipelineColorBlendAttachmentState blend{};
	blend.blendEnable = VK_TRUE;
	blend.srcColorBlendFactor = blend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	blend.dstColorBlendFactor = blend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blend.colorBlendOp = blend.alphaBlendOp = VK_BLEND_OP_ADD;
	blend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	auto blending = VkInfo<VkPipelineColorBlendStateCreateInfo>(VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO);
	blending.attachmentCount = 1;
	blending.pAttachments = &blend;
	const VkDynamicState dynamics[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
	auto dynamic = VkInfo<VkPipelineDynamicStateCreateInfo>(VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO);
	dynamic.dynamicStateCount = 2;
	dynamic.pDynamicStates = dynamics;
	auto rendering = VkInfo<VkPipelineRenderingCreateInfo>(VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO);
	rendering.colorAttachmentCount = 1;
	rendering.pColorAttachmentFormats = &m_Format;
	auto pipeline = VkInfo<VkGraphicsPipelineCreateInfo>(VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO);
	pipeline.pNext = &rendering;
	pipeline.stageCount = 2;
	pipeline.pStages = stages.data();
	pipeline.pVertexInputState = &input;
	pipeline.pInputAssemblyState = &assembly;
	pipeline.pViewportState = &viewport;
	pipeline.pRasterizationState = &raster;
	pipeline.pMultisampleState = &multisample;
	pipeline.pColorBlendState = &blending;
	pipeline.pDynamicState = &dynamic;
	pipeline.layout = m_Device.m_PipelineLayout;
	result = vkCreateGraphicsPipelines(m_Device.m_Device, VK_NULL_HANDLE, 1, &pipeline, nullptr, &m_Pipeline);
	vkDestroyShaderModule(m_Device.m_Device, vertex, nullptr);
	vkDestroyShaderModule(m_Device.m_Device, fragment, nullptr);
	if (result != VK_SUCCESS) return GraphicsError("vkCreateGraphicsPipelines", result);
	return {};
}
}
