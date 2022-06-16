set(SHADER_SOURCES
	shaders/triangle.slang
)

create_shaders_target(shaders ${SHADER_SOURCES})

add_executable(triangle main.cpp)
target_link_libraries(triangle 
	PRIVATE sample_common)
add_dependencies(triangle shaders)
executable_add_slang_dependencies(triangle)