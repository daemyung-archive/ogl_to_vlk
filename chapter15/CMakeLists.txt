add_executable(chapter15
	main.cpp
)

set_target_properties(chapter15
PROPERTIES
	CXX_STANDARD 17
)

find_package(Vulkan REQUIRED)

target_include_directories(chapter15
PUBLIC
	${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(chapter15
	${Vulkan_LIBRARIES}
	platform
	sc
)

if(CMAKE_SYSTEM_NAME MATCHES Darwin)
	target_link_libraries(chapter15
		"-framework Metal"
		"-framework QuartzCore"
	)

	target_compile_options(chapter15
	PUBLIC
		-x objective-c++
	)
endif()

