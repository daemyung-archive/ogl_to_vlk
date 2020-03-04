add_executable(chapter7
	main.cpp
)

set_target_properties(chapter7
PROPERTIES
	CXX_STANDARD 17
)

find_package(Vulkan REQUIRED)

target_include_directories(chapter7
PUBLIC
	${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(chapter7
	${Vulkan_LIBRARIES}
)
