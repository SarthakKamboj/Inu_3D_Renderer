cmake_minimum_required(VERSION 3.22.1)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
endif()

project(three_d_renderer)

message(${CMAKE_BUILD_TYPE})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

function(configure_project)

    set(FILES)
    list(APPEND FILES 
        src/main.cpp

        src/windowing/window.cpp

        src/gfx_api/opengl_impl/opengl_gfx.cpp
        src/gfx_api/opengl_impl/opengl_gfx_helper.cpp
        src/gfx_api/opengl_impl/opengl_sys/glew.c

        src/lights/dirlight.cpp
        src/lights/spotlight.cpp
        src/lights/light_probe.cpp

        src/geometry/gltf/gltf.cpp
        src/geometry/image/stb_image.cpp
        src/geometry/model.cpp

        src/utils/general.cpp
        src/utils/inu_math.cpp
        src/utils/mats.cpp
        src/utils/quaternion.cpp
        src/utils/vectors.cpp
        src/utils/inu_time.cpp
        src/utils/transform.cpp

        src/scene/scene.cpp
        src/scene/camera.cpp
        src/scene/online_renderer.cpp

        src/animation/interpolation.cpp
        src/animation/animation.cpp
        src/animation/skin.cpp

        src/render_passes/pbr_render_pass.cpp

        src/renderer/renderer.cpp

        src/editor/pixel_perfect_sel.cpp
    )

    add_executable(${PROJECT_NAME} 
        WIN32
        ${FILES}
    )

    target_include_directories(${PROJECT_NAME} 
        PUBLIC src
        PUBLIC src/gfx_api/opengl_impl/opengl_sys
        PUBLIC src/gfx_api/opengl_impl/opengl_sys/GL

        PUBLIC ext/glm
    )

    target_link_libraries(${PROJECT_NAME}
        opengl32
    )

endfunction()


configure_project()
