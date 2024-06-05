#pragma once

#include <string>

#define VK_USE_PLATFORM_WIN32_KHR
#include "Volk/volk.h"

#include "error.h"

inline void checkVulkanError(VkResult errorCode, int lineNo)
{
    if (errorCode != VK_SUCCESS) {
        const std::string message("VULKAN ERROR ON LINE " + std::to_string(lineNo));
        throw Error(message.c_str());
    }
}

#undef VKCHECK
#define VKCHECK(ErrCode) checkVulkanError(ErrCode, __LINE__)