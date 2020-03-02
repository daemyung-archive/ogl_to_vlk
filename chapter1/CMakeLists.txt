add_executable(chapter1
	main.cpp
)

set_target_properties(chapter1
PROPERTIES
	CXX_STANDARD 17
)

find_package(Vulkan REQUIRED)

target_include_directories(chapter1
PUBLIC
	${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(chapter1
	${Vulkan_LIBRARIES}
)
