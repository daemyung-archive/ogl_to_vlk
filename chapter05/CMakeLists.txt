add_executable(chapter05
	main.cpp
)

set_target_properties(chapter05
PROPERTIES
	CXX_STANDARD 17
)

find_package(Vulkan REQUIRED)

target_include_directories(chapter05
PUBLIC
	${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(chapter05
	${Vulkan_LIBRARIES}
	platform
)

if(CMAKE_SYSTEM_NAME MATCHES Darwin)
	target_link_libraries(chapter05
		"-framework Metal"
		"-framework QuartzCore"
	)

	target_compile_options(chapter05
	PUBLIC
		-x objective-c++
	)
endif()

