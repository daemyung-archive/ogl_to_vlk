add_executable(chapter4
	main.cpp
)

set_target_properties(chapter4
PROPERTIES
	CXX_STANDARD 17
)

target_link_libraries(chapter4
	sc
)
