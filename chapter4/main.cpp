//
// This file is part of the "ogl_to_vlk" project
// See "LICENSE" for license information.
//

#if defined(__APPLE__)
#define VK_USE_PLATFORM_MACOS_MVK 1
#include <QuartzCore/CAMetalLayer.h>
#elif defined(_WIN64)
#define VK_USE_PLATFORM_WIN32_KHR 1
#endif

#include <iostream>
#include <vector>
#include <array>
#include <vulkan/vulkan.h>
#include <platform/Window.h>

using namespace std;
using namespace Platform_lib;

//----------------------------------------------------------------------------------------------------------------------

constexpr auto image_available_index {0};
constexpr auto rendering_done_index {1};

//----------------------------------------------------------------------------------------------------------------------

class Chapter4 {
public:
    Chapter4(Window* window) :
        window_ {window},
        instance_ {VK_NULL_HANDLE},
        physical_device_ {VK_NULL_HANDLE},
        queue_family_index_ {UINT32_MAX},
        device_ {VK_NULL_HANDLE},
        queue_ {VK_NULL_HANDLE},
        command_pool_ {VK_NULL_HANDLE},
        command_buffer_ {VK_NULL_HANDLE},
        semaphores_ {},
        fences_ {},
        surface_ {VK_NULL_HANDLE},
        swapchain_ {VK_NULL_HANDLE}
    {
        init_signals_();
        init_instance_();
        find_best_physical_device_();
        find_queue_family_index_();
        init_device_();
        init_queue_();
        init_command_pool_();
        init_command_buffer_();
        init_semaphores_();
        init_fences_();
    }

    ~Chapter3()
    {
        fini_fences_();
        fini_semaphores_();
        fini_command_pool_();
        fini_device_();
        fini_instance_();
    }

private:
    void init_signals_()
    {
        window_->startup_signal.connect(this, &Chapter4::on_startup);
        window_->shutdown_signal.connect(this, &Chapter4::on_shutdown);
        window_->render_signal.connect(this, &Chapter4::on_render);
    }

    void init_instance_()
    {
        vector<const char*> layer_names {
            "VK_LAYER_KHRONOS_validation"
        };

        vector<const char*> extension_names {
            "VK_KHR_surface",
            "VK_MVK_macos_surface"
        };

        VkInstanceCreateInfo create_info {};

        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_info.enabledLayerCount = layer_names.size();
        create_info.ppEnabledLayerNames = &layer_names[0];
        create_info.enabledExtensionCount = extension_names.size();
        create_info.ppEnabledExtensionNames = &extension_names[0];

        auto result = vkCreateInstance(&create_info, nullptr, &instance_);
        switch (result) {
            case VK_ERROR_OUT_OF_HOST_MEMORY:
                cout << "VK_ERROR_OUT_OF_HOST_MEMORY" << endl;
                break;
            case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                cout << "VK_ERROR_OUT_OF_DEVICE_MEMORY" << endl;
                break;
            case VK_ERROR_INITIALIZATION_FAILED:
                cout << "VK_ERROR_INITIALIZATION_FAILED" << endl;
                break;
            case VK_ERROR_LAYER_NOT_PRESENT:
                cout << "VK_ERROR_LAYER_NOT_PRESENT" << endl;
                break;
            case VK_ERROR_INCOMPATIBLE_DRIVER:
                cout << "VK_ERROR_INCOMPATIBLE_DRIVER" << endl;
                break;
            default:
                break;
        }
        assert(result == VK_SUCCESS);
    }

    void find_best_physical_device_()
    {
        uint32_t count {0};
        vkEnumeratePhysicalDevices(instance_, &count, nullptr);

        vector<VkPhysicalDevice> physical_devices {count};
        vkEnumeratePhysicalDevices(instance_, &count, &physical_devices[0]);

        for (auto &physical_device : physical_devices) {
            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(physical_device, &properties);
        }

        physical_device_ = physical_devices[0];
    }

    void find_queue_family_index_()
    {
        uint32_t count {0};
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device_, &count, 0);

        vector<VkQueueFamilyProperties> properties {count};
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device_, &count, &properties[0]);

        for (auto i = 0; i != properties.size(); ++i) {
            if (properties[i].queueCount < 1)
                continue;

            if (VK_QUEUE_GRAPHICS_BIT & properties[i].queueFlags) {
                queue_family_index_ = i;
                break;
            }
        }
        assert(queue_family_index_ != UINT32_MAX);
    }

    void init_device_()
    {
        constexpr auto priority = 1.0f;

        VkDeviceQueueCreateInfo queue_create_info {};

        queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.queueFamilyIndex = queue_family_index_;
        queue_create_info.queueCount = 1;
        queue_create_info.pQueuePriorities = &priority;

        vector<const char*> extension_names {
                "VK_KHR_swapchain",
        };

        VkDeviceCreateInfo create_info {};

        create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        create_info.queueCreateInfoCount = 1;
        create_info.pQueueCreateInfos = &queue_create_info;
        create_info.enabledExtensionCount = extension_names.size();
        create_info.ppEnabledExtensionNames = &extension_names[0];

        auto result = vkCreateDevice(physical_device_, &create_info, nullptr, &device_);
        switch (result) {
            case VK_ERROR_OUT_OF_HOST_MEMORY:
                cout << "VK_ERROR_OUT_OF_HOST_MEMORY" << endl;
                break;
            case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                cout << "VK_ERROR_OUT_OF_DEVICE_MEMORY" << endl;
                break;
            case VK_ERROR_INITIALIZATION_FAILED:
                cout << "VK_ERROR_INITIALIZATION_FAILED" << endl;
                break;
            case VK_ERROR_EXTENSION_NOT_PRESENT:
                cout << "VK_ERROR_EXTENSION_NOT_PRESENT" << endl;
                break;
            case VK_ERROR_FEATURE_NOT_PRESENT:
                cout << "VK_ERROR_FEATURE_NOT_PRESENT" << endl;
                break;
            case VK_ERROR_TOO_MANY_OBJECTS:
                cout << "VK_ERROR_TOO_MANY_OBJECTS" << endl;
                break;
            case VK_ERROR_DEVICE_LOST:
                cout << "VK_ERROR_DEVICE_LOST" << endl;
                break;
            default:
                break;
        }
        assert(result == VK_SUCCESS);
    }

    void init_queue_()
    {
        vkGetDeviceQueue(device_, queue_family_index_, 0, &queue_);
    }

    void init_command_pool_()
    {
        VkCommandPoolCreateInfo create_info {};

        create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        create_info.queueFamilyIndex = queue_family_index_;

        auto result = vkCreateCommandPool(device_, &create_info, nullptr, &command_pool_);
        switch (result) {
            case VK_ERROR_OUT_OF_HOST_MEMORY:
                cout << "VK_ERROR_OUT_OF_HOST_MEMORY" << endl;
                break;
            case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                cout << "VK_ERROR_OUT_OF_DEVICE_MEMORY" << endl;
                break;
            default:
                break;
        }
        assert(result == VK_SUCCESS);
    }

    void init_command_buffer_()
    {
        VkCommandBufferAllocateInfo allocate_info {};

        allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocate_info.commandPool = command_pool_;
        allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocate_info.commandBufferCount = 1;

        auto result = vkAllocateCommandBuffers(device_, &allocate_info, &command_buffer_);
        switch (result) {
            case VK_ERROR_OUT_OF_HOST_MEMORY:
                cout << "VK_ERROR_OUT_OF_HOST_MEMORY" << endl;
                break;
            case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                cout << "VK_ERROR_OUT_OF_DEVICE_MEMORY" << endl;
                break;
            default:
                break;
        }
        assert(result == VK_SUCCESS);
    }

    void init_semaphores_()
    {
        VkSemaphoreCreateInfo create_info {};

        create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        for (auto& semaphore : semaphores_) {
            auto result = vkCreateSemaphore(device_, &create_info, nullptr, &semaphore);
            switch (result) {
                case VK_ERROR_OUT_OF_HOST_MEMORY:
                    cout << "VK_ERROR_OUT_OF_HOST_MEMORY" << endl;
                    break;
                case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                    cout << "VK_ERROR_OUT_OF_DEVICE_MEMORY" << endl;
                    break;
                default:
                    break;
            }
            assert(result == VK_SUCCESS);
        }
    }

    void init_fences_()
    {
        VkFenceCreateInfo create_info {};

        create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

        for (auto i = 0; i != 2; ++i) {
            create_info.flags = (i == rendering_done_index) ? VK_FENCE_CREATE_SIGNALED_BIT : 0;

            auto result = vkCreateFence(device_, &create_info, nullptr, &fences_[i]);
            switch (result) {
                case VK_ERROR_OUT_OF_HOST_MEMORY:
                    cout << "VK_ERROR_OUT_OF_HOST_MEMORY" << endl;
                    break;
                case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                    cout << "VK_ERROR_OUT_OF_DEVICE_MEMORY" << endl;
                    break;
                default:
                    break;
            }
            assert(result == VK_SUCCESS);
        }
    }

    void init_surface_()
    {
#if defined(__APPLE__)
        auto ns_window = (__bridge NSWindow*)window_->window();
        auto content_view = [ns_window contentView];

        //
        [content_view setLayer:[CAMetalLayer layer]];

        VkMacOSSurfaceCreateInfoMVK create_info {};

        create_info.sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK;
        create_info.pView = (__bridge void*)content_view;

        auto result = vkCreateMacOSSurfaceMVK(instance_, &create_info, nullptr, &surface_);
#elif defined(_WIN64)
        VkWin32SurfaceCreateInfoKHR create_info {};

        create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        create_info.hinstance = GetModuleHandle(NULL);
        create_info.hwnd = static_cast<HWND>(window_->window());

        auto result = vkCreateWin32SurfaceKHR(instance_, &create_info, nullptr, &surface_);
#endif
        switch (result) {
            case VK_ERROR_OUT_OF_HOST_MEMORY:
                cout << "VK_ERROR_OUT_OF_HOST_MEMORY" << endl;
                break;
            case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                cout << "VK_ERROR_OUT_OF_DEVICE_MEMORY" << endl;
                break;
            case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
                cout << "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR" << endl;
                break;
            default:
                break;
        }
        assert(result == VK_SUCCESS);

        VkBool32 supported;
        vkGetPhysicalDeviceSurfaceSupportKHR(physical_device_, queue_family_index_, surface_, &supported);
        assert(supported == VK_TRUE);
    }

    auto default_surface_format_()
    {
        uint32_t count;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device_, surface_, &count, nullptr);

        vector<VkSurfaceFormatKHR> formats {count};
        vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device_, surface_, &count, &formats[0]);

        return formats[0];
    }

    void init_swapchain_()
    {
        auto surface_format = default_surface_format_();
        auto present_mode = VK_PRESENT_MODE_FIFO_KHR;

        VkSurfaceCapabilitiesKHR surface_capabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device_, surface_, &surface_capabilities);

        VkCompositeAlphaFlagBitsKHR composite_alpha;
        for (auto i = 1; i != 32; ++i) {
            VkCompositeAlphaFlagBitsKHR flag = static_cast<VkCompositeAlphaFlagBitsKHR>(0x1 << i);
            if (surface_capabilities.supportedUsageFlags & flag) {
                composite_alpha = flag;
                break;
            }
        }

        VkSwapchainCreateInfoKHR create_info {};

        create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        create_info.surface = surface_;
        create_info.minImageCount = 2;
        create_info.imageFormat = surface_format.format;
        create_info.imageColorSpace = surface_format.colorSpace;
        create_info.imageExtent = surface_capabilities.currentExtent;
        create_info.imageArrayLayers = 1;
        create_info.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.preTransform = surface_capabilities.currentTransform;
        create_info.compositeAlpha = composite_alpha;
        create_info.presentMode = present_mode;

        auto result = vkCreateSwapchainKHR(device_, &create_info, nullptr, &swapchain_);
        switch (result) {
            case VK_ERROR_OUT_OF_HOST_MEMORY:
                cout << "VK_ERROR_OUT_OF_HOST_MEMORY" << endl;
                break;
            case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                cout << "VK_ERROR_OUT_OF_DEVICE_MEMORY" << endl;
                break;
            case VK_ERROR_DEVICE_LOST:
                cout << "VK_ERROR_DEVICE_LOST" << endl;
                break;
            case VK_ERROR_SURFACE_LOST_KHR:
                cout << "VK_ERROR_SURFACE_LOST_KHR" << endl;
                break;
            case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
                cout << "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR" << endl;
                break;
            case VK_ERROR_INITIALIZATION_FAILED:
                cout << "VK_ERROR_INITIALIZATION_FAILED" << endl;
                break;
            default:
                break;
        }
        assert(result == VK_SUCCESS);
    }

    void init_swapchain_images_()
    {
        uint32_t count;
        vkGetSwapchainImagesKHR(device_, swapchain_, &count, nullptr);

        swapchain_images_.resize(count);
        vkGetSwapchainImagesKHR(device_, swapchain_, &count, &swapchain_images_[0]);

        VkCommandBufferBeginInfo begin_info {};

        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        vkBeginCommandBuffer(command_buffer_, &begin_info);

        vector<VkImageMemoryBarrier> barriers;
        for (auto& image : swapchain_images_) {
            VkImageMemoryBarrier barrier {};

            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            barrier.srcQueueFamilyIndex = queue_family_index_;
            barrier.dstQueueFamilyIndex = queue_family_index_;
            barrier.image = image;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.layerCount = 1;

            barriers.push_back(barrier);
        }

        vkCmdPipelineBarrier(command_buffer_,
                             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                             0,
                             0, nullptr,
                             0, nullptr,
                             barriers.size(), &barriers[0]);

        vkEndCommandBuffer(command_buffer_);

        VkSubmitInfo submit_info {};

        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffer_;

        vkQueueSubmit(queue_, 1, &submit_info, VK_NULL_HANDLE);

        vkDeviceWaitIdle(device_);
    }

    void fini_instance_()
    {
        vkDestroyInstance(instance_, nullptr);
    }

    void fini_device_()
    {
        vkDestroyDevice(device_, nullptr);
    }

    void fini_command_pool_()
    {
        vkDestroyCommandPool(device_, command_pool_, nullptr);
    }

    void fini_semaphores_()
    {
        for (auto& semaphore : semaphores_)
            vkDestroySemaphore(device_, semaphore, nullptr);
    }

    void fini_fences_()
    {
        for (auto& fence : fences_)
            vkDestroyFence(device_, fence, nullptr);
    }

    void fini_surface_()
    {
        vkDestroySurfaceKHR(instance_, surface_, nullptr);
    }

    void fini_swapchain_()
    {
        vkDestroySwapchainKHR(device_, swapchain_, nullptr);
    }

    void on_startup()
    {
        init_surface_();
        init_swapchain_();
        init_swapchain_images_();
    }

    void on_shutdown()
    {
        fini_swapchain_();
        fini_surface_();
    }

    void on_render()
    {
        uint32_t swapchain_index;
        vkAcquireNextImageKHR(device_, swapchain_, 0,
                              semaphores_[image_available_index], fences_[image_available_index],
                              &swapchain_index);
        vkWaitForFences(device_, 1, &fences_[image_available_index], VK_TRUE, UINT64_MAX);
        vkResetFences(device_, 1, &fences_[image_available_index]);

        if (VK_NOT_READY == vkGetFenceStatus(device_, fences_[rendering_done_index]))
            vkWaitForFences(device_, 1, &fences_[rendering_done_index], VK_TRUE, UINT64_MAX);

        vkResetFences(device_, 1, &fences_[rendering_done_index]);
        vkResetCommandBuffer(command_buffer_, 0);

        auto& swapchain_image = swapchain_images_[swapchain_index];

        VkCommandBufferBeginInfo begin_info {};

        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        vkBeginCommandBuffer(command_buffer_, &begin_info);

        {
            VkImageMemoryBarrier barrier {};

            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.srcQueueFamilyIndex = queue_family_index_;
            barrier.dstQueueFamilyIndex = queue_family_index_;
            barrier.image = swapchain_image;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.layerCount = 1;

            vkCmdPipelineBarrier(command_buffer_,
                                 VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);
        }

        VkClearColorValue clear_color;

        clear_color.float32[0] = 1.0f; // R
        clear_color.float32[1] = 0.0f; // G
        clear_color.float32[2] = 1.0f; // B
        clear_color.float32[3] = 1.0f; // A

        VkImageSubresourceRange subresource_range {};

        subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresource_range.levelCount = 1;
        subresource_range.layerCount = 1;

        vkCmdClearColorImage(command_buffer_,
                             swapchain_image,
                             VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                             &clear_color, 1,
                             &subresource_range);

        {
            VkImageMemoryBarrier barrier {};

            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            barrier.srcQueueFamilyIndex = queue_family_index_;
            barrier.dstQueueFamilyIndex = queue_family_index_;
            barrier.image = swapchain_image;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.layerCount = 1;

            vkCmdPipelineBarrier(command_buffer_,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);
        }

        vkEndCommandBuffer(command_buffer_);

        constexpr VkPipelineStageFlags wait_dst_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkSubmitInfo submit_info {};

        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.waitSemaphoreCount = 1;
        submit_info.pWaitSemaphores = &semaphores_[image_available_index];
        submit_info.pWaitDstStageMask = &wait_dst_stage_mask;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffer_;
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores = &semaphores_[rendering_done_index];

        vkQueueSubmit(queue_, 1, &submit_info, fences_[rendering_done_index]);

        // vkDeviceWaitIdle(device_);

        VkPresentInfoKHR present_info {};

        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = &semaphores_[rendering_done_index];
        present_info.swapchainCount = 1;
        present_info.pSwapchains = &swapchain_;
        present_info.pImageIndices = &swapchain_index;

        vkQueuePresentKHR(queue_, &present_info);
    }

private:
    Window* window_;
    VkInstance instance_;
    VkPhysicalDevice physical_device_;
    uint32_t queue_family_index_;
    VkDevice device_;
    VkQueue queue_;
    VkCommandPool command_pool_;
    VkCommandBuffer command_buffer_;
    std::array<VkFence, 2> fences_;
    std::array<VkSemaphore, 2> semaphores_;
    VkSurfaceKHR surface_;
    VkSwapchainKHR swapchain_;
    vector<VkImage> swapchain_images_;
};

//----------------------------------------------------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    Window_desc window_desc;

    window_desc.title = L"Chapter4"s;
    window_desc.extent = {512, 512, 1};

    Window window {window_desc};

    Chapter4 chapter4 {&window};

    window.run();

    return 0;
}

//----------------------------------------------------------------------------------------------------------------------
