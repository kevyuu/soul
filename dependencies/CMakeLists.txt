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
add_library(imgui INTERFACE)
target_sources(imgui INTERFACE $<BUILD_INTERFACE:${IMGUI_SOURCES}>)
target_include_directories(imgui INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} imgui)
target_compile_definitions(imgui INTERFACE IMGUI_DEFINE_MATH_OPERATORS)

set(IMGUI_NODE_EDITOR_SOURCES
    imgui-node-editor/crude_json.cpp
    imgui-node-editor/imgui_canvas.cpp
    imgui-node-editor/imgui_node_editor.cpp
    imgui-node-editor/imgui_node_editor_api.cpp)
add_library(imgui_node_editor INTERFACE)
target_sources(imgui_node_editor INTERFACE $<BUILD_INTERFACE:${IMGUI_NODE_EDITOR_SOURCES}>)
target_include_directories(imgui_node_editor INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# Imgui addons
add_library(imgui_filesystem STATIC imguifilesystem.cpp)
target_include_directories(imgui_filesystem PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(imgui_filesystem PRIVATE imgui)

add_library(imguizmo STATIC imguizmo/ImGuizmo.cpp)
target_include_directories(imguizmo PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(imguizmo PRIVATE imgui)

add_library(mikktspace STATIC mikktspace/mikktspace.cpp)
target_include_directories(mikktspace PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# single header library
add_library(stb_image_write INTERFACE)
target_include_directories(stb_image_write INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_library(stb_image INTERFACE)
target_include_directories(stb_image INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_library(stb_rect_pack INTERFACE)
target_include_directories(stb_rect_pack INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_library(stb_textedit INTERFACE)
target_include_directories(stb_textedit INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_library(stb_truetype INTERFACE)
target_include_directories(stb_truetype INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_library(cgltf INTERFACE)
target_include_directories(cgltf INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(cgltf INTERFACE _CRT_SECURE_NO_WARNING)

add_library(tiny_gltf INTERFACE)
target_include_directories(tiny_gltf INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(tiny_gltf INTERFACE TINYGLTF_IMPLEMENTATION)

add_library(tiny_obj_loader INTERFACE)
target_include_directories(tiny_obj_loader INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(tiny_obj_loader INTERFACE TINYOBJLOADER_IMPLEMENTATION)
