//
// This file is part of the "ogl_to_vlk" project
// See "LICENSE" for license information.
//

#include <iostream>
#include <vector>
#include <vulkan/vulkan.h>

using namespace std;

//----------------------------------------------------------------------------------------------------------------------

VkInstance instance_ {VK_NULL_HANDLE};
VkPhysicalDevice  physical_device_ {VK_NULL_HANDLE};
uint32_t queue_family_index_ {UINT32_MAX};
VkDevice device_ {VK_NULL_HANDLE};
VkQueue queue_ {VK_NULL_HANDLE};

//----------------------------------------------------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    VkResult result = VK_SUCCESS;

    // 지원되는 Vulkan Layer 조사하기
    {
        uint32_t count {0};
        vkEnumerateInstanceLayerProperties(&count, nullptr);

        vector<VkLayerProperties> properties {count};
        vkEnumerateInstanceLayerProperties(&count, &properties[0]);

        for (auto& p : properties) {
            cout << "Layer Name             : " << p.layerName             << '\n'
                 << "Spec Version           : " << p.specVersion           << '\n'
                 << "Implementation Version : " << p.implementationVersion << '\n'
                 << "Description            : " << p.description           << '\n' << endl;
        }
    }

    // VkInstance 만들기
    if (result == VK_SUCCESS) {
        constexpr auto layer_name {"VK_LAYER_KHRONOS_validation"};

        VkInstanceCreateInfo create_info {};

        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_info.enabledLayerCount = 1;
        create_info.ppEnabledLayerNames = &layer_name;

        result = vkCreateInstance(&create_info, nullptr, &instance_);
        assert(result == VK_SUCCESS);
    }

    if (result == VK_SUCCESS) {
        uint32_t count {0};
        vkEnumeratePhysicalDevices(instance_, &count, nullptr);

        vector<VkPhysicalDevice> physical_devices {count};
        vkEnumeratePhysicalDevices(instance_, &count, &physical_devices[0]);

        for (auto &physical_device : physical_devices) {
            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(physical_device, &properties);

            cout << "Device Name : " << properties.deviceName << endl;
        }

        physical_device_ = physical_devices[0];
    }

    if (result == VK_SUCCESS) {
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
    }

    if (result == VK_SUCCESS) {
        constexpr auto priority = 1.0f;

        VkDeviceQueueCreateInfo queue_create_info {};

        queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.queueFamilyIndex = queue_family_index_;
        queue_create_info.queueCount = 1;
        queue_create_info.pQueuePriorities = &priority;

        VkDeviceCreateInfo create_info {};

        create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        create_info.queueCreateInfoCount = 1;
        create_info.pQueueCreateInfos = &queue_create_info;

        result = vkCreateDevice(physical_device_, &create_info, nullptr, &device_);
        assert(result == VK_SUCCESS);
    }

    if (result == VK_SUCCESS) {
        vkGetDeviceQueue(device_, queue_family_index_, 0, &queue_);
    }

    return 0;
}

//----------------------------------------------------------------------------------------------------------------------
