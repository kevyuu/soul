set(KHAOS_HEADERS
    streaming_buffer.h
    textgen_backend.h
    textgen_system.h
    type.h
    view.h)

set(KHAOS_SOURCES
    main.cpp
    type.cpp
    streaming_buffer.cpp
    textgen_backend.cpp
    textgen_system.cpp
    hf_tokenizer.cpp
    script_system.cpp
    view.cpp
    store/store.cpp
    modules/textgen.cpp
    ui/app_setting_view.cpp
    ui/chatbot_setting_view.cpp
    ui/chat_view.cpp
    ui/sampler_presets_view.cpp
    ui/new_project_popup.cpp
    ui/popup_new_item.cpp
    ui/project_selection_panel.cpp
    ui/dialog_text.cpp
    ui/game_view.cpp
    ui/menu_bar.cpp
    ui/journey_list_panel.cpp)

find_package(CURL REQUIRED)
# find_package(unofficial-luau CONFIG REQUIRED)

add_executable(khaos ${KHAOS_HEADERS} ${KHAOS_SOURCES})
# target_link_libraries( khaos PRIVATE soul_app CURL::libcurl unofficial::luau::Luau.Compiler
# unofficial::luau::Luau.VM unofficial::luau::Luau.Ast unofficial::luau::Luau.EqSat
# unofficial::luau::Luau.Common)
target_link_libraries(
  khaos
  PRIVATE soul_app
          CURL::libcurl
          luajit
          tokenizers_cpp)
target_include_directories(khaos PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/tokenizers_data DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

executable_add_dxc_dependencies(khaos)
executable_add_luajit_dependencies(khaos)
