#pragma once

#include <Windows.h>
#include <wrl/client.h>

#include <d3d12.h>

#include <cstdint>

struct DescriptorHandle
{
    D3D12_CPU_DESCRIPTOR_HANDLE Cpu = {};
    D3D12_GPU_DESCRIPTOR_HANDLE Gpu = {};
    uint32_t Index = UINT32_MAX;

    bool IsValid() const
    {
        return Cpu.ptr != 0 && Index != UINT32_MAX;
    }
};

class DescriptorAllocator
{
public:
    bool Initialize(
        ID3D12Device *device,
        D3D12_DESCRIPTOR_HEAP_TYPE type,
        uint32_t descriptorCount,
        bool shaderVisible,
        const wchar_t *debugName);

    void Shutdown();

    DescriptorHandle Allocate(uint32_t count = 1);

    ID3D12DescriptorHeap *GetHeap() const;
    D3D12_DESCRIPTOR_HEAP_TYPE GetType() const;

    uint32_t GetCapacity() const;
    uint32_t GetAllocatedCount() const;
    uint32_t GetDescriptorSize() const;

    bool IsShaderVisible() const;

private:
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_heap;

    D3D12_DESCRIPTOR_HEAP_TYPE m_type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

    uint32_t m_capacity = 0;
    uint32_t m_allocatedCount = 0;
    uint32_t m_descriptorSize = 0;

    bool m_shaderVisible = false;
};