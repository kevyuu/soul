cmake_minimum_required(VERSION 3.1)
project(soul_lib)

set (PUBLIC_HEADER_DIR src)
file (GLOB_RECURSE SOUL_HEADERS src/core/*.h src/extern/*.h src/gpu/*.h src/job/*.h)
file (GLOB_RECURSE SOUL_SOURCES src/core/*.cpp src/extern/*.cpp src/gpu/intern/*.cpp src/job/*.cpp)

add_library(soul STATIC ${SOUL_HEADERS} ${SOUL_SOURCES})
target_link_libraries(soul 
	PUBLIC glfw ${GLFW_LIBRARIES})
target_include_directories(soul 
	PUBLIC src/ extern/ ${EXTERNAL}/ ${EXTERNAL}/glfw/include}
)
target_compile_definitions(soul
	PUBLIC SOUL_OPTION_VULKAN_ENABLE_VALIDATION SOUL_OS_APPLE
)

add_executable(demo src/main.cpp)
target_link_libraries(demo 
	PUBLIC soul volk)
target_include_directories(demo
	PUBLIC src/shaders/)