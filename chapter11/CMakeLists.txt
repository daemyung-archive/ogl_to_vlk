add_executable(chapter11
	main.cpp
)

set_target_properties(chapter11
PROPERTIES
	CXX_STANDARD 17
)

find_package(Vulkan REQUIRED)

target_include_directories(chapter11
PUBLIC
	${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(chapter11
	${Vulkan_LIBRARIES}
	platform
	sc
)

if(CMAKE_SYSTEM_NAME MATCHES Darwin)
	target_link_libraries(chapter11
		"-framework Metal"
		"-framework QuartzCore"
	)

	target_compile_options(chapter11
	PUBLIC
		-x objective-c++
	)
endif()

