#include "vulkan_swapchain.h"

#include <vector>

#include "error.h"
#include "vulkan_device.h"

void createVulkanSwapchain(VkInstance instance, const Device& device, HINSTANCE hInstance, HWND hWnd, Swapchain& swapchain)
{
    VkWin32SurfaceCreateInfoKHR surfaceInfo{};
    surfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surfaceInfo.hinstance = hInstance;
    surfaceInfo.hwnd = hWnd;
    VKCHECK(vkCreateWin32SurfaceKHR(instance, &surfaceInfo, nullptr, &swapchain.surface));
    VkBool32 surface_supported = VK_FALSE;
    VKCHECK(vkGetPhysicalDeviceSurfaceSupportKHR(device.physicalDevice, 0, swapchain.surface, &surface_supported));
    if (surface_supported != VK_TRUE) {
        throw Error("Surface is unsupported!");
    }
    uint32_t surface_format_count = 0;
    VKCHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(device.physicalDevice, swapchain.surface, &surface_format_count, nullptr));
    if (surface_format_count == 0) {
        throw Error("No surface formats found!");
    }
    std::vector<VkSurfaceFormatKHR> surface_formats(surface_format_count);
    VKCHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(device.physicalDevice, swapchain.surface, &surface_format_count, surface_formats.data()));

    swapchain.surface_format = surface_formats[0];
    for (VkSurfaceFormatKHR format : surface_formats) {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB &&
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            swapchain.surface_format = format; // prefer using srgb non linear colors
        }
    }

    VkSurfaceCapabilitiesKHR surface_caps{};
    VKCHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.physicalDevice, swapchain.surface, &surface_caps));

    swapchain.extent = surface_caps.currentExtent;

    /* get min image count */
    uint32_t min_image_count = surface_caps.minImageCount + 1;
    if (surface_caps.maxImageCount > 0 && min_image_count > surface_caps.maxImageCount) {
        min_image_count = surface_caps.maxImageCount;
    }

    // use mailbox if supported
    uint32_t num_present_modes = 0;
    VKCHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(device.physicalDevice, swapchain.surface, &num_present_modes, nullptr));
    std::vector<VkPresentModeKHR> available_present_modes(num_present_modes);
    VKCHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(device.physicalDevice, swapchain.surface, &num_present_modes, available_present_modes.data()));
    VkPresentModeKHR chosen_present_mode = VK_PRESENT_MODE_FIFO_KHR;
    for (VkPresentModeKHR mode : available_present_modes) {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            chosen_present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
        }
    }

    VkSwapchainCreateInfoKHR sc_info{};
    sc_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    sc_info.surface = swapchain.surface;
    sc_info.minImageCount = min_image_count;
    sc_info.imageFormat = swapchain.surface_format.format;
    sc_info.imageColorSpace = swapchain.surface_format.colorSpace;
    sc_info.imageExtent = surface_caps.currentExtent;
    sc_info.imageArrayLayers = 1;
    sc_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    sc_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    sc_info.preTransform = surface_caps.currentTransform;
    sc_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    sc_info.presentMode = chosen_present_mode;
    sc_info.clipped = VK_TRUE;
    sc_info.oldSwapchain = VK_NULL_HANDLE;
    VKCHECK(vkCreateSwapchainKHR(device.device, &sc_info, nullptr, &swapchain.swapchain));

    uint32_t swapchainImageCount = 0;
    VKCHECK(vkGetSwapchainImagesKHR(device.device, swapchain.swapchain, &swapchainImageCount, nullptr));
    std::vector<VkImage> swapchainImages(swapchainImageCount);
    VKCHECK(vkGetSwapchainImagesKHR(device.device, swapchain.swapchain, &swapchainImageCount, swapchainImages.data()));

    for (size_t i = 0; i < swapchainImages.size(); ++i) {
        // create image view
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.pNext = nullptr;
        viewInfo.flags = 0;
        viewInfo.image = swapchainImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = sc_info.imageFormat;
        viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
        VkImageView imageView = VK_NULL_HANDLE;
        VKCHECK(vkCreateImageView(device.device, &viewInfo, nullptr, &imageView));
        swapchain.images.emplace_back(std::make_pair(swapchainImages[i], imageView));
    }
}

void destroyVulkanSwapchain(VkInstance instance, const Device& device, const Swapchain& swapchain)
{
    for (auto [image, view] : swapchain.images) {
        vkDestroyImageView(device.device, view, nullptr);
    }
    vkDestroySwapchainKHR(device.device, swapchain.swapchain, nullptr);
    vkDestroySurfaceKHR(instance, swapchain.surface, nullptr);
}
