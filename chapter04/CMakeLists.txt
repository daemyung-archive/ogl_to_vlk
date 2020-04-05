add_executable(chapter04
	main.cpp
)

set_target_properties(chapter04
PROPERTIES
	CXX_STANDARD 17
)

find_package(Vulkan REQUIRED)

target_include_directories(chapter04
PUBLIC
	${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(chapter04
	${Vulkan_LIBRARIES}
	platform
)

if(CMAKE_SYSTEM_NAME MATCHES Darwin)
	target_link_libraries(chapter04
		"-framework Metal"
		"-framework QuartzCore"
	)

	target_compile_options(chapter04
	PUBLIC
		-x objective-c++
	)
endif()

