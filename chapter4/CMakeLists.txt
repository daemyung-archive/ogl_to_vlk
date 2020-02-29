add_executable(chapter4
	main.cpp
)

set_target_properties(chapter4
PROPERTIES
	CXX_STANDARD 17
)

find_package(Vulkan REQUIRED)

target_include_directories(chapter4
PUBLIC
	${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(chapter4
	${Vulkan_LIBRARIES}
	platform
)

if(CMAKE_SYSTEM_NAME MATCHES Darwin)
	target_link_libraries(chapter4
		"-framework Metal"
		"-framework QuartzCore"
	)

	target_compile_options(chapter4
	PUBLIC
		-x objective-c++
	)
endif()

