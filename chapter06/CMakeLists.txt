add_executable(chapter06
	main.cpp
)

set_target_properties(chapter06
PROPERTIES
	CXX_STANDARD 17
)

find_package(Vulkan REQUIRED)

target_include_directories(chapter06
PUBLIC
	${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(chapter06
	${Vulkan_LIBRARIES}
	platform
)

if(CMAKE_SYSTEM_NAME MATCHES Darwin)
	target_link_libraries(chapter06
		"-framework Metal"
		"-framework QuartzCore"
	)

	target_compile_options(chapter06
	PUBLIC
		-x objective-c++
	)
endif()

