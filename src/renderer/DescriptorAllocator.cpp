#include "renderer/DescriptorAllocator.h"

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

    bool CanBeShaderVisible(D3D12_DESCRIPTOR_HEAP_TYPE type)
    {
        return type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV ||
               type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
    }
}

bool DescriptorAllocator::Initialize(
    ID3D12Device *device,
    D3D12_DESCRIPTOR_HEAP_TYPE type,
    uint32_t descriptorCount,
    bool shaderVisible,
    const wchar_t *debugName)
{
    if (!device || descriptorCount == 0)
    {
        return false;
    }

    if (shaderVisible && !CanBeShaderVisible(type))
    {
        return false;
    }

    m_type = type;
    m_capacity = descriptorCount;
    m_allocatedCount = 0;
    m_shaderVisible = shaderVisible;

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Type = type;
    heapDesc.NumDescriptors = descriptorCount;
    heapDesc.Flags = shaderVisible
                         ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
                         : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    heapDesc.NodeMask = 0;

    try
    {
        ThrowIfFailed(device->CreateDescriptorHeap(
            &heapDesc,
            IID_PPV_ARGS(&m_heap)));

        if (debugName)
        {
            m_heap->SetName(debugName);
        }

        m_descriptorSize = device->GetDescriptorHandleIncrementSize(type);
    }
    catch (...)
    {
        Shutdown();
        return false;
    }

    return true;
}

void DescriptorAllocator::Shutdown()
{
    m_heap.Reset();

    m_type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    m_capacity = 0;
    m_allocatedCount = 0;
    m_descriptorSize = 0;
    m_shaderVisible = false;
}

DescriptorHandle DescriptorAllocator::Allocate(uint32_t count)
{
    if (!m_heap || count == 0)
    {
        throw std::runtime_error("Invalid descriptor allocation.");
    }

    if (m_allocatedCount + count > m_capacity)
    {
        throw std::runtime_error("Descriptor heap is full.");
    }

    const uint32_t index = m_allocatedCount;
    m_allocatedCount += count;

    D3D12_CPU_DESCRIPTOR_HANDLE cpuStart =
        m_heap->GetCPUDescriptorHandleForHeapStart();

    DescriptorHandle handle = {};
    handle.Index = index;
    handle.Cpu.ptr = cpuStart.ptr + static_cast<SIZE_T>(index) * m_descriptorSize;

    if (m_shaderVisible)
    {
        D3D12_GPU_DESCRIPTOR_HANDLE gpuStart =
            m_heap->GetGPUDescriptorHandleForHeapStart();

        handle.Gpu.ptr = gpuStart.ptr + static_cast<UINT64>(index) * m_descriptorSize;
    }

    return handle;
}

ID3D12DescriptorHeap *DescriptorAllocator::GetHeap() const
{
    return m_heap.Get();
}

D3D12_DESCRIPTOR_HEAP_TYPE DescriptorAllocator::GetType() const
{
    return m_type;
}

uint32_t DescriptorAllocator::GetCapacity() const
{
    return m_capacity;
}

uint32_t DescriptorAllocator::GetAllocatedCount() const
{
    return m_allocatedCount;
}

uint32_t DescriptorAllocator::GetDescriptorSize() const
{
    return m_descriptorSize;
}

bool DescriptorAllocator::IsShaderVisible() const
{
    return m_shaderVisible;
}