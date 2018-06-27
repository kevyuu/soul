cmake_minimum_required(VERSION 3.10)
project(soul)

set(CMAKE_CXX_STANDARD 11)

include_directories(dependencies)
include_directories(dependencies/glew-2.1.0)
include_directories(dependencies/glew-2.1.0/include)
include_directories(dependencies/glew-2.1.0/include/GL)
include_directories(dependencies/GLFW)
include_directories(dependencies/GLFW/include)
include_directories(dependencies/GLFW/include/GLFW)
include_directories(dependencies/KHR)
include_directories(soul/src)
include_directories(soul/src/render)

find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIRS})
find_package(PkgConfig REQUIRED)
add_subdirectory(dependencies/glfw-3.2.1)

link_libraries(${OPENGL_LIBRARIES} glfw)


add_executable(soul
        dependencies/glew-2.1.0/include/GL/eglew.h
        dependencies/glew-2.1.0/include/GL/glew.h
        dependencies/glew-2.1.0/include/GL/glxew.h
        dependencies/glew-2.1.0/include/GL/wglew.h
        dependencies/GLFW/include/GLFW/glfw3.h
        dependencies/GLFW/include/GLFW/glfw3native.h
        dependencies/KHR/khrplatform.h

        soul/src/extern/imgui.h
        soul/src/extern/imgui_internal.h
        soul/src/extern/stb_rect_pack.h
        soul/src/extern/stb_textedit.h
        soul/src/extern/stb_truetype.h
        soul/src/extern/imgui_impl_glfw.h
        soul/src/extern/imgui_impl_opengl3.h
        soul/src/extern/imconfig.h
        soul/src/extern/imgui.cpp
        soul/src/extern/imgui_draw.cpp
        soul/src/extern/imgui_impl_glfw.cpp
        soul/src/extern/imgui_impl_opengl3.cpp

        soul/src/render/intern/pbr_scene_rp.cpp
        soul/src/render/intern/shadow_map_rp.cpp
        soul/src/render/intern/shadow_map_view_rp.cpp
        soul/src/render/intern/skybox_rp.cpp
        soul/src/render/intern/panorama_to_cubemap_rp.cpp
        soul/src/render/intern/diffuse_envmap_filter_rp.cpp
        soul/src/render/intern/specular_envmap_filter.cpp
        soul/src/render/intern/brdf_map_rp.cpp
        soul/src/render/intern/ssr_trace_rp.cpp
        soul/src/render/intern/ssr_resolve_rp.cpp
        soul/src/render/intern/gaussian_blur_rp.cpp
        soul/src/render/intern/system.cpp
        soul/src/render/system.h
        soul/src/render/type.h
        soul/src/render/intern/util.h
        soul/src/render/intern/asset.h

        soul/src/extern/glad.c
        soul/src/extern/glad.h
        soul/src/main.cpp
        soul/src/soul_array.h
        soul/src/soul_core.cpp
        soul/src/soul_core.h
        soul/src/core/math.cpp
        soul/src/core/math.h
        soul/src/stb_image.h
        soul/src/core/type.h
        soul/src/render/intern/specular_envmap_filter.cpp
        soul/src/render/intern/brdf_map_rp.cpp
        soul/src/render/intern/lighting_rp.cpp soul/src/render/intern/gbuffer_gen_rp.cpp)

