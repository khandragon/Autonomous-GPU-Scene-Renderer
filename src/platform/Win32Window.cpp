#include "platform/Win32Window.h"

#include <windowsx.h>

bool Win32Window::Create(const wchar_t *title, uint32_t width, uint32_t height)
{
    m_width = width;
    m_height = height;

    HINSTANCE instance = GetModuleHandleW(nullptr);

    const wchar_t *className = L"AutonomousGpuSceneRendererWindowClass";

    WNDCLASSEXW windowClass = {};
    windowClass.cbSize = sizeof(WNDCLASSEXW);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = Win32Window::WindowProc;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.lpszClassName = className;

    if (!RegisterClassExW(&windowClass))
    {
        return false;
    }

    RECT windowRect = {};
    windowRect.left = 0;
    windowRect.top = 0;
    windowRect.right = static_cast<LONG>(width);
    windowRect.bottom = static_cast<LONG>(height);

    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    const int windowWidth = windowRect.right - windowRect.left;
    const int windowHeight = windowRect.bottom - windowRect.top;

    m_hwnd = CreateWindowExW(
        0,
        className,
        title,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        windowWidth,
        windowHeight,
        nullptr,
        nullptr,
        instance,
        this);

    if (!m_hwnd)
    {
        return false;
    }

    ShowWindow(m_hwnd, SW_SHOW);
    UpdateWindow(m_hwnd);

    return true;
}

void Win32Window::Destroy()
{
    if (m_hwnd && IsWindow(m_hwnd))
    {
        DestroyWindow(m_hwnd);
    }

    m_hwnd = nullptr;
}

void Win32Window::PollEvents()
{
    m_input.MouseDeltaX = 0;
    m_input.MouseDeltaY = 0;

    MSG message = {};

    while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
}

bool Win32Window::IsRunning() const
{
    return !m_shouldClose;
}

bool Win32Window::IsWindowMinimized() const
{
    return m_isMinimized;
}

HWND Win32Window::GetHwnd() const
{
    return m_hwnd;
}

uint32_t Win32Window::GetWidth() const
{
    return m_width;
}

uint32_t Win32Window::GetHeight() const
{
    return m_height;
}

const InputState &Win32Window::GetInput() const
{
    return m_input;
}

LRESULT CALLBACK Win32Window::WindowProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    Win32Window *window = nullptr;

    if (message == WM_NCCREATE)
    {
        auto *createStruct = reinterpret_cast<CREATESTRUCTW *>(lParam);
        window = reinterpret_cast<Win32Window *>(createStruct->lpCreateParams);

        SetWindowLongPtrW(
            hwnd,
            GWLP_USERDATA,
            reinterpret_cast<LONG_PTR>(window));
    }
    else
    {
        window = reinterpret_cast<Win32Window *>(
            GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (window)
    {
        return window->HandleMessage(hwnd, message, wParam, lParam);
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT Win32Window::HandleMessage(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (message)
    {
    case WM_SIZE:
    {
        const uint32_t newWidth = LOWORD(lParam);
        const uint32_t newHeight = HIWORD(lParam);

        m_width = newWidth;
        m_height = newHeight;

        m_isMinimized = (wParam == SIZE_MINIMIZED);

        return 0;
    }

    case WM_CLOSE:
    {
        DestroyWindow(hwnd);
        return 0;
    }

    case WM_DESTROY:
    {
        m_shouldClose = true;
        PostQuitMessage(0);
        return 0;
    }

    case WM_NCDESTROY:
    {
        if (m_hwnd == hwnd)
        {
            m_hwnd = nullptr;
        }

        SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }

    case WM_KEYDOWN:
    {
        const uint32_t key = static_cast<uint32_t>(wParam);

        if (key < 256)
        {
            m_input.Keys[key] = true;
        }

        if (key == VK_ESCAPE)
        {
            DestroyWindow(hwnd);
        }

        return 0;
    }

    case WM_KEYUP:
    {
        const uint32_t key = static_cast<uint32_t>(wParam);

        if (key < 256)
        {
            m_input.Keys[key] = false;
        }

        return 0;
    }

    case WM_MOUSEMOVE:
    {
        const int x = GET_X_LPARAM(lParam);
        const int y = GET_Y_LPARAM(lParam);

        m_input.MouseDeltaX += x - m_input.MouseX;
        m_input.MouseDeltaY += y - m_input.MouseY;

        m_input.MouseX = x;
        m_input.MouseY = y;

        return 0;
    }

    case WM_RBUTTONDOWN:
    {
        m_input.RightMouseDown = true;
        SetCapture(hwnd);
        return 0;
    }

    case WM_RBUTTONUP:
    {
        m_input.RightMouseDown = false;
        ReleaseCapture();
        return 0;
    }
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}