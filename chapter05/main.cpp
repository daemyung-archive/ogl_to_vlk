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
#include <vulkan/vulkan.h>
#include <platform/Window.h>

using namespace std;
using namespace Platform;

//----------------------------------------------------------------------------------------------------------------------

class Chapter5 {
public:
    Chapter5(Window* window) :
        window_ {window},
        instance_ {VK_NULL_HANDLE},
        physical_device_ {VK_NULL_HANDLE},
        queue_family_index_ {UINT32_MAX},
        device_ {VK_NULL_HANDLE},
        queue_ {VK_NULL_HANDLE},
        command_pool_ {VK_NULL_HANDLE},
        command_buffer_ {VK_NULL_HANDLE},
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
    }

    ~Chapter5()
    {
        fini_command_pool_();
        fini_device_();
        fini_instance_();
    }

private:
    void init_signals_()
    {
        window_->startup_signal.connect(this, &Chapter5::on_startup);
        window_->shutdown_signal.connect(this, &Chapter5::on_shutdown);
        window_->render_signal.connect(this, &Chapter5::on_render);
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
        create_info.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
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

        // 스왑체인 이미지의 레이아웃을 PRESENT_SRC로 변경하는 베리어를 정의한다.
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

        // 정의된 베리어를 실행하는 커맨드를 커맨드 버퍼에 기록한다.
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
        // 현재 출력 가능한 스왑체인 이미지의 인덱스를 가져오기 위한 변수룰 선언합니다.
        uint32_t swapchain_index;

        // 현재 출력 가능한 스왑체인 이미지의 인덱스를 가져옵니다.
        vkAcquireNextImageKHR(device_, swapchain_, UINT64_MAX, VK_NULL_HANDLE, VK_NULL_HANDLE, &swapchain_index);

        auto& swapchain_image = swapchain_images_[swapchain_index];

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
            // 이미지 베리어를 정의하기 위한 변수를 선언합니다.
            VkImageMemoryBarrier barrier {};

            // 이미지 베리어를 정의합니다. 이미지를 클리어하기 위해서는
            // 이미지 레이아웃이 반드시 아래 레이아웃 중에 한 레이아웃이어야 합니다.
            // - SHARED_PRESENT_KHR
            // - GENERAL
            // - TRANSFER_DST_OPTIMAL
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.srcQueueFamilyIndex = queue_family_index_;
            barrier.dstQueueFamilyIndex = queue_family_index_;
            barrier.image = swapchain_image;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.layerCount = 1;

            // 이미지 레이아웃 변경을 위한 파이프라인 베리어 커맨드를 기록합니다.
            vkCmdPipelineBarrier(command_buffer_,
                                 VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);
        }

        // 클리어 색상을 정의하기 위한 변수를 선언합니다.
        VkClearColorValue clear_color;

        // 클리어 색상을 정의합니다.
        clear_color.float32[0] = 1.0f; // R
        clear_color.float32[1] = 0.0f; // G
        clear_color.float32[2] = 1.0f; // B
        clear_color.float32[3] = 1.0f; // A

        // 클리어할 이미지 영역을 정의하기 위한 변수를 선언합니다.
        VkImageSubresourceRange subresource_range {};

        // 클리어할 이미지 영역을 정의합니다.
        subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresource_range.levelCount = 1;
        subresource_range.layerCount = 1;

        // 이미지를 클리어하는 커맨드를 기록합니다.
        vkCmdClearColorImage(command_buffer_,
                             swapchain_image,
                             VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                             &clear_color,
                             1,
                             &subresource_range);

        {
            // 이미지 베리어를 정의하기 위한 변수를 선언합니다.
            VkImageMemoryBarrier barrier {};

            // 이미지 베리어를 정의합니다. 이미지를 화면에 출력하기 위해서는
            // 이미지 레이아웃이 반드시 아래 레이아웃 중에 한 레이아웃이어야 합니다.
            // - PRESENT_SRC_KHR
            // - SHARED_PRESENT_KHR
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            barrier.srcQueueFamilyIndex = queue_family_index_;
            barrier.dstQueueFamilyIndex = queue_family_index_;
            barrier.image = swapchain_image;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.layerCount = 1;

            // 이미지 레이아웃 변경을 위한 파이프라인 베리어 커맨드를 기록합니다.
            vkCmdPipelineBarrier(command_buffer_,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);
        }

        // 필요한 모든 명령들을 기록했기 때문에 커맨드 버퍼의 커맨드 기록을 끝마칩니다.
        // 커맨드 버퍼의 상태는 실행 가능 상태입니다.
        vkEndCommandBuffer(command_buffer_);

        // 큐에 제출할 커맨드 버퍼와 동기화를 정의하기 위한 변수를 선언합니다.
        VkSubmitInfo submit_info {};

        // 큐에 제출할 커맨드 버퍼를 정의합니다.
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffer_;

        // 커맨드 버퍼를 큐에 제출합니다.
        vkQueueSubmit(queue_, 1, &submit_info, VK_NULL_HANDLE);

        // 제출된 커맨드 버퍼들이 모두 처리될 때까지 기다립니다.
        vkDeviceWaitIdle(device_);

        // 화면에 출력되는 이미지를 정의하기 위한 변수를 선언합니다.
        VkPresentInfoKHR present_info {};

        // 화면에 출력되는 이미지를 정의합니다.
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
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
    VkSurfaceKHR surface_;
    VkSwapchainKHR swapchain_;
    vector<VkImage> swapchain_images_;
};

//----------------------------------------------------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    Window_desc window_desc;

    window_desc.title = L"Chapter05"s;
    window_desc.extent = {512, 512, 1};

    Window window {window_desc};

    Chapter5 chapter5 {&window};

    window.run();

    return 0;
}

//----------------------------------------------------------------------------------------------------------------------
