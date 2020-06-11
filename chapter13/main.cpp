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
#include <filesystem>
#include <vulkan/vulkan.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <platform/Window.h>
#include <sc/Spirv_compiler.h>

using namespace std;
using namespace Platform;
using namespace Sc;

//----------------------------------------------------------------------------------------------------------------------

constexpr auto image_available_index {0};
constexpr auto rendering_done_index {1};

//----------------------------------------------------------------------------------------------------------------------

// Vertex에 텍스쳐 좌표계를 추가로 정의한다.
struct Vertex {
    float x, y;
    float r, g, b;
    float u, v;
};

//----------------------------------------------------------------------------------------------------------------------

struct Material {
    float r, g, b;
};

//----------------------------------------------------------------------------------------------------------------------

class Chapter13 {
public:
    Chapter13(Window* window) :
        window_ {window},
        instance_ {VK_NULL_HANDLE},
        physical_device_ {VK_NULL_HANDLE},
        physical_device_memory_properties_ {},
        queue_family_index_ {UINT32_MAX},
        device_ {VK_NULL_HANDLE},
        queue_ {VK_NULL_HANDLE},
        command_pool_ {VK_NULL_HANDLE},
        command_buffer_ {VK_NULL_HANDLE},
        fence_ {VK_NULL_HANDLE},
        semaphores_ {},
        vertex_buffer_ {VK_NULL_HANDLE},
        vertex_device_memory_ {VK_NULL_HANDLE},
        index_buffer_ {VK_NULL_HANDLE},
        index_device_memory_ {VK_NULL_HANDLE},
        uniform_buffer_ {VK_NULL_HANDLE},
        uniform_device_memory_ {VK_NULL_HANDLE},
        texture_image_ {VK_NULL_HANDLE},
        texture_device_memory_ {VK_NULL_HANDLE},
        texture_image_view_ {VK_NULL_HANDLE},
        texture_sampler_ {VK_NULL_HANDLE},
        surface_ {VK_NULL_HANDLE},
        swapchain_ {VK_NULL_HANDLE},
        swapchain_image_extent_ {0, 0},
        render_pass_ {VK_NULL_HANDLE},
        framebuffers_{},
        shader_modules_{},
        material_descriptor_set_layout_ {VK_NULL_HANDLE},
        texture_descriptor_set_layout_{VK_NULL_HANDLE},
        pipeline_layout_ {VK_NULL_HANDLE},
        pipeline_ {VK_NULL_HANDLE},
        descriptor_pool_ {VK_NULL_HANDLE},
        material_descriptor_set_ {VK_NULL_HANDLE},
        texture_descriptor_set_ {VK_NULL_HANDLE}
    {
        init_signals_();
        init_instance_();
        find_best_physical_device_();
        init_physical_device_memory_properties_();
        find_queue_family_index_();
        init_device_();
        init_queue_();
        init_command_pool_();
        init_command_buffer_();
        init_semaphores_();
        init_fence_();
        init_vertex_resources_();
        init_index_resources_();
        init_uniform_resources_();
        init_texture_resources_();
    }

    ~Chapter13()
    {
        fini_texture_resources_();
        fini_uniform_resources_();
        fini_index_resources_();
        fini_vertex_resources_();
        fini_fence_();
        fini_semaphores_();
        fini_command_pool_();
        fini_device_();
        fini_instance_();
    }

private:
    void init_signals_()
    {
        window_->startup_signal.connect(this, &Chapter13::on_startup);
        window_->shutdown_signal.connect(this, &Chapter13::on_shutdown);
        window_->render_signal.connect(this, &Chapter13::on_render);
    }

    void init_instance_()
    {
        vector<const char*> layer_names {
            "VK_LAYER_KHRONOS_validation"
        };

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
        }

