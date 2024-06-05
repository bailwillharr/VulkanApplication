#pragma once

#include "vulkan_headers.h"

constexpr uint32_t PUSH_CONSTANT_SIZE = 16;

VkPipeline createPipelineAndLayout(VkDevice device, VkFormat color_attachment_format, VkExtent2D extent, VkPipelineLayout& layout);

void destroyPipelineAndLayout(VkDevice device, VkPipeline pipeline, VkPipelineLayout layout);