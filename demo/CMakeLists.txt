set (DEMO_SOURCES

	test.cpp
	imgui_impl_glfw.cpp 
	shader_generator.cpp
	camera_manipulator.cpp
	utils.cpp
	ktx_bundle.cpp
	
	ui/widget.cpp
	ui/menu_bar.cpp  
	ui/scene_panel.cpp 
	 
	ui/metric_panel.cpp  
	
	render_pipeline/filament/exposure.cpp
	render_pipeline/filament/gpu_program_registry.cpp
	render_pipeline/filament/render_module/shadow_map.cpp
	render_pipeline/filament/render_module/lighting_pass.cpp
	render_pipeline/filament/render_module/structure_pass.cpp
	render_pipeline/filament/render_module/depth_mipmap.cpp
	render_pipeline/filament/render_module/draw_item.cpp
	render_pipeline/filament/renderer.cpp
	render_pipeline/filament/scene.cpp
	
)

add_executable(demo ${DEMO_SOURCES} )

target_link_libraries(demo
	INTERFACE argparse
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
