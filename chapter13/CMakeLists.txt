add_executable(chapter13
	main.cpp
)

set_target_properties(chapter13
PROPERTIES
	CXX_STANDARD 17
)

find_package(Vulkan REQUIRED)

target_include_directories(chapter13
PUBLIC
	${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(chapter13
	${Vulkan_LIBRARIES}
	platform
	sc
)

if(CMAKE_SYSTEM_NAME MATCHES Darwin)
	target_link_libraries(chapter13
		"-framework Metal"
		"-framework QuartzCore"
	)

	target_compile_options(chapter13
	PUBLIC
		-x objective-c++
	)
endif()

