add_executable(test test.cpp)
target_link_libraries(test
	PRIVATE soul glfw ${GLFW_LIBRARIES} tiny_gltf)