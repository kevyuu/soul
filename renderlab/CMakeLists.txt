set(RENDERLAB_SOURCES
    main.cpp
    scene.cpp
    camera_controller.cpp
    mesh_preprocessor.cpp
    render_pipeline.cpp
    render_nodes/ddgi/ddgi_node.cpp
    render_nodes/rtao/rtao_node.cpp
    render_nodes/rt_reflection/rt_reflection_node.cpp
    render_nodes/deferred_shading/deferred_shading_node.cpp
    render_nodes/gbuffer_generate/gbuffer_generate_node.cpp
    render_nodes/shadow/shadow_node.cpp
    render_nodes/taa/taa_node.cpp
    render_nodes/tone_map/tone_map_node.cpp
    importer/gltf_importer.cpp
    demo/sponza_demo.cpp
    demo/pica_pica_demo.cpp
    editor/store.cpp
    editor/view.cpp
    editor/panels/comfy_graph_editor_panel.cpp
    editor/panels/menu_bar.cpp
    editor/panels/entity_property_panel.cpp
    editor/panels/render_pipeline_panel.cpp
    editor/panels/scene_hierarchy_panel.cpp
    editor/panels/scene_setting_panel.cpp
    editor/panels/viewport_panel.cpp)

add_executable(renderlab ${RENDERLAB_SOURCES})
target_link_libraries(renderlab PRIVATE soul_app cgltf mikktspace)

target_include_directories(renderlab PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
executable_add_dxc_dependencies(renderlab)

function(create_shaders_target shader_target_name shader_sources)
  set(SHADER_TARGET_BASE_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders)
  file(MAKE_DIRECTORY ${SHADER_TARGET_BASE_DIR})
  set(SHADER_TARGET_SOURCES)
  foreach(shader_src ${shader_sources})
    get_filename_component(directory "${shader_src}" DIRECTORY)
    set(shader_target_dir "${SHADER_TARGET_BASE_DIR}/${directory}")
    set(output_path "${SHADER_TARGET_BASE_DIR}/${shader_src}")
    file(MAKE_DIRECTORY ${shader_target_dir})
    message(COPY_SHADER_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/${shader_src})
    message(COPY_TARGET_DIR=${shader_target_dir})
    add_custom_command(
      OUTPUT ${output_path}
      COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/${shader_src}
              ${shader_target_dir}
      MAIN_DEPENDENCY ${shader_src}
      COMMENT "Copying shader file")
    list(APPEND SHADER_TARGET_SOURCES ${output_path})
  endforeach()
  message(SHADER_TARGET_SOURCES=${SHADER_TARGET_SOURCES})
  add_custom_target(${shader_target_name} DEPENDS ${SHADER_TARGET_SOURCES})
endfunction()

set(RENDERLAB_SHADERS
    env_map.hlsl
    scene.hlsl
    type.shared.hlsl
    render_pipeline.shared.hlsl
    render_pipeline_main.hlsl
    math/matrix.hlsl
    utils/bnd_sampler.hlsl
    utils/constant.hlsl
    utils/math.hlsl
    utils/reprojection.hlsl
    utils/color_helpers.hlsl
    utils/sampling/sample_generator.hlsl
    render_nodes/common/brdf.hlsl
    render_nodes/common/edge_stopping.hlsl
    render_nodes/common/lighting.hlsl
    render_nodes/common/ray_query.hlsl
    render_nodes/common/sv.hlsl
    render_nodes/common/color.hlsl
    render_nodes/common/misc.hlsl
    render_nodes/ddgi/ddgi.shared.hlsl
    render_nodes/ddgi/probe_update_main.hlsl
    render_nodes/ddgi/probe_border_update_main.hlsl
    render_nodes/ddgi/probe_overlay_main.hlsl
    render_nodes/ddgi/ray_overlay_main.hlsl
    render_nodes/ddgi/sample_irradiance_main.hlsl
    render_nodes/ddgi/ray_trace_main.hlsl
    render_nodes/rtao/rtao.shared.hlsl
    render_nodes/rtao/bilateral_blur_main.hlsl
    render_nodes/rtao/init_dispatch_args_main.hlsl
    render_nodes/rtao/ray_query_main.hlsl
    render_nodes/rtao/temporal_accumulation_main.hlsl
    render_nodes/rt_reflection/rt_reflection.shared.hlsl
    render_nodes/rt_reflection/ray_trace_main.hlsl
    render_nodes/rt_reflection/init_dispatch_args_main.hlsl
    render_nodes/rt_reflection/temporal_accumulation_main.hlsl
    render_nodes/rt_reflection/copy_tile_main.hlsl
    render_nodes/rt_reflection/filter_tile_main.hlsl
    render_nodes/deferred_shading/deferred_shading.shared.hlsl
    render_nodes/deferred_shading/deferred_shading_main.hlsl
    render_nodes/gbuffer_generate/gbuffer_generate_type.hlsl
    render_nodes/gbuffer_generate/gbuffer_generate_main.hlsl
    render_nodes/shadow/shadow_type.hlsl
    render_nodes/shadow/copy_tile_main.hlsl
    render_nodes/shadow/filter_tile_main.hlsl
    render_nodes/shadow/init_dispatch_args_main.hlsl
    render_nodes/shadow/temporal_denoise_main.hlsl
    render_nodes/shadow/shadow_main.hlsl
    render_nodes/taa/taa.shared.hlsl
    render_nodes/taa/taa_main.hlsl
    render_nodes/tone_map/tone_map.shared.hlsl
    render_nodes/tone_map/tone_map_main.hlsl)
create_shaders_target(renderlab_shaders "${RENDERLAB_SHADERS}")

add_dependencies(renderlab renderlab_shaders)

# Scan through resource folder for updated files and copy if none existing or changed
file(GLOB_RECURSE resources "resources/*.*")
foreach(resource ${resources})
  get_filename_component(filename ${resource} NAME)
  get_filename_component(dir ${resource} DIRECTORY)
  get_filename_component(dirname ${dir} NAME)

  set(output "")

  while(
    NOT
    ${dirname}
    STREQUAL
    resources)
    get_filename_component(path_component ${dir} NAME)
    set(output "${path_component}/${output}")
    get_filename_component(dir ${dir} DIRECTORY)
    get_filename_component(dirname ${dir} NAME)
  endwhile()

  set(output "${CMAKE_CURRENT_BINARY_DIR}/resources/${output}/${filename}")

  add_custom_command(
    COMMENT "Moving updated resource-file '${filename}'"
    OUTPUT ${output}
    DEPENDS ${resource}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${resource} ${output})
  add_custom_target(${filename} ALL DEPENDS ${resource} ${output})

endforeach()
