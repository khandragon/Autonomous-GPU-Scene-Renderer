#include "renderer/UploadBuffer.h"

#include <cstring>
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

bool UploadBuffer::Initialize(
    ID3D12Device *device,
    uint64_t sizeInBytes,
    const wchar_t *debugName)
{
    if (!device || sizeInBytes == 0)
    {
        return false;
    }

    m_sizeInBytes = sizeInBytes;
    m_currentOffset = 0;

    D3D12_HEAP_PROPERTIES heapProperties = {};
    heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
    heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProperties.CreationNodeMask = 1;
    heapProperties.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Alignment = 0;
    resourceDesc.Width = sizeInBytes;
    resourceDesc.Height = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.SampleDesc.Quality = 0;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    try
    {
        ThrowIfFailed(device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_resource)));

        if (debugName)
        {
            m_resource->SetName(debugName);
        }

        D3D12_RANGE readRange = {};
        readRange.Begin = 0;
        readRange.End = 0;

        ThrowIfFailed(m_resource->Map(
            0,
            &readRange,
            reinterpret_cast<void **>(&m_mappedData)));
    }
    catch (...)
    {
        Shutdown();
        return false;
    }

    return true;
}

void UploadBuffer::Shutdown()
{
    if (m_resource && m_mappedData)
    {
        D3D12_RANGE writtenRange = {};
        writtenRange.Begin = 0;
        writtenRange.End = static_cast<SIZE_T>(m_currentOffset);

        m_resource->Unmap(0, &writtenRange);
    }

    m_mappedData = nullptr;
    m_resource.Reset();

    m_sizeInBytes = 0;
    m_currentOffset = 0;
}

void UploadBuffer::Reset()
{
    m_currentOffset = 0;
}

uint64_t UploadBuffer::Allocate(
    uint64_t sizeInBytes,
    uint64_t alignment)
{
    if (!m_resource || !m_mappedData)
    {
        throw std::runtime_error("UploadBuffer is not initialized.");
    }

    if (alignment == 0)
    {
        alignment = 1;
    }

    const uint64_t alignedOffset =
        AlignTo(m_currentOffset, alignment);

    const uint64_t endOffset =
        alignedOffset + sizeInBytes;

    if (endOffset > m_sizeInBytes)
    {
        throw std::runtime_error("UploadBuffer overflow.");
    }

    m_currentOffset = endOffset;

    return alignedOffset;
}

uint64_t UploadBuffer::UploadData(
    const void *data,
    uint64_t sizeInBytes,
    uint64_t alignment)
{
    const uint64_t offset = Allocate(sizeInBytes, alignment);
    Write(offset, data, sizeInBytes);
    return offset;
}

void UploadBuffer::Write(
    uint64_t offset,
    const void *data,
    uint64_t sizeInBytes)
{
    if (!data || sizeInBytes == 0)
    {
        return;
    }

    if (offset + sizeInBytes > m_sizeInBytes)
    {
        throw std::runtime_error("UploadBuffer write out of range.");
    }

    std::memcpy(
        m_mappedData + offset,
        data,
        static_cast<size_t>(sizeInBytes));
}

ID3D12Resource *UploadBuffer::GetResource() const
{
    return m_resource.Get();
}

D3D12_GPU_VIRTUAL_ADDRESS UploadBuffer::GetGpuAddress(
    uint64_t offset) const
{
    return m_resource->GetGPUVirtualAddress() + offset;
}

uint8_t *UploadBuffer::GetCpuBase() const
{
    return m_mappedData;
}

uint64_t UploadBuffer::GetSize() const
{
    return m_sizeInBytes;
}

uint64_t UploadBuffer::GetUsedSize() const
{
    return m_currentOffset;
}