#pragma once

#include <Windows.h>
#include <wrl/client.h>

#include <d3d12.h>

#include <cstddef>
#include <cstdint>

constexpr uint32_t AlignCB(uint32_t size)
{
    return (size + 255u) & ~255u;
}

constexpr uint64_t AlignTo(uint64_t value, uint64_t alignment)
{
    return (value + alignment - 1u) & ~(alignment - 1u);
}

class UploadBuffer
{
public:
    bool Initialize(
        ID3D12Device *device,
        uint64_t sizeInBytes,
        const wchar_t *debugName);

    void Shutdown();

    void Reset();

    uint64_t Allocate(
        uint64_t sizeInBytes,
        uint64_t alignment);

    template <typename T>
    uint64_t Upload(const T &data, uint64_t alignment = alignof(T))
    {
        const uint64_t offset = Allocate(sizeof(T), alignment);
        Write(offset, &data, sizeof(T));
        return offset;
    }

    uint64_t UploadData(
        const void *data,
        uint64_t sizeInBytes,
        uint64_t alignment);

    void Write(
        uint64_t offset,
        const void *data,
        uint64_t sizeInBytes);

    ID3D12Resource *GetResource() const;
    D3D12_GPU_VIRTUAL_ADDRESS GetGpuAddress(uint64_t offset = 0) const;

    uint8_t *GetCpuBase() const;

    uint64_t GetSize() const;
    uint64_t GetUsedSize() const;

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> m_resource;

    uint8_t *m_mappedData = nullptr;

    uint64_t m_sizeInBytes = 0;
    uint64_t m_currentOffset = 0;
};