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

// 위치와 색상 정보를 가진 버텍스 정보를 정의합니다.
struct Vertex {
    float x, y, z; // 위치 정보
    float r, g, b; // 색상 정보
    float u, v;    // 텍스처 좌표 정보
};

//----------------------------------------------------------------------------------------------------------------------

// 색상 정보를 가진 유니폼 버퍼를 정의합니다.
struct Material {
    float r, g, b; // 색상 정보
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

    void init_physical_device_memory_properties_()
    {
        // 물리 디바이스 메모리 성질을 얻어옵니다.
        vkGetPhysicalDeviceMemoryProperties(physical_device_, &physical_device_memory_properties_);
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

    void init_semaphores_()
    {
        // 세마포어는 큐와 큐의 동기화를 위해 사용됩니다.

        // 생성하려는 세마포어를 정의합니다.
        VkSemaphoreCreateInfo create_info {};

        create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        // 두 개의 세마포어를 생성합니다.
        // - 첫 번째 세마포어는 스왑체인 이미지가 준비된 후에 제출한 커맨드 버퍼가 처리되기 위해 사용됩니다.
        // - 두 번째 세마포어는 제출한 커맨드 버퍼가 처리된 후에 화면에 출력을 하기 위해 사용됩니다.
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
        // 펜스는 호스트와 디바이스의 동기화를 위해 사용됩니다.

        // 생성하려는 펜스를 정의합니다.
        VkFenceCreateInfo create_info {};

        create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        // 커맨드 버퍼가 모두 처리됬는지를 알기 위해 사용된 펜스는 처음에 시그널 상태면 어플리케이션의 로직이 단순해집니다.
        create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        // 펜스를 생성합니다.
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
        // 메모리 요구사항과 필요한 메모리 성질을 모두 만족하는 메모리 타입 인덱스를 찾습니다.
        for (auto i = 0; i != physical_device_memory_properties_.memoryTypeCount; ++i) {
            // 메모리가 i번째 메모리 타입에 할당 가능한지 검사합니다.
            if (!(requirements.memoryTypeBits & (1 << i)))
                continue;

            // 개발자가 요구한 메모리 성질을 만족하는지 검사합니다.
            if ((physical_device_memory_properties_.memoryTypes[i].propertyFlags & properties) != properties)
                continue;

            return i;
        }

        assert(false);
        return UINT32_MAX;
    }

    void init_vertex_resources_()
    {
        // 삼각형을 그리기 위해 필요한 버텍스 정보를 정의합니다.
        vector<Vertex> vertices = {
             /*    위치     */ /*   색상    */ /* 텍스처 좌표 */
            { 0.0, -0.5, 0.0, 1.0, 0.0, 0.0, 0.5, 0.0},
            { 0.5,  0.5, 0.0, 0.0, 1.0, 0.0, 1.0, 1.0},
            {-0.5,  0.5, 0.0, 0.0, 0.0, 1.0, 0.0, 1.0}
        };

        // 생성하려는 버텍스 버퍼를 정의합니다.
        VkBufferCreateInfo create_info {};

        create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        create_info.size = sizeof(Vertex) * vertices.size();
        // 버퍼의 사용처를 VERTEX_BUFFER_BIT을 설정하지 않으면 버텍스 버퍼로 사용할 수 없습니다.
        create_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

        // 버퍼를 생성합니다.
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

        // 메모리 요구사항을 얻어올 변수를 선언합니다.
        VkMemoryRequirements requirements;

        // 버퍼를 위해 필요한 메모리 요구사항을 얻어옵니다.
        vkGetBufferMemoryRequirements(device_, vertex_buffer_, &requirements);

        // 할당하려는 메모리를 정의합니다.
        VkMemoryAllocateInfo alloc_info {};

        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize = requirements.size;
        // CPU에서 접근 가능하고 CPU와 GPU의 메모리 동기화가 보장되는 메모리 타입을 선택합니다.
        alloc_info.memoryTypeIndex = find_memory_type_index(requirements, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        // 메모리를 할당합니다.
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

        // 버퍼와 메모리를 바인드합니다.
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

        // CPU에서 메모리에 접근하기 위한 버추얼 어드레스를 가져옵니다.
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

        // 버텍스 정보를 메모리에 복사합니다.
        memcpy(contents, &vertices[0], sizeof(Vertex) * vertices.size());

        // CPU에서 메모리의 접근을 끝마칩니다.
        vkUnmapMemory(device_, vertex_device_memory_);

        // CPU에서 메모리를 자주 접근하는 경우에 언맵을 하지 않고
        // 맵을 통해 얻어온 버츄얼 어드레스를 계속 사용해도 됩니다.
    }

    void init_index_resources_()
    {
        // 일반적으로 인덱스 정보는 변경되지 않습니다. 즉 CPU의 접근이 필요하지 않고 GPU의 접근만 필요합니다.
        // 그러므로 GPU가 빠르게 접근할 수 있는 메모리 타입에 할당하는것이 효율적입니다.

        // 인덱스 정보를 정의합니다.
        vector<uint16_t> indices {0, 1, 2};

        {
            // 생성하려는 인덱스 버퍼를 정의한다.
            VkBufferCreateInfo create_info {};

            create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            create_info.size = sizeof(uint16_t) * indices.size();
            create_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

            // 정의한 인덱스 버퍼를 생성한다.
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

            // 메모리 요구사항을 얻어올 변수를 선언합니다.
            VkMemoryRequirements requirements;

            // 버퍼를 위해 필요한 메모리 요구사항을 얻어옵니다.
            vkGetBufferMemoryRequirements(device_, index_buffer_, &requirements);

            // 할당하려는 메모리를 정의합니다.
            VkMemoryAllocateInfo alloc_info {};

            alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            alloc_info.allocationSize = requirements.size;
            // GPU의 접근이 빠르나 CPU는 접근할 수 없습니다.
            alloc_info.memoryTypeIndex = find_memory_type_index(requirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            // 메모리를 할당합니다.
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

            // 이 메모리를 CPU가 접근할 수 없기 때문에 메모리의 버추얼 어드레스를 얻어올 수 없습니다.
            // void* contents;
            // vkMapMemory(device_, index_device_memory_, 0, VK_WHOLE_SIZE, 0, &contents);

            // 버퍼와 메모리를 바인드합니다.
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

        VkBuffer staging_buffer;
        VkDeviceMemory staging_device_memory;

        {
            // 생성하려는 스테이징 버퍼를 정의합니다.
            VkBufferCreateInfo create_info {};

            create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            create_info.size = sizeof(uint16_t) * indices.size();
            create_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

            // 버퍼를 생성합니다.
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

            // 메모리 요구사항을 얻어올 변수를 선언합니다.
            VkMemoryRequirements requirements;

            // 버퍼를 위해 필요한 메모리 요구사항을 얻어옵니다.
            vkGetBufferMemoryRequirements(device_, staging_buffer, &requirements);

            // 할당하려는 메모리를 정의합니다.
            VkMemoryAllocateInfo alloc_info {};

            alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            alloc_info.allocationSize = requirements.size;
            // CPU에서 접근 가능하고 CPU와 GPU의 메모리 동기화가 보장되는 메모리 타입을 선택합니다.
            alloc_info.memoryTypeIndex = find_memory_type_index(requirements, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

            // 메모리를 할당합니다.
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

            // 버퍼와 메모리를 바인드합니다.
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

            // CPU에서 메모리에 접근하기 위한 버추얼 어드레스를 가져옵니다.
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

            // 인덱스 정보를 메모리에 복사한다.
            memcpy(contents, &indices[0], sizeof(uint16_t) * indices.size());

            // CPU에서 메모리의 접근을 끝마칩니다.
            vkUnmapMemory(device_, staging_device_memory);
        }

        // 생성할 커맨드 버퍼를 정의합니다.
        VkCommandBufferAllocateInfo allocate_info {};

        allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocate_info.commandPool = command_pool_;
        allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocate_info.commandBufferCount = 1;

        // 커맨드 버퍼를 생성합니다.
        VkCommandBuffer command_buffer;
        vkAllocateCommandBuffers(device_, &allocate_info, &command_buffer);

        // 커맨드를 기록하기 위해 커맨드 버퍼의 사용목적을 정의한다.
        VkCommandBufferBeginInfo begin_info {};

        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        // 커맨드 버퍼에 커맨드 기록을 시작한다.
        vkBeginCommandBuffer(command_buffer, &begin_info);

        // 복사할 영역을 정의합니다.
        VkBufferCopy region {};

        region.size = sizeof(uint16_t) * indices.size();

        // 스테이징 버퍼를 인덱스 버퍼로 정의한 영역만큼 복사합니다.
        vkCmdCopyBuffer(command_buffer, staging_buffer, index_buffer_, 1, &region);

        // 커맨드 버퍼에 커맨드 기록을 끝마친다.
        vkEndCommandBuffer(command_buffer);

        // 어떤 기록된 커맨드를 큐에 제출할지 정의한다.
        VkSubmitInfo submit_info {};

        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffer;

        // 기록된 커맨드를 큐에 제출한다.
        vkQueueSubmit(queue_, 1, &submit_info, VK_NULL_HANDLE);

        // 커맨드 버퍼들이 모두 처리될 때까지 기다린다.
        vkDeviceWaitIdle(device_);

        // 할당된 메모리를 해제합니다.
        vkFreeMemory(device_, staging_device_memory, nullptr);

        // 생성된 버퍼를 파괴합니다.
        vkDestroyBuffer(device_, staging_buffer, nullptr);
    }

    void init_uniform_resources_()
    {
        // 생성하려는 유니폼 버퍼를 정의합니다.
        VkBufferCreateInfo create_info {};

        create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        create_info.size = sizeof(Material);
        // 버퍼의 사용처를 UNIFORM_BUFFER_BIT을 설정하지 않으면 유니폼 버퍼로 사용할 수 없습니다.
        create_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

        // 버퍼를 생성합니다.
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

        // 메모리 요구사항을 얻어올 변수를 선언합니다.
        VkMemoryRequirements requirements;

        // 버퍼를 위해 필요한 메모리 요구사항을 얻어옵니다.
        vkGetBufferMemoryRequirements(device_, vertex_buffer_, &requirements);

        // 할당하려는 메모리를 정의합니다.
        VkMemoryAllocateInfo alloc_info {};

        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize = requirements.size;
        // CPU에서 접근 가능하고 CPU와 GPU의 메모리 동기화가 보장되는 메모리 타입을 선택합니다.
        alloc_info.memoryTypeIndex = find_memory_type_index(requirements,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        // 메모리를 할당합니다.
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

        // 버퍼와 메모리를 바인드합니다.
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

        {
            // 생성하려는 이미지를 정의합니다.
            VkImageCreateInfo create_info {};

            create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            create_info.imageType = VK_IMAGE_TYPE_2D;
            create_info.format = VK_FORMAT_R8G8B8A8_UNORM;
            create_info.extent = {static_cast<uint32_t>(w), static_cast<uint32_t>(h), 1};
            create_info.mipLevels = 1;
            create_info.arrayLayers = 1;
            create_info.samples = VK_SAMPLE_COUNT_1_BIT;
            // GPU에서 이미지를 효율적으로 처리하기 위해 옵티말 타일링을 사용합니다.
            create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
            // 복사와 셰이더에서 사용하기 위한 이미지 사용처를 정의합니다.
            create_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            // 이미지를 생성합니다.
            auto result = vkCreateImage(device_, &create_info, nullptr, &texture_image_);
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

            // 메모리 요구사항을 얻어올 변수를 선언합니다.
            VkMemoryRequirements requirements;

            // 이미지를 위해 필요한 메모리 요구사항을 얻어옵니다.
            vkGetImageMemoryRequirements(device_, texture_image_, &requirements);

            // 할당하려는 메모리를 정의한다.
            VkMemoryAllocateInfo alloc_info {};

            alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            alloc_info.allocationSize = requirements.size;
            // GPU에서 접근이 빠른 메모리 타입을 선택합니다.
            alloc_info.memoryTypeIndex = find_memory_type_index(requirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            // 메모리를 할당합니다.
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

            // 이미지와 메모리를 바인딩합니다.
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


        // 이미지는 파일로부터 한번 읽고 특수한 경우를 제외하곤 변경되지 않는다.
        // 그러므로 GPU의 접근이 용이한 메모리를 할당한다.

        VkBuffer staging_buffer;
        VkDeviceMemory staging_device_memory;

        {
            // 생성하려는 스테이징 버퍼를 정의합니다.
            VkBufferCreateInfo create_info {};

            create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            create_info.size = w * h * STBI_rgb_alpha;
            create_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

            // 버퍼를 생성합니다.
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

            // 메모리 요구사항을 얻어올 변수를 선언합니다.
            VkMemoryRequirements requirements;

            // 버퍼를 위해 필요한 메모리 요구사항을 얻어옵니다.
            vkGetBufferMemoryRequirements(device_, staging_buffer, &requirements);

            // 할당하려는 메모리를 정의합니다.
            VkMemoryAllocateInfo alloc_info {};

            alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            alloc_info.allocationSize = requirements.size;
            // CPU에서 접근 가능하고 CPU와 GPU의 메모리 동기화가 보장되는 메모리 타입을 선택합니다.
            alloc_info.memoryTypeIndex = find_memory_type_index(requirements, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                                                                              | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

            // 메모리를 할당합니다.
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

            // 버퍼와 메모리를 바인드합니다.
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

            // CPU에서 메모리에 접근하기 위한 버추얼 어드레스를 가져옵니다.
            void* contents;
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

            // 비트맵을 메모리에 복사합니다.
            memcpy(contents, data, w * h * STBI_rgb_alpha);

            // CPU에서 메모리의 접근을 끝마칩니다.
            vkUnmapMemory(device_, staging_device_memory);
        }

        // 이미지를 읽기 위한 메모리를 해제한다.
        stbi_image_free(data);

        // 비록 이미지를 생성했지만 이미지 데이터는 스테이징 버퍼에 담겨있다.
        // 그러므로 스테이징 버퍼의 내용을 이미지로 복사해주는 작업을 해야한다.

        {
            // 생성할 커맨드 버퍼를 정의합니다.
            VkCommandBufferAllocateInfo allocate_info {};

            allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocate_info.commandPool = command_pool_;
            allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocate_info.commandBufferCount = 1;

            // 커맨드 버퍼를 생성합니다.
            VkCommandBuffer command_buffer;
            vkAllocateCommandBuffers(device_, &allocate_info, &command_buffer);

            // 커맨드를 기록하기 위해 커맨드 버퍼의 사용목적을 정의한다.
            VkCommandBufferBeginInfo begin_info {};

            begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

            // 커맨드 버퍼에 커맨드 기록을 시작한다.
            vkBeginCommandBuffer(command_buffer, &begin_info);

            {
                // 이미지 배리어를 정의하기 위한 변수를 선언합니다.
                VkImageMemoryBarrier barrier {};

                // 이미지 배리어를 정의합니다.
                // 이미지에 복사를 하기 위해서는 TRANSFER_DST_OPTIMAL이어야 합니다.
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

            // 복사될 이미지 서브리소스를 정의합니다.
            VkImageSubresourceLayers subresource_layers = {};

            subresource_layers.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            subresource_layers.mipLevel = 0;
            subresource_layers.layerCount = 1;

            // 복사할 영역을 정의합니다.
            VkBufferImageCopy region {};

            // 버퍼 영역을 정의하지 않을 경우 이미지와 동일한 크기를 가지고 있다고 간주합니다.
            region.imageSubresource = subresource_layers;
            region.imageExtent = {static_cast<uint32_t>(w), static_cast<uint32_t>(h), 1};

            // 스테이징 버퍼를 이미지로 정의한 영역만큼 복사합니다.
            vkCmdCopyBufferToImage(command_buffer,
                                   staging_buffer,
                                   texture_image_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                   1, &region);

            {
                // 이미지 배리어를 정의하기 위한 변수를 선언합니다.
                VkImageMemoryBarrier barrier {};

                // 이미지 배리어를 정의합니다.
                // 이미지가 셰이더에서 사용되기 위해서는 SHADER_READ_ONLY_OPTIMAL이어야 합니다.
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

            // 커맨드 버퍼에 커맨드 기록을 끝마칩니다.
            vkEndCommandBuffer(command_buffer);

            // 어떤 기록된 커맨드를 큐에 제출할지 정의합니다.
            VkSubmitInfo submit_info {};

            submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submit_info.commandBufferCount = 1;
            submit_info.pCommandBuffers = &command_buffer;

            // 기록된 커맨드를 큐에 제출합니다.
            vkQueueSubmit(queue_, 1, &submit_info, VK_NULL_HANDLE);

            // 커맨드 버퍼들이 모두 처리될 때까지 기다립니다.
            vkDeviceWaitIdle(device_);

            // 커맨드 버퍼를 해제합니다.
            vkFreeCommandBuffers(device_, command_pool_, 1, &command_buffer);

            // 할당된 메모리를 해제합니다.
            vkFreeMemory(device_, staging_device_memory, nullptr);

            // 생성된 버퍼를 파괴합니다.
            vkDestroyBuffer(device_, staging_buffer, nullptr);
        }

        {
            // 그래픽스 파이프라인에서 이미지를 접근하기 위해선 이미지 뷰가 필요합니다.

            // 생성하려는 이미지 뷰를 정의합니다.
            VkImageViewCreateInfo create_info {};

            create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            create_info.image = texture_image_;
            create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            create_info.format = VK_FORMAT_R8G8B8A8_UNORM;
            create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            create_info.subresourceRange.levelCount = 1;
            create_info.subresourceRange.layerCount = 1;

            // 이미지 뷰를 생성합니다.
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

        {
            // 셰이더에서 이미지를 읽기 위해선 샘플러가 반드시 필요합니다.

            // 생성하려는 샘플러를 정의합니다.
            VkSamplerCreateInfo create_info {};

            create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            create_info.magFilter = VK_FILTER_LINEAR;
            create_info.minFilter = VK_FILTER_LINEAR;
            create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

            // 샘플러를 생성합니다.
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

        // 스왑체인 이미지의 크기를 저장합니다.
        swapchain_image_extent_ = surface_capabilities.currentExtent;

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
        create_info.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
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

        // 스왑체인 이미지의 레이아웃을 PRESENT_SRC로 변경하는 배리어를 정의한다.
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

        // 정의된 배리어를 실행하는 커맨드를 커맨드 버퍼에 기록한다.
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

    void init_swapchain_image_views_()
    {
        // 그래픽스 파이프라인에서 이미지를 접근하기 위해선 이미지 뷰가 필요합니다.

        auto surface_format = default_surface_format_();

        // 생성하려는 이미지 뷰를 정의합니다.
        VkImageViewCreateInfo create_info {};

        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format = surface_format.format;
        create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.layerCount = 1;

        // 이미지 뷰를 위한 메모리를 할당합니다.
        swapchain_image_views_.resize(swapchain_images_.size());

        for (auto i = 0; i != swapchain_images_.size(); ++i) {
            // 이미지 뷰를 생성할 이미지를 정의합니다.
            create_info.image = swapchain_images_[i];

            // 이미지 뷰를 생성합니다.
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

        // 렌더패스의 어태치먼트를 정의합니다.
        VkAttachmentDescription attachment_desc {};

        attachment_desc.format = surface_format.format;
        attachment_desc.samples = VK_SAMPLE_COUNT_1_BIT;
        attachment_desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment_desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment_desc.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachment_desc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        // 서브패스에서 렌더패스의 어떤 어태치먼트를 참조할지 정의합니다.
        VkAttachmentReference color_attachment_ref {};

        color_attachment_ref.attachment = 0;
        color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        // 렌더패스의 서브패스를 정의합니다.
        VkSubpassDescription subpass_desc {};

        subpass_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass_desc.colorAttachmentCount = 1;
        subpass_desc.pColorAttachments = &color_attachment_ref;

        // 렌더패스를 정의합니다.
        VkRenderPassCreateInfo create_info {};

        create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        create_info.attachmentCount = 1;
        create_info.pAttachments = &attachment_desc;
        create_info.subpassCount = 1;
        create_info.pSubpasses = &subpass_desc;

        // 렌더패스를 생성합니다.
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
        // 생성하려는 프레임버퍼를 정의합니다.
        VkFramebufferCreateInfo create_info {};

        create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        create_info.renderPass = render_pass_;
        create_info.attachmentCount = 1;
        create_info.width = swapchain_image_extent_.width;
        create_info.height = swapchain_image_extent_.height;
        create_info.layers = 1;

        // 프레임버퍼를 위한 메모리를 할당합니다.
        framebuffers_.resize(swapchain_image_views_.size());

        for (auto i = 0; i != swapchain_image_views_.size(); ++i) {
            create_info.pAttachments = &swapchain_image_views_[i];

            // 프레임버퍼를 생성합니다.
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
            const string vksl = {
                "precision mediump float;                 \n"
                "                                         \n"
                "layout(location = 0) in vec2 i_pos;      \n"
                "layout(location = 1) in vec3 i_col;      \n"
                // 버텍스에서 텍스처 좌표 정보가 제공됩니다.
                "layout(location = 2) in vec2 i_uv;       \n"
                "                                         \n"
                "layout(location = 0) out vec3 o_col;     \n"
                // 프래그먼트 셰이더로 텍스처 좌표 정보를 전달합니다.
                "layout(location = 1) out vec2 o_uv;      \n"
                "                                         \n"
                "void main() {                            \n"
                "                                         \n"
                "    gl_Position = vec4(i_pos, 0.0, 1.0); \n"
                "    o_col = i_col;                       \n"
                "    o_uv = i_uv;                         \n"
                "}                                        \n"
            };

            // 런타임 컴파일러를 사용해서 VKSL을 SPIRV로 컴파일합니다.
            auto spirv = Spirv_compiler().compile(Shader_type::vertex, vksl);

            // 생성하려는 셰이더 모듈을 정의합니다.
            VkShaderModuleCreateInfo create_info {};

            create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            create_info.codeSize = spirv.size() * sizeof(uint32_t);
            create_info.pCode = &spirv[0];

            // 셰이더 모듈을 생성합니다.
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
            const string vksl = {
                "precision mediump float;                            \n"
                "                                                    \n"
                "layout(location = 0) in vec3 i_col;                 \n"
                // 버텍스 셰이더로부터 텍스처 좌표 정보를 전달 받습니다.
                "layout(location = 1) in vec2 i_uv;                  \n"
                "                                                    \n"
                "layout(location = 0) out vec4 fragment_color0;      \n"
                "                                                    \n"
                "layout(set = 0, binding = 0) uniform Material {     \n"
                "    vec3 col;                                       \n"
                "} material;                                         \n"
                "                                                    \n"
                // 컴바인드 이미지 샘플러를 정의합니다.
                "layout(set = 1, binding = 0) uniform sampler2D tex; \n"
                "                                                    \n"
                "void main() {                                       \n"
                "    vec3 col = i_col;                               \n"
                "    col *= material.col;                            \n"
                     // 텍스처 좌표 정보를 이용해서 컴바인드 이미지 샘플러로부터 텍셀을 가져옵니다.
                "    col *= texture(tex, i_uv).rgb;                  \n"
                "                                                    \n"
                "    fragment_color0 = vec4(col, 1.0);               \n"
                "}                                                   \n"
            };

            // 런타임 컴파일러를 사용해서 VKSL을 SPIRV로 컴파일합니다.
            auto spirv = Spirv_compiler().compile(Shader_type::fragment, vksl);

            // 생성하려는 셰이더 모듈을 정의합니다.
            VkShaderModuleCreateInfo create_info {};

            create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            create_info.codeSize = spirv.size() * sizeof(uint32_t);
            create_info.pCode = &spirv[0];

            // 셰이더 모듈을 생성합니다.
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
            // 유니폼 블록을 위한 디스크립터 셋 레이아웃 바인딩을 정의합니다.
            VkDescriptorSetLayoutBinding binding {};

            binding.binding = 0;
            binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            binding.descriptorCount = 1;
            // 프래그먼트 셰이더에서만 접근됩니다.
            binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

            // 생성하려는 디스크립터 셋 레이아웃을 정의합니다.
            VkDescriptorSetLayoutCreateInfo create_info {};

            create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            create_info.bindingCount = 1;
            create_info.pBindings = &binding;

            // 디스크립터 셋 레이아웃을 생성합니다.
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
            // 컴바인드 이미지 샘플러를 위한 디스크립터 셋 레이아웃 바인딩을 정의합니다.
            VkDescriptorSetLayoutBinding binding {};

            binding.binding = 0;
            binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            binding.descriptorCount = 1;
            // 프래그먼트 셰이더에서만 접근됩니다.
            binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

            // 생성하려는 디스크립터 셋 레이아웃을 정의합니다.
            VkDescriptorSetLayoutCreateInfo create_info {};

            create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            create_info.bindingCount = 1;
            create_info.pBindings = &binding;

            // 디스크립터 셋 레이아웃을 생성합니다.
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
        // 파이프라인 레이아웃을 정의하기 위한 디스크립터 셋 레이아웃을 나열합니다.
        // 0번 디스크립터 셋은 유니폼 버퍼입니다.
        // 1번 디스크립터 셋은 컴바인드 이미지 샘플러입니다.
        vector<VkDescriptorSetLayout> set_layouts {
            material_descriptor_set_layout_,
            texture_descriptor_set_layout_
        };

        // 생성하려는 파이프라인 레이아웃을 정의합니다.
        VkPipelineLayoutCreateInfo create_info {};

        create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        create_info.setLayoutCount = set_layouts.size();
        // layout(set = 0, location = Y)에 해당하는 리소스들의 디스크립터 셋 레이아웃입니다.
        create_info.pSetLayouts = &set_layouts[0];

        // 파이프라인 레이아웃을 생성합니다.
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
        // 파이프라인 셰이더 스테이지들을 정의합니다.
        array<VkPipelineShaderStageCreateInfo, 2> stages;

        {
            // 생성하려는 파이프라인 버텍스 셰이더 스테이지를 정의합니다.
            VkPipelineShaderStageCreateInfo stage {};

            stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            stage.stage = VK_SHADER_STAGE_VERTEX_BIT;
            stage.module = shader_modules_[0];
            stage.pName = "main";

            stages[0] = stage;
        }

        {
            // 생성하려는 파이프라인 프래그먼트 셰이더 스테이지를 정의합니다.
            VkPipelineShaderStageCreateInfo stage {};

            stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            stage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
            stage.module = shader_modules_[1];
            stage.pName = "main";

            stages[1] = stage;
        }

        // 버텍스 인풋 바인딩을 정의합니다.
        VkVertexInputBindingDescription vertex_input_binding {};

        vertex_input_binding.binding = 0;
        vertex_input_binding.stride = sizeof(Vertex);
        vertex_input_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        vector<VkVertexInputAttributeDescription> vertex_input_attributes;

        {
            // 위치에 대한 버텍스 인풋 애트리뷰트를 정의합니다.
            VkVertexInputAttributeDescription vertex_input_attribute {};

            vertex_input_attribute.location = 0; // 셰이더에서 정의한 로케이션
            vertex_input_attribute.binding = 0;  // 버텍스 버퍼의 바인딩 인덱스
            vertex_input_attribute.format = VK_FORMAT_R32G32B32_SFLOAT;
            vertex_input_attribute.offset = offsetof(Vertex, x);

            vertex_input_attributes.push_back(vertex_input_attribute);
        }

        {
            // 색상에 대한 버텍스 인풋 애트리뷰트를 정의합니다.
            VkVertexInputAttributeDescription vertex_input_attribute {};

            vertex_input_attribute.location = 1; // 셰이더에서 정의한 로케이션
            vertex_input_attribute.binding = 0;  // 버텍스 버퍼의 바인딩 인덱스
            vertex_input_attribute.format = VK_FORMAT_R32G32B32_SFLOAT;
            vertex_input_attribute.offset = offsetof(Vertex, r);

            vertex_input_attributes.push_back(vertex_input_attribute);
        }

        {
            // 텍스처 좌표에 대한 버텍스 인풋 애트리뷰트를 정의합니다.
            VkVertexInputAttributeDescription vertex_input_attribute {};

            vertex_input_attribute.location = 2; // 셰이더에서 정의한 로케이션
            vertex_input_attribute.binding = 0;  // 버텍스 버퍼의 바인딩 인덱스
            vertex_input_attribute.format = VK_FORMAT_R32G32_SFLOAT;
            vertex_input_attribute.offset = offsetof(Vertex, u);

            vertex_input_attributes.push_back(vertex_input_attribute);
        }

        // 파이프라인 버텍스 인풋 스테이지를 정의합니다.
        VkPipelineVertexInputStateCreateInfo vertex_input_state {};

        vertex_input_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertex_input_state.vertexBindingDescriptionCount = 1;
        vertex_input_state.pVertexBindingDescriptions = &vertex_input_binding;
        vertex_input_state.vertexAttributeDescriptionCount = vertex_input_attributes.size();
        vertex_input_state.pVertexAttributeDescriptions = &vertex_input_attributes[0];

        // 파이프라인 인풋 어셈블리 스테이지를 정의합니다.
        VkPipelineInputAssemblyStateCreateInfo input_assembly_state {};

        input_assembly_state.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        input_assembly_state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        // 뷰포트를 정의합니다.
        VkViewport viewport {};

        viewport.width = swapchain_image_extent_.width;
        viewport.height = swapchain_image_extent_.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        // 시져 영역을 정의합니다.
        VkRect2D scissor {};

        scissor.extent = swapchain_image_extent_;

        // 파이프라인 뷰포트 스테이지를 정의합니다.
        VkPipelineViewportStateCreateInfo viewport_state {};

        viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport_state.viewportCount = 1;
        viewport_state.pViewports = &viewport;
        viewport_state.scissorCount = 1;
        viewport_state.pScissors = &scissor;

        // 파이프라인 래스터라이제이션 스테이지를 정의합니다.
        VkPipelineRasterizationStateCreateInfo rasterization_state {};

        rasterization_state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterization_state.polygonMode = VK_POLYGON_MODE_FILL;
        rasterization_state.cullMode = VK_CULL_MODE_NONE;
        rasterization_state.lineWidth = 1.0f;

        // 파이프라인 멀티샘플 스테이지를 정의합니다.
        // 멀티샘플을 사용하지 않는 경우 래스터라이제이션 샘플 개수를 1로 정의해야합니다.
        VkPipelineMultisampleStateCreateInfo multisample_state {};

        multisample_state.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisample_state.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        // 파이프라인 뎁스 스텐실 스테이지를 정의합니다.
        VkPipelineDepthStencilStateCreateInfo depth_stencil_state {};

        depth_stencil_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

        // 파이프라인 컬러 블렌드 어테치먼트에 대해 정의합니다.
        // colorWriteMask가 0이면 프래그먼트의 결과가 이미지에 기록되지 않습니다.
        VkPipelineColorBlendAttachmentState attachment {};

        attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT
                                  | VK_COLOR_COMPONENT_G_BIT
                                  | VK_COLOR_COMPONENT_B_BIT
                                  | VK_COLOR_COMPONENT_A_BIT;

        // 파이프라인 컬러 블렌드 스테이지를 정의합니다.
        VkPipelineColorBlendStateCreateInfo color_blend_state {};

        color_blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        color_blend_state.attachmentCount = 1;
        color_blend_state.pAttachments = &attachment;

        // 생성하려는 그래픽스 파이프라인을 정의합니다.
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

        // 그래픽스 파이프라인을 생성합니다.
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
        // 파이프라인에서 자원을 접근하기 위해서는 디스크립터 셋이 필요합니다.
        // 디스크립터 셋은 디스크립터 풀을 통해 할당받을 수 있습니다.

        vector<VkDescriptorPoolSize> pool_size {
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
            // 컴바인드 이미지 샘플러를 할당하기 위해 디스크립터 풀 크기를 변경합니다.
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}
        };

        // 생성하려는 디스크립터 풀을 정의합니다.
        VkDescriptorPoolCreateInfo create_info {};

        create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        // 유니폼 버퍼와 컴바인드 이미지 샘플러를 위한 디스크립터 셋이 2개 필요합니다.
        create_info.maxSets = 2;
        create_info.poolSizeCount = pool_size.size();
        create_info.pPoolSizes = &pool_size[0];

        // 디스크립터 풀을 생성합니다.
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
            // 할당 받으려는 디스크립터 셋을 정의합니다.
            VkDescriptorSetAllocateInfo allocate_info {};

            allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocate_info.descriptorPool = descriptor_pool_;
            allocate_info.descriptorSetCount = 1;
            allocate_info.pSetLayouts = &material_descriptor_set_layout_;

            // 디스크립터 셋을 할당 받습니다.
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
            // 할당 받으려는 디스크립터 셋을 정의합니다.
            VkDescriptorSetAllocateInfo allocate_info {};

            allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocate_info.descriptorPool = descriptor_pool_;
            allocate_info.descriptorSetCount = 1;
            allocate_info.pSetLayouts = &texture_descriptor_set_layout_;

            // 디스크립터 셋을 할당 받습니다.
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

    void fini_semaphores_()
    {
        // 생성한 세마포어를 파괴합니다.
        for (auto& semaphore : semaphores_)
            vkDestroySemaphore(device_, semaphore, nullptr);
    }

    void fini_fence_()
    {
        // 생성한 펜스를 파괴합니다.
        vkDestroyFence(device_, fence_, nullptr);
    }

    void fini_vertex_resources_()
    {
        // 할당된 메모리를 해제합니다.
        vkFreeMemory(device_, vertex_device_memory_, nullptr);

        // 생성된 버퍼를 파괴합니다.
        vkDestroyBuffer(device_, vertex_buffer_, nullptr);
    }

    void fini_index_resources_()
    {
        // 할당된 메모리를 해제합니다.
        vkFreeMemory(device_, index_device_memory_, nullptr);

        // 생성된 버퍼를 파괴합니다.
        vkDestroyBuffer(device_, index_buffer_, nullptr);
    }

    void fini_uniform_resources_()
    {
        // 할당된 메모리를 해제합니다.
        vkFreeMemory(device_, uniform_device_memory_, nullptr);

        // 생성된 버퍼를 파괴합니다.
        vkDestroyBuffer(device_, uniform_buffer_, nullptr);
    }

    void fini_texture_resources_()
    {
        // 생성한 샘플러를 파괴합니다.
        vkDestroySampler(device_, texture_sampler_, nullptr);

        // 생성한 이미지 뷰를 파괴합니다.
        vkDestroyImageView(device_, texture_image_view_, nullptr);

        // 할당된 메모리를 해제합니다.
        vkFreeMemory(device_, texture_device_memory_, nullptr);

        // 생성한 이미지를 파괴합니다.
        vkDestroyImage(device_, texture_image_, nullptr);
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

    void fini_swapchain_image_views_()
    {
        // 생성한 이미지 뷰를 파괴합니다.
        for (auto& image_view : swapchain_image_views_)
            vkDestroyImageView(device_, image_view, nullptr);
    }

    void fini_render_pass_()
    {
        // 생성한 렌더 패스를 파괴합니다.
        vkDestroyRenderPass(device_, render_pass_, nullptr);
    }

    void fini_framebuffers_()
    {
        // 생성한 프레임버퍼를 파괴합니다.
        for (auto& framebuffer : framebuffers_)
            vkDestroyFramebuffer(device_, framebuffer, nullptr);
    }

    void fini_shader_modules_()
    {
        // 생성한 셰이더 모듈을 파괴합니다.
        for (auto& shader_module : shader_modules_)
            vkDestroyShaderModule(device_, shader_module, nullptr);
    }

    void fini_descriptor_set_layouts_()
    {
        // 생성한 디스크립터 셋 레이아웃을 파괴합니다.
        vkDestroyDescriptorSetLayout(device_, material_descriptor_set_layout_, nullptr);
        vkDestroyDescriptorSetLayout(device_, texture_descriptor_set_layout_, nullptr);
    }

    void fini_pipeline_layout_()
    {
        // 생성한 파이프라인 레이아웃을 파괴합니다.
        vkDestroyPipelineLayout(device_, pipeline_layout_, nullptr);
    }

    void fini_pipeline_()
    {
        // 생성한 그래픽스 파이프라인을 파괴합니다.
        vkDestroyPipeline(device_, pipeline_, nullptr);
    }

    void fini_descriptor_pool_()
    {
        // 생성한 디스크립터 풀을 파괴합니다.
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
        // 스왑체인으로부터 사용가능한 이미지 인덱스를 얻어옵니다.
        // 세마포어를 통해서 GPU에서 스왑체인 이미지가 언제 사용 가능한지 알 수 있습니다.
        uint32_t swapchain_index;
        vkAcquireNextImageKHR(device_, swapchain_, UINT64_MAX,
                              semaphores_[image_available_index], VK_NULL_HANDLE,
                              &swapchain_index);

        auto& swapchain_image = swapchain_images_[swapchain_index];

        // 새로운 프레임을 렌더링하기 위해선 제출한 커맨드 버퍼가 처리되었는지를 확인해야합니다.
        // 다 처리되었는지를 알기 위해서 큐에 커맨드 버퍼를 제출할 때 파라미터로 펜스를 넘길 수 있습니다.
        // 커맨드 버퍼를 리셋하기 전에 펜스의 상태를 확인한 뒤 언시그널드 상태이면 시그널 상태가 될 때까지 기다립니다.
        if (VK_NOT_READY == vkGetFenceStatus(device_, fence_))
            vkWaitForFences(device_, 1, &fence_, VK_TRUE, UINT64_MAX);

        // 펜스를 언시그널 상태로 변경합니다.
        vkResetFences(device_, 1, &fence_);

        {
            // 디스크립터 셋이 가리킬 버퍼 정보를 정의합니다.
            VkDescriptorBufferInfo buffer_info {};

            buffer_info.buffer = uniform_buffer_;
            buffer_info.offset = 0;
            buffer_info.range = sizeof(Material);

            // 디스크립터 셋이 어떤 리소스를 가리킬지 정의합니다.
            VkWriteDescriptorSet descriptor_write {};

            descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptor_write.dstSet = material_descriptor_set_;
            descriptor_write.dstBinding = 0;
            descriptor_write.descriptorCount = 1;
            descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptor_write.pBufferInfo = &buffer_info;

            // 디스크립터 셋을 업데이트 합니다.
            vkUpdateDescriptorSets(device_, 1, &descriptor_write, 0, nullptr);
        }

        {
            // 디스크립터 셋이 가리킬 컴바인드 이미지 샘플러 정보를 정의합니다.
            VkDescriptorImageInfo image_info {};

            image_info.sampler = texture_sampler_;
            image_info.imageView = texture_image_view_;
            image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            // 디스크립터 셋이 어떤 리소스를 가리킬지 정의합니다.
            VkWriteDescriptorSet descriptor_write {};

            descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptor_write.dstSet = texture_descriptor_set_;
            descriptor_write.dstBinding = 0;
            descriptor_write.descriptorCount = 1;
            descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptor_write.pImageInfo = &image_info;

            // 디스크립터 셋을 업데이트 합니다.
            vkUpdateDescriptorSets(device_, 1, &descriptor_write, 0, nullptr);
        }

        // 매 프레임마다 디스크립터 셋이 업데이트됩니다. 그러나 동일한 유니폼 버퍼를 가리키기 때문에
        // 처음 한번만 업데이트하면 될 뿐 매 프레임마다 업데이트 할 필요는 없습니다.
        // 단지 벌칸을 보다 쉽게 이해하기 위해 매 프레임마다 업데이트 합니다.

        // CPU에서 유니폼 버퍼 메모리에 접근하기 위한 버추얼 어드레스를 가져옵니다.
        void* contents;
        vkMapMemory(device_, uniform_device_memory_, 0, sizeof(Material), 0, &contents);

        // CPU에서 정의한 메터리얼 구조체로 캐스팅합니다.
        auto material = static_cast<Material*>(contents);

        // 흘러간 시간을 저장할 변수를 선언합니다.
        static auto time = 0.0f;

        // 색상을 계산합니다.
        // 시간을 이용해서 코사인 값을 계산합니다. 코사인의 결과는 [-1, 1] 범위의 값을 가지고 있습니다.
        // 코사인 결과에 1.0을 더한 뒤 2.0을 나눠서 [0, 1]의 범위의 값으로 변경합니다.
        float value = (cos(time) + 1.0f) / 2.0f;

        time += 0.05f;

        // 메터리얼 데이터를 업데이트 합니다.
        material->r = value;
        material->g = value;
        material->b = value;

        // CPU에서 메모리의 접근을 끝마칩니다.
        vkUnmapMemory(device_, uniform_device_memory_);

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
            // 이미지 배리어를 정의하기 위한 변수를 선언합니다.
            VkImageMemoryBarrier barrier {};

            // 이미지 배리어를 정의합니다.
            // 이미지에 렌더링을 하기 위해서는 COLOR_ATTACHMENT_OPTIMAL이어야 합니다.
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

        // 클리어 색상을 정의하기 위한 변수를 선언합니다.
        VkClearValue clear_value;

        // 클리어 색상을 정의합니다.
        clear_value.color.float32[0] = 0.15f; // R
        clear_value.color.float32[1] = 0.15f; // G
        clear_value.color.float32[2] = 0.15f; // B
        clear_value.color.float32[3] = 1.0f; // A

        // 렌더 패스를 어떻게 시작할지 정의합니다.
        VkRenderPassBeginInfo render_pass_begin_info {};

        render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_begin_info.renderPass = render_pass_;
        render_pass_begin_info.framebuffer = framebuffers_[swapchain_index];
        render_pass_begin_info.renderArea.extent = swapchain_image_extent_;
        render_pass_begin_info.clearValueCount = 1;
        render_pass_begin_info.pClearValues = &clear_value;

        // 렌더 패스를 시작합니다.
        vkCmdBeginRenderPass(command_buffer_, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

        // 삼각형을 그리기 위해 필요한 버텍스 버퍼를 0번에 바인드 합니다.
        VkDeviceSize vertex_buffer_offset {0};
        vkCmdBindVertexBuffers(command_buffer_, 0, 1, &vertex_buffer_, &vertex_buffer_offset);

        // 인덱스 버퍼를 바인드 합니다.
        vkCmdBindIndexBuffer(command_buffer_, index_buffer_, 0, VK_INDEX_TYPE_UINT16);

        // 삼각형을 그리기 위한 그래픽스 파이프라인을 바인딩합니다.
        vkCmdBindPipeline(command_buffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);

        // 디스크립터 셋을 바인드합니다.
        vkCmdBindDescriptorSets(command_buffer_, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pipeline_layout_, 0, 1, &material_descriptor_set_,
                                0, nullptr);
        vkCmdBindDescriptorSets(command_buffer_, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pipeline_layout_, 1, 1, &texture_descriptor_set_,
                                0, nullptr);

        // 인덱스 버퍼를 이용한 드로우 커맨드를 기록합니다.
        vkCmdDrawIndexed(command_buffer_, 3, 1, 0, 0, 0);

        // 렌더 패스를 종료합니다.
        vkCmdEndRenderPass(command_buffer_);

        {
            // 이미지 배리어를 정의하기 위한 변수를 선언합니다.
            VkImageMemoryBarrier barrier {};

            // 이미지 배리어를 정의합니다. 이미지를 화면에 출력하기 위해서는
            // 이미지 레이아웃이 반드시 아래 레이아웃 중에 한 레이아웃이어야 합니다.
            // - PRESENT_SRC_KHR
            // - SHARED_PRESENT_KHR
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            barrier.srcQueueFamilyIndex = queue_family_index_;
            barrier.dstQueueFamilyIndex = queue_family_index_;
            barrier.image = swapchain_image;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.layerCount = 1;

            // 이미지 레이아웃 변경을 위한 파이프라인 배리어 커맨드를 기록합니다.
            vkCmdPipelineBarrier(command_buffer_,
                                 VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);
        }

        // 필요한 모든 커맨드들을 기록했기 때문에 커맨드 버퍼의 커맨드 기록을 끝마칩니다.
        // 커맨드 버퍼의 상태는 실행 가능 상태입니다.
        vkEndCommandBuffer(command_buffer_);

        // 세마포어가 반드시 시그널 되야하는 파이프라인 스테이지를 정의합니다.
        constexpr VkPipelineStageFlags wait_dst_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        // 큐에 제출할 커맨드 버퍼와 동기화를 정의하기 위한 변수를 선언합니다.
        VkSubmitInfo submit_info {};

        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        // 픽셀의 결과를 계산한 후에 스왑체인 이미지가 준비될 때까지 기다립니다.
        // 기다리지 않고 픽셀의 결과를 쓴다면 원하지 않는 결과가 화면에 출력됩니다.
        submit_info.waitSemaphoreCount = 1;
        submit_info.pWaitSemaphores = &semaphores_[image_available_index];
        submit_info.pWaitDstStageMask = &wait_dst_stage_mask;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffer_;
        // 결과를 올바르게 출력하기 위해선 커맨드 버퍼가 처리될 때까지 기다려야합니다,
        // 커맨드 버퍼가 처리된 시점을 알려주는 세마포어를 정의합니다.
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores = &semaphores_[rendering_done_index];

        // 큐에 커맨드 버퍼를 제출합니다.
        vkQueueSubmit(queue_, 1, &submit_info, fence_);

        // 화면에 출력되는 이미지를 정의합니다.
        VkPresentInfoKHR present_info {};

        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        // 커맨드 버퍼가 처리되기를 보장하기 위해 기다려야하는 세마포어를 정의합니다.
        // 기다리지 않고 출력한다면 원하지 않는 결과가 화면에 출력됩니다.
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = &semaphores_[rendering_done_index];
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
