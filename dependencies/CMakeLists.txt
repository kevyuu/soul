cmake_minimum_required(VERSION 3.21)

add_subdirectory(dxc)
add_subdirectory(slang)
# Imgui
set(IMGUI_SOURCES
    imgui/imgui.cpp
    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_widgets.cpp
    imgui/imgui_tables.cpp)
add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(imgui
	PRIVATE glfw ${GLFW_LIBRARIES})

# Imgui addons
add_library(imgui_filesystem STATIC imguifilesystem.cpp)
target_include_directories(imgui_filesystem
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(imgui_filesystem
	PRIVATE imgui)

# single header library
add_library(stb_image_write INTERFACE)
target_include_directories(stb_image_write
	INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_library(stb_image INTERFACE)
target_include_directories(stb_image
	INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_library(stb_rect_pack INTERFACE)
target_include_directories(stb_rect_pack
	INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_library(stb_textedit INTERFACE)
target_include_directories(stb_textedit	
	INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_library(stb_truetype INTERFACE)
target_include_directories(stb_truetype
	INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_library(tiny_gltf INTERFACE)
target_include_directories(tiny_gltf
	INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(tiny_gltf
	INTERFACE TINYGLTF_IMPLEMENTATION)

add_library(tiny_obj_loader INTERFACE)
target_include_directories(tiny_obj_loader
	INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(tiny_obj_loader
	INTERFACE TINYOBJLOADER_IMPLEMENTATION)
