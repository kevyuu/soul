cmake_minimum_required(VERSION 3.10)

set(CMAKE_MACOSX_RPATH 1)

add_subdirectory(glfw)

add_subdirectory(volk)
target_include_directories(volk
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(glslang)
add_subdirectory(SPIRV-Headers)
add_subdirectory(SPIRV-Tools)
add_subdirectory(SPIRV-Cross)

set(SHADERC_SKIP_TESTS ON)
set(SHADERC_SKIP_INSTALL ON)
add_subdirectory(shaderc)

# Tracy
set (TRACY_SOURCES
	tracy/TracyClient.cpp)

add_library(tracy STATIC ${TRACY_SOURCES})
target_include_directories(tracy
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(tracy
	PUBLIC TRACY_ENABLE NOMINMAX)

# Imgui
set(IMGUI_SOURCES
	imgui/imgui.cpp
	imgui/imgui_demo.cpp
	imgui/imgui_draw.cpp
	imgui/imgui_widgets.cpp
	imgui/imgui_tables.cpp
)
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

add_library(vk_mem_alloc INTERFACE)
target_include_directories(vk_mem_alloc
	INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/VulkanMemoryAllocator/src ${Vulkan_INCLUDE_DIR})
