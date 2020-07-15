add_executable(chapter09
    vk_mem_alloc.h
	main.cpp
)

set_target_properties(chapter09
PROPERTIES
	CXX_STANDARD 17
)

find_package(Vulkan REQUIRED)

target_include_directories(chapter09
PUBLIC
	${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(chapter09
	${Vulkan_LIBRARIES}
)
