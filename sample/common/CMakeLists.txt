set(SAMPLE_COMMON_SOURCES
	app.cpp
)

add_library(sample_common ${SAMPLE_COMMON_SOURCES})
target_include_directories(sample_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(sample_common
	PUBLIC soul)