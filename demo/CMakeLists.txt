set (DEMO_SOURCES

	test.cpp
	imgui_impl_glfw.cpp 
	shader_generator.cpp
	utils.cpp
	
	ui/menu_bar.cpp  
	ui/scene_panel.cpp 
	ui/scene_config_panel.cpp 
	ui/metric_panel.cpp  
	
	final_gather_render_module.cpp
	gbuffer_gen_render_module.cpp
	shadow_map_gen_render_module.cpp    

	render_pipeline/deferred/renderer.cpp
	render_pipeline/deferred/scene.cpp
	render_pipeline/filament/scene.cpp
	render_pipeline/filament/renderer.cpp
	render_pipeline/filament/gpu_program_registry.cpp)

add_executable(demo ${DEMO_SOURCES})

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
