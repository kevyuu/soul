set(KHAOS_SOURCES
    main.cpp
    type.cpp
    store.cpp
    streaming_buffer.cpp
    text_completion_backend.cpp
    view.cpp
    ui/chatbot_setting_panel.cpp
    ui/game_setting_panel.cpp
    ui/new_project_popup.cpp
    ui/project_selection_panel.cpp
    ui/dialog_text.cpp
    ui/game_panel.cpp
    ui/menu_bar.cpp
    ui/journey_list_panel.cpp)

find_package(CURL REQUIRED)
add_executable(khaos ${KHAOS_SOURCES})
target_link_libraries(khaos PRIVATE soul_app CURL::libcurl)

target_include_directories(khaos PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
executable_add_dxc_dependencies(khaos)
