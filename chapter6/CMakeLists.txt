add_executable(chapter6
	main.cpp
)

set_target_properties(chapter6
PROPERTIES
	CXX_STANDARD 17
)

find_package(Vulkan REQUIRED)

target_include_directories(chapter6
PUBLIC
	${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(chapter6
	${Vulkan_LIBRARIES}
	platform
	sc
)

if(CMAKE_SYSTEM_NAME MATCHES Darwin)
	target_link_libraries(chapter6
		"-framework Metal"
		"-framework QuartzCore"
	)

	target_compile_options(chapter6
	PUBLIC
		-x objective-c++
	)
endif()

