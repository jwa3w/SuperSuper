cmake_minimum_required(VERSION 3.18.1)
project("hypertrack")

add_library(
    hypertrack
    SHARED
    shaders.cpp
    textures.cpp
    track.cpp
    car_mesh.cpp
    physics.cpp
    renderer.cpp
    jni_bridge.cpp
)

target_compile_features(hypertrack PRIVATE cxx_std_17)

find_library(log-lib log)
find_library(gles3-lib GLESv3)
find_library(egl-lib EGL)
find_library(android-lib android)

target_link_libraries(
    hypertrack
    ${gles3-lib}
    ${egl-lib}
    ${android-lib}
    ${log-lib}
    m
)
