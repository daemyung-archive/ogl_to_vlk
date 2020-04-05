add_executable(chapter08
	main.cpp
)

set_target_properties(chapter08
PROPERTIES
	CXX_STANDARD 17
)

find_package(Vulkan REQUIRED)

target_include_directories(chapter08
PUBLIC
	${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(chapter08
	${Vulkan_LIBRARIES}
	platform
	sc
)

if(CMAKE_SYSTEM_NAME MATCHES Darwin)
	target_link_libraries(chapter08
		"-framework Metal"
		"-framework QuartzCore"
	)

	target_compile_options(chapter08
	PUBLIC
		-x objective-c++
	)
endif()

