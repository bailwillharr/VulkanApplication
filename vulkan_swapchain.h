#pragma once

#include <vector>
#include <tuple>

#include "framework.h"

#include "vulkan_headers.h"

struct Device;

struct Swapchain {
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    std::vector<std::pair<VkImage, VkImageView>> images{};
    VkSurfaceFormatKHR surface_format{};
    VkExtent2D extent{};
};

void createVulkanSwapchain(VkInstance instance, const Device& device, HINSTANCE hInstance, HWND hWnd, Swapchain& swapchain);
void destroyVulkanSwapchain(VkInstance instance, const Device& device, const Swapchain& swapchain);