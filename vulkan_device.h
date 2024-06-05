#pragma once

#include "vulkan_headers.h"

struct Device {
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device = VK_NULL_HANDLE;
	VkQueue queue = VK_NULL_HANDLE;
};

Device createVulkanDevice(VkInstance instance);
void destroyVulkanDevice(const Device& device);