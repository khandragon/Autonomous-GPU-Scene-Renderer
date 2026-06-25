#include "renderer/Swapchain.h"

#include <stdexcept>

//
namespace
{
    void ThrowIfFailed(HRESULT hr)
    {
        if (FAILED(hr))
        {
            throw std::runtime_error("HRESULT failed.");
        }
    }
}

//
bool Swapchain::Initialize(
    HWND hwnd,
    IDXGIFactory6 *factory,
    ID3D12Device *device,
    ID3D12CommandQueue *graphicsQueue,
    DescriptorAllocator *rtvAllocator,
    uint32_t width,
    uint32_t height)
{
    if (!hwnd || !factory || !device || !graphicsQueue || !rtvAllocator)
    {
        return false;
    }

    if (width == 0 || height == 0)
    {
        return false;
    }

    m_width = width;
    m_height = height;
    m_rtvAllocator = rtvAllocator;

    try
    {
        if (!CreateSwapchain(hwnd, factory, graphicsQueue, width, height))
        {
            return false;
        }

        if (!AllocateRtvHandles())
        {
            return false;
        }

        if (!CreateRenderTargetViews(device))
        {
            return false;
        }
    }
    catch (...)
    {
        Shutdown();
        return false;
    }

    return true;
}

//
void Swapchain::Shutdown()
{
    ReleaseBackBuffers();

    m_swapchain.Reset();
    m_rtvAllocator = nullptr;

    for (auto &handle : m_rtvHandles)
    {
        handle = {};
    }

    m_width = 0;
    m_height = 0;
}

// When the application window is resized.
bool Swapchain::Resize(
    ID3D12Device *device,
    uint32_t width,
    uint32_t height)
{
    if (!device || !m_swapchain)
    {
        return false;
    }

    if (width == 0 || height == 0)
    {
        return false;
    }

    m_width = width;
    m_height = height;

    ReleaseBackBuffers();

    HRESULT hr = m_swapchain->ResizeBuffers(
        CommandContext::FrameCount,
        width,
        height,
        BackBufferFormat,
        0);

    if (FAILED(hr))
    {
        return false;
    }

    return CreateRenderTargetViews(device);
}

//
void Swapchain::Present(bool enableVSync)
{
    const UINT syncInterval = enableVSync ? 1u : 0u;
    const UINT presentFlags = 0;

    //
    ThrowIfFailed(m_swapchain->Present(syncInterval, presentFlags));
}

//
ID3D12Resource *Swapchain::GetCurrentBackBuffer() const
{
    return m_backBuffers[GetCurrentBackBufferIndex()].Get();
}

//
D3D12_CPU_DESCRIPTOR_HANDLE Swapchain::GetCurrentBackBufferRtv() const
{
    return m_rtvHandles[GetCurrentBackBufferIndex()].Cpu;
}

//
uint32_t Swapchain::GetCurrentBackBufferIndex() const
{
    return m_swapchain->GetCurrentBackBufferIndex();
}

//
uint32_t Swapchain::GetWidth() const
{
    return m_width;
}

//
uint32_t Swapchain::GetHeight() const
{
    return m_height;
}

// Creates a swapchain for your Win32 HWND; for D3D12, the first parameter is the direct command queue
bool Swapchain::CreateSwapchain(
    HWND hwnd,
    IDXGIFactory6 *factory,
    ID3D12CommandQueue *graphicsQueue,
    uint32_t width,
    uint32_t height)
{
    DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
    swapchainDesc.Width = width;
    swapchainDesc.Height = height;
    swapchainDesc.Format = BackBufferFormat;
    swapchainDesc.Stereo = FALSE;
    swapchainDesc.SampleDesc.Count = 1;
    swapchainDesc.SampleDesc.Quality = 0;
    swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchainDesc.BufferCount = CommandContext::FrameCount;
    swapchainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapchainDesc.Flags = 0;

    Microsoft::WRL::ComPtr<IDXGISwapChain1> swapchain1;

    ThrowIfFailed(factory->CreateSwapChainForHwnd(
        graphicsQueue,
        hwnd,
        &swapchainDesc,
        nullptr,
        nullptr,
        &swapchain1));

    ThrowIfFailed(swapchain1.As(&m_swapchain));

    // Disable Alt+Enter fullscreen handling from DXGI.
    // We will control fullscreen/window behavior ourselves later if needed.
    factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);

    return true;
}

bool Swapchain::AllocateRtvHandles()
{
    if (!m_rtvAllocator)
    {
        return false;
    }

    try
    {
        for (uint32_t i = 0; i < CommandContext::FrameCount; ++i)
        {
            m_rtvHandles[i] = m_rtvAllocator->Allocate();
        }
    }
    catch (...)
    {
        return false;
    }

    return true;
}

// writes an RTV descriptor into a CPU descriptor handle.
bool Swapchain::CreateRenderTargetViews(ID3D12Device *device)
{
    for (uint32_t i = 0; i < CommandContext::FrameCount; ++i)
    {
        ThrowIfFailed(m_swapchain->GetBuffer(
            i,
            IID_PPV_ARGS(&m_backBuffers[i])));

        wchar_t name[128] = {};
        swprintf_s(name, L"Swapchain Back Buffer %u", i);
        m_backBuffers[i]->SetName(name);

        device->CreateRenderTargetView(
            m_backBuffers[i].Get(),
            nullptr,
            m_rtvHandles[i].Cpu);
    }

    return true;
}

//
void Swapchain::ReleaseBackBuffers()
{
    for (auto &backBuffer : m_backBuffers)
    {
        backBuffer.Reset();
    }
}

D3D12_VIEWPORT Swapchain::GetViewport() const
{
    D3D12_VIEWPORT viewport = {};
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.Width = static_cast<float>(m_width);
    viewport.Height = static_cast<float>(m_height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    return viewport;
}

D3D12_RECT Swapchain::GetScissorRect() const
{
    D3D12_RECT rect = {};
    rect.left = 0;
    rect.top = 0;
    rect.right = static_cast<LONG>(m_width);
    rect.bottom = static_cast<LONG>(m_height);
    return rect;
}