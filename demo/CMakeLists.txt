add_executable(demo test.cpp imgui_render_module.h)
target_link_libraries(demo
	PRIVATE soul glfw ${GLFW_LIBRARIES} tiny_gltf imgui tracy)