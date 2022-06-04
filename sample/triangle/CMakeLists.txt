set(SHADER_SOURCES
	shaders/triangle.slang
)

set(SHADER_TARGET_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders)
message(${SHADER_TARGET_DIR})
file(MAKE_DIRECTORY ${SHADER_TARGET_DIR})
set(SHADER_TARGET_SOURCES)
foreach (shader_src ${SHADER_SOURCES})
    message(${shader_src})
    get_filename_component(localname "${shader_src}" NAME)
    set(output_path "${SHADER_TARGET_DIR}/${localname}")
    add_custom_command(
        OUTPUT ${output_path}
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/${shader_src} ${SHADER_TARGET_DIR}
        MAIN_DEPENDENCY ${shader_src}
        COMMENT "Copying shader file"
    )
    list(APPEND SHADER_TARGET_SOURCES ${output_path})
endforeach()

add_custom_target(
    shaders
    DEPENDS ${SHADER_TARGET_SOURCES}
)

add_executable(triangle main.cpp)
target_link_libraries(triangle 
	INTERFACE argparse
	PRIVATE sample_common)
add_dependencies(triangle shaders)