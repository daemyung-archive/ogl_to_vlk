add_executable(chapter0
	main.cpp
)

set_target_properties(chapter0
PROPERTIES
	CXX_STANDARD 17
)

find_package(Vulkan REQUIRED)

target_include_directories(chapter0
PUBLIC
	${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(chapter0
	${Vulkan_LIBRARIES}
)
