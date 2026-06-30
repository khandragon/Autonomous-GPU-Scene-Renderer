#include "platform/Win32Window.h"
#include "renderer/D3D12Device.h"
#include "renderer/CommandContext.h"
#include "renderer/DescriptorAllocator.h"
#include "renderer/Swapchain.h"
#include "renderer/DepthBuffer.h"
#include "renderer/ResourceBarrier.h"
#include "renderer/ShaderCompiler.h"
#include "renderer/PipelineState.h"

#include <Windows.h>

#include <chrono>
#include <cstdint>
#include <string>

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

    CommandContext commandContext;

    if (!commandContext.Initialize(d3d12Device.GetDevice()))
    {
        MessageBoxW(
            nullptr,
            L"Failed to initialize command context.",
            L"Error",
            MB_OK | MB_ICONERROR);

        return -1;
    }

    DescriptorAllocator rtvAllocator;

    if (!rtvAllocator.Initialize(
            d3d12Device.GetDevice(),
            D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
            CommandContext::FrameCount,
            false,
            L"RTV Descriptor Allocator"))
    {
        MessageBoxW(
            nullptr,
            L"Failed to initialize RTV descriptor allocator.",
            L"Error",
            MB_OK | MB_ICONERROR);

        return -1;
    }

    DescriptorAllocator dsvAllocator;

    if (!dsvAllocator.Initialize(
            d3d12Device.GetDevice(),
            D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
            16,
            false,
            L"DSV Descriptor Allocator"))
    {
        MessageBoxW(
            nullptr,
            L"Failed to initialize DSV descriptor allocator.",
            L"Error",
            MB_OK | MB_ICONERROR);

        return -1;
    }

    DescriptorAllocator cbvSrvUavAllocator;

    if (!cbvSrvUavAllocator.Initialize(
            d3d12Device.GetDevice(),
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
            1024,
            true,
            L"CBV SRV UAV Descriptor Allocator"))
    {
        MessageBoxW(
            nullptr,
            L"Failed to initialize CBV/SRV/UAV descriptor allocator.",
            L"Error",
            MB_OK | MB_ICONERROR);

        return -1;
    }

    DescriptorAllocator samplerAllocator;

    if (!samplerAllocator.Initialize(
            d3d12Device.GetDevice(),
            D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
            64,
            true,
            L"Sampler Descriptor Allocator"))
    {
        MessageBoxW(
            nullptr,
            L"Failed to initialize sampler descriptor allocator.",
            L"Error",
            MB_OK | MB_ICONERROR);

        return -1;
    }

    Swapchain swapchain;

    if (!swapchain.Initialize(
            window.GetHwnd(),
            d3d12Device.GetFactory(),
            d3d12Device.GetDevice(),
            commandContext.GetGraphicsQueue(),
            &rtvAllocator,
            window.GetWidth(),
            window.GetHeight()))
    {
        MessageBoxW(
            nullptr,
            L"Failed to initialize swapchain.",
            L"Error",
            MB_OK | MB_ICONERROR);

        return -1;
    }

    DepthBuffer depthBuffer;

    if (!depthBuffer.Initialize(
            d3d12Device.GetDevice(),
            &dsvAllocator,
            window.GetWidth(),
            window.GetHeight()))
    {
        MessageBoxW(
            nullptr,
            L"Failed to initialize depth buffer.",
            L"Error",
            MB_OK | MB_ICONERROR);

        return -1;
    }

    ShaderCompiler shaderCompiler;

    if (!shaderCompiler.Initialize())
    {
        MessageBoxW(
            nullptr,
            L"Failed to initialize shader compiler.",
            L"Error",
            MB_OK | MB_ICONERROR);

        return -1;
    }

    PipelineState PipelineState;

    if (!PipelineState.Initialize(
            d3d12Device.GetDevice(),
            &shaderCompiler))
    {
        MessageBoxW(
            nullptr,
            L"Failed to initialize graphics pipeline.",
            L"Error",
            MB_OK | MB_ICONERROR);

        return -1;
    }

    const RendererCapabilities &caps = d3d12Device.GetCapabilities();

    wchar_t descriptorMessage[256] = {};
    swprintf_s(
        descriptorMessage,
        L"Descriptors allocated: RTV=%u/%u, DSV=%u/%u, CBV/SRV/UAV=%u/%u\n",
        rtvAllocator.GetAllocatedCount(),
        rtvAllocator.GetCapacity(),
        dsvAllocator.GetAllocatedCount(),
        dsvAllocator.GetCapacity(),
        cbvSrvUavAllocator.GetAllocatedCount(),
        cbvSrvUavAllocator.GetCapacity());

    OutputDebugStringW(descriptorMessage);

    std::wstring capabilityMessage =
        L"Selected GPU: " + d3d12Device.GetAdapterName() + L"\n\n" +
        L"ExecuteIndirect: " + std::wstring(caps.SupportsExecuteIndirect ? L"Yes" : L"No") + L"\n" +
        L"Timestamp Queries: " + std::wstring(caps.SupportsTimestampQueries ? L"Yes" : L"No") + L"\n" +
        L"Mesh Shaders: " + std::wstring(caps.SupportsMeshShaders ? L"Yes" : L"No") + L"\n" +
        L"Work Graphs: " + std::wstring(caps.SupportsWorkGraphs ? L"Yes" : L"No");

    OutputDebugStringW(capabilityMessage.c_str());
    OutputDebugStringW(L"\n");

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

        if (!window.IsRunning())
        {
            break;
        }

        const float dt = timer.Tick();

        if (!window.IsWindowMinimized())
        {
            const uint32_t currentWidth = window.GetWidth();
            const uint32_t currentHeight = window.GetHeight();

            if (currentWidth != previousWidth || currentHeight != previousHeight)
            {
                if (currentWidth == 0 || currentHeight == 0)
                {
                    previousWidth = currentWidth;
                    previousHeight = currentHeight;
                    continue;
                }

                commandContext.Flush();

                if (!swapchain.Resize(
                        d3d12Device.GetDevice(),
                        currentWidth,
                        currentHeight))
                {
                    MessageBoxW(
                        nullptr,
                        L"Failed to resize swapchain.",
                        L"Error",
                        MB_OK | MB_ICONERROR);

                    return -1;
                }

                if (!depthBuffer.Resize(
                        d3d12Device.GetDevice(),
                        currentWidth,
                        currentHeight))
                {
                    MessageBoxW(
                        nullptr,
                        L"Failed to resize depth buffer.",
                        L"Error",
                        MB_OK | MB_ICONERROR);

                    return -1;
                }

                renderer.Resize(currentWidth, currentHeight);

                previousWidth = currentWidth;
                previousHeight = currentHeight;
            }

            renderer.Update(dt);

            commandContext.BeginFrame();

            ID3D12GraphicsCommandList *commandList =
                commandContext.GetCommandList();

            commandList->SetGraphicsRootSignature(
                PipelineState.GetRootSignature());

            commandList->SetPipelineState(
                PipelineState.GetPipelineState());

            ID3D12Resource *backBuffer =
                swapchain.GetCurrentBackBuffer();

            if (!commandList || !backBuffer)
            {
                MessageBoxW(
                    nullptr,
                    L"Invalid command list or back buffer.",
                    L"Error",
                    MB_OK | MB_ICONERROR);

                return -1;
            }

            D3D12_VIEWPORT viewport = swapchain.GetViewport();
            D3D12_RECT scissorRect = swapchain.GetScissorRect();

            commandList->RSSetViewports(1, &viewport);
            commandList->RSSetScissorRects(1, &scissorRect);

            D3D12_RESOURCE_BARRIER presentToRenderTarget =
                TransitionBarrier(
                    backBuffer,
                    D3D12_RESOURCE_STATE_PRESENT,
                    D3D12_RESOURCE_STATE_RENDER_TARGET);

            commandList->ResourceBarrier(1, &presentToRenderTarget);

            D3D12_CPU_DESCRIPTOR_HANDLE rtv =
                swapchain.GetCurrentBackBufferRtv();

            D3D12_CPU_DESCRIPTOR_HANDLE dsv =
                depthBuffer.GetDsv();

            commandList->OMSetRenderTargets(
                1,
                &rtv,
                FALSE,
                &dsv);

            const float clearColor[] =
                {
                    0.05f,
                    0.08f,
                    0.12f,
                    1.0f};

            commandList->ClearRenderTargetView(
                rtv,
                clearColor,
                0,
                nullptr);

            commandList->ClearDepthStencilView(
                dsv,
                D3D12_CLEAR_FLAG_DEPTH,
                DepthBuffer::ClearDepth,
                0,
                0,
                nullptr);

            D3D12_RESOURCE_BARRIER renderTargetToPresent =
                TransitionBarrier(
                    backBuffer,
                    D3D12_RESOURCE_STATE_RENDER_TARGET,
                    D3D12_RESOURCE_STATE_PRESENT);

            commandList->ResourceBarrier(1, &renderTargetToPresent);

            commandContext.ExecuteCommandList();

            swapchain.Present(true);

            commandContext.EndFrame();
        }
    }

    commandContext.Flush();

    PipelineState.Shutdown();
    shaderCompiler.Shutdown();

    depthBuffer.Shutdown();
    swapchain.Shutdown();

    samplerAllocator.Shutdown();
    cbvSrvUavAllocator.Shutdown();
    dsvAllocator.Shutdown();
    rtvAllocator.Shutdown();

    commandContext.Shutdown();
    d3d12Device.Shutdown();
    window.Destroy();

    return 0;
}