add_executable(chapter5
	main.cpp
)

set_target_properties(chapter5
PROPERTIES
	CXX_STANDARD 17
)

target_link_libraries(chapter5
	sc
)
