add_executable(chapter9
	main.cpp
)

set_target_properties(chapter9
PROPERTIES
	CXX_STANDARD 17
)

find_package(Vulkan REQUIRED)

target_include_directories(chapter9
PUBLIC
	${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(chapter9
	${Vulkan_LIBRARIES}
	platform
	sc
)

if(CMAKE_SYSTEM_NAME MATCHES Darwin)
	target_link_libraries(chapter9
		"-framework Metal"
		"-framework QuartzCore"
	)

	target_compile_options(chapter9
	PUBLIC
		-x objective-c++
	)
endif()