        physical_device_ = physical_devices[0];
    }

    void init_physical_device_memory_properties_()
    {
        vkGetPhysicalDeviceMemoryProperties(physical_device_, &physical_device_memory_properties_);
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

    void init_command_pool_()
    {
        VkCommandPoolCreateInfo create_info {};

        create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        create_info.queueFamilyIndex = queue_family_index_;

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
        VkCommandBufferAllocateInfo allocate_info {};

        allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocate_info.commandPool = command_pool_;
        allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocate_info.commandBufferCount = 1;

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
        VkSemaphoreCreateInfo create_info {};

        create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

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
        VkFenceCreateInfo create_info {};

        create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

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

    uint32_t find_memory_type_index(const VkMemoryRequirements& requirements, VkMemoryPropertyFlags properties)
    {
        for (auto i = 0; i != physical_device_memory_properties_.memoryTypeCount; ++i) {
            if (!(requirements.memoryTypeBits & (1 << i)))
                continue;

            if ((physical_device_memory_properties_.memoryTypes[i].propertyFlags & properties) != properties)
                continue;

            return i;
        }

        return UINT32_MAX;
    }

    void init_vertex_resources_()
    {
        // 텍스쳐 좌표계 데이터를 정의한다.
        vector<Vertex> vertices = {
            {-0.5,  0.5, 1.0, 0.3, 0.3, 0.0, 1.0},
            { 0.5,  0.5, 0.3, 1.0, 0.3, 1.0, 1.0},
            { 0.0, -0.5, 0.3, 0.3, 1.0, 0.5, 0.0}
        };

        VkBufferCreateInfo create_info {};

        create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        create_info.size = sizeof(Vertex) * vertices.size();
        create_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

        auto result = vkCreateBuffer(device_, &create_info, nullptr, &vertex_buffer_);
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

        VkMemoryRequirements requirements;
        vkGetBufferMemoryRequirements(device_, vertex_buffer_, &requirements);

        VkMemoryAllocateInfo alloc_info {};

        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize = requirements.size;
        alloc_info.memoryTypeIndex = find_memory_type_index(requirements, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        result = vkAllocateMemory(device_, &alloc_info, nullptr, &vertex_device_memory_);
        switch (result) {
            case VK_ERROR_OUT_OF_HOST_MEMORY:
                cout << "VK_ERROR_OUT_OF_HOST_MEMORY" << endl;
                break;
            case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                cout << "VK_ERROR_OUT_OF_DEVICE_MEMORY" << endl;
                break;
            case VK_ERROR_TOO_MANY_OBJECTS:
                cout << "VK_ERROR_TOO_MANY_OBJECTS" << endl;
                break;
            default:
                break;
        }
        assert(result == VK_SUCCESS);

        result = vkBindBufferMemory(device_, vertex_buffer_, vertex_device_memory_, 0);
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

        void* contents;
        result = vkMapMemory(device_, vertex_device_memory_, 0, sizeof(Vertex) * vertices.size(), 0, &contents);
        switch (result) {
            case VK_ERROR_OUT_OF_HOST_MEMORY:
                cout << "VK_ERROR_OUT_OF_HOST_MEMORY" << endl;
                break;
            case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                cout << "VK_ERROR_OUT_OF_DEVICE_MEMORY" << endl;
                break;
            case VK_ERROR_MEMORY_MAP_FAILED:
                cout << "VK_ERROR_MEMORY_MAP_FAILED" << endl;
                break;
            default:
                break;
        }
        assert(result == VK_SUCCESS);

        memcpy(contents, &vertices[0], sizeof(Vertex) * vertices.size());

        vkUnmapMemory(device_, vertex_device_memory_);
    }

    void init_index_resources_()
    {
        vector<uint16_t> indices {0, 1, 2};

        VkBuffer staging_buffer;
        VkDeviceMemory staging_device_memory;

        {
            VkBufferCreateInfo create_info {};

            create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            create_info.size = sizeof(uint16_t) * indices.size();
            create_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

            auto result = vkCreateBuffer(device_, &create_info, nullptr, &staging_buffer);
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

            VkMemoryRequirements requirements;
            vkGetBufferMemoryRequirements(device_, staging_buffer, &requirements);

            VkMemoryAllocateInfo alloc_info {};

            alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            alloc_info.allocationSize = requirements.size;
            alloc_info.memoryTypeIndex = find_memory_type_index(requirements, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                                                                            | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

            result = vkAllocateMemory(device_, &alloc_info, nullptr, &staging_device_memory);
            switch (result) {
                case VK_ERROR_OUT_OF_HOST_MEMORY:
                    cout << "VK_ERROR_OUT_OF_HOST_MEMORY" << endl;
                    break;
                case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                    cout << "VK_ERROR_OUT_OF_DEVICE_MEMORY" << endl;
                    break;
                case VK_ERROR_TOO_MANY_OBJECTS:
                    cout << "VK_ERROR_TOO_MANY_OBJECTS" << endl;
                    break;
                default:
                    break;
            }
            assert(result == VK_SUCCESS);

            result = vkBindBufferMemory(device_, staging_buffer, staging_device_memory, 0);
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

            void* contents;
            result = vkMapMemory(device_, staging_device_memory, 0, sizeof(uint16_t) * indices.size(), 0, &contents);
            switch (result) {
                case VK_ERROR_OUT_OF_HOST_MEMORY:
                    cout << "VK_ERROR_OUT_OF_HOST_MEMORY" << endl;
                    break;
                case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                    cout << "VK_ERROR_OUT_OF_DEVICE_MEMORY" << endl;
                    break;
                case VK_ERROR_MEMORY_MAP_FAILED:
                    cout << "VK_ERROR_MEMORY_MAP_FAILED" << endl;
                    break;
                default:
                    break;
            }
            assert(result == VK_SUCCESS);

            memcpy(contents, &indices[0], sizeof(uint16_t) * indices.size());

            vkUnmapMemory(device_, staging_device_memory);
        }

        {
            VkBufferCreateInfo create_info {};

            create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            create_info.size = sizeof(uint16_t) * indices.size();
            create_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT
                                | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

            auto result = vkCreateBuffer(device_, &create_info, nullptr, &index_buffer_);
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

            VkMemoryRequirements requirements;
            vkGetBufferMemoryRequirements(device_, index_buffer_, &requirements);

            VkMemoryAllocateInfo alloc_info {};

            alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            alloc_info.allocationSize = requirements.size;
            alloc_info.memoryTypeIndex = find_memory_type_index(requirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            result = vkAllocateMemory(device_, &alloc_info, nullptr, &index_device_memory_);
            switch (result) {
                case VK_ERROR_OUT_OF_HOST_MEMORY:
                    cout << "VK_ERROR_OUT_OF_HOST_MEMORY" << endl;
                    break;
                case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                    cout << "VK_ERROR_OUT_OF_DEVICE_MEMORY" << endl;
                    break;
                case VK_ERROR_TOO_MANY_OBJECTS:
                    cout << "VK_ERROR_TOO_MANY_OBJECTS" << endl;
                    break;
                default:
                    break;
            }
            assert(result == VK_SUCCESS);

            result = vkBindBufferMemory(device_, index_buffer_, index_device_memory_, 0);
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

        VkCommandBufferAllocateInfo allocate_info {};

        allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocate_info.commandPool = command_pool_;
        allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocate_info.commandBufferCount = 1;

        VkCommandBuffer command_buffer;
        vkAllocateCommandBuffers(device_, &allocate_info, &command_buffer);

        VkCommandBufferBeginInfo begin_info {};

        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(command_buffer, &begin_info);

        VkBufferCopy region {};

        region.size = sizeof(uint16_t) * indices.size();

        vkCmdCopyBuffer(command_buffer, staging_buffer, index_buffer_, 1, &region);

        vkEndCommandBuffer(command_buffer);

        VkSubmitInfo submit_info {};

        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffer;

        vkQueueSubmit(queue_, 1, &submit_info, VK_NULL_HANDLE);
        vkDeviceWaitIdle(device_);

        vkFreeMemory(device_, staging_device_memory, nullptr);
        vkDestroyBuffer(device_, staging_buffer, nullptr);
    }

    void init_uniform_resources_()
    {
        VkBufferCreateInfo create_info {};

        create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        create_info.size = sizeof(Material);
        create_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

        auto result = vkCreateBuffer(device_, &create_info, nullptr, &uniform_buffer_);
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

        VkMemoryRequirements requirements;
        vkGetBufferMemoryRequirements(device_, vertex_buffer_, &requirements);

        VkMemoryAllocateInfo alloc_info {};

        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize = requirements.size;
        alloc_info.memoryTypeIndex = find_memory_type_index(requirements, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                                                                          | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        result = vkAllocateMemory(device_, &alloc_info, nullptr, &uniform_device_memory_);
        switch (result) {
            case VK_ERROR_OUT_OF_HOST_MEMORY:
                cout << "VK_ERROR_OUT_OF_HOST_MEMORY" << endl;
                break;
            case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                cout << "VK_ERROR_OUT_OF_DEVICE_MEMORY" << endl;
                break;
            case VK_ERROR_TOO_MANY_OBJECTS:
                cout << "VK_ERROR_TOO_MANY_OBJECTS" << endl;
                break;
            default:
                break;
        }
        assert(result == VK_SUCCESS);

        result = vkBindBufferMemory(device_, uniform_buffer_, uniform_device_memory_, 0);
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


    auto find_asset_path()
    {
        auto path = fs::current_path();

        while (path.filename() != "ogl_to_vlk")
            path = path.parent_path();

        path /= "asset";

        return path;
    }

    void init_texture_resources_()
    {
        auto path = find_asset_path() / "logo.png";

        int w, h, c;
        // 이미지 파일을 읽는다.
        auto data = stbi_load(path.c_str(), &w, &h, &c, STBI_rgb_alpha);

        // 이미지는 파일로부터 한번 읽고 특수한 경우를 제외하곤 변경되지 않는다.
        // 그러므로 GPU의 접근이 용이한 메모리를 할당한다.

        VkBuffer staging_buffer;
        VkDeviceMemory staging_device_memory;

        {
            // 스테이징 버퍼를 정의한다.
            VkBufferCreateInfo create_info {};

            create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            create_info.size = w * h * STBI_rgb_alpha;
            create_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

            // 정의한 스테이징 버퍼를 생성한다.
            auto result = vkCreateBuffer(device_, &create_info, nullptr, &staging_buffer);
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

            // 스테이징 버퍼를 위해 필요한 메모리 공간을 쿼리한다.
            VkMemoryRequirements requirements;
            vkGetBufferMemoryRequirements(device_, staging_buffer, &requirements);

            // 스테이징 메모리를 정의한다.
            VkMemoryAllocateInfo alloc_info {};

            alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            alloc_info.allocationSize = requirements.size;
            alloc_info.memoryTypeIndex = find_memory_type_index(requirements, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                                                                              | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

            // 스테이징 메모리를 할당한다.
            result = vkAllocateMemory(device_, &alloc_info, nullptr, &staging_device_memory);
            switch (result) {
                case VK_ERROR_OUT_OF_HOST_MEMORY:
                    cout << "VK_ERROR_OUT_OF_HOST_MEMORY" << endl;
                    break;
                case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                    cout << "VK_ERROR_OUT_OF_DEVICE_MEMORY" << endl;
                    break;
                case VK_ERROR_TOO_MANY_OBJECTS:
                    cout << "VK_ERROR_TOO_MANY_OBJECTS" << endl;
                    break;
                default:
                    break;
            }
            assert(result == VK_SUCCESS);

            // 스테이징 버퍼와 스테이징 메모리를 바인딩한다.
            result = vkBindBufferMemory(device_, staging_buffer, staging_device_memory, 0);
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

            void* contents;
            // 스테이징 메모리에 접근가능한 포인터를 쿼리한다.
            result = vkMapMemory(device_, staging_device_memory, 0, w * h * STBI_rgb_alpha, 0, &contents);
            switch (result) {
                case VK_ERROR_OUT_OF_HOST_MEMORY:
                    cout << "VK_ERROR_OUT_OF_HOST_MEMORY" << endl;
                    break;
                case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                    cout << "VK_ERROR_OUT_OF_DEVICE_MEMORY" << endl;
                    break;
                case VK_ERROR_MEMORY_MAP_FAILED:
                    cout << "VK_ERROR_MEMORY_MAP_FAILED" << endl;
                    break;
                default:
                    break;
            }
            assert(result == VK_SUCCESS);

            // 이미지 데이터를 스테이징 버퍼로 복사한다.
            memcpy(contents, data, w * h * STBI_rgb_alpha);

            // 스테이징 버퍼에 접근을 마친다.
            vkUnmapMemory(device_, staging_device_memory);
        }

        // 이미지를 읽기 위한 메모리를 해제한다.
        stbi_image_free(data);

        {
            // 생성하려는 이미지를 정의한다.
            VkImageCreateInfo create_info {};

            create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            create_info.imageType = VK_IMAGE_TYPE_2D;
            create_info.format = VK_FORMAT_R8G8B8A8_UNORM;
            create_info.extent = {static_cast<uint32_t>(w), static_cast<uint32_t>(h), 1};
            create_info.mipLevels = 1;
            create_info.arrayLayers = 1;
            create_info.samples = VK_SAMPLE_COUNT_1_BIT;

            // 이미지를 보다 효율적으로 읽기 위해 TILING_OPTIMAL을 사용한다.
            // 미디어 이미지를 다루는 경우는 출력을 위해 TILING_OPTIMAL에서 LINEAR로 변경되어야 하기 때문에
            // TILING_OPTIMAL이 적합하지 않을 수 있다. 그러므로 여러 가정을 고려하여 선택해야한다.
            create_info.tiling = VK_IMAGE_TILING_OPTIMAL;

            create_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT; // 셰이더에서 읽기 위하 SAMPLED를 추가한다.
            create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            // 정의된 이미지를 생성한다.
            auto result = vkCreateImage(device_, &create_info, nullptr, &texture_image_);

            // 이미지에 필요한 메모리 요구사항을 쿼리한다.
            VkMemoryRequirements requirements;
            vkGetImageMemoryRequirements(device_, texture_image_, &requirements);

            // 할당하려는 메모리를 정의한다.
            VkMemoryAllocateInfo alloc_info {};

            alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            alloc_info.allocationSize = requirements.size;
            alloc_info.memoryTypeIndex = find_memory_type_index(requirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            // 정의한 메모리를 할당한다.
            result = vkAllocateMemory(device_, &alloc_info, nullptr, &texture_device_memory_);
            switch (result) {
                case VK_ERROR_OUT_OF_HOST_MEMORY:
                    cout << "VK_ERROR_OUT_OF_HOST_MEMORY" << endl;
                    break;
                case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                    cout << "VK_ERROR_OUT_OF_DEVICE_MEMORY" << endl;
                    break;
                case VK_ERROR_TOO_MANY_OBJECTS:
                    cout << "VK_ERROR_TOO_MANY_OBJECTS" << endl;
                    break;
                default:
                    break;
            }
            assert(result == VK_SUCCESS);

            // 이미지와 메모리를 바인딩 한다.
            result = vkBindImageMemory(device_, texture_image_, texture_device_memory_, 0);
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

        // 비록 이미지를 생성했지만 이미지 데이터는 스테이징 버퍼에 담겨있다.
        // 그러므로 스테이징 버퍼의 내용을 이미지로 복사해주는 작업을 해야한다.

        {
            VkCommandBufferAllocateInfo allocate_info {};

            allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocate_info.commandPool = command_pool_;
            allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocate_info.commandBufferCount = 1;

            VkCommandBuffer command_buffer;
            vkAllocateCommandBuffers(device_, &allocate_info, &command_buffer);

            VkCommandBufferBeginInfo begin_info {};

            begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

            vkBeginCommandBuffer(command_buffer, &begin_info);

            {
                VkImageMemoryBarrier barrier {};

                barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                barrier.srcQueueFamilyIndex = queue_family_index_;
                barrier.dstQueueFamilyIndex = queue_family_index_;
                barrier.image = texture_image_;
                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                barrier.subresourceRange.levelCount = 1;
                barrier.subresourceRange.layerCount = 1;

                vkCmdPipelineBarrier(command_buffer,
                                     VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                     0,
                                     0, nullptr,
                                     0, nullptr,
                                     1, &barrier);
            }

            // 복사할 버퍼의 영역과 복사될 이미지의 영역을 정의한다.
            VkBufferImageCopy region {};

            // 버퍼 영역을 정의하지 않을 경우 이미지와 정확히 동일한 크기로 간주한다.
            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = 0;
            region.imageSubresource.layerCount = 1;
            region.imageExtent = {static_cast<uint32_t>(w), static_cast<uint32_t>(h), 1};

            // 버퍼를 이미지로 복사한다.
            vkCmdCopyBufferToImage(command_buffer,
                                   staging_buffer,
                                   texture_image_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                   1, &region);

            {
                VkImageMemoryBarrier barrier {};

                barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                barrier.srcQueueFamilyIndex = queue_family_index_;
                barrier.dstQueueFamilyIndex = queue_family_index_;
                barrier.image = texture_image_;
                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                barrier.subresourceRange.levelCount = 1;
                barrier.subresourceRange.layerCount = 1;

                vkCmdPipelineBarrier(command_buffer,
                                     VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                     0,
                                     0, nullptr,
                                     0, nullptr,
                                     1, &barrier);
            }

            vkEndCommandBuffer(command_buffer);

            VkSubmitInfo submit_info {};

            submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submit_info.commandBufferCount = 1;
            submit_info.pCommandBuffers = &command_buffer;

            vkQueueSubmit(queue_, 1, &submit_info, VK_NULL_HANDLE);
            vkDeviceWaitIdle(device_);
            vkFreeCommandBuffers(device_, command_pool_, 1, &command_buffer);

            // 제출한 커맨드버퍼가 처리된 이후에 관련된 리소스들을 해제할 수 있다.
            vkFreeMemory(device_, staging_device_memory, nullptr);
            vkDestroyBuffer(device_, staging_buffer, nullptr);
        }

        {
            // 파이프라인에서 이미지를 접근하기 위해선 반드시 이미지 뷰가 필요하다.
            // 이미지 뷰는 이미지를 접근하기 위한 또 다른 메타데이터라고 생각하면 이해하기 쉽다.

            // 생성하려는 이미지 뷰를 정의한다.
            VkImageViewCreateInfo create_info {};

            create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            create_info.image = texture_image_;
            create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            create_info.format = VK_FORMAT_R8G8B8A8_UNORM;
            create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            create_info.subresourceRange.levelCount = 1;
            create_info.subresourceRange.layerCount = 1;

            // 정의한 이미지 뷰를 생성한다.
            auto result = vkCreateImageView(device_, &create_info, nullptr, &texture_image_view_);
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

        // 셰이더에서 texture(...) 함수를 사용하여 특정 위치의 이미지 데이터를 가져온다.
        // 이를 위해선 반드시 샘플러란 하드웨어가 필요하다. 즉 이미지의 데이터는 샘플러를 통해 이미지의 데이터를 가져오는것이다.

        {
            // 생성하려는 샘플러를 정의한다.
            VkSamplerCreateInfo create_info {};

            create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            create_info.magFilter = VK_FILTER_LINEAR;
            create_info.minFilter = VK_FILTER_LINEAR;
            create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

            // 정의한 샘플러를 생성한다.
            auto result = vkCreateSampler(device_, &create_info, nullptr, &texture_sampler_);
            switch (result) {
                case VK_ERROR_OUT_OF_HOST_MEMORY:
                    cout << "VK_ERROR_OUT_OF_HOST_MEMORY" << endl;
                    break;
                case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                    cout << "VK_ERROR_OUT_OF_DEVICE_MEMORY" << endl;
                    break;
                case VK_ERROR_TOO_MANY_OBJECTS:
                    cout << "VK_ERROR_TOO_MANY_OBJECTS" << endl;
                    break;
                default:
                    break;
            }
            assert(result == VK_SUCCESS);
        }
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

        VkCompositeAlphaFlagBitsKHR composite_alpha {static_cast<VkCompositeAlphaFlagBitsKHR>(0)};
        for (auto i = 0; i != 32; ++i) {
            VkCompositeAlphaFlagBitsKHR flag = static_cast<VkCompositeAlphaFlagBitsKHR>(0x1 << i);
            if (surface_capabilities.supportedUsageFlags & flag) {
                composite_alpha = flag;
                break;
            }
        }

        swapchain_image_extent_ = surface_capabilities.currentExtent;

        VkSwapchainCreateInfoKHR create_info {};

        create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        create_info.surface = surface_;
        create_info.minImageCount = 2;
        create_info.imageFormat = surface_format.format;
        create_info.imageColorSpace = surface_format.colorSpace;
        create_info.imageExtent = swapchain_image_extent_;
        create_info.imageArrayLayers = 1;
        create_info.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
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

        VkCommandBufferBeginInfo begin_info {};

        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        vkBeginCommandBuffer(command_buffer_, &begin_info);

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

        vkCmdPipelineBarrier(command_buffer_,
                             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                             0,
                             0, nullptr,
                             0, nullptr,
                             barriers.size(), &barriers[0]);

        vkEndCommandBuffer(command_buffer_);

        VkSubmitInfo submit_info {};

        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffer_;

        vkQueueSubmit(queue_, 1, &submit_info, VK_NULL_HANDLE);

        vkDeviceWaitIdle(device_);
    }

    void init_swapchain_image_views_()
    {
        auto surface_format = default_surface_format_();

        VkImageViewCreateInfo create_info {};

        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format = surface_format.format;
        create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.layerCount = 1;

        swapchain_image_views_.resize(swapchain_images_.size());
        for (auto i = 0; i != swapchain_images_.size(); ++i) {
            create_info.image = swapchain_images_[i];

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

        VkAttachmentDescription attachment_desc {};

        attachment_desc.format = surface_format.format;
        attachment_desc.samples = VK_SAMPLE_COUNT_1_BIT;
        attachment_desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment_desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment_desc.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachment_desc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference color_attachment_ref {};

        color_attachment_ref.attachment = 0;
        color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass_desc {};

        subpass_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass_desc.colorAttachmentCount = 1;
        subpass_desc.pColorAttachments = &color_attachment_ref;

        VkRenderPassCreateInfo create_info {};

        create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        create_info.attachmentCount = 1;
        create_info.pAttachments = &attachment_desc;
        create_info.subpassCount = 1;
        create_info.pSubpasses = &subpass_desc;

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
        VkFramebufferCreateInfo create_info {};

        create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        create_info.renderPass = render_pass_;
        create_info.attachmentCount = 1;
        create_info.width = swapchain_image_extent_.width;
        create_info.height = swapchain_image_extent_.height;
        create_info.layers = 1;

        framebuffers_.resize(swapchain_image_views_.size());
        for (auto i = 0; i != swapchain_image_views_.size(); ++i) {
            create_info.pAttachments = &swapchain_image_views_[i];

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

    void init_shader_modules_()
    {
        {
            // 텍스쳐 좌표계를 넘겨주기 위한 in, out이 추가되었다.
            const string vksl = {
                "precision mediump float;                 \n"
                "                                         \n"
                "layout(location = 0) in vec2 i_pos;      \n"
                "layout(location = 1) in vec3 i_col;      \n"
                "layout(location = 2) in vec2 i_uv;       \n"
                "                                         \n"
                "layout(location = 0) out vec3 o_col;     \n"
                "layout(location = 1) out vec2 o_uv;      \n"
                "                                         \n"
                "void main() {                            \n"
                "                                         \n"
                "    gl_Position = vec4(i_pos, 0.0, 1.0); \n"
                "    o_col = i_col;                       \n"
                "    o_uv = i_uv;                         \n"
                "}                                        \n"
            };

            auto spirv = Spirv_compiler().compile(Shader_type::vertex, vksl);

            VkShaderModuleCreateInfo create_info {};

            create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            create_info.codeSize = spirv.size() * sizeof(uint32_t);
            create_info.pCode = &spirv[0];

            auto result = vkCreateShaderModule(device_, &create_info, nullptr, &shader_modules_[0]);
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

        {
            // 텍스쳐를 접근하기 위한 유니폼이 추가되었고 샘플러를 이용하여 텍스쳐를 읽는다.
            const string vksl = {
                "precision mediump float;                            \n"
                "                                                    \n"
                "layout(location = 0) in vec3 i_col;                 \n"
                "layout(location = 1) in vec2 i_uv;                  \n"
                "                                                    \n"
                "layout(location = 0) out vec4 fragment_color0;      \n"
                "                                                    \n"
                "layout(set = 0, binding = 0) uniform Material {     \n"
                "    vec3 col;                                       \n"
                "} material;                                         \n"
                "                                                    \n"
                "layout(set = 1, binding = 0) uniform sampler2D tex; \n"
                "                                                    \n"
                "void main() {                                       \n"
                "    vec3 col = i_col;                               \n"
                "    col *= material.col;                            \n"
                "    col *= texture(tex, i_uv).rgb;                  \n"
                "                                                    \n"
                "    fragment_color0 = vec4(col, 1.0);               \n"
                "}                                                   \n"
            };

            auto spirv = Spirv_compiler().compile(Shader_type::fragment, vksl);

            VkShaderModuleCreateInfo create_info {};

            create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            create_info.codeSize = spirv.size() * sizeof(uint32_t);
            create_info.pCode = &spirv[0];

            auto result = vkCreateShaderModule(device_, &create_info, nullptr, &shader_modules_[1]);
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

    void init_descriptor_set_layouts_()
    {
        {
            VkDescriptorSetLayoutBinding binding {};

            binding.binding = 0;
            binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            binding.descriptorCount = 1;
            binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

            VkDescriptorSetLayoutCreateInfo create_info {};

            create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            create_info.bindingCount = 1;
            create_info.pBindings = &binding;

            auto result = vkCreateDescriptorSetLayout(device_, &create_info, nullptr, &material_descriptor_set_layout_);
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

        {
            // 텍스쳐를 위한 디스크립터 셋 바인딩을 정의한다.
            VkDescriptorSetLayoutBinding binding {};

            binding.binding = 0;
            binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            binding.descriptorCount = 1;
            binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

            // 덱스쳐 디스크립터 셋 바인딩이 포함된 디스크립터 셋 레이아웃을 정의한다.
            VkDescriptorSetLayoutCreateInfo create_info {};

            create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            create_info.bindingCount = 1;
            create_info.pBindings = &binding;

            // 정의한 디스크립터 셋 레이아웃을 생성한다.
            auto result = vkCreateDescriptorSetLayout(device_, &create_info, nullptr, &texture_descriptor_set_layout_);
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

    void init_pipeline_layout_()
    {
        // 파이프라인의 2개의 디스크립터 셋이 정의되어있기 때문에 반드시 2개의 디스크립터 셋이 필요하다.
        vector<VkDescriptorSetLayout> set_layouts {
            material_descriptor_set_layout_,
            texture_descriptor_set_layout_
        };

        VkPipelineLayoutCreateInfo create_info {};

        create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        create_info.setLayoutCount = set_layouts.size();
        create_info.pSetLayouts = &set_layouts[0];

        auto result = vkCreatePipelineLayout(device_, &create_info, nullptr, &pipeline_layout_);
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

    void init_pipeline_()
    {
        array<VkPipelineShaderStageCreateInfo, 2> stages;

        {
            VkPipelineShaderStageCreateInfo stage {};

            stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            stage.stage = VK_SHADER_STAGE_VERTEX_BIT;
            stage.module = shader_modules_[0];
            stage.pName = "main";

            stages[0] = stage;
        }

        {
            VkPipelineShaderStageCreateInfo stage {};

            stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            stage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
            stage.module = shader_modules_[1];
            stage.pName = "main";

            stages[1] = stage;
        }

        VkVertexInputBindingDescription vertex_input_binding {};

        vertex_input_binding.binding = 0;
        vertex_input_binding.stride = sizeof(Vertex);
        vertex_input_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        vector<VkVertexInputAttributeDescription> vertex_input_attributes;

        {
            VkVertexInputAttributeDescription vertex_input_attribute {};

            vertex_input_attribute.location = 0;
            vertex_input_attribute.binding = 0;
            vertex_input_attribute.format = VK_FORMAT_R32G32_SFLOAT;
            vertex_input_attribute.offset = offsetof(Vertex, x);

            vertex_input_attributes.push_back(vertex_input_attribute);
        }

        {
            VkVertexInputAttributeDescription vertex_input_attribute {};

            vertex_input_attribute.location = 1;
            vertex_input_attribute.binding = 0;
            vertex_input_attribute.format = VK_FORMAT_R32G32B32_SFLOAT;
            vertex_input_attribute.offset = offsetof(Vertex, r);

            vertex_input_attributes.push_back(vertex_input_attribute);
        }

        {
            // 텍스쳐 좌표계를 위한 버텍스 인풋 어트리뷰트를 정의한다.
            VkVertexInputAttributeDescription vertex_input_attribute {};

            vertex_input_attribute.location = 2;
            vertex_input_attribute.binding = 0;
            vertex_input_attribute.format = VK_FORMAT_R32G32_SFLOAT;
            vertex_input_attribute.offset = offsetof(Vertex, u);

            vertex_input_attributes.push_back(vertex_input_attribute);
        }

        VkPipelineVertexInputStateCreateInfo vertex_input_state {};

        vertex_input_state.vertexBindingDescriptionCount = 1;
        vertex_input_state.pVertexBindingDescriptions = &vertex_input_binding;
        vertex_input_state.vertexAttributeDescriptionCount = vertex_input_attributes.size();
        vertex_input_state.pVertexAttributeDescriptions = &vertex_input_attributes[0];

        vertex_input_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        VkPipelineInputAssemblyStateCreateInfo input_assembly_state {};

        input_assembly_state.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        input_assembly_state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        VkViewport viewport {};

        viewport.width = swapchain_image_extent_.width;
        viewport.height = swapchain_image_extent_.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor {};

        scissor.extent = swapchain_image_extent_;

        VkPipelineViewportStateCreateInfo viewport_state {};

        viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport_state.viewportCount = 1;
        viewport_state.pViewports = &viewport;
        viewport_state.scissorCount = 1;
        viewport_state.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterization_state {};

        rasterization_state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterization_state.polygonMode = VK_POLYGON_MODE_FILL;
        rasterization_state.cullMode = VK_CULL_MODE_NONE;
        rasterization_state.lineWidth = 1.0f;

        VkPipelineMultisampleStateCreateInfo multisample_state {};

        multisample_state.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisample_state.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineDepthStencilStateCreateInfo depth_stencil_state {};

        depth_stencil_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

        VkPipelineColorBlendAttachmentState attachment {};

        attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT
                                  | VK_COLOR_COMPONENT_G_BIT
                                  | VK_COLOR_COMPONENT_B_BIT
                                  | VK_COLOR_COMPONENT_A_BIT;

        VkPipelineColorBlendStateCreateInfo color_blend_state {};

        color_blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        color_blend_state.attachmentCount = 1;
        color_blend_state.pAttachments = &attachment;

        VkGraphicsPipelineCreateInfo create_info {};

        create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        create_info.stageCount = stages.size();
        create_info.pStages = &stages[0];
        create_info.pVertexInputState = & vertex_input_state;
        create_info.pInputAssemblyState = &input_assembly_state;
        create_info.pViewportState = &viewport_state;
        create_info.pRasterizationState = &rasterization_state;
        create_info.pMultisampleState = &multisample_state;
        create_info.pDepthStencilState = &depth_stencil_state;
        create_info.pColorBlendState = &color_blend_state;
        create_info.layout = pipeline_layout_;
        create_info.renderPass = render_pass_;

        auto result = vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &create_info, nullptr, &pipeline_);
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

    void init_descriptor_pool_()
    {
        // 텍스쳐를 위한 디스크립터 셋을 할당하기 위해 디스크립터 풀 크기를 변경한다.
        vector<VkDescriptorPoolSize> pool_size {
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},

            // COMBINED_IMAGE_SAMPLER는 텍스쳐와 동일한 개념이다.
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}
        };

        VkDescriptorPoolCreateInfo create_info {};

        create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        create_info.maxSets = 2;
        create_info.poolSizeCount = pool_size.size();
        create_info.pPoolSizes = &pool_size[0];

        auto result = vkCreateDescriptorPool(device_, &create_info, nullptr, &descriptor_pool_);
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

    void init_descriptor_sets_()
    {
        {
            VkDescriptorSetAllocateInfo allocate_info {};

            allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocate_info.descriptorPool = descriptor_pool_;
            allocate_info.descriptorSetCount = 1;
            allocate_info.pSetLayouts = &material_descriptor_set_layout_;

            auto result = vkAllocateDescriptorSets(device_, &allocate_info, &material_descriptor_set_);
            switch (result) {
                case VK_ERROR_OUT_OF_HOST_MEMORY:
                    cout << "VK_ERROR_OUT_OF_HOST_MEMORY" << endl;
                    break;
                case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                    cout << "VK_ERROR_OUT_OF_DEVICE_MEMORY" << endl;
                    break;
                case VK_ERROR_OUT_OF_POOL_MEMORY:
                    cout << "VK_ERROR_OUT_OF_POOL_MEMORY" << endl;
                    break;
                default:
                    break;
            }
            assert(result == VK_SUCCESS);
        }

        {
            // 텍스쳐를 위한 디스크립터 셋을 정의한다.
            VkDescriptorSetAllocateInfo allocate_info {};

            allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocate_info.descriptorPool = descriptor_pool_;
            allocate_info.descriptorSetCount = 1;
            allocate_info.pSetLayouts = &texture_descriptor_set_layout_;

            // 정의한 디스크립터 셋을 할당한다.
            auto result = vkAllocateDescriptorSets(device_, &allocate_info, &texture_descriptor_set_);
            switch (result) {
                case VK_ERROR_OUT_OF_HOST_MEMORY:
                    cout << "VK_ERROR_OUT_OF_HOST_MEMORY" << endl;
                    break;
                case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                    cout << "VK_ERROR_OUT_OF_DEVICE_MEMORY" << endl;
                    break;
                case VK_ERROR_OUT_OF_POOL_MEMORY:
                    cout << "VK_ERROR_OUT_OF_POOL_MEMORY" << endl;
                    break;
                default:
                    break;
            }
            assert(result == VK_SUCCESS);
        }
    }

    void fini_instance_()
    {
        vkDestroyInstance(instance_, nullptr);
    }

    void fini_device_()
    {
        vkDestroyDevice(device_, nullptr);
    }

    void fini_command_pool_()
    {
        vkDestroyCommandPool(device_, command_pool_, nullptr);
    }

    void fini_semaphores_()
    {
        for (auto& semaphore : semaphores_)
            vkDestroySemaphore(device_, semaphore, nullptr);
    }

    void fini_fence_()
    {
        vkDestroyFence(device_, fence_, nullptr);
    }

    void fini_vertex_resources_()
    {
        vkFreeMemory(device_, vertex_device_memory_, nullptr);
        vkDestroyBuffer(device_, vertex_buffer_, nullptr);
    }

    void fini_index_resources_()
    {
        vkFreeMemory(device_, index_device_memory_, nullptr);
        vkDestroyBuffer(device_, index_buffer_, nullptr);
    }

    void fini_uniform_resources_()
    {
        vkFreeMemory(device_, uniform_device_memory_, nullptr);
        vkDestroyBuffer(device_, uniform_buffer_, nullptr);
    }

    void fini_texture_resources_()
    {
        // 생성된 텍스쳐 샘플러를 파괴한다.
        vkDestroySampler(device_, texture_sampler_, nullptr);
        // 생성된 텍스쳐 이미지 뷰를 파괴한다.
        vkDestroyImageView(device_, texture_image_view_, nullptr);
        // 생성된 텍스쳐 메모리를 파괴한다.
        vkFreeMemory(device_, texture_device_memory_, nullptr);
        // 생성된 텍스쳐 이미지를 파괴한다.
        vkDestroyImage(device_, texture_image_, nullptr);
    }

    void fini_surface_()
    {
        vkDestroySurfaceKHR(instance_, surface_, nullptr);
    }

    void fini_swapchain_()
    {
        vkDestroySwapchainKHR(device_, swapchain_, nullptr);
    }

    void fini_swapchain_image_views_()
    {
        for (auto& image_view : swapchain_image_views_)
            vkDestroyImageView(device_, image_view, nullptr);
    }

    void fini_render_pass_()
    {
        vkDestroyRenderPass(device_, render_pass_, nullptr);
    }

    void fini_framebuffers_()
    {
        for (auto& framebuffer : framebuffers_)
            vkDestroyFramebuffer(device_, framebuffer, nullptr);
    }

    void fini_shader_modules_()
    {
        for (auto& shader_module : shader_modules_)
            vkDestroyShaderModule(device_, shader_module, nullptr);
    }

    void fini_descriptor_set_layouts_()
    {
        vkDestroyDescriptorSetLayout(device_, material_descriptor_set_layout_, nullptr);
    }

    void fini_pipeline_layout_()
    {
        vkDestroyPipelineLayout(device_, pipeline_layout_, nullptr);
    }

    void fini_pipeline_()
    {
        vkDestroyPipeline(device_, pipeline_, nullptr);
    }

    void fini_descriptor_pool_()
    {
        vkDestroyDescriptorPool(device_, descriptor_pool_, nullptr);
    }

    void on_startup()
    {
        init_surface_();
        init_swapchain_();
        init_swapchain_images_();
        init_swapchain_image_views_();
        init_render_pass_();
        init_framebuffers_();
        init_shader_modules_();
        init_descriptor_set_layouts_();
        init_pipeline_layout_();
        init_pipeline_();
        init_descriptor_pool_();
        init_descriptor_sets_();
    }

    void on_shutdown()
    {
        vkDeviceWaitIdle(device_);

        fini_descriptor_pool_();
        fini_pipeline_();
        fini_pipeline_layout_();
        fini_descriptor_set_layouts_();
        fini_shader_modules_();
        fini_framebuffers_();
        fini_render_pass_();
        fini_swapchain_image_views_();
        fini_swapchain_();
        fini_surface_();
    }

    void on_render()
    {
        uint32_t swapchain_index;
        vkAcquireNextImageKHR(device_, swapchain_, UINT64_MAX,
                              semaphores_[image_available_index], VK_NULL_HANDLE,
                              &swapchain_index);

        if (VK_NOT_READY == vkGetFenceStatus(device_, fence_))
            vkWaitForFences(device_, 1, &fence_, VK_TRUE, UINT64_MAX);

        vkResetFences(device_, 1, &fence_);

        {
            VkDescriptorBufferInfo buffer_info {};

            buffer_info.buffer = uniform_buffer_;
            buffer_info.offset = 0;
            buffer_info.range = sizeof(Material);

            VkWriteDescriptorSet descriptor_write {};

            descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptor_write.dstSet = material_descriptor_set_;
            descriptor_write.dstBinding = 0;
            descriptor_write.descriptorCount = 1;
            descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptor_write.pBufferInfo = &buffer_info;

            vkUpdateDescriptorSets(device_, 1, &descriptor_write, 0, nullptr);
        }

        {
            VkDescriptorImageInfo image_info {};

            image_info.sampler = texture_sampler_;
            image_info.imageView = texture_image_view_;
            image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            VkWriteDescriptorSet descriptor_write {};

            descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptor_write.dstSet = texture_descriptor_set_;
            descriptor_write.dstBinding = 0;
            descriptor_write.descriptorCount = 1;
            descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptor_write.pImageInfo = &image_info;

            vkUpdateDescriptorSets(device_, 1, &descriptor_write, 0, nullptr);
        }

        void* contents;
        vkMapMemory(device_, uniform_device_memory_, 0, sizeof(Material), 0, &contents);

        auto material = static_cast<Material*>(contents);

        static auto t = 0.0f;
        float value = (cos(t) + 1.0f) / 2.0f;

        t += 0.05f;

        material->r = 1.0;
        material->g = value;
        material->b = 1.0 - value;

        vkUnmapMemory(device_, uniform_device_memory_);

        vkResetCommandBuffer(command_buffer_, 0);

        auto& swapchain_image = swapchain_images_[swapchain_index];

        VkCommandBufferBeginInfo begin_info {};

        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        vkBeginCommandBuffer(command_buffer_, &begin_info);

        {
            VkImageMemoryBarrier barrier {};

            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            barrier.srcQueueFamilyIndex = queue_family_index_;
            barrier.dstQueueFamilyIndex = queue_family_index_;
            barrier.image = swapchain_image;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.layerCount = 1;

            vkCmdPipelineBarrier(command_buffer_,
                                 VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);
        }

        VkClearValue clear_value;

        clear_value.color.float32[0] = 0.15f; // R
        clear_value.color.float32[1] = 0.15f; // G
        clear_value.color.float32[2] = 0.15f; // B
        clear_value.color.float32[3] = 1.0f; // A

        VkRenderPassBeginInfo render_pass_begin_info {};

        render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_begin_info.renderPass = render_pass_;
        render_pass_begin_info.framebuffer = framebuffers_[swapchain_index];
        render_pass_begin_info.renderArea.extent = swapchain_image_extent_;
        render_pass_begin_info.clearValueCount = 1;
        render_pass_begin_info.pClearValues = &clear_value;

        vkCmdBeginRenderPass(command_buffer_, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

        VkDeviceSize vertex_buffer_offset {0};
        vkCmdBindVertexBuffers(command_buffer_, 0, 1, &vertex_buffer_, &vertex_buffer_offset);
        vkCmdBindIndexBuffer(command_buffer_, index_buffer_, 0, VK_INDEX_TYPE_UINT16);
        vkCmdBindDescriptorSets(command_buffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout_, 0, 1, &material_descriptor_set_, 0, nullptr);

        // 텍스쳐 디스크립터 셋을 바인딩 한다.
        vkCmdBindDescriptorSets(command_buffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout_, 1, 1, &texture_descriptor_set_, 0, nullptr);

        // 메터리얼과 텍스쳐를 위한 디스크립터 셋 바인딩을 한번의 호출로 수행할 수 있다.
        // 하지만 이해도오 가독성을 높히기 위해 따로따로 호출하였다.

        vkCmdBindPipeline(command_buffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);
        vkCmdDrawIndexed(command_buffer_, 3, 1, 0, 0, 0);

        vkCmdEndRenderPass(command_buffer_);

        {
            VkImageMemoryBarrier barrier {};

            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            barrier.srcQueueFamilyIndex = queue_family_index_;
            barrier.dstQueueFamilyIndex = queue_family_index_;
            barrier.image = swapchain_image;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.layerCount = 1;

            vkCmdPipelineBarrier(command_buffer_,
                                 VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);
        }

        vkEndCommandBuffer(command_buffer_);

        constexpr VkPipelineStageFlags wait_dst_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkSubmitInfo submit_info {};

        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.waitSemaphoreCount = 1;
        submit_info.pWaitSemaphores = &semaphores_[image_available_index];
        submit_info.pWaitDstStageMask = &wait_dst_stage_mask;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffer_;
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores = &semaphores_[rendering_done_index];

        vkQueueSubmit(queue_, 1, &submit_info, fence_);

        VkPresentInfoKHR present_info {};

        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = &semaphores_[rendering_done_index];
        present_info.swapchainCount = 1;
        present_info.pSwapchains = &swapchain_;
        present_info.pImageIndices = &swapchain_index;

        vkQueuePresentKHR(queue_, &present_info);
    }

private:
    Window* window_;
    VkInstance instance_;
    VkPhysicalDevice physical_device_;
    VkPhysicalDeviceMemoryProperties physical_device_memory_properties_;
    uint32_t queue_family_index_;
    VkDevice device_;
    VkQueue queue_;
    VkCommandPool command_pool_;
    VkCommandBuffer command_buffer_;
    VkFence fence_;
    std::array<VkSemaphore, 2> semaphores_;
    VkBuffer vertex_buffer_;
    VkDeviceMemory vertex_device_memory_;
    VkBuffer index_buffer_;
    VkDeviceMemory index_device_memory_;
    VkBuffer uniform_buffer_;
    VkDeviceMemory uniform_device_memory_;
    VkImage texture_image_;
    VkDeviceMemory texture_device_memory_;
    VkImageView texture_image_view_;
    VkSampler texture_sampler_;
    VkSurfaceKHR surface_;
    VkSwapchainKHR swapchain_;
    vector<VkImage> swapchain_images_;
    VkExtent2D swapchain_image_extent_;
    vector<VkImageView> swapchain_image_views_;
    VkRenderPass render_pass_;
    vector<VkFramebuffer> framebuffers_;
    array<VkShaderModule, 2> shader_modules_;
    VkDescriptorSetLayout material_descriptor_set_layout_;
    VkDescriptorSetLayout texture_descriptor_set_layout_;
    VkPipelineLayout pipeline_layout_;
    VkPipeline pipeline_;
    VkDescriptorPool descriptor_pool_;
    VkDescriptorSet material_descriptor_set_;
    VkDescriptorSet texture_descriptor_set_;
};

//----------------------------------------------------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    Window_desc window_desc;

    window_desc.title = L"Chapter13"s;
    window_desc.extent = {512, 512, 1};

    Window window {window_desc};

    Chapter13 chapter13 {&window};

    window.run();

    return 0;
}

//----------------------------------------------------------------------------------------------------------------------
