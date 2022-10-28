set(SAMPLE_COMMON_SOURCES
	app.cpp
	camera_manipulator.cpp
	imgui_pass.cpp
	imgui_impl_glfw.cpp
	obj_loader.cpp
	ktx_bundle.cpp
	texture_2d_pass.cpp
)

add_library(sample_common "${SAMPLE_COMMON_SOURCES}")
target_include_directories(sample_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(sample_common
	PUBLIC soul imgui
	INTERFACE tiny_obj_loader)