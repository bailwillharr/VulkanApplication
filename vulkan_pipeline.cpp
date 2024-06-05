#include "vulkan_pipeline.h"

#include <array>

#include "vulkan_headers.h"

#include "shaders.h"

VkPipeline createPipelineAndLayout(VkDevice device, VkFormat color_attachment_format, VkExtent2D extent, VkPipelineLayout& layout)
{
    VkShaderModuleCreateInfo module_info{};
    module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    module_info.pNext = nullptr;
    module_info.flags = 0;

    module_info.codeSize = spv_vertex.size();
    module_info.pCode = reinterpret_cast<const uint32_t*>(spv_vertex.data());
    VkShaderModule vertex_module = VK_NULL_HANDLE;
    VKCHECK(vkCreateShaderModule(device, &module_info, nullptr, &vertex_module));

    module_info.codeSize = spv_fragment.size();
    module_info.pCode = reinterpret_cast<const uint32_t*>(spv_fragment.data());
    VkShaderModule fragment_module = VK_NULL_HANDLE;
    VKCHECK(vkCreateShaderModule(device, &module_info, nullptr, &fragment_module));

    std::array<VkPipelineShaderStageCreateInfo, 2> stage_infos{};
    stage_infos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage_infos[0].pNext = nullptr;
    stage_infos[0].flags = 0;
    stage_infos[0].pName = "main";
    stage_infos[0].pSpecializationInfo = nullptr;
    stage_infos[1] = stage_infos[0];
    stage_infos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stage_infos[0].module = vertex_module;
    stage_infos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stage_infos[1].module = fragment_module;

    VkPipelineVertexInputStateCreateInfo vertex_input_state{};
    vertex_input_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_state.pNext = nullptr;
    vertex_input_state.flags = 0;
    vertex_input_state.vertexBindingDescriptionCount = 0;
    vertex_input_state.pVertexBindingDescriptions = nullptr;
    vertex_input_state.vertexAttributeDescriptionCount = 0;
    vertex_input_state.pVertexAttributeDescriptions = nullptr;

    VkPipelineInputAssemblyStateCreateInfo input_assembly_state{};
    input_assembly_state.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_state.pNext = nullptr;
    input_assembly_state.flags = 0;
    input_assembly_state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_state.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(extent.width);
    viewport.height = static_cast<float>(extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    VkRect2D scissor{};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent = extent;

    VkPipelineViewportStateCreateInfo viewport_state{};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.pViewports = &viewport;
    viewport_state.scissorCount = 1;
    viewport_state.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterization_state{};
    rasterization_state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization_state.depthClampEnable = VK_FALSE;
    rasterization_state.rasterizerDiscardEnable = VK_FALSE; // enabling this will not run the fragment shaders at all
    rasterization_state.polygonMode = VK_POLYGON_MODE_FILL;
    rasterization_state.lineWidth = 1.0f;
    rasterization_state.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterization_state.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterization_state.depthBiasEnable = VK_FALSE;
    rasterization_state.depthBiasConstantFactor = 0.0f; // ignored
    rasterization_state.depthBiasClamp = 0.0f;          // ignored
    rasterization_state.depthBiasSlopeFactor = 0.0f;    // ignored

    VkPipelineRenderingCreateInfo rendering_info{};
    rendering_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    rendering_info.pNext = nullptr;
    rendering_info.viewMask = 0;
    rendering_info.colorAttachmentCount = 1;
    rendering_info.pColorAttachmentFormats = &color_attachment_format;
    rendering_info.depthAttachmentFormat = VK_FORMAT_UNDEFINED;
    rendering_info.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

    VkPipelineMultisampleStateCreateInfo multisample_state{};
    multisample_state.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample_state.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisample_state.sampleShadingEnable = VK_FALSE;
    multisample_state.minSampleShading = 1.0f;          // ignored
    multisample_state.pSampleMask = nullptr;            // ignored
    multisample_state.alphaToCoverageEnable = VK_FALSE; // ignored
    multisample_state.alphaToOneEnable = VK_FALSE;      // ignored

    VkPipelineDepthStencilStateCreateInfo depth_stencil_state{};
    depth_stencil_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil_state.depthTestEnable = VK_FALSE;
    depth_stencil_state.depthWriteEnable = VK_FALSE;
    depth_stencil_state.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depth_stencil_state.depthBoundsTestEnable = VK_FALSE;
    depth_stencil_state.minDepthBounds = 0.0f;
    depth_stencil_state.maxDepthBounds = 1.0f;
    depth_stencil_state.stencilTestEnable = VK_FALSE;
    depth_stencil_state.front = {};
    depth_stencil_state.back = {};

    VkPipelineColorBlendAttachmentState color_blend_attachment{};
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo color_blend_state{};
    color_blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_state.logicOpEnable = VK_FALSE;
    color_blend_state.logicOp = VK_LOGIC_OP_COPY; // ignored
    color_blend_state.attachmentCount = 1;
    color_blend_state.pAttachments = &color_blend_attachment;
    color_blend_state.blendConstants[0] = 0.0f; // ignored
    color_blend_state.blendConstants[1] = 0.0f; // ignored
    color_blend_state.blendConstants[2] = 0.0f; // ignored
    color_blend_state.blendConstants[3] = 0.0f; // ignored

    VkPushConstantRange push_constant_range{};
    push_constant_range.offset = 0;
    push_constant_range.size = PUSH_CONSTANT_SIZE;
    push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkPipelineLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_info.setLayoutCount = 0;
    layout_info.pSetLayouts = nullptr;
    layout_info.pushConstantRangeCount = 1;
    layout_info.pPushConstantRanges = &push_constant_range;

    VKCHECK(vkCreatePipelineLayout(device, &layout_info, nullptr, &layout));

    VkGraphicsPipelineCreateInfo pl_info{};
    pl_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pl_info.pNext = &rendering_info;
    pl_info.flags = 0;
    pl_info.stageCount = static_cast<uint32_t>(stage_infos.size());
    pl_info.pStages = stage_infos.data();
    pl_info.pVertexInputState = &vertex_input_state;
    pl_info.pInputAssemblyState = &input_assembly_state;
    pl_info.pTessellationState = nullptr;
    pl_info.pViewportState = &viewport_state;
    pl_info.pRasterizationState = &rasterization_state;
    pl_info.pMultisampleState = &multisample_state;
    pl_info.pDepthStencilState = &depth_stencil_state;
    pl_info.pColorBlendState = &color_blend_state;
    pl_info.pDynamicState = nullptr;
    pl_info.layout = layout;
    pl_info.renderPass = VK_NULL_HANDLE;
    pl_info.subpass = 0;
    pl_info.basePipelineHandle = VK_NULL_HANDLE;
    pl_info.basePipelineIndex = -1;

    VkPipeline pipeline = VK_NULL_HANDLE;
    VKCHECK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pl_info, nullptr, &pipeline));

    vkDestroyShaderModule(device, fragment_module, nullptr);
    vkDestroyShaderModule(device, vertex_module, nullptr);

    return pipeline;
}

void destroyPipelineAndLayout(VkDevice device, VkPipeline pipeline, VkPipelineLayout layout) {
    vkDestroyPipeline(device, pipeline, nullptr);
    vkDestroyPipelineLayout(device, layout, nullptr);
}