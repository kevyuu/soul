cmake_minimum_required(VERSION 3.21)

function(create_shaders_target shader_target_name shader_sources)
  set(SHADER_TARGET_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders)
  file(MAKE_DIRECTORY ${SHADER_TARGET_DIR})
  set(SHADER_TARGET_SOURCES)
  foreach(shader_src ${shader_sources})
    get_filename_component(localname "${shader_src}" NAME)
    set(output_path "${SHADER_TARGET_DIR}/${localname}")
    add_custom_command(
      OUTPUT ${output_path}
      COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/${shader_src}
              ${SHADER_TARGET_DIR}
      MAIN_DEPENDENCY ${shader_src}
      COMMENT "Copying shader file")
    list(APPEND SHADER_TARGET_SOURCES ${output_path})
  endforeach()
  add_custom_target(${shader_target_name} DEPENDS ${SHADER_TARGET_SOURCES})
endfunction()

message(STATUS "COPY ${CMAKE_CURRENT_SOURCE_DIR}/media  to  ${CMAKE_CURRENT_BINARY_DIR}")
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/media DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(common)
add_subdirectory(triangle)
add_subdirectory(index_buffer)
add_subdirectory(storage_buffer)
add_subdirectory(buffer_transfer_command)
add_subdirectory(multithread_raster_sample)
add_subdirectory(msaa_sample)
add_subdirectory(texture_2d)
add_subdirectory(texture_3d)
add_subdirectory(texture_cube)
add_subdirectory(texture_transfer_command)
add_subdirectory(imgui)
add_subdirectory(compute_shader)
add_subdirectory(rt_basic)
