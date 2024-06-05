#include "vulkan_device.h"

#include <vector>

#include "error.h"

static VkPhysicalDevice getPhysicalDevice(VkInstance instance)
{
    uint32_t physicalDeviceCount = 0;
    if (vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr) == VK_SUCCESS) {
        if (physicalDeviceCount > 0) {
            physicalDeviceCount = 1;
            VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
            const VkResult res = vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, &physicalDevice);
            if (res == VK_SUCCESS || res == VK_INCOMPLETE) {
                return physicalDevice;
            }
        }
    }
    return VK_NULL_HANDLE;
}

Device createVulkanDevice(VkInstance instance)
{
    Device device{};
    device.physicalDevice = getPhysicalDevice(instance);
    if (device.physicalDevice == VK_NULL_HANDLE) {
        throw Error("Failed to get physical device!");
    }

    std::vector<VkExtensionProperties> availableExts{};
    { // get available extensions
        uint32_t availableExtCount = 0;
        VKCHECK(vkEnumerateDeviceExtensionProperties(device.physicalDevice, nullptr, &availableExtCount, nullptr));
        availableExts.resize(availableExtCount);
        VKCHECK(vkEnumerateDeviceExtensionProperties(device.physicalDevice, nullptr, &availableExtCount, availableExts.data()));
    }

    const std::vector<const char*> requiredExtensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    { // check for required extensions
        bool foundExtensions = true;
        for (const char* extToFind : requiredExtensions) {
            bool extFound = false;
            for (const auto& ext : availableExts) {
                if (strcmp(extToFind, ext.extensionName) == 0) {
                    extFound = true;
                    break;
                }
            }
            if (!extFound) {
                foundExtensions = false;
                break;
            }
        }
        if (!foundExtensions) throw Error("Missing required extensions!");
    }

    VkPhysicalDeviceProperties devProps{};
    vkGetPhysicalDeviceProperties(device.physicalDevice, &devProps);

    { // check that device supports vulkan 1.3
        if (devProps.apiVersion < VK_API_VERSION_1_3) {
            throw Error("Vulkan device must support 1.3");
        }
    }

    { // check features
        VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeatures{};
        dynamicRenderingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
        VkPhysicalDeviceMemoryPriorityFeaturesEXT memoryPriorityFeatures{};
        memoryPriorityFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PRIORITY_FEATURES_EXT;
        memoryPriorityFeatures.pNext = &dynamicRenderingFeatures;
        VkPhysicalDeviceSynchronization2Features synchronization2Features{};
        synchronization2Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
        synchronization2Features.pNext = &memoryPriorityFeatures;
        VkPhysicalDeviceFeatures2 devFeatures{};
        devFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        devFeatures.pNext = &synchronization2Features;
        vkGetPhysicalDeviceFeatures2(device.physicalDevice, &devFeatures);

        // we need dynamic_rendering and synchronization2
        if (dynamicRenderingFeatures.dynamicRendering == VK_FALSE) {
            throw Error("Device feature dynamicRendering not available");
        }
        if (synchronization2Features.synchronization2 == VK_FALSE) {
            throw Error("Device feature synchronization2 not available");
        }
    }

    // check for required formats here
    {
        // no special formats needed yet
        // surface format is found by createVulkanSwapchain()
    }

    const float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueInfo{};
    queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.queueFamilyIndex = 0;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = &queuePriority;

    /* set enabled features */
    VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeatures{};
    dynamicRenderingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
    dynamicRenderingFeatures.dynamicRendering = VK_TRUE;
    VkPhysicalDeviceSynchronization2Features synchronization2Features{};
    synchronization2Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
    synchronization2Features.pNext = &dynamicRenderingFeatures;
    synchronization2Features.synchronization2 = VK_TRUE;
    VkPhysicalDeviceFeatures2 featuresToEnable{};
    featuresToEnable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    featuresToEnable.pNext = &synchronization2Features;

    VkDeviceCreateInfo devInfo{};
    devInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    devInfo.pNext = &featuresToEnable;
    devInfo.queueCreateInfoCount = 1;
    devInfo.pQueueCreateInfos = &queueInfo;
    devInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
    devInfo.ppEnabledExtensionNames = requiredExtensions.data();
    devInfo.enabledLayerCount = 0;
    devInfo.ppEnabledLayerNames = nullptr;
    devInfo.pEnabledFeatures = nullptr;
    VKCHECK(vkCreateDevice(device.physicalDevice, &devInfo, nullptr, &device.device));
    vkGetDeviceQueue(device.device, 0, 0, &device.queue);
    return device;
}

void destroyVulkanDevice(const Device& device) { vkDestroyDevice(device.device, nullptr); }