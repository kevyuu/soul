cmake_minimum_required(VERSION 3.10)

add_subdirectory(glfw)

add_subdirectory(volk)
target_include_directories(volk
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(SPIRV-Cross)

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

add_library(vk_mem_alloc INTERFACE)
target_include_directories(vk_mem_alloc
	INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/VulkanMemoryAllocator/src)
