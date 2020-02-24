add_executable(chapter3
	main.cpp
)

set_target_properties(chapter3
PROPERTIES
	CXX_STANDARD 17
)

find_package(Vulkan REQUIRED)

target_include_directories(chapter3
PUBLIC
	${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(chapter3
	${Vulkan_LIBRARIES}
	platform
)

if(CMAKE_SYSTEM_NAME MATCHES Darwin)
	target_link_libraries(chapter3
		"-framework Metal"
		"-framework QuartzCore"
	)

	target_compile_options(chapter3
	PUBLIC
		-x objective-c++
	)
endif()

