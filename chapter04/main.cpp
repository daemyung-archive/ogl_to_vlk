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
using namespace Platform_lib;

//----------------------------------------------------------------------------------------------------------------------

class Chapter2 {
public:
    Chapter2(Window* window) :
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

    ~Chapter2()
    {
        fini_device_();
        fini_instance_();
    }

private:
    void init_signals_()
    {
        // 윈도우에서 제공되는 이벤트 함수들 중 필요한 이벤트를 구독한다.
        window_->startup_signal.connect(this, &Chapter2::on_startup);
        window_->shutdown_signal.connect(this, &Chapter2::on_shutdown);
    }

    void print_instance_extensions_()
    {
        // 사용가능한 익스텐션의 개수를 가져온다.
        uint32_t count {0};
        vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);

        // 사용가능한 익스텐션의 정보를 가져온다.
        vector<VkExtensionProperties> properties {count};
        vkEnumerateInstanceExtensionProperties(nullptr, &count, &properties[0]);

        // 사용가능한 익스텐션의 정보를 출력한다.
        for (auto& props : properties) {
            cout << "Extension Name : " << props.extensionName << '\n'
                 << "Spec Version   : " << props.specVersion   << '\n' << endl;
        }

        // 이러한 과정을 통해서 어플리케이션 동작에 필요한 익스텐션들이 지원되는지 확인할 수 있다.
        // 반드시 필요한 익스텐션이 지원되지 않는다면 이 과정을 통해 실행을 중단하고 사용자에게 원인을 알릴 수 있다.
    }

    void init_instance_()
    {
        vector<const char*> layer_names {
            "VK_LAYER_KHRONOS_validation"
        };

        // 사용하고자 하는 인스턴스 익스텐션을 정의한다.
        // 인스턴스 생성시 사용하려는 인스턴스 익스텐션을 정의하지 않으면 사용할 수 없다.
        vector<const char*> extension_names {
            "VK_KHR_surface",
#if defined(_WIN64)
            "VK_KHR_win32_surface"
#else
            "VK_MVK_macos_surface"
#endif
        };

        VkInstanceCreateInfo create_info {};

        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_info.enabledLayerCount = layer_names.size();
        create_info.ppEnabledLayerNames = &layer_names[0];

        // 사용하려는 인스턴스 익스텐션이 정의된 메모리를 정의한다.
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

    void print_device_extensions_()
    {
        // 사용가능한 디바이스 익스텐션의 개수를 가져온다.
        uint32_t count {0};
        vkEnumerateDeviceExtensionProperties(physical_device_, nullptr, &count, nullptr);

        // 사용가능한 디바이스 익스텐션의 정보를 가져온다.
        vector<VkExtensionProperties> properties {count};
        vkEnumerateDeviceExtensionProperties(physical_device_, nullptr, &count, &properties[0]);

        // 사용가능한 디바이스 익스텐션의 정보를 출력한다.
        for (auto& props : properties) {
            cout << "Extension Name : " << props.extensionName << '\n'
                 << "Spec Version   : " << props.specVersion   << '\n' << endl;
        }

        // 이러한 과정을 통해서 어플리케이션 동작에 필요한 익스텐션들이 지원되는지 확인할 수 있다.
        // 반드시 필요한 익스텐션이 지원되지 않는다면 이 과정을 통해 실행을 중단하고 사용자에게 원인을 알릴 수 있다.
    }

    void init_device_()
    {
        constexpr auto priority = 1.0f;

        VkDeviceQueueCreateInfo queue_create_info {};

        queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.queueFamilyIndex = queue_family_index_;
        queue_create_info.queueCount = 1;
        queue_create_info.pQueuePriorities = &priority;

        // 사용하려는 디바이스 익스텐션을 정의한다.
        vector<const char*> extension_names {
            "VK_KHR_swapchain",
        };

        VkDeviceCreateInfo create_info {};

        create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        create_info.queueCreateInfoCount = 1;
        create_info.pQueueCreateInfos = &queue_create_info;

        // 사용하려는 인스턴스 익스텐션이 정의된 메모리를 정의한다.
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

    void init_surface_()
    {
        // VkSurface는 플랫폼마다 호출되야하는 함수 이름이 다르다. 왜냐하면 윈도우 시스템은 플랫폼 종속적이기 때문이다.
        // 하지만 플랫폼은 다를지라도 생성되는 과정을 살펴보면 생성방법은 동일한것을 알 수 있다.
        // 생성하기 위해서 윈도우 네이티브 핸들이 필요하며 이를 통하 VkSurfaceKHR을 생성할 수 있다.

#if defined(__APPLE__)
        auto ns_window = (__bridge NSWindow*)window_->window();
        auto content_view = [ns_window contentView];

        // CAMetalLayer는 Vulkan을 애플 플랫폼에서 동작시키기위해 반드시 필요하다.
        // 그러므로 어플리케이션이 생성하지 않았다면 직접 생성한다.
        [content_view setLayer:[CAMetalLayer layer]];

        // 생성하려는 서피스를 정의한다.
        VkMacOSSurfaceCreateInfoMVK create_info {};

        create_info.sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK;
        create_info.pView = (__bridge void*)content_view; // 윈도우 네이티브 핸들을 정의한다.

        // MacOS에 해당하는 생성함수를 호출한다.
        auto result = vkCreateMacOSSurfaceMVK(instance_, &create_info, nullptr, &surface_);
#elif defined(_WIN64)
        // 생성하려는 서비스를 정의한다.
        VkWin32SurfaceCreateInfoKHR create_info {};

        create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        create_info.hinstance = GetModuleHandle(NULL);
        create_info.hwnd = static_cast<HWND>(window_->window()); // 윈도우 네이티브 핸들을 정의한다.

        // Win32에 해당하는 생성함수를 호출한다.
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

        // 사용하고자 하는 VkPhysicalDevice가 서피스를 지원하는지 확인한다.
        VkBool32 supported;
        vkGetPhysicalDeviceSurfaceSupportKHR(physical_device_, queue_family_index_, surface_, &supported);
        assert(supported == VK_TRUE);
    }

    void print_present_modes_()
    {
        // 사용가능한 프레젠트 모드의 개수를 가져온다.
        uint32_t count;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device_, surface_, &count, nullptr);

        // 사용가능한 프레젠트 모드를 가져온다.
        vector<VkPresentModeKHR> modes {count};
        vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device_, surface_, &count, &modes[0]);

        // 사용가능한 프레젠트를 출력한다.
        for (auto& mode : modes) {
            switch (mode) {
                case VK_PRESENT_MODE_IMMEDIATE_KHR:
                    cout << "VK_PRESENT_MODE_IMMEDIATE_KHR" << '\n';
                    break;
                case VK_PRESENT_MODE_MAILBOX_KHR:
                    cout << "VK_PRESENT_MODE_MAILBOX_KHR" << '\n';
                    break;
                case VK_PRESENT_MODE_FIFO_KHR:
                    cout << "VK_PRESENT_MODE_FIFO_KHR" << '\n';
                    break;
                case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
                    cout << "VK_PRESENT_MODE_FIFO_RELAXED_KHR" << '\n';
                    break;
                default:
                    break;
            }
            cout << endl;
        }

        // 프레젠트 모드는 서로 다른 특징이 있으며 어플리케이션의 특성에 맞게 선택되어야 한다.
    }

    auto default_surface_format_()
    {
        // 사용가능한 서피스 포멧의 개수를 가져온다.
        uint32_t count;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device_, surface_, &count, nullptr);

        // 사용가능한 서비스 포멧을 가져온다.
        vector<VkSurfaceFormatKHR> formats {count};
        vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device_, surface_, &count, &formats[0]);

        // 상용화 어플리케이션은 여러가지 조건을 따져서 최적의 서피스 포멧을 선택해야한다.
        // 그러나 여기선 가장 첫번째 서비스 포멧을 선택한다.
        return formats[0];
    }

    void init_swapchain_()
    {
        auto surface_format = default_surface_format_();
        auto present_mode = VK_PRESENT_MODE_FIFO_KHR;

        // 서피스의 사용가능한 능력를 가져온다.
        VkSurfaceCapabilitiesKHR surface_capabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device_, surface_, &surface_capabilities);

        // 사용할 수 있는 컴포짓 알파 모드를 검색한다.
        VkCompositeAlphaFlagBitsKHR composite_alpha {static_cast<VkCompositeAlphaFlagBitsKHR>(0)};
        for (auto i = 0; i != 32; ++i) {
            VkCompositeAlphaFlagBitsKHR flag = static_cast<VkCompositeAlphaFlagBitsKHR>(0x1 << i);
            if (surface_capabilities.supportedUsageFlags & flag) {
                composite_alpha = flag;
                break;
            }
        }

        // 생성하려는 스왑채인을 정의한다.
        VkSwapchainCreateInfoKHR create_info {};

        create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        create_info.surface = surface_;
        create_info.minImageCount = 2;
        create_info.imageFormat = surface_format.format;
        create_info.imageColorSpace = surface_format.colorSpace;
        create_info.imageExtent = surface_capabilities.currentExtent; // 스왑체인의 이미지의 크기는 VkSurfaceCapabilitiesKHR의 범위 내에서 생성되야 한다.
        create_info.imageArrayLayers = 1; // 스왑체인 이미지는 여러개의 레이어가 특수한 경우를 제외하곤 항상 1이다.
        create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.preTransform = surface_capabilities.currentTransform;
        create_info.compositeAlpha = composite_alpha;
        create_info.presentMode = present_mode;

        // 스왑체인을 생성한다.
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
        // 사용가능한 스왑체인 이미지의 개수를 가져온다.
        uint32_t count;
        vkGetSwapchainImagesKHR(device_, swapchain_, &count, nullptr);

        // 사용가능한 스왑체인 이미지를 가져온다.
        swapchain_images_.resize(count);
        vkGetSwapchainImagesKHR(device_, swapchain_, &count, &swapchain_images_[0]);

        // 스왑체인을 생성할 때 정의한 이미지의 개수는 최소 이미지의 개수이다 그러므로 실제로 생성되는것은 더 많을 수 있다.
    }

    void fini_instance_()
    {
        vkDestroyInstance(instance_, nullptr);
    }

    void fini_device_()
    {
        vkDestroyDevice(device_, nullptr);
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
    // 생성할 윈도우를 정의한다.
    Window_desc window_desc;

    window_desc.title = L"Chapter04"s;
    window_desc.extent = {512, 512, 1};

    // 윈도우를 생성한다.
    Window window {window_desc};

    // Chapter2 예제를 생성한다.
    Chapter2 chapter2 {&window};

    // 윈도우를 실행시킨다.
    window.run();

    return 0;
}

//----------------------------------------------------------------------------------------------------------------------
