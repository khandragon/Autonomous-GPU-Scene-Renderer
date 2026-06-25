#include "renderer/DepthBuffer.h"

#include <stdexcept>

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

bool DepthBuffer::Initialize(
    ID3D12Device *device,
    DescriptorAllocator *dsvAllocator,
    uint32_t width,
    uint32_t height)
{
    if (!device || !dsvAllocator)
    {
        return false;
    }

    if (width == 0 || height == 0)
    {
        return false;
    }

    m_dsvAllocator = dsvAllocator;

    try
    {
        m_dsv = m_dsvAllocator->Allocate();

        if (!CreateResourceAndView(device, width, height))
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

void DepthBuffer::Shutdown()
{
    ReleaseResource();

    m_dsv = {};
    m_dsvAllocator = nullptr;

    m_width = 0;
    m_height = 0;
}

bool DepthBuffer::Resize(
    ID3D12Device *device,
    uint32_t width,
    uint32_t height)
{
    if (!device)
    {
        return false;
    }

    if (width == 0 || height == 0)
    {
        return false;
    }

    ReleaseResource();

    return CreateResourceAndView(device, width, height);
}

ID3D12Resource *DepthBuffer::GetResource() const
{
    return m_resource.Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE DepthBuffer::GetDsv() const
{
    return m_dsv.Cpu;
}

uint32_t DepthBuffer::GetWidth() const
{
    return m_width;
}

uint32_t DepthBuffer::GetHeight() const
{
    return m_height;
}

bool DepthBuffer::CreateResourceAndView(
    ID3D12Device *device,
    uint32_t width,
    uint32_t height)
{
    if (!m_dsv.IsValid())
    {
        return false;
    }

    m_width = width;
    m_height = height;

    D3D12_HEAP_PROPERTIES heapProperties = {};
    heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
    heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProperties.CreationNodeMask = 1;
    heapProperties.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resourceDesc.Alignment = 0;
    resourceDesc.Width = width;
    resourceDesc.Height = height;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.Format = Format;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.SampleDesc.Quality = 0;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format = Format;
    clearValue.DepthStencil.Depth = ClearDepth;
    clearValue.DepthStencil.Stencil = 0;

    ThrowIfFailed(device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &clearValue,
        IID_PPV_ARGS(&m_resource)));

    m_resource->SetName(L"Main Depth Buffer");

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = Format;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
    dsvDesc.Texture2D.MipSlice = 0;

    device->CreateDepthStencilView(
        m_resource.Get(),
        &dsvDesc,
        m_dsv.Cpu);

    return true;
}

void DepthBuffer::ReleaseResource()
{
    m_resource.Reset();
}