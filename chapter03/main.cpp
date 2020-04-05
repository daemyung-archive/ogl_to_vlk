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

    {
        // 사용가능한 인스턴스 레이어의 개수를 조사한다.
        uint32_t count {0};
        vkEnumerateInstanceLayerProperties(&count, nullptr);

        // 사용가능한 인스턴스 레이어의 정보를 가져온다.
        vector<VkLayerProperties> properties {count};
        vkEnumerateInstanceLayerProperties(&count, &properties[0]);

        for (auto& p : properties) {
            cout << "Layer Name             : " << p.layerName             << '\n'
                 << "Spec Version           : " << p.specVersion           << '\n'
                 << "Implementation Version : " << p.implementationVersion << '\n'
                 << "Description            : " << p.description           << '\n' << endl;
        }
    }

    if (result == VK_SUCCESS) {
        // 개발에 반드시 필요한 레이어이며 잘못된 API의 사용을 바로잡아준다.
        // CPU를 많이 사용하기 때문에 반드시 개발중일 때만 사용해야한다.
        constexpr auto layer_name {"VK_LAYER_KHRONOS_validation"};

        // 생성할 VkInstance를 정의한다.
        VkInstanceCreateInfo create_info {};

        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_info.enabledLayerCount = 1;
        create_info.ppEnabledLayerNames = &layer_name;

        // VkInstance를 생성한다.
        result = vkCreateInstance(&create_info, nullptr, &instance_);
        assert(result == VK_SUCCESS);
    }

    if (result == VK_SUCCESS) {
        // 사용가능한 VkPhysicalDevice의 개수를 조사한다.
        uint32_t count {0};
        vkEnumeratePhysicalDevices(instance_, &count, nullptr);

        // 사용가능한 VkPhysicalDevice를 가져온다.
        vector<VkPhysicalDevice> physical_devices {count};
        vkEnumeratePhysicalDevices(instance_, &count, &physical_devices[0]);

        // 사용가능한 VkPhysicalDevice 대한 정보를 출력한다.
        // 여러개의에 VkPhysicalDevice 중 어플리케이션에 맞는 가장 최적의 VkPhysicalDevice를 선택해야한다.
        // 예를 들어 어플리케이션이 빠른 성능이 필요할 때는 소모전류가 높더라도 성능이 좋은 외장형 GPU를 사용한다.
        // 반대로 어플리케이션이 발열과 소모전류에 민감하다면 내장용 GPU를 사용한다.
        for (auto &physical_device : physical_devices) {
            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(physical_device, &properties);

            cout << "Device Name : " << properties.deviceName << endl;
        }

        // 여기선 가장 첫번째로 얻어진 VkPhysicalDevice를 사용한다.
        physical_device_ = physical_devices[0];
    }

    if (result == VK_SUCCESS) {
        // VkPhysicalDevice에서 사용가능한 큐 패밀리의 개수를 조사한다.
        uint32_t count {0};
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device_, &count, 0);

        // VkPhysicalDevice에서 사용가능한 큐 패밀리의 정보를 가져온다.
        vector<VkQueueFamilyProperties> properties {count};
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device_, &count, &properties[0]);

        // 사용가능한 여러개의 큐 패밀리 중에 그래픽스가 지원되는 큐 패밀리 인덱스를 찾는다.
        // 여기선 한개의 큐를 사용하지만 목적에 따른 여러개의 큐를 사용하면 성능을 향상시킬 수 있다.
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
        // 여러개의 VkQueue를 사용할 경우 우선순위에 따라 큐를 통해 제출된 커맨드버퍼가 스케쥴링이 된다.
        constexpr auto priority = 1.0f;

        // 생성할 VkQueue에 정의한다.
        VkDeviceQueueCreateInfo queue_create_info {};

        queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.queueFamilyIndex = queue_family_index_;
        queue_create_info.queueCount = 1;
        queue_create_info.pQueuePriorities = &priority;

        // 생성할 VkDevice를 정의한다.
        VkDeviceCreateInfo create_info {};

        create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        create_info.queueCreateInfoCount = 1;
        create_info.pQueueCreateInfos = &queue_create_info;

        // VkDevice를 생성한다.
        result = vkCreateDevice(physical_device_, &create_info, nullptr, &device_);
        assert(result == VK_SUCCESS);
    }

    if (result == VK_SUCCESS) {
        // VkQueue를 VkDevice로부터 가져온다.
        vkGetDeviceQueue(device_, queue_family_index_, 0, &queue_);
    }

    return 0;
}

//----------------------------------------------------------------------------------------------------------------------
