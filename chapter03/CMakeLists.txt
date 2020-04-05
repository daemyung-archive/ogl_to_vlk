add_executable(chapter03
	main.cpp
)

set_target_properties(chapter03
PROPERTIES
	CXX_STANDARD 17
)

find_package(Vulkan REQUIRED)

target_include_directories(chapter03
PUBLIC
	${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(chapter03
	${Vulkan_LIBRARIES}
)
