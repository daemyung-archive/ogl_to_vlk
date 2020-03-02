add_executable(chapter0
	main.cpp
)

set_target_properties(chapter0
PROPERTIES
	CXX_STANDARD 17
)

target_link_libraries(chapter0
	sc
)
