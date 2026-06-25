#pragma once

#include "renderer/CommandContext.h"

#include <Windows.h>
#include <wrl/client.h>

#include <d3d12.h>
#include <dxgi1_6.h>

#include <array>
#include <cstdint>

class Swapchain
{
public:
    static constexpr DXGI_FORMAT BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

public:
    bool Initialize(
        HWND hwnd,
        IDXGIFactory6 *factory,
        ID3D12Device *device,
        ID3D12CommandQueue *graphicsQueue,
        uint32_t width,
        uint32_t height);

    void Shutdown();

    bool Resize(
        ID3D12Device *device,
        uint32_t width,
        uint32_t height);

    void Present(bool enableVSync);

    ID3D12Resource *GetCurrentBackBuffer() const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferRtv() const;

    uint32_t GetCurrentBackBufferIndex() const;
    uint32_t GetWidth() const;
    uint32_t GetHeight() const;

private:
    bool CreateSwapchain(
        HWND hwnd,
        IDXGIFactory6 *factory,
        ID3D12CommandQueue *graphicsQueue,
        uint32_t width,
        uint32_t height);

    bool CreateRtvHeap(ID3D12Device *device);
    bool CreateRenderTargetViews(ID3D12Device *device);

    void ReleaseBackBuffers();

private:
    Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swapchain;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap;

    std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, CommandContext::FrameCount> m_backBuffers = {};
    std::array<D3D12_CPU_DESCRIPTOR_HANDLE, CommandContext::FrameCount> m_rtvHandles = {};

    uint32_t m_rtvDescriptorSize = 0;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
};