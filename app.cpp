#include "app.h"

// std lib C
#include <cassert>
#include <cmath>
#include <cinttypes>
#include <cstdint>

// std lib
#include <array>
#include <atomic>
#include <memory>
#include <thread>
#include <chrono>

// other libs
#include "framework.h"
#include "vulkan_headers.h"

// project includes
#include "error.h"
#include "vulkan_device.h"
#include "vulkan_instance.h"
#include "vulkan_swapchain.h"
#include "vulkan_pipeline.h"

// GLOBALS
// Should only be accessed by the rendering thread after initialisation by main thread
struct Globals {
    VkInstance instance = VK_NULL_HANDLE;
    Device device{};
    Swapchain swapchain{};

    VkCommandPool cmd_pool = VK_NULL_HANDLE;
    VkCommandBuffer cmd_buf = VK_NULL_HANDLE;

    VkFence fence = VK_NULL_HANDLE;
    VkSemaphore present_semaphore = VK_NULL_HANDLE;
    VkSemaphore render_semaphore = VK_NULL_HANDLE;

    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;

    std::atomic<bool> running = true;
    std::unique_ptr<std::thread> loop_thread{};
};

static Globals globals;

static void recordCommandBuffer(uint32_t image_index, double dt)
{
    static double current_time = 0.0;
    current_time += dt;

    // reset cmd buffer
    VKCHECK(vkResetCommandPool(globals.device.device, globals.cmd_pool, 0));

    // record cmd buffer
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    beginInfo.pInheritanceInfo = nullptr;
    VKCHECK(vkBeginCommandBuffer(globals.cmd_buf, &beginInfo));

    { // transition swapchain image to color attachment layout
        VkImageMemoryBarrier2 colorImageBarrier{};
        colorImageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        colorImageBarrier.pNext = nullptr;
        colorImageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
        colorImageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
        colorImageBarrier.srcAccessMask = 0;
        colorImageBarrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT; // vkCmdRendering load op
        colorImageBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorImageBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorImageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        colorImageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        colorImageBarrier.image = globals.swapchain.images[image_index].first;
        VkImageSubresourceRange colorImageRange{};
        colorImageRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        colorImageRange.baseMipLevel = 0;
        colorImageRange.levelCount = 1;
        colorImageRange.baseArrayLayer = 0;
        colorImageRange.layerCount = 1;
        colorImageBarrier.subresourceRange = colorImageRange;

        VkDependencyInfo imageDependencyInfo{};
        imageDependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        imageDependencyInfo.imageMemoryBarrierCount = 1;
        imageDependencyInfo.pImageMemoryBarriers = &colorImageBarrier;
        vkCmdPipelineBarrier2(globals.cmd_buf, &imageDependencyInfo);
    }

    // now begin rendering
    VkRenderingAttachmentInfo colorAttachment{};
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.pNext = nullptr;
    colorAttachment.imageView = globals.swapchain.images[image_index].second;
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.resolveMode = VK_RESOLVE_MODE_NONE;
    colorAttachment.resolveImageView = VK_NULL_HANDLE;              // don't care
    colorAttachment.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED; // don't care
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.clearValue.color.float32[0] = 1.0f;
    colorAttachment.clearValue.color.float32[1] = 1.0f;
    colorAttachment.clearValue.color.float32[2] = 1.0f;
    colorAttachment.clearValue.color.float32[3] = 1.0f;
    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.pNext = nullptr;
    renderingInfo.flags = 0;
    renderingInfo.renderArea = VkRect2D{VkOffset2D{0, 0}, globals.swapchain.extent};
    renderingInfo.layerCount = 1;
    renderingInfo.viewMask = 0;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachment;
    renderingInfo.pDepthAttachment = nullptr;
    renderingInfo.pStencilAttachment = nullptr;
    vkCmdBeginRendering(globals.cmd_buf, &renderingInfo);

    // do rendering things here

    vkCmdBindPipeline(globals.cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, globals.pipeline);

    /* 2x2 matrix
     * [ 0 2 ]
     * [ 1 3 ]
     */
    /* 2D rotation matrix
     * [ cos -sin ]
     * [ sin  cos ]
     */
    float transform[4];
    transform[0] = static_cast<float>(cos(current_time));
    transform[1] = static_cast<float>(sin(current_time));
    transform[2] = static_cast<float>(sin(current_time)) * -1.0f;
    transform[3] = static_cast<float>(cos(current_time));
    vkCmdPushConstants(globals.cmd_buf, globals.pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, PUSH_CONSTANT_SIZE, &transform);

    vkCmdDraw(globals.cmd_buf, 3, 1, 0, 0);

    // finish rendering
    vkCmdEndRendering(globals.cmd_buf);

    { // make the color attachment presentable
        VkImageMemoryBarrier2 colorImageBarrier{};
        colorImageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        colorImageBarrier.pNext = nullptr;
        colorImageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
        colorImageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT; // semaphore takes care of this
        colorImageBarrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;         // vkCmdRendering store op
        colorImageBarrier.dstAccessMask = 0;
        colorImageBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorImageBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        colorImageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        colorImageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        colorImageBarrier.image = globals.swapchain.images[image_index].first;
        VkImageSubresourceRange colorImageRange{};
        colorImageRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        colorImageRange.baseMipLevel = 0;
        colorImageRange.levelCount = 1;
        colorImageRange.baseArrayLayer = 0;
        colorImageRange.layerCount = 1;
        colorImageBarrier.subresourceRange = colorImageRange;

        VkDependencyInfo imageDependencyInfo{};
        imageDependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        imageDependencyInfo.imageMemoryBarrierCount = 1;
        imageDependencyInfo.pImageMemoryBarriers = &colorImageBarrier;
        vkCmdPipelineBarrier2(globals.cmd_buf, &imageDependencyInfo);
    }

    // command buffer recording is complete
    VKCHECK(vkEndCommandBuffer(globals.cmd_buf));
}

static void print(const std::string& text)
{
#ifndef NDEBUG
    if (WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), text.c_str(), static_cast<DWORD>(text.length()), NULL, NULL) == FALSE)
        throw Error("Failed to write to console");
#else
    (void)text;
#endif
}

[[maybe_unused]] static void printDouble(double d)
{
    std::array<char, 64> buf{};
    snprintf(buf.data(), buf.size(), "%f\n", d);
    print(buf.data());
}

[[maybe_unused]] static void printInt64(int64_t i)
{
    std::array<char, 64> buf{};
    snprintf(buf.data(), buf.size(), "%" PRIi64 "\n", i);
    print(buf.data());
}

static void gameLoop()
{
    try {

#ifndef NDEBUG
        // create a console window for debugging
        if (AllocConsole() == FALSE) throw Error("AllocConsole() failure from render thread");
#endif

        constexpr int64_t FPS_LIMIT = 240;
        constexpr auto FRAMETIME_LIMIT = std::chrono::nanoseconds(1'000'000'000LL / FPS_LIMIT);

        auto begin_frame = std::chrono::steady_clock::now();
        auto end_frame = begin_frame + FRAMETIME_LIMIT;

        while (globals.running) {

            auto last_begin_frame = begin_frame;
            begin_frame = std::chrono::steady_clock::now();

            [[maybe_unused]] const double dt = std::chrono::duration<double>(begin_frame - last_begin_frame).count();

            end_frame = begin_frame + FRAMETIME_LIMIT;

            // calculate delta time


            VkResult res{};

            // wait until previous frame's rendering has finished
            // this fence is signalled when the last frame's command buffer finishes execution.
            VKCHECK(vkWaitForFences(globals.device.device, 1, &globals.fence, VK_TRUE, UINT64_MAX));
            VKCHECK(vkResetFences(globals.device.device, 1, &globals.fence));

            uint32_t image_index = 0;
            res =
                vkAcquireNextImageKHR(globals.device.device, globals.swapchain.swapchain, UINT64_MAX, globals.present_semaphore, VK_NULL_HANDLE, &image_index);
            if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR) throw Error("Failed to acquire swapchain image");

            recordCommandBuffer(image_index, dt);

            // submit rendering commands
            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.pNext = nullptr;
            submitInfo.waitSemaphoreCount = 1;
            submitInfo.pWaitSemaphores = &globals.present_semaphore;
            constexpr VkPipelineStageFlags semaphore_wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            submitInfo.pWaitDstStageMask = &semaphore_wait_stage;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &globals.cmd_buf;
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = &globals.render_semaphore;
            VKCHECK(vkQueueSubmit(globals.device.queue, 1, &submitInfo, globals.fence));

            // present
            VkPresentInfoKHR presentInfo{};
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            presentInfo.pNext = nullptr;
            presentInfo.waitSemaphoreCount = 1;
            presentInfo.pWaitSemaphores = &globals.render_semaphore;
            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = &globals.swapchain.swapchain;
            presentInfo.pImageIndices = &image_index;
            presentInfo.pResults = nullptr;
            res = vkQueuePresentKHR(globals.device.queue, &presentInfo);
            if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR) throw Error("Failed to present swapchain image");

            std::this_thread::sleep_until(end_frame);
        }
    }
    catch (const Error& error) {
        MessageBoxA(NULL, error.what(), "Application Error!", MB_OK | MB_ICONERROR);
        abort();
    }
}

