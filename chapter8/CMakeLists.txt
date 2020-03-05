add_executable(chapter8
	main.cpp
)

set_target_properties(chapter8
PROPERTIES
	CXX_STANDARD 17
)

find_package(Vulkan REQUIRED)

target_include_directories(chapter8
PUBLIC
	${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(chapter8
	${Vulkan_LIBRARIES}
	platform
	sc
)

if(CMAKE_SYSTEM_NAME MATCHES Darwin)
	target_link_libraries(chapter8
		"-framework Metal"
		"-framework QuartzCore"
	)

	target_compile_options(chapter8
	PUBLIC
		-x objective-c++
	)
endif()

