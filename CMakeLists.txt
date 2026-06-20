cmake_minimum_required(VERSION 3.28)

project(AutonomousGpuSceneRenderer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(AutonomousGpuSceneRenderer WIN32
    src/main.cpp

    src/platform/Win32Window.h
    src/platform/Win32Window.cpp

    src/renderer/D3D12Device.h
    src/renderer/D3D12Device.cpp
)

target_include_directories(AutonomousGpuSceneRenderer PRIVATE
    src
)

target_compile_definitions(AutonomousGpuSceneRenderer PRIVATE
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

target_link_libraries(AutonomousGpuSceneRenderer PRIVATE
    user32
    d3d12
    dxgi
    dxguid
)

if(MSVC)
    target_compile_options(AutonomousGpuSceneRenderer PRIVATE
        /W4
        /permissive-
        /Zc:preprocessor
    )
else()
    target_compile_options(AutonomousGpuSceneRenderer PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()