void initApp(HINSTANCE hInstance, HWND hWnd)
{

    { // instance creation
        if (volkInitialize() != VK_SUCCESS) {
            throw Error("Failed to initialise Volk");
        }

        globals.instance = initVulkanInstance();

        volkLoadInstance(globals.instance);

        if (volkGetInstanceVersion() < VK_API_VERSION_1_3) {
            throw Error("Unsupported Vulkan version. Need at least Vulkan 1.3.");
        }
    }

    { // device creation
        globals.device = createVulkanDevice(globals.instance);
        volkLoadDevice(globals.device.device);
    }

    { // swapchain creation
        createVulkanSwapchain(globals.instance, globals.device, hInstance, hWnd, globals.swapchain);
    }

    { // create command pool
        // command buffers in this pool will be reset after they have finished execution.
        // 2 command buffers will be used.
        // This allows the next frame's command buffer to be recorded while the previous frame cmd buffer is still being executed
        VkCommandPoolCreateInfo cmd_pool_info{};
        cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmd_pool_info.flags = 0;
        cmd_pool_info.queueFamilyIndex = 0;
        VKCHECK(vkCreateCommandPool(globals.device.device, &cmd_pool_info, nullptr, &globals.cmd_pool));
    }

    { // create command buffers
        VkCommandBufferAllocateInfo cmd_buf_info{};
        cmd_buf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmd_buf_info.commandPool = globals.cmd_pool;
        cmd_buf_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmd_buf_info.commandBufferCount = 1;
        VKCHECK(vkAllocateCommandBuffers(globals.device.device, &cmd_buf_info, &globals.cmd_buf));
    }

    // create the fence and semaphores
    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.pNext = nullptr;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT; // for first frame fence must be signalled otherwise app will wait forever
    VKCHECK(vkCreateFence(globals.device.device, &fence_info, nullptr, &globals.fence));

    VkSemaphoreCreateInfo semaphore_info{};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphore_info.pNext = nullptr;
    semaphore_info.flags = 0;
    VKCHECK(vkCreateSemaphore(globals.device.device, &semaphore_info, nullptr, &globals.render_semaphore));
    VKCHECK(vkCreateSemaphore(globals.device.device, &semaphore_info, nullptr, &globals.present_semaphore));

    globals.pipeline =
        createPipelineAndLayout(globals.device.device, globals.swapchain.surface_format.format, globals.swapchain.extent, globals.pipeline_layout);
}

void startGameLoop()
{
    globals.running.store(true);
    globals.loop_thread = std::make_unique<std::thread>(gameLoop);
}

void endLoopAndShutdown()
{
    globals.running.store(false);
    globals.loop_thread->join();

    vkDeviceWaitIdle(globals.device.device);

    destroyPipelineAndLayout(globals.device.device, globals.pipeline, globals.pipeline_layout);

    vkDestroySemaphore(globals.device.device, globals.present_semaphore, nullptr);
    vkDestroySemaphore(globals.device.device, globals.render_semaphore, nullptr);
    vkDestroyFence(globals.device.device, globals.fence, nullptr);
    vkDestroyCommandPool(globals.device.device, globals.cmd_pool, nullptr);

    destroyVulkanSwapchain(globals.instance, globals.device, globals.swapchain);
    destroyVulkanDevice(globals.device);
    destroyVulkanInstance(globals.instance);
}