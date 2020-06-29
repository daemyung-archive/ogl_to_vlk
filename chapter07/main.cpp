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
using namespace Platform;

//----------------------------------------------------------------------------------------------------------------------

constexpr auto image_available_index {0};
constexpr auto rendering_done_index {1};

//----------------------------------------------------------------------------------------------------------------------

class Chapter7 {
public:
    Chapter7(Window* window) :
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

    ~Chapter7()
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
        window_->startup_signal.connect(this, &Chapter7::on_startup);
        window_->shutdown_signal.connect(this, &Chapter7::on_shutdown);
        window_->render_signal.connect(this, &Chapter7::on_render);
    }

    void init_instance_()
    {
        // 벌칸 프로그램을 작성하는데 있어서 반드시 필요한 레이어입니다.
        // 하지만 CPU를 굉장히 많이 사용하기 때문에 개발중에만 사용해야 합니다.
        vector<const char*> layer_names {
            "VK_LAYER_KHRONOS_validation"
        };

        // 사용하려는 인스턴스 익스텐션의 이름을 정의합니다.
        // 사용하려는 익스텐션들이 시스템에서 지원되는지 먼저 확인해야합니다.
        // 예제의 간소화를 위해서 사용하려는 익스텐션들을 확인 없이 사용합니다.
        vector<const char*> extension_names {
            "VK_KHR_surface",
#if defined(_WIN64)
            "VK_KHR_win32_surface"
#else
            "VK_MVK_macos_surface"
#endif
        };

        // 생성하려는 인스턴스를 정의합니다.
        VkInstanceCreateInfo create_info {};

        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_info.enabledLayerCount = layer_names.size();
        create_info.ppEnabledLayerNames = &layer_names[0];
        create_info.enabledExtensionCount = extension_names.size();
        create_info.ppEnabledExtensionNames = &extension_names[0];

        // 인스턴스를 생성합니다.
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
        // 사용가능한 물리 디바이스의 수를 얻기 위한 변수를 선언합니다.
        uint32_t count {0};

        // 사용가능한 물리 디바이스의 수를 얻어옵니다.
        vkEnumeratePhysicalDevices(instance_, &count, nullptr);

        // 사용가능한 물리 디바이스들의 정보를 얻기 위한 변수를 선언합니다.
        vector<VkPhysicalDevice> physical_devices;

        // 사용가능한 물리 디바이스들의 정보를 얻기 위한 메모리를 할당합니다.
        physical_devices.resize(count);

        // 사용가능한 물리 디바이스들의 핸들을 얻어옵니다.
        vkEnumeratePhysicalDevices(instance_, &count, &physical_devices[0]);

        // 어플리케이션에선 사용가능한 물리 디바이스들 중에서 어플리케이션에
        // 적합한 물리 디바이스를 찾아야 합니다.
        // 예제의 간소화를 위해서 첫 번째 물리 디바이스를 사용합니다.
        physical_device_ = physical_devices[0];
    }

    void find_queue_family_index_()
    {
        // 사용가능한 큐 패밀리의 수를 얻기 위한 변수를 선언합니다.
        uint32_t count {0};

        // 사용가능한 큐 패밀리의 수를 얻어옵니다.
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device_, &count, 0);

        // 사용가능한 큐 패밀리의 정보를 얻기 위한 변수를 선언합니다.
        vector<VkQueueFamilyProperties> properties;

        // 사용가능한 큐 패밀리의 정보를 얻기 위한 메모리를 할당합니다.
        properties.resize(count);

        // 사용가능한 큐 패밀리의 정보를 얻어옵니다.
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device_, &count, &properties[0]);

        for (auto i = 0; i != properties.size(); ++i) {
            // 큐 패밀리에서 사용가능한 큐가 있는지 확인합니다.
            if (properties[i].queueCount < 1)
                continue;

            // 큐 패밀리가 그래픽스 기능을 제공하는지 확인합니다.
            if (VK_QUEUE_GRAPHICS_BIT & properties[i].queueFlags) {
                queue_family_index_ = i;
                break;
            }

            // 컴퓨트, 복사에 특화된 큐 패밀리가 존재할 수 있습니다.
            // 목적에 맞는 큐 패밀리를 사용하시면 어플리케이션을 더 효율적으로 동작시킬 수 있습니다.
            // 예제의 간소화를 위해서 그래픽스를 지원하는 한개의 큐 패밀리만 사용합니다.
        }
        assert(queue_family_index_ != UINT32_MAX);
    }

    void init_device_()
    {
        // 큐의 우선순위를 지정합니다.
        // 여러개의 큐를 사용할 경우 다르게 우선순위를 지정할 수 있습니다.
        constexpr auto priority = 1.0f;

        // 생성하려는 큐를 정의합니다.
        VkDeviceQueueCreateInfo queue_create_info {};

        queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.queueFamilyIndex = queue_family_index_;
        queue_create_info.queueCount = 1;
        queue_create_info.pQueuePriorities = &priority;

        // 사용하려는 디바이스 익스텐션의 이름을 정의합니다.
        // 사용하려는 익스텐션들이 시스템에서 지원되는지 먼저 확인해야합니다.
        // 예제의 간소화를 위해서 사용하려는 익스텐션들을 확인 없이 사용합니다.
        vector<const char*> extension_names {
            "VK_KHR_swapchain",
        };

        // 생성하려는 디바이스를 정의합니다.
        VkDeviceCreateInfo create_info {};

        create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        create_info.queueCreateInfoCount = 1;
        create_info.pQueueCreateInfos = &queue_create_info;
        create_info.enabledExtensionCount = extension_names.size();
        create_info.ppEnabledExtensionNames = &extension_names[0];

        // 디바이스를 생성합니다.
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
        // 디바이스로 부터 생성된 큐의 핸들을 얻어옵니다.
        vkGetDeviceQueue(device_, queue_family_index_, 0, &queue_);
    }

    void init_command_pool_()
    {
        // 생성하려는 커맨드 풀을 정의합니다.
        VkCommandPoolCreateInfo create_info {};

        create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        create_info.queueFamilyIndex = queue_family_index_;

        // 커맨드 풀을 생성합니다.
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
        // 생성할 커맨드 버퍼를 정의합니다.
        VkCommandBufferAllocateInfo allocate_info {};

        allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocate_info.commandPool = command_pool_;
        allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocate_info.commandBufferCount = 1;

        // 커맨드 버퍼를 생성합니다.
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
        // 세마포어는 큐와 큐의 동기화를 위해 사용됩니다.

        // 생성하려는 세마포어를 정의합니다.
        VkSemaphoreCreateInfo create_info {};

        create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        // 두 개의 세마포어를 생성합니다.
        // - 첫 번째 세마포어는 스왑체인 이미지가 준비된 후에 제출한 커맨드 버퍼가 처리되기 위해 사용됩니다.
        // - 두 번째 세마포어는 제출한 커맨드 버퍼가 처리된 후에 화면에 출력을 하기 위해 사용됩니다.
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
        // 펜스는 호스트와 디바이스의 동기화를 위해 사용됩니다.

        // 생성하려는 펜스를 정의합니다.
        VkFenceCreateInfo create_info {};

        create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        // 커맨드 버퍼가 모두 처리됬는지를 알기 위해 사용된 펜스는 처음에 시그널 상태면 어플리케이션의 로직이 단순해집니다.
        create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        // 펜스를 생성합니다.
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
        // 서피스는 윈도우 시스템마다 생성하는 API가 다릅니다.
        // 하지만 서피스를 생성하기 위해서 필요한 정보와 근본적인 방법은 같다는 것을 알아차릴 수 있습니다.

#if defined(__APPLE__)
        auto ns_window = (__bridge NSWindow*)window_->window();
        auto content_view = [ns_window contentView];

        // 애플 운영 체제에서 벌칸은 메탈을 기반으로 동작합니다. 즉 벌칸 API를 메탈로 구현한 것입니다.
        // 그러므로 애플 운영 체제에서 벌칸을 사용하기 위해선 CAMetalLayer에 대한 설정이 필요합니다.
        [content_view setLayer:[CAMetalLayer layer]];

        // 생성하려는 서피스를 정의합니다.
        VkMacOSSurfaceCreateInfoMVK create_info {};

        create_info.sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK;
        create_info.pView = (__bridge void*)content_view;

        // 서피스를 생성합니다.
        auto result = vkCreateMacOSSurfaceMVK(instance_, &create_info, nullptr, &surface_);
#elif defined(_WIN64)
        // 생성하려는 서피스를 정의합니다.
        VkWin32SurfaceCreateInfoKHR create_info {};

        create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        create_info.hinstance = GetModuleHandle(NULL);
        create_info.hwnd = static_cast<HWND>(window_->window());

        // 서피스를 생성합니다.
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

        // 서피스의 지원 여부를 얻어올 변수를 선언합니다.
        VkBool32 supported;

        // 물리 디바이스의 큐 패밀리가 서피스를 지원하는지 확인합니다.
        vkGetPhysicalDeviceSurfaceSupportKHR(physical_device_, queue_family_index_, surface_, &supported);
        assert(supported == VK_TRUE);
    }

    auto default_surface_format_()
    {
        // 사용 가능한 서피스 포맷의 수를 얻기 위한 변수를 선언합니다.
        uint32_t count;

        // 사용 가능한 서피스 포맷의 수를 얻어옵니다.
        vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device_, surface_, &count, nullptr);

        // 사용가능한 서피스 포맷들을 얻기 위한 변수를 선언합니다.
        vector<VkSurfaceFormatKHR> formats;

        // 사용가능한 서피스 포맷들을 얻기 위한 메모리를 할당합니다.
        formats.resize(count);

        // 사용가능한 서피스 포맷들을 얻어옵니다.
        vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device_, surface_, &count, &formats[0]);

        // 어플리케이션은 여러 조건을 고려해서 최적의 서피스 포맷을 선택해야합니다.
        // 예제의 간소화를 위해서 첫 번째 서피스 포맷을 사용합니다.
        return formats[0];
    }

    void init_swapchain_()
    {
        auto surface_format = default_surface_format_();

        // 서피스의 능력을 얻기 위한 변수를 선언합니다.
        VkSurfaceCapabilitiesKHR surface_capabilities;

        // 서피스의 능력을 얻어옵니다.
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device_, surface_, &surface_capabilities);

        // 사용할 수 있는 컴포짓 알파 모드를 검색한다.
        VkCompositeAlphaFlagBitsKHR composite_alpha {static_cast<VkCompositeAlphaFlagBitsKHR>(0)};

        // 사용 가능한 컴포지트 알파 모드를 검색합니다.
        for (auto i = 0; i != 32; ++i) {
            VkCompositeAlphaFlagBitsKHR flag = static_cast<VkCompositeAlphaFlagBitsKHR>(0x1 << i);

            // 예제의 간소화를 위해 사용가능한 첫 번째 컴포지트 알파 모드를 사용합니다.
            if (surface_capabilities.supportedUsageFlags & flag) {
                composite_alpha = flag;
                break;
            }
        }

        // 스왑체인 이미지의 크기를 저장합니다.
        swapchain_image_extent_ = surface_capabilities.currentExtent;

        // 생성하려는 스왑체인을 정의합니다.
        // 예제의 간소화를 위해 FIFO 프레젠트 모드를 사용합니다.
        VkSwapchainCreateInfoKHR create_info {};

        create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        create_info.surface = surface_;
        create_info.minImageCount = 2; // 더블 버퍼링을 사용합니다.
        create_info.imageFormat = surface_format.format;
        create_info.imageColorSpace = surface_format.colorSpace;
        create_info.imageExtent = surface_capabilities.currentExtent;
        create_info.imageArrayLayers = 1; // VR과 같은 특수한 경우를 제외하곤 항상 1입니다.
        create_info.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.preTransform = surface_capabilities.currentTransform;
        create_info.compositeAlpha = composite_alpha;
        create_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;

        // 스왑체인을 생성합니다.
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
        // 사용 가능한 스왑체인 이미지의 개수를 얻기 위한 변수를 선언합니다.
        uint32_t count;

        // 사용 가능한 스왑체인 이미지의 개수를 얻어옵니다.
        vkGetSwapchainImagesKHR(device_, swapchain_, &count, nullptr);

        // 사용 가능한 스왑체인 이미지를 얻기 위한 메모리를 할당합니다.
        swapchain_images_.resize(count);

        // 사용 가능한 스왑체인 이미지를 얻어옵니다.
        vkGetSwapchainImagesKHR(device_, swapchain_, &count, &swapchain_images_[0]);

        // 스왑체인 생성시 정의한 이미지의 개수는 최소 이미지의 개수이기 때문에
        // 실제로 더 많은 이미지가 생성될 수 있습니다.

        // 커맨드를 기록하기 위해 커맨드 버퍼의 사용목적을 정의한다.
        VkCommandBufferBeginInfo begin_info {};

        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        // 커맨드 버퍼에 커맨드 기록을 시작한다.
        vkBeginCommandBuffer(command_buffer_, &begin_info);

        // 스왑체인 이미지의 레이아웃을 PRESENT_SRC로 변경하는 배리어를 정의한다.
        // PRESENT_SRC로 변경하는 이유는 렌더링을 위한 커맨드를 단순화하기 위해서이다.
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

        // 정의된 배리어를 실행하는 커맨드를 커맨드 버퍼에 기록한다.
        vkCmdPipelineBarrier(command_buffer_,
                             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                             0,
                             0, nullptr,
                             0, nullptr,
                             barriers.size(), &barriers[0]);

        // 커맨드 버퍼에 커맨드 기록을 끝마친다.
        vkEndCommandBuffer(command_buffer_);

        // 어떤 기록된 커맨드를 큐에 제출할지 정의한다.
        VkSubmitInfo submit_info {};

        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffer_;

        // 기록된 커맨드를 큐에 제출한다.
        vkQueueSubmit(queue_, 1, &submit_info, VK_NULL_HANDLE);

        // 커맨드 버퍼들이 모두 처리될 때까지 기다린다.
        vkDeviceWaitIdle(device_);
    }

    void init_swapchain_image_views_()
    {
        // 그래픽스 파이프라인에서 이미지를 접근하기 위해선 이미지 뷰가 필요합니다.

        auto surface_format = default_surface_format_();

        // 생성하려는 이미지 뷰를 정의합니다.
        VkImageViewCreateInfo create_info {};

        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format = surface_format.format;
        create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.layerCount = 1;

        // 이미지 뷰를 위한 메모리를 할당합니다.
        swapchain_image_views_.resize(swapchain_images_.size());

        for (auto i = 0; i != swapchain_images_.size(); ++i) {
            // 이미지 뷰를 생성할 이미지를 정의합니다.
            create_info.image = swapchain_images_[i];

            // 이미지 뷰를 생성합니다.
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

        // 렌더패스의 어태치먼트를 정의합니다.
        VkAttachmentDescription attachment_desc {};

        attachment_desc.format = surface_format.format;
        attachment_desc.samples = VK_SAMPLE_COUNT_1_BIT;
        attachment_desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment_desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment_desc.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachment_desc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        // 서브패스에서 렌더패스의 어떤 어태치먼트를 참조할지 정의합니다.
        VkAttachmentReference color_attachment_ref {};

        color_attachment_ref.attachment = 0;
        color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        // 렌더패스의 서브패스를 정의합니다.
        VkSubpassDescription subpass_desc {};

        subpass_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass_desc.colorAttachmentCount = 1;
        subpass_desc.pColorAttachments = &color_attachment_ref;

        // 렌더패스를 정의합니다.
        VkRenderPassCreateInfo create_info {};

        create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        create_info.attachmentCount = 1;
        create_info.pAttachments = &attachment_desc;
        create_info.subpassCount = 1;
        create_info.pSubpasses = &subpass_desc;

        // 렌더패스를 생성합니다.
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
        // 생성하려는 프레임버퍼를 정의합니다.
        VkFramebufferCreateInfo create_info {};

        create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        create_info.renderPass = render_pass_;
        create_info.attachmentCount = 1;
        create_info.width = swapchain_image_extent_.width;
        create_info.height = swapchain_image_extent_.height;
        create_info.layers = 1;

        // 프레임버퍼를 위한 메모리를 할당합니다.
        framebuffers_.resize(swapchain_image_views_.size());

        for (auto i = 0; i != swapchain_image_views_.size(); ++i) {
            create_info.pAttachments = &swapchain_image_views_[i];

            // 프레임버퍼를 생성합니다.
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

    void fini_instance_()
    {
        // 생성한 인스턴스를 파괴합니다.
        vkDestroyInstance(instance_, nullptr);
    }

    void fini_device_()
    {
        // 생성한 디바이스를 파괴합니다.
        vkDestroyDevice(device_, nullptr);
    }

    void fini_command_pool_()
    {
        // 생성한 커맨드 풀을 파괴합니다.
        // 커맨드 풀이 파괴되면 커맨드 버퍼도 파괴됩니다.
        vkDestroyCommandPool(device_, command_pool_, nullptr);
    }

    void fini_semaphores_()
    {
        // 생성한 세마포어를 파괴합니다.
        for (auto& semaphore : semaphores_)
            vkDestroySemaphore(device_, semaphore, nullptr);
    }

    void fini_fence_()
    {
        // 생성한 펜스를 파괴합니다.
        vkDestroyFence(device_, fence_, nullptr);
    }

    void fini_surface_()
    {
        // 생성한 서피스를 파괴합니다.
        vkDestroySurfaceKHR(instance_, surface_, nullptr);
    }

    void fini_swapchain_()
    {
        // 생성한 스왑체인을 파괴합니다.
        vkDestroySwapchainKHR(device_, swapchain_, nullptr);
    }

    void fini_swapchain_image_views_()
    {
        // 생성한 이미지 뷰를 파괴합니다.
        for (auto& image_view : swapchain_image_views_)
            vkDestroyImageView(device_, image_view, nullptr);
    }

    void fini_render_pass_()
    {
        // 생성한 렌더 패스를 파괴합니다.
        vkDestroyRenderPass(device_, render_pass_, nullptr);
    }

    void fini_framebuffers_()
    {
        // 생성한 프레임버퍼를 파괴합니다.
        for (auto& framebuffer : framebuffers_)
            vkDestroyFramebuffer(device_, framebuffer, nullptr);
    }

    void on_startup()
    {
        init_surface_();
        init_swapchain_();
        init_swapchain_images_();
        init_swapchain_image_views_();
        init_render_pass_();
        init_framebuffers_();
    }

    void on_shutdown()
    {
        vkDeviceWaitIdle(device_);

        fini_framebuffers_();
        fini_render_pass_();
        fini_swapchain_image_views_();
        fini_swapchain_();
        fini_surface_();
    }

    void on_render()
    {
        // 스왑체인으로부터 사용가능한 이미지 인덱스를 얻어옵니다.
        // 세마포어를 통해서 GPU에서 스왑체인 이미지가 언제 사용 가능한지 알 수 있습니다.
        uint32_t swapchain_index;
        vkAcquireNextImageKHR(device_, swapchain_, UINT64_MAX,
                              semaphores_[image_available_index], VK_NULL_HANDLE,
                              &swapchain_index);

        auto& swapchain_image = swapchain_images_[swapchain_index];

        // 새로운 프레임을 렌더링하기 위해선 제출한 커맨드 버퍼가 처리되었는지를 확인해야합니다.
        // 다 처리되었는지를 알기 위해서 큐에 커맨드 버퍼를 제출할 때 파라미터로 펜스를 넘길 수 있습니다.
        // 커맨드 버퍼를 리셋하기 전에 펜스의 상태를 확인한 뒤 언시그널드 상태이면 시그널 상태가 될 때까지 기다립니다.
        if (VK_NOT_READY == vkGetFenceStatus(device_, fence_))
            vkWaitForFences(device_, 1, &fence_, VK_TRUE, UINT64_MAX);

        // 펜스를 언시그널 상태로 변경합니다.
        vkResetFences(device_, 1, &fence_);

        // 커맨드 버퍼를 재사용하기 위해서 커맨드 버퍼를 리셋합니다.
        vkResetCommandBuffer(command_buffer_, 0);
        // 커맨드 버퍼에 기록하는 커맨드들은 변하지 않기 때문에 다시 기록할 필요는 없습니다.
        // 하지만 일반적인 경우에 매 프레임마다 필요한 커맨드들이 다르기 때문에 리셋하고 다시 기록합니다.

        // 커맨드를 기록하기 위해 커맨드 버퍼를 기록중 상태로 전이할 때 필요한 정보를 정의합니다.
        VkCommandBufferBeginInfo begin_info {};

        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        // 커맨드 버퍼를 기록중 상태로 전이합니다.
        vkBeginCommandBuffer(command_buffer_, &begin_info);

        {
            // 이미지 배리어를 정의하기 위한 변수를 선언합니다.
            VkImageMemoryBarrier barrier {};

            // 이미지 배리어를 정의합니다.
            // 이미지에 렌더링을 하기 위해서는 COLOR_ATTACHMENT_OPTIMAL이어야 합니다.
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            barrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            barrier.srcQueueFamilyIndex = queue_family_index_;
            barrier.dstQueueFamilyIndex = queue_family_index_;
            barrier.image = swapchain_image;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.layerCount = 1;

            // 이미지 레이아웃 변경을 위한 파이프라인 배리어 커맨드를 기록합니다.
            vkCmdPipelineBarrier(command_buffer_,
                                 VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);
        }

        // 클리어 색상을 정의하기 위한 변수를 선언합니다.
        VkClearValue clear_value;

        // 클리어 색상을 정의합니다.
        clear_value.color.float32[0] = 1.0f; // R
        clear_value.color.float32[1] = 1.0f; // G
        clear_value.color.float32[2] = 0.0f; // B
        clear_value.color.float32[3] = 1.0f; // A

        // 렌더 패스를 어떻게 시작할지 정의합니다.
        VkRenderPassBeginInfo render_pass_begin_info {};

        render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_begin_info.renderPass = render_pass_;
        render_pass_begin_info.framebuffer = framebuffers_[swapchain_index];
        render_pass_begin_info.renderArea.extent = swapchain_image_extent_;
        render_pass_begin_info.clearValueCount = 1;
        render_pass_begin_info.pClearValues = &clear_value;

        // 렌더 패스를 시작합니다.
        vkCmdBeginRenderPass(command_buffer_, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

        // 렌더 패스를 종료합니다.
        vkCmdEndRenderPass(command_buffer_);

        {
            // 이미지 배리어를 정의하기 위한 변수를 선언합니다.
            VkImageMemoryBarrier barrier {};

            // 이미지 배리어를 정의합니다. 이미지를 화면에 출력하기 위해서는
            // 이미지 레이아웃이 반드시 아래 레이아웃 중에 한 레이아웃이어야 합니다.
            // - PRESENT_SRC_KHR
            // - SHARED_PRESENT_KHR
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            barrier.dstAccessMask = 0;
            barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            barrier.srcQueueFamilyIndex = queue_family_index_;
            barrier.dstQueueFamilyIndex = queue_family_index_;
            barrier.image = swapchain_image;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.layerCount = 1;

            // 이미지 레이아웃 변경을 위한 파이프라인 배리어 커맨드를 기록합니다.
            vkCmdPipelineBarrier(command_buffer_,
                                 VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);
        }

        // 필요한 모든 커맨드들을 기록했기 때문에 커맨드 버퍼의 커맨드 기록을 끝마칩니다.
        // 커맨드 버퍼의 상태는 실행 가능 상태입니다.
        vkEndCommandBuffer(command_buffer_);

        // 세마포어가 반드시 시그널 되야하는 파이프라인 스테이지를 정의합니다.
        constexpr VkPipelineStageFlags wait_dst_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        // 큐에 제출할 커맨드 버퍼와 동기화를 정의하기 위한 변수를 선언합니다.
        VkSubmitInfo submit_info {};

        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        // 픽셀의 결과를 계산한 후에 스왑체인 이미지가 준비될 때까지 기다립니다.
        // 기다리지 않고 픽셀의 결과를 쓴다면 원하지 않는 결과가 화면에 출력됩니다.
        submit_info.waitSemaphoreCount = 1;
        submit_info.pWaitSemaphores = &semaphores_[image_available_index];
        submit_info.pWaitDstStageMask = &wait_dst_stage_mask;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffer_;
        // 결과를 올바르게 출력하기 위해선 커맨드 버퍼가 처리될 때까지 기다려야합니다,
        // 커맨드 버퍼가 처리된 시점을 알려주는 세마포어를 정의합니다.
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores = &semaphores_[rendering_done_index];

        // 큐에 커맨드 버퍼를 제출합니다.
        vkQueueSubmit(queue_, 1, &submit_info, fence_);

        // 화면에 출력되는 이미지를 정의합니다.
        VkPresentInfoKHR present_info {};

        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        // 커맨드 버퍼가 처리되기를 보장하기 위해 기다려야하는 세마포어를 정의합니다.
        // 기다리지 않고 출력한다면 원하지 않는 결과가 화면에 출력됩니다.
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = &semaphores_[rendering_done_index];
        present_info.swapchainCount = 1;
        present_info.pSwapchains = &swapchain_;
        present_info.pImageIndices = &swapchain_index;

        // 화면에 이미지를 출력합니다.
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
};

//----------------------------------------------------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    Window_desc window_desc;

    window_desc.title = L"Chapter07"s;
    window_desc.extent = {512, 512, 1};

    Window window {window_desc};

    Chapter7 chapter7 {&window};

    window.run();

    return 0;
}

//----------------------------------------------------------------------------------------------------------------------
