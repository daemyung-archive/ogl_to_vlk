add_executable(chapter2
	main.cpp
)

set_target_properties(chapter2
PROPERTIES
	CXX_STANDARD 17
)

find_package(Vulkan REQUIRED)

target_include_directories(chapter2
PUBLIC
	${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(chapter2
	${Vulkan_LIBRARIES}
	platform
)

if(CMAKE_SYSTEM_NAME MATCHES Darwin)
	target_link_libraries(chapter2
		"-framework Metal"
		"-framework QuartzCore"
	)

	target_compile_options(chapter2
	PUBLIC
		-x objective-c++
	)
endif()

