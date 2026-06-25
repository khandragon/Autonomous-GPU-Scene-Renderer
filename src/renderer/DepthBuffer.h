#pragma once

#include "renderer/DescriptorAllocator.h"

#include <Windows.h>
#include <wrl/client.h>

#include <d3d12.h>
#include <dxgi1_6.h>

#include <cstdint>

class DepthBuffer
{
public:
    static constexpr DXGI_FORMAT Format = DXGI_FORMAT_D32_FLOAT;
    static constexpr float ClearDepth = 1.0f;

public:
    bool Initialize(
        ID3D12Device *device,
        DescriptorAllocator *dsvAllocator,
        uint32_t width,
        uint32_t height);

    void Shutdown();

    bool Resize(
        ID3D12Device *device,
        uint32_t width,
        uint32_t height);

    ID3D12Resource *GetResource() const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetDsv() const;

    uint32_t GetWidth() const;
    uint32_t GetHeight() const;

private:
    bool CreateResourceAndView(
        ID3D12Device *device,
        uint32_t width,
        uint32_t height);

    void ReleaseResource();

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> m_resource;

    DescriptorAllocator *m_dsvAllocator = nullptr;
    DescriptorHandle m_dsv = {};

    uint32_t m_width = 0;
    uint32_t m_height = 0;
};