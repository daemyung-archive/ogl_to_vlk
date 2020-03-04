//
// This file is part of the "ogl_to_vlk" project
// See "LICENSE" for license information.
//

#include <iostream>
#include <sstream>
#include <vector>
#include <vulkan/vulkan.h>

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

class Chapter7 {
public:
    Chapter7() :
        instance_ {VK_NULL_HANDLE},
        physical_device_ {VK_NULL_HANDLE}
    {
        init_instance_();
        init_physical_device_();
    }

    ~Chapter7()
    {
        fini_instance_();
    }

    void print_memory_properties()
    {
        VkPhysicalDeviceMemoryProperties properties;
        vkGetPhysicalDeviceMemoryProperties(physical_device_, &properties);

        for (auto i = 0; i != VK_MAX_MEMORY_TYPES; ++i) {
            auto heap_index = properties.memoryTypes[i].heapIndex;

            cout << i << " Memory Type" << '\n'
                 << '\t' << properties.memoryTypes[i] << '\n'
                 << '\t' << properties.memoryHeaps[heap_index] << '\n';
        }
    }

private:
    void init_instance_()
    {
        constexpr auto layer_name {"VK_LAYER_KHRONOS_validation"};

        VkInstanceCreateInfo create_info {};

        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_info.enabledLayerCount = 1;
        create_info.ppEnabledLayerNames = &layer_name;

        auto result = vkCreateInstance(&create_info, nullptr, &instance_);
        assert(result == VK_SUCCESS);
    }

    void init_physical_device_()
    {
        uint32_t count{1};
        vkEnumeratePhysicalDevices(instance_, &count, &physical_device_);
    }

    void fini_instance_()
    {
        vkDestroyInstance(instance_, nullptr);
    }

private:
    VkInstance instance_;
    VkPhysicalDevice  physical_device_;
};

//----------------------------------------------------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    Chapter7 chapter7;

    chapter7.print_memory_properties();

    return 0;
}

//----------------------------------------------------------------------------------------------------------------------
