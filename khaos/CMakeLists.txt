set(KHAOS_SOURCES
    main.cpp
    type.cpp
    store.cpp
    view.cpp
    panels/chatbot_setting_panel.cpp
    panels/game_setting_panel.cpp
    panels/new_project_popup.cpp
    panels/project_selection_panel.cpp
    panels/game_panel.cpp
    panels/menu_bar.cpp
    panels/journey_list_panel.cpp
    services/text_completion_api.cpp)

add_executable(khaos ${KHAOS_SOURCES})
target_link_libraries(khaos PRIVATE soul_app)

target_include_directories(khaos PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
executable_add_dxc_dependencies(khaos)
