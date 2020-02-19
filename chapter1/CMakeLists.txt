add_executable(chapter1
	main.cpp
)

set_target_properties(chapter1
PROPERTIES
	CXX_STANDARD 17
)

find_package(Vulkan REQUIRED)

target_include_directories(chapter1
PUBLIC
	${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(chapter1
	${Vulkan_LIBRARIES}
	platform
)

if(CMAKE_SYSTEM_NAME MATCHES Darwin)
	target_link_libraries(chapter1
		"-framework Metal"
		"-framework QuartzCore"
	)

	target_compile_options(chapter1
	PUBLIC
		-x objective-c++
	)
endif()

