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

class Chapter4 {
public:
    Chapter4(Window* window) :
        window_ {window},
        instance_ {VK_NULL_HANDLE},
        physical_device_ {VK_NULL_HANDLE},
        queue_family_index_ {UINT32_MAX},
        device_ {VK_NULL_HANDLE},
        queue_ {VK_NULL_HANDLE},
        surface_ {VK_NULL_HANDLE},
        swapchain_ {VK_NULL_HANDLE}
    {
        init_signals_();
        print_instance_extensions_();
        init_instance_();
        find_best_physical_device_();
        find_queue_family_index_();
        print_device_extensions_();
        init_device_();
        init_queue_();
    }

    ~Chapter4()
    {
        fini_device_();
        fini_instance_();
    }

private:
    void init_signals_()
    {
        // 윈도우에서 제공되는 이벤트 중에 필요한 이벤트들을 구독합니다.
        window_->startup_signal.connect(this, &Chapter4::on_startup);
        window_->shutdown_signal.connect(this, &Chapter4::on_shutdown);
    }

    void print_instance_extensions_()
    {
        // 사용가능한 익스텐션의 수를 얻기 위한 변수를 선언합니다.
        uint32_t count {0};

        // 사용가능한 익스텐션의 수를 얻어옵니다.
        vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);

        // 사용가능한 익스텐션들의 정보를 얻기 위한 변수를 선언합니다.
        vector<VkExtensionProperties> properties;

        // 사용가능한 익스텐션들의 정보를 얻기 위한 메모리를 할당합니다.
        properties.resize(count);

        // 사용가능한 익스텐션들의 정보를 얻어옵니다.
        vkEnumerateInstanceExtensionProperties(nullptr, &count, &properties[0]);

        // 익스텐션들의 정보를 출력합니다.
        for (auto& props : properties) {
            cout << "Name: "    << props.extensionName << '\n'
                 << "Version: " << props.specVersion   << endl;
        }
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

    void print_device_extensions_()
    {
        // 사용가능한 익스텐션의 수를 얻기 위한 변수를 선언합니다.
        uint32_t count {0};

        // 사용가능한 익스텐션의 수를 얻어옵니다.
        vkEnumerateDeviceExtensionProperties(physical_device_, nullptr, &count, nullptr);

        // 사용가능한 익스텐션들의 정보를 얻기 위한 변수를 선언합니다.
        vector<VkExtensionProperties> properties;

        // 사용가능한 익스텐션들의 정보를 얻기 위한 메모리를 할당합니다.
        properties.resize(count);

        // 사용가능한 익스텐션들의 정보를 얻어옵니다.
        vkEnumerateDeviceExtensionProperties(physical_device_, nullptr, &count, &properties[0]);

        // 익스텐션들의 정보를 출력합니다.
        for (auto& props : properties) {
            cout << "Name: "    << props.extensionName << '\n'
                 << "Version: " << props.specVersion   << endl;
        }
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

    void print_present_modes_()
    {
        // 사용 가능한 프레젠트 모드의 수를 얻기 위한 변수를 선언합니다.
        uint32_t count;

        // 사용 가능한 프레젠트 모드의 수를 얻어옵니다.
        vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device_, surface_, &count, nullptr);

        // 사용 가능한 프레젠트 모드들을 얻기 위한 변수를 선언합니다.
        vector<VkPresentModeKHR> modes;

        // 사용 가능한 프레젠트 모드들을 얻기 위한 메모리를 할당합니다.
        modes.resize(count);

        // 사용 가능한 프레젠트 모드들을 얻어옵니다.
        vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device_, surface_, &count, &modes[0]);

        // 사용 가능한 프레젠트 모드들을 출력합니다.
        for (auto& mode : modes) {
            switch (mode) {
                case VK_PRESENT_MODE_IMMEDIATE_KHR:
                    cout << "VK_PRESENT_MODE_IMMEDIATE_KHR";
                    break;
                case VK_PRESENT_MODE_MAILBOX_KHR:
                    cout << "VK_PRESENT_MODE_MAILBOX_KHR";
                    break;
                case VK_PRESENT_MODE_FIFO_KHR:
                    cout << "VK_PRESENT_MODE_FIFO_KHR";
                    break;
                case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
                    cout << "VK_PRESENT_MODE_FIFO_RELAXED_KHR";
                    break;
                default:
                    break;
            }
            cout << endl;
        }

        // 어플리케이션에 적합한 프레젠트 모드를 선택해야 합니다.
        // 예제의 간소화를 위해 FIFO 모드를 사용합니다.
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
        create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
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
        print_present_modes_();
        init_swapchain_();
        init_swapchain_images_();
    }

    void on_shutdown()
    {
        fini_swapchain_();
        fini_surface_();
    }

private:
    Window* window_;
    VkInstance instance_;
    VkPhysicalDevice physical_device_;
    uint32_t queue_family_index_;
    VkDevice device_;
    VkQueue queue_;
    VkSurfaceKHR surface_;
    VkSwapchainKHR swapchain_;
    vector<VkImage> swapchain_images_;
};

//----------------------------------------------------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    // 생성할 윈도우를 정의합니다.
    Window_desc window_desc;

    window_desc.title = L"Chapter04"s;
    window_desc.extent = {512, 512, 1};

    // 윈도우를 생성합니다.
    Window window {window_desc};

    // Chapter4 예제를 생성합니다.
    Chapter4 chapter4 {&window};

    // 윈도우를 실행시킵니다.
    window.run();

    return 0;
}

//----------------------------------------------------------------------------------------------------------------------
