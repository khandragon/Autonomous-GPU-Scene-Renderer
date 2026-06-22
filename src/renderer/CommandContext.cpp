#include "renderer/CommandContext.h"

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

bool CommandContext::Initialize(ID3D12Device *device)
{
    if (!device)
    {
        return false;
    }

    try
    {
        // Tells the device to create a queue with the following conditions
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.NodeMask = 0;

        ThrowIfFailed(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_graphicsQueue)));

        m_graphicsQueue->SetName(L"Main Graphics Command Queue");

        // Create an independent memory space(CommandAllocator) for each individual frame
        // Frame 1 is Monitor, GPU is 2 and CPU is 3
        for (uint32_t i = 0; i < FrameCount; ++i)
        {
            ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_frameContexts[i].CommandAllocator)));
            wchar_t name[128] = {};
            swprintf_s(name, L"Graphics Command Allocator %u", i);
            m_frameContexts[i].CommandAllocator->SetName(name);
        }

        ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_frameContexts[0].CommandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_commandList)));

        m_commandList->SetName(L"Main Graphics Command List");

        // Command lists are created in the recording state.
        // Close it once here so BeginFrame can reset it normally.
        ThrowIfFailed(m_commandList->Close());

        // Create a GPU fence initialized to 0, that when reaching certain events will have tasks
        ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));

        m_fence->SetName(L"Main Graphics Fence");

        // Create a standard Win32 CPU synchronization event
        m_fenceEvent = CreateEventW(
            nullptr,
            FALSE,
            FALSE,
            nullptr);

        if (!m_fenceEvent)
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

void CommandContext::Shutdown()
{
    Flush();

    if (m_fenceEvent)
    {
        CloseHandle(m_fenceEvent);
        m_fenceEvent = nullptr;
    }

    m_commandList.Reset();

    for (FrameContext &frame : m_frameContexts)
    {
        frame.CommandAllocator.Reset();
        frame.FenceValue = 0;
    }

    m_fence.Reset();
    m_graphicsQueue.Reset();

    m_nextFenceValue = 1;
    m_frameIndex = 0;
}

void CommandContext::BeginFrame()
{
    WaitForFrame(m_frameIndex);

    FrameContext &frame = m_frameContexts[m_frameIndex];

    ThrowIfFailed(frame.CommandAllocator->Reset());

    ThrowIfFailed(m_commandList->Reset(
        frame.CommandAllocator.Get(),
        nullptr));
}

void CommandContext::EndFrame()
{
    ThrowIfFailed(m_commandList->Close());

    ID3D12CommandList *commandLists[] =
        {
            m_commandList.Get()};

    m_graphicsQueue->ExecuteCommandLists(
        static_cast<UINT>(std::size(commandLists)),
        commandLists);

    SignalFrame(m_frameIndex);

    m_frameIndex = (m_frameIndex + 1) % FrameCount;
}

void CommandContext::Flush()
{
    if (!m_graphicsQueue || !m_fence)
    {
        return;
    }

    const uint64_t fenceValue = m_nextFenceValue++;

    if (FAILED(m_graphicsQueue->Signal(m_fence.Get(), fenceValue)))
    {
        return;
    }

    if (m_fence->GetCompletedValue() < fenceValue)
    {
        if (SUCCEEDED(m_fence->SetEventOnCompletion(fenceValue, m_fenceEvent)))
        {
            WaitForSingleObject(m_fenceEvent, INFINITE);
        }
    }

    for (FrameContext &frame : m_frameContexts)
    {
        frame.FenceValue = 0;
    }
}

ID3D12CommandQueue *CommandContext::GetGraphicsQueue() const
{
    return m_graphicsQueue.Get();
}

ID3D12GraphicsCommandList *CommandContext::GetCommandList() const
{
    return m_commandList.Get();
}

uint32_t CommandContext::GetFrameIndex() const
{
    return m_frameIndex;
}

void CommandContext::WaitForFrame(uint32_t frameIndex)
{
    FrameContext &frame = m_frameContexts[frameIndex];

    if (frame.FenceValue == 0)
    {
        return;
    }

    if (m_fence->GetCompletedValue() < frame.FenceValue)
    {
        ThrowIfFailed(m_fence->SetEventOnCompletion(
            frame.FenceValue,
            m_fenceEvent));

        WaitForSingleObject(m_fenceEvent, INFINITE);
    }

    frame.FenceValue = 0;
}

void CommandContext::SignalFrame(uint32_t frameIndex)
{
    const uint64_t fenceValue = m_nextFenceValue++;

    ThrowIfFailed(m_graphicsQueue->Signal(
        m_fence.Get(),
        fenceValue));

    m_frameContexts[frameIndex].FenceValue = fenceValue;
}