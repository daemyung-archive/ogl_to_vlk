add_executable(chapter02
	main.cpp
)

set_target_properties(chapter02
PROPERTIES
	CXX_STANDARD 17
)

target_link_libraries(chapter02
	sc
)
