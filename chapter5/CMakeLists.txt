add_executable(chapter5
	main.cpp
)

set_target_properties(chapter5
PROPERTIES
	CXX_STANDARD 17
)

find_package(Vulkan REQUIRED)

target_include_directories(chapter5
PUBLIC
	${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(chapter5
	${Vulkan_LIBRARIES}
	platform
)

if(CMAKE_SYSTEM_NAME MATCHES Darwin)
	target_link_libraries(chapter5
		"-framework Metal"
		"-framework QuartzCore"
	)

	target_compile_options(chapter5
	PUBLIC
		-x objective-c++
	)
endif()

