add_executable(demo test.cpp imgui_impl_glfw.cpp ui/menu_bar.cpp  ui/scene_panel.cpp "ui/scene_config_panel.cpp" "ui/metric_panel.cpp"  "final_gather_render_module.cpp" "gbuffer_gen_render_module.cpp" "shadow_map_gen_render_module.cpp"    "render_pipeline/deferred/renderer.cpp" "render_pipeline/deferred/scene.cpp" "utils.cpp"  "imgui_impl_glfw.h")
target_link_libraries(demo
	PRIVATE soul glfw ${GLFW_LIBRARIES} tiny_gltf imgui tracy imgui_filesystem)

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
