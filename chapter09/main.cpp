//
// This file is part of the "ogl_to_vlk" project
// See "LICENSE" for license information.
//

#include <cassert>
#include <iostream>
#include <sstream>
#include <vector>
#include <vulkan/vulkan.h>

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

using namespace std;

//----------------------------------------------------------------------------------------------------------------------

string to_string(VkMemoryPropertyFlags flags)
{
    if (!flags)
        return "HOST_LOCAL";

    stringstream ss;
    bool delimiter{false};

    if (flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
        if (delimiter)
            ss << " | ";

        delimiter = true;
        ss << "DEVICE_LOCAL";
    }

    if (flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
        if (delimiter)
            ss << " | ";

        delimiter = true;
        ss << "HOST_VISIBLE";
    }

    if (flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) {
        if (delimiter)
            ss << " | ";

        delimiter = true;
        ss << "HOST_COHERENT";
    }

    if (flags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) {
        if (delimiter)
            ss << " | ";

        delimiter = true;
        ss << "HOST_CACHED";
    }

    if (flags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT) {
        if (delimiter)
            ss << " | ";

        delimiter = true;
        ss << "LAZILY_ALLOCATED";
    }

    return ss.str();
}

//----------------------------------------------------------------------------------------------------------------------

auto& operator<<(ostream& os, const VkMemoryType& memory_type)
{
    os << ::to_string(memory_type.propertyFlags);

    return os;
}

//----------------------------------------------------------------------------------------------------------------------

auto& operator<<(ostream& os, const VkMemoryHeap memory_heap)
{
    os << "size : " << memory_heap.size;

    return os;
}

//----------------------------------------------------------------------------------------------------------------------

class Chapter9 {
public:
    Chapter9() :
        instance_ {VK_NULL_HANDLE},
        physical_device_ {VK_NULL_HANDLE}
    {
        init_instance_();
        find_best_physical_device_();
        find_queue_family_index_();
        init_device_();
        init_allocator_();
    }

    ~Chapter9()
    {
        fini_device_();
        fini_instance_();
    }

    void print_memory_properties()
    {
        // 메모리 성질을 얻어오기 위한 변수를 선언합니다.
        VkPhysicalDeviceMemoryProperties properties;

        // 물리 디바이스로부터 메모리의 성질을 얻어옵니다.
        vkGetPhysicalDeviceMemoryProperties(physical_device_, &properties);

        // 사용가능한 디바이스 메모리의 성질을 출력합니다.
        for (auto i = 0; i != VK_MAX_MEMORY_TYPES; ++i) {
            auto heap_index = properties.memoryTypes[i].heapIndex;

            cout << i << " Memory Type" << '\n'
                 << '\t' << properties.memoryTypes[i] << '\n'
                 << '\t' << properties.memoryHeaps[heap_index] << '\n';
        }

        // 메모리 접근은 성능에 엄청난 영향을 미치게 됩니다.
        // 그러므로 반드시 각각의 특징을 이해하고 올바른 메모리 성질을 선택해야합니다.
    }

private:
    void init_instance_()
    {
        // 벌칸 프로그램을 작성하는데 있어서 반드시 필요한 레이어입니다.
        // 하지만 CPU를 굉장히 많이 사용하기 때문에 개발중에만 사용해야 합니다.
        vector<const char*> layer_names {
            "VK_LAYER_KHRONOS_validation"
        };

        // 생성하려는 인스턴스를 정의합니다.
        VkInstanceCreateInfo create_info {};

        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_info.enabledLayerCount = layer_names.size();
        create_info.ppEnabledLayerNames = &layer_names[0];

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

        // 생성하려는 디바이스를 정의합니다.
        VkDeviceCreateInfo create_info {};

        create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        create_info.queueCreateInfoCount = 1;
        create_info.pQueueCreateInfos = &queue_create_info;

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

    void init_allocator_()
    {
        // 생성하려는 얼로케이터를 정의합니다.
        VmaAllocatorCreateInfo create_info = {};
        create_info.device = device_;
        create_info.instance = instance_;
        create_info.physicalDevice = physical_device_;

        // 얼로케이터를 생성합니다.
        auto result = vmaCreateAllocator(&create_info, &allocator_);
        assert(result == VK_SUCCESS);
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

    void fini_allocator_()
    {
        // 얼로케이터를 파괴합니다.
        vmaDestroyAllocator(allocator_);
    }

private:
    VkInstance instance_;
    VkPhysicalDevice  physical_device_;
    uint32_t queue_family_index_;
    VkDevice device_;
    VmaAllocator allocator_;
};

//----------------------------------------------------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    Chapter9 chapter9;

    chapter9.print_memory_properties();

    return 0;
}

//----------------------------------------------------------------------------------------------------------------------
