add_executable(chapter07
	main.cpp
)

set_target_properties(chapter07
PROPERTIES
	CXX_STANDARD 17
)

find_package(Vulkan REQUIRED)

target_include_directories(chapter07
PUBLIC
	${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(chapter07
	${Vulkan_LIBRARIES}
	platform
)

if(CMAKE_SYSTEM_NAME MATCHES Darwin)
	target_link_libraries(chapter07
		"-framework Metal"
		"-framework QuartzCore"
	)

	target_compile_options(chapter07
	PUBLIC
		-x objective-c++
	)
endif()

