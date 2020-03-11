add_executable(chapter10
	main.cpp
)

set_target_properties(chapter10
PROPERTIES
	CXX_STANDARD 17
)

find_package(Vulkan REQUIRED)

target_include_directories(chapter10
PUBLIC
	${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(chapter10
	${Vulkan_LIBRARIES}
	platform
	sc
)

if(CMAKE_SYSTEM_NAME MATCHES Darwin)
	target_link_libraries(chapter10
		"-framework Metal"
		"-framework QuartzCore"
	)

	target_compile_options(chapter10
	PUBLIC
		-x objective-c++
	)
endif()

