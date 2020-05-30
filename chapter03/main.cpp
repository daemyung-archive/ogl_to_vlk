//
// This file is part of the "ogl_to_vlk" project
// See "LICENSE" for license information.
//

#include <cassert>
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
        // 사용가능한 레이어의 수를 얻기 위한 변수를 선언합니다.
        uint32_t count {0};

        // 사용가능한 레이어의 수를 얻어옵니다.
        vkEnumerateInstanceLayerProperties(&count, nullptr);

        // 사용가능한 레이어들의 정보를 얻기 위한 변수를 선언합니다.
        vector<VkLayerProperties> properties;

        // 사용가능한 레이어들의 정보를 얻기 위한 메모리를 할당합니다.
        properties.resize(count);

        // 사용가능한 레이어들의 정보를 얻어옵니다.
        vkEnumerateInstanceLayerProperties(&count, &properties[0]);

        // 레이어들의 정보를 출력합니다.
        for (auto& p : properties) {
            cout << "Layer Name             : " << p.layerName             << '\n'
                 << "Spec Version           : " << p.specVersion           << '\n'
                 << "Implementation Version : " << p.implementationVersion << '\n'
                 << "Description            : " << p.description           << '\n' << endl;
        }
    }

    if (result == VK_SUCCESS) {
        // 벌칸 프로그램을 작성하는데 있어서 반드시 필요한 레이어입니다.
        // 하지만 CPU를 굉장히 많이 사용하기 때문에 개발중에만 사용해야 합니다.
        constexpr auto layer_name {"VK_LAYER_KHRONOS_validation"};

        // 생성하려는 인스턴스를 정의합니다.
        VkInstanceCreateInfo create_info {};

        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_info.enabledLayerCount = 1;
        create_info.ppEnabledLayerNames = &layer_name;

        // 인스턴스를 생성합니다.
        result = vkCreateInstance(&create_info, nullptr, &instance_);
        assert(result == VK_SUCCESS);
    }

    if (result == VK_SUCCESS) {
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

        for (auto &physical_device : physical_devices) {
            // 물리 디바이스의 속성을 얻기 위한 변수를 선언합니다.
            VkPhysicalDeviceProperties properties;

            // 물리 디바이스의 속성을 얻어옵니다.
            vkGetPhysicalDeviceProperties(physical_device, &properties);

            // 물리 디바이스의 이름을 출력합니다.
            cout << "Device Name : " << properties.deviceName << endl;
        }

        // 어플리케이션에선 사용가능한 물리 디바이스들 중에서 어플리케이션에
        // 적합한 물리 디바이스를 찾아야 합니다.
        // 예제의 간소화를 위해서 첫 번째 물리 디바이스를 사용합니다.
        physical_device_ = physical_devices[0];
    }

    if (result == VK_SUCCESS) {
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
    }

    if (result == VK_SUCCESS) {
        // 큐의 우선순위를 지정합니다.
        // 여러개의 큐를 사용할 경우 다르게 우선순위를 지정할 수 있습니다.
        constexpr auto priority = 1.0f;

        // 생성하려는 큐를 정의합니다.
        VkDeviceQueueCreateInfo queue_create_info {};

        queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.queueFamilyIndex = queue_family_index_;
        queue_create_info.queueCount = 1;
        queue_create_info.pQueuePriorities = &priority;

        // 생성하려는 디바이스를 정의합니다.
        VkDeviceCreateInfo create_info {};

        create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        create_info.queueCreateInfoCount = 1;
        create_info.pQueueCreateInfos = &queue_create_info;

        // 디바이스를 생성합니다.
        result = vkCreateDevice(physical_device_, &create_info, nullptr, &device_);
        assert(result == VK_SUCCESS);
    }

    if (result == VK_SUCCESS) {
        // 디바이스로 부터 생성된 큐의 핸들을 얻어옵니다.
        vkGetDeviceQueue(device_, queue_family_index_, 0, &queue_);
    }

    return 0;
}

//----------------------------------------------------------------------------------------------------------------------
