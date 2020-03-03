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
#include <sc/Spirv_compiler.h>

using namespace std;
using namespace Platform_lib;
using namespace Sc_lib;

//----------------------------------------------------------------------------------------------------------------------

constexpr auto image_available_index {0};
constexpr auto rendering_done_index {1};

//----------------------------------------------------------------------------------------------------------------------

class Chapter6 {
public:
    Chapter6(Window* window) :
        window_ {window},
        instance_ {VK_NULL_HANDLE},
        physical_device_ {VK_NULL_HANDLE},
        queue_family_index_ {UINT32_MAX},
        device_ {VK_NULL_HANDLE},
        queue_ {VK_NULL_HANDLE},
        command_pool_ {VK_NULL_HANDLE},
        command_buffer_ {VK_NULL_HANDLE},
        fence_ {VK_NULL_HANDLE},
        semaphores_ {},
        surface_ {VK_NULL_HANDLE},
        swapchain_ {VK_NULL_HANDLE},
        swapchain_image_extent_ {0, 0},
        render_pass_ {VK_NULL_HANDLE}
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
        init_fence_();
    }

    ~Chapter6()
    {
        fini_fence_();
        fini_semaphores_();
        fini_command_pool_();
        fini_device_();
        fini_instance_();
    }

private:
    void init_signals_()
    {
        window_->startup_signal.connect(this, &Chapter6::on_startup);
        window_->shutdown_signal.connect(this, &Chapter6::on_shutdown);
        window_->render_signal.connect(this, &Chapter6::on_render);
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

    void init_fence_()
    {
        VkFenceCreateInfo create_info {};

        create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        auto result = vkCreateFence(device_, &create_info, nullptr, &fence_);
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

        swapchain_image_extent_ = surface_capabilities.currentExtent;

        VkSwapchainCreateInfoKHR create_info {};

        create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        create_info.surface = surface_;
        create_info.minImageCount = 2;
        create_info.imageFormat = surface_format.format;
        create_info.imageColorSpace = surface_format.colorSpace;
        create_info.imageExtent = swapchain_image_extent_;
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

    void init_swapchain_image_views_()
    {
        auto surface_format = default_surface_format_();

        VkImageViewCreateInfo create_info {};

        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format = surface_format.format;
        create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.layerCount = 1;

        swapchain_image_views_.resize(swapchain_images_.size());
        for (auto i = 0; i != swapchain_images_.size(); ++i) {
            create_info.image = swapchain_images_[i];

            auto result = vkCreateImageView(device_, &create_info, nullptr, &swapchain_image_views_[i]);
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

    void init_render_pass_()
    {
        auto surface_format = default_surface_format_();

        VkAttachmentDescription attachment_desc {};

        attachment_desc.format = surface_format.format;
        attachment_desc.samples = VK_SAMPLE_COUNT_1_BIT;
        attachment_desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment_desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment_desc.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachment_desc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference color_attachment_ref {};

        color_attachment_ref.attachment = 0;
        color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass_desc {};

        subpass_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass_desc.colorAttachmentCount = 1;
        subpass_desc.pColorAttachments = &color_attachment_ref;

        VkRenderPassCreateInfo create_info {};

        create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        create_info.attachmentCount = 1;
        create_info.pAttachments = &attachment_desc;
        create_info.subpassCount = 1;
        create_info.pSubpasses = &subpass_desc;

        auto result = vkCreateRenderPass(device_, &create_info, nullptr, &render_pass_);
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

    void init_framebuffers_()
    {
        VkFramebufferCreateInfo create_info {};

        create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        create_info.renderPass = render_pass_;
        create_info.attachmentCount = 1;
        create_info.width = swapchain_image_extent_.width;
        create_info.height = swapchain_image_extent_.height;
        create_info.layers = 1;

        framebuffers_.resize(swapchain_image_views_.size());
        for (auto i = 0; i != swapchain_image_views_.size(); ++i) {
            create_info.pAttachments = &swapchain_image_views_[i];

            auto result = vkCreateFramebuffer(device_, &create_info, nullptr, &framebuffers_[i]);
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

    void init_shader_modules_()
    {
        {
            const string vksl = {
                "void main() {                                          \n"
                "    vec2 pos[3] = vec2[3](vec2(-0.5,  0.5),            \n"
                "                          vec2( 0.5,  0.5),            \n"
                "                          vec2( 0.0, -0.5));           \n"
                "                                                       \n"
                "    gl_Position = vec4(pos[gl_VertexIndex], 0.0, 1.0); \n"
                "}                                                      \n"
            };

            auto spirv = Spirv_compiler().compile(Shader_type::vertex, vksl);

            VkShaderModuleCreateInfo create_info {};

            create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            create_info.codeSize = spirv.size() * sizeof(uint32_t);
            create_info.pCode = &spirv[0];

            auto result = vkCreateShaderModule(device_, &create_info, nullptr, &shader_modules_[0]);
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

        {
            const string vksl = {
                "precision mediump float;                        \n"
                "                                                \n"
                "layout(location = 0) out vec4 fragment_color0;  \n"
                "                                                \n"
                "void main() {                                   \n"
                "    fragment_color0 = vec4(0.0, 1.0, 0.0, 1.0); \n"
                "}                                               \n"
            };

            auto spirv = Spirv_compiler().compile(Shader_type::fragment, vksl);

            VkShaderModuleCreateInfo create_info {};

            create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            create_info.codeSize = spirv.size() * sizeof(uint32_t);
            create_info.pCode = &spirv[0];

            auto result = vkCreateShaderModule(device_, &create_info, nullptr, &shader_modules_[1]);
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

    void init_pipeline_layout_()
    {
        VkPipelineLayoutCreateInfo create_info {};

        create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

        auto result = vkCreatePipelineLayout(device_, &create_info, nullptr, &pipeline_layout_);
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

    void init_pipeline_()
    {
        array<VkPipelineShaderStageCreateInfo, 2> stages;

        {
            VkPipelineShaderStageCreateInfo stage {};

            stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            stage.stage = VK_SHADER_STAGE_VERTEX_BIT;
            stage.module = shader_modules_[0];
            stage.pName = "main";

            stages[0] = stage;
        }

        {
            VkPipelineShaderStageCreateInfo stage {};

            stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            stage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
            stage.module = shader_modules_[1];
            stage.pName = "main";

            stages[1] = stage;
        }

        VkPipelineVertexInputStateCreateInfo vertex_input_state {};

        vertex_input_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        VkPipelineInputAssemblyStateCreateInfo input_assembly_state {};

        input_assembly_state.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        input_assembly_state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        VkViewport viewport {};

        viewport.width = swapchain_image_extent_.width;
        viewport.height = swapchain_image_extent_.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor {};

        scissor.extent = swapchain_image_extent_;

        VkPipelineViewportStateCreateInfo viewport_state {};

        viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport_state.viewportCount = 1;
        viewport_state.pViewports = &viewport;
        viewport_state.scissorCount = 1;
        viewport_state.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterization_state {};

        rasterization_state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterization_state.polygonMode = VK_POLYGON_MODE_FILL;
        rasterization_state.cullMode = VK_CULL_MODE_NONE;
        rasterization_state.lineWidth = 1.0f;

        VkPipelineMultisampleStateCreateInfo multisample_state {};

        multisample_state.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisample_state.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineDepthStencilStateCreateInfo depth_stencil_state {};

        depth_stencil_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

        VkPipelineColorBlendAttachmentState attachment {};

        attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT
                                  | VK_COLOR_COMPONENT_G_BIT
                                  | VK_COLOR_COMPONENT_B_BIT
                                  | VK_COLOR_COMPONENT_A_BIT;

        VkPipelineColorBlendStateCreateInfo color_blend_state {};

        color_blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        color_blend_state.attachmentCount = 1;
        color_blend_state.pAttachments = &attachment;

        VkGraphicsPipelineCreateInfo create_info {};

        create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        create_info.stageCount = stages.size();
        create_info.pStages = &stages[0];
        create_info.pVertexInputState = & vertex_input_state;
        create_info.pInputAssemblyState = &input_assembly_state;
        create_info.pViewportState = &viewport_state;
        create_info.pRasterizationState = &rasterization_state;
        create_info.pMultisampleState = &multisample_state;
        create_info.pDepthStencilState = &depth_stencil_state;
        create_info.pColorBlendState = &color_blend_state;
        create_info.layout = pipeline_layout_;
        create_info.renderPass = render_pass_;

        auto result = vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &create_info, nullptr, &pipeline_);
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

    void fini_fence_()
    {
        vkDestroyFence(device_, fence_, nullptr);
    }

    void fini_surface_()
    {
        vkDestroySurfaceKHR(instance_, surface_, nullptr);
    }

    void fini_swapchain_()
    {
        vkDestroySwapchainKHR(device_, swapchain_, nullptr);
    }

    void fini_swapchain_image_views_()
    {
        for (auto& image_view : swapchain_image_views_)
            vkDestroyImageView(device_, image_view, nullptr);
    }

    void fini_render_pass_()
    {
        vkDestroyRenderPass(device_, render_pass_, nullptr);
    }

    void fini_framebuffers_()
    {
        for (auto& framebuffer : framebuffers_)
            vkDestroyFramebuffer(device_, framebuffer, nullptr);
    }

    void fini_shader_modules_()
    {
        for (auto& shader_module : shader_modules_)
            vkDestroyShaderModule(device_, shader_module, nullptr);
    }

    void fini_pipeline_layout_()
    {
        vkDestroyPipelineLayout(device_, pipeline_layout_, nullptr);
    }

    void fini_pipeline_()
    {
        vkDestroyPipeline(device_, pipeline_, nullptr);
    }

    void on_startup()
    {
        init_surface_();
        init_swapchain_();
        init_swapchain_images_();
        init_swapchain_image_views_();
        init_render_pass_();
        init_framebuffers_();
        init_shader_modules_();
        init_pipeline_layout_();
        init_pipeline_();
    }

    void on_shutdown()
    {
        vkDeviceWaitIdle(device_);

        fini_pipeline_();
        fini_pipeline_layout_();
        fini_shader_modules_();
        fini_framebuffers_();
        fini_render_pass_();
        fini_swapchain_image_views_();
        fini_swapchain_();
        fini_surface_();
    }

    void on_render()
    {
        uint32_t swapchain_index;
        vkAcquireNextImageKHR(device_, swapchain_, 0,
                              semaphores_[image_available_index], VK_NULL_HANDLE,
                              &swapchain_index);

        if (VK_NOT_READY == vkGetFenceStatus(device_, fence_))
            vkWaitForFences(device_, 1, &fence_, VK_TRUE, UINT64_MAX);

        vkResetFences(device_, 1, &fence_);
        vkResetCommandBuffer(command_buffer_, 0);

        auto& swapchain_image = swapchain_images_[swapchain_index];

        VkCommandBufferBeginInfo begin_info {};

        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        vkBeginCommandBuffer(command_buffer_, &begin_info);

        {
            VkImageMemoryBarrier barrier {};

            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            barrier.srcQueueFamilyIndex = queue_family_index_;
            barrier.dstQueueFamilyIndex = queue_family_index_;
            barrier.image = swapchain_image;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.layerCount = 1;

            vkCmdPipelineBarrier(command_buffer_,
                                 VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);
        }

        VkClearValue clear_value;

        clear_value.color.float32[0] = 0.15f; // R
        clear_value.color.float32[1] = 0.15f; // G
        clear_value.color.float32[2] = 0.15f; // B
        clear_value.color.float32[3] = 1.0f; // A

        VkRenderPassBeginInfo render_pass_begin_info {};

        render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_begin_info.renderPass = render_pass_;
        render_pass_begin_info.framebuffer = framebuffers_[swapchain_index];
        render_pass_begin_info.renderArea.extent = swapchain_image_extent_;
        render_pass_begin_info.clearValueCount = 1;
        render_pass_begin_info.pClearValues = &clear_value;

        vkCmdBeginRenderPass(command_buffer_, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(command_buffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);
        vkCmdDraw(command_buffer_, 3, 1, 0, 0);

        vkCmdEndRenderPass(command_buffer_);

        {
            VkImageMemoryBarrier barrier {};

            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            barrier.srcQueueFamilyIndex = queue_family_index_;
            barrier.dstQueueFamilyIndex = queue_family_index_;
            barrier.image = swapchain_image;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.layerCount = 1;

            vkCmdPipelineBarrier(command_buffer_,
                                 VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
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

        vkQueueSubmit(queue_, 1, &submit_info, fence_);

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
    VkFence fence_;
    std::array<VkSemaphore, 2> semaphores_;
    VkSurfaceKHR surface_;
    VkSwapchainKHR swapchain_;
    vector<VkImage> swapchain_images_;
    VkExtent2D swapchain_image_extent_;
    vector<VkImageView> swapchain_image_views_;
    VkRenderPass render_pass_;
    vector<VkFramebuffer> framebuffers_;
    array<VkShaderModule, 2> shader_modules_;
    VkPipelineLayout pipeline_layout_;
    VkPipeline pipeline_;
};

//----------------------------------------------------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    Window_desc window_desc;

    window_desc.title = L"Chapter6"s;
    window_desc.extent = {512, 512, 1};

    Window window {window_desc};

    Chapter6 chapter6 {&window};

    window.run();

    return 0;
}

//----------------------------------------------------------------------------------------------------------------------
