add_executable(chapter12
	main.cpp
)

set_target_properties(chapter12
PROPERTIES
	CXX_STANDARD 17
)

find_package(Vulkan REQUIRED)

target_include_directories(chapter12
PUBLIC
	${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(chapter12
	${Vulkan_LIBRARIES}
	platform
	sc
)

if(CMAKE_SYSTEM_NAME MATCHES Darwin)
	target_link_libraries(chapter12
		"-framework Metal"
		"-framework QuartzCore"
	)

	target_compile_options(chapter12
	PUBLIC
		-x objective-c++
	)
endif()

