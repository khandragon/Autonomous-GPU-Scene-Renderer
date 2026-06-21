#include "platform/Win32Window.h"

#include <Windows.h>

#include <chrono>
#include <cstdint>
#include "renderer/D3D12Device.h"

// FrameTimer is a simple utility class to measure the time between frames.
class FrameTimer
{
public:
    FrameTimer()
    {
        m_previousTime = Clock::now();
    }

    float Tick()
    {
        const auto currentTime = Clock::now();
        const std::chrono::duration<float> delta = currentTime - m_previousTime;
        m_previousTime = currentTime;

        return delta.count();
    }

private:
    // Using steady_clock for frame timing to ensure consistent time intervals.
    using Clock = std::chrono::steady_clock;
    Clock::time_point m_previousTime;
};

// StubRenderer is a placeholder renderer that will be replaced with the actual renderer implementation later. It currently does nothing but can be used to test the application loop and window resizing.
class StubRenderer
{
public:
    void Initialize(HWND hwnd, uint32_t width, uint32_t height)
    {
        m_hwnd = hwnd;
        m_width = width;
        m_height = height;
    }

    void Update(float dt)
    {
        m_time += dt;
    }

    void Render()
    {
    }

    void Resize(uint32_t width, uint32_t height)
    {
        m_width = width;
        m_height = height;
    }

private:
    HWND m_hwnd = nullptr;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    float m_time = 0.0f;
};

int WINAPI wWinMain(
    HINSTANCE instance,
    HINSTANCE previousInstance,
    PWSTR commandLine,
    int commandShow)
{
    (void)instance;
    (void)previousInstance;
    (void)commandLine;
    (void)commandShow;

    Win32Window window;

    if (!window.Create(L"Autonomous GPU Scene Renderer", 1280, 720))
    {
        MessageBoxW(
            nullptr,
            L"Failed to create window.",
            L"Error",
            MB_OK | MB_ICONERROR);

        return -1;
    }

    D3D12Device d3d12Device;

    if (!d3d12Device.Initialize(false))
    {
        MessageBoxW(
            nullptr,
            L"Failed to initialize D3D12 device.",
            L"Error",
            MB_OK | MB_ICONERROR);

        return -1;
    }

    const RendererCapabilities &caps = d3d12Device.GetCapabilities();

    std::wstring capabilityMessage =
        L"Selected GPU: " + d3d12Device.GetAdapterName() + L"\n\n" +
        L"ExecuteIndirect: " + std::wstring(caps.SupportsExecuteIndirect ? L"Yes" : L"No") + L"\n" +
        L"Timestamp Queries: " + std::wstring(caps.SupportsTimestampQueries ? L"Yes" : L"No") + L"\n" +
        L"Mesh Shaders: " + std::wstring(caps.SupportsMeshShaders ? L"Yes" : L"No") + L"\n" +
        L"Work Graphs: " + std::wstring(caps.SupportsWorkGraphs ? L"Yes" : L"No");

    MessageBoxW(
        window.GetHwnd(),
        capabilityMessage.c_str(),
        L"D3D12 Capabilities",
        MB_OK);

    StubRenderer renderer;
    renderer.Initialize(
        window.GetHwnd(),
        window.GetWidth(),
        window.GetHeight());

    FrameTimer timer;

    uint32_t previousWidth = window.GetWidth();
    uint32_t previousHeight = window.GetHeight();

    while (window.IsRunning())
    {
        window.PollEvents();

        const float dt = timer.Tick();

        if (!window.IsWindowMinimized())
        {
            const uint32_t currentWidth = window.GetWidth();
            const uint32_t currentHeight = window.GetHeight();

            if (currentWidth != previousWidth || currentHeight != previousHeight)
            {
                renderer.Resize(currentWidth, currentHeight);

                previousWidth = currentWidth;
                previousHeight = currentHeight;
            }

            renderer.Update(dt);
            renderer.Render();
        }
    }

    d3d12Device.Shutdown();
    window.Destroy();

    return 0;
}