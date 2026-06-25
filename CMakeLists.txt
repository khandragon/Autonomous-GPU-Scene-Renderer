cmake_minimum_required(VERSION 3.28)

project(AutonomousGpuSceneRenderer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(D3D12_AGILITY_SDK_DIR
    "${CMAKE_SOURCE_DIR}/external/agility-sdk/Microsoft.Direct3D.D3D12.1.619.3"
)

set(D3D12_AGILITY_SDK_INCLUDE_DIR
    "${D3D12_AGILITY_SDK_DIR}/build/native/include"
)

set(D3D12_AGILITY_SDK_BIN_DIR
    "${D3D12_AGILITY_SDK_DIR}/build/native/bin/x64"
)

add_executable(AutonomousGpuSceneRenderer WIN32
    src/main.cpp

    src/platform/Win32Window.h
    src/platform/Win32Window.cpp

    src/renderer/D3D12AgilitySDK.cpp
    src/renderer/D3D12Device.h
    src/renderer/D3D12Device.cpp
    src/renderer/RendererCapabilities.h
    src/renderer/CommandContext.h
    src/renderer/CommandContext.cpp
    src/renderer/DescriptorAllocator.h
    src/renderer/DescriptorAllocator.cpp
    src/renderer/Swapchain.h
    src/renderer/Swapchain.cpp
)

target_include_directories(AutonomousGpuSceneRenderer PRIVATE
    ${D3D12_AGILITY_SDK_INCLUDE_DIR}
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

add_custom_command(TARGET AutonomousGpuSceneRenderer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
    "$<TARGET_FILE_DIR:AutonomousGpuSceneRenderer>/D3D12"
)

add_custom_command(TARGET AutonomousGpuSceneRenderer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${D3D12_AGILITY_SDK_BIN_DIR}/D3D12Core.dll"
    "$<TARGET_FILE_DIR:AutonomousGpuSceneRenderer>/D3D12/D3D12Core.dll"
)

add_custom_command(TARGET AutonomousGpuSceneRenderer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${D3D12_AGILITY_SDK_BIN_DIR}/d3d12SDKLayers.dll"
    "$<TARGET_FILE_DIR:AutonomousGpuSceneRenderer>/D3D12/d3d12SDKLayers.dll"
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
