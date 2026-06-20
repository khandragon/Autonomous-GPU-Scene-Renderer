#pragma once

#include <Windows.h>

#include <cstdint>

struct InputState
{
    bool Keys[256] = {};
    bool RightMouseDown = false;

    int MouseX = 0;
    int MouseY = 0;
    int MouseDeltaX = 0;
    int MouseDeltaY = 0;
};

class Win32Window
{
public:
    bool Create(const wchar_t *title, uint32_t width, uint32_t height);
    void Destroy();

    void PollEvents();

    bool IsRunning() const;
    bool IsWindowMinimized() const;

    HWND GetHwnd() const;
    uint32_t GetWidth() const;
    uint32_t GetHeight() const;

    const InputState &GetInput() const;

private:
    static LRESULT CALLBACK WindowProc(
        HWND hwnd,
        UINT message,
        WPARAM wParam,
        LPARAM lParam);

    LRESULT HandleMessage(
        HWND hwnd,
        UINT message,
        WPARAM wParam,
        LPARAM lParam);

private:
    HWND m_hwnd = nullptr;

    uint32_t m_width = 0;
    uint32_t m_height = 0;

    bool m_shouldClose = false;
    bool m_isMinimized = false;

    InputState m_input = {};
};