add_executable(demo test.cpp imgui_impl_glfw.cpp)
target_link_libraries(demo
	PRIVATE soul glfw ${GLFW_LIBRARIES} tiny_gltf imgui tracy)

add_custom_command(
        TARGET demo POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${PROJECT_SOURCE_DIR}/assets
                ${CMAKE_CURRENT_BINARY_DIR}/assets)

add_custom_command(
    TARGET demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${PROJECT_SOURCE_DIR}/shaders
        ${CMAKE_CURRENT_BINARY_DIR}/shaders)
