#pragma once

#include <Windows.h>
#include <wrl/client.h>

#include <d3d12.h>

#include <array>
#include <cstdint>

class CommandContext
{
public:
    static constexpr uint32_t FrameCount = 3;

    struct FrameContext
    {
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CommandAllocator;
        uint64_t FenceValue = 0;
    };

public:
    bool Initialize(ID3D12Device *device);
    void Shutdown();

    void BeginFrame();
    void ExecuteCommandList();
    void EndFrame();

    void Flush();

    ID3D12CommandQueue *GetGraphicsQueue() const;
    ID3D12GraphicsCommandList *GetCommandList() const;

    uint32_t GetFrameIndex() const;

private:
    void WaitForFrame(uint32_t frameIndex);
    void SignalFrame(uint32_t frameIndex);

private:
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_graphicsQueue;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
    Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;

    std::array<FrameContext, FrameCount> m_frameContexts = {};

    HANDLE m_fenceEvent = nullptr;

    uint64_t m_nextFenceValue = 1;
    uint32_t m_frameIndex = 0;
};