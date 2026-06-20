#include "platform/Win32Window.h"

#include <Windows.h>

#include <chrono>
#include <cstdint>

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
    using Clock = std::chrono::steady_clock;
    Clock::time_point m_previousTime;
};

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

    window.Destroy();

    return 0;
}