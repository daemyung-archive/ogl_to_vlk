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
        print_instance_layers_();
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
        window_->startup_signal.connect(this, &Chapter2::on_startup);
        window_->shutdown_signal.connect(this, &Chapter2::on_shutdown);
        window_->render_signal.connect(this, &Chapter2::on_render);
    }

    void print_instance_layers_()
    {
        uint32_t count {0};
        vkEnumerateInstanceLayerProperties(&count, nullptr);

        vector<VkLayerProperties> properties {count};
        vkEnumerateInstanceLayerProperties(&count, &properties[0]);

        for (auto& props : properties) {
            cout << "Layer Name             : " << props.layerName             << '\n'
                 << "Spec Version           : " << props.specVersion           << '\n'
                 << "Implementation Version : " << props.implementationVersion << '\n'
                 << "Description            : " << props.description           << '\n' << endl;
        }
    }

    void print_instance_extensions_()
    {
        uint32_t count {0};
        vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);

        vector<VkExtensionProperties> properties {count};
        vkEnumerateInstanceExtensionProperties(nullptr, &count, &properties[0]);

        for (auto& props : properties) {
            cout << "Extension Name : " << props.extensionName << '\n'
                 << "Spec Version   : " << props.specVersion   << '\n' << endl;
        }
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

            cout << "Device Name : " << properties.deviceName << '\n' << endl;
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

    void print_device_extensions_()
    {
        uint32_t count {0};
        vkEnumerateDeviceExtensionProperties(physical_device_, nullptr, &count, nullptr);

        vector<VkExtensionProperties> properties {count};
        vkEnumerateDeviceExtensionProperties(physical_device_, nullptr, &count, &properties[0]);

        for (auto& props : properties) {
            cout << "Extension Name : " << props.extensionName << '\n'
                 << "Spec Version   : " << props.specVersion   << '\n' << endl;
        }
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

    void print_present_modes_()
    {
        uint32_t count;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device_, surface_, &count, nullptr);

        vector<VkPresentModeKHR> modes {count};
        vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device_, surface_, &count, &modes[0]);

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
        create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
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

    void on_render()
    {
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
    Window_desc window_desc;

    window_desc.title = L"Chapter2"s;
    window_desc.extent = {512, 512, 1};

    Window window {window_desc};

    Chapter2 chapter2 {&window};

    window.run();

    return 0;
}

//----------------------------------------------------------------------------------------------------------------------
