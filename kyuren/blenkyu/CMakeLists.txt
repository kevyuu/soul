set(BLENKYU_SOURCES
	"depsgraph.cpp"
	"mesh.cpp"
	"object.cpp"
)

add_library(blenkyu STATIC ${BLENKYU_SOURCES})
target_link_libraries(blenkyu
	PUBLIC blenderlib
	PRIVATE soul tracy)
target_include_directories(blenkyu
    PUBLIC 
		${CMAKE_CURRENT_SOURCE_DIR}/blender_headers)