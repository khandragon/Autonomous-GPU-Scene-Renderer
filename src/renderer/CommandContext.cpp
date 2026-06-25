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

// Ensure the GPU finishes all pending work before safely releasing
//  memory trackers, commands lists, fence synchronizations and hardware queues.
void CommandContext::Shutdown()
{
    // Waits for all currently scheduled GPU work to finish preventing deleting objects that are currently in use
    Flush();

    // checks for CPU side OS event handle exists
    if (m_fenceEvent)
    {
        // Clsoes the handle to free OS resources and resets the pointer to nullptr
        CloseHandle(m_fenceEvent);
        m_fenceEvent = nullptr;
    }

    // Releases the smart pointer holding the GPU command List and clears the recording state of the grpahics commands
    m_commandList.Reset();

    // Loop through a collections of structures mangaging individual animation frames
    for (FrameContext &frame : m_frameContexts)
    {
        // Free the ubnderlying memory backing the recorded commands for that frame and reset the synchronization counter to its initial state
        frame.CommandAllocator.Reset();
        frame.FenceValue = 0;
    }

    // Destroy the GPU fence object used to synchonize CPU and GPU timelines and the direct command queue
    m_fence.Reset();
    m_graphicsQueue.Reset();

    // Reset the context tracking integers back to their default starting values
    m_nextFenceValue = 1;
    m_frameIndex = 0;
}

// To ensure the CPU doesnt overwrite data the GPU is still processing, it retrieves the specific memory storage for the current frame,
// and resets the command list so new rendering instructions can be recorded.
void CommandContext::BeginFrame()
{
    // Stall the CPU if the GPU is still processing this specific frame from a previous loop to prevent CPU rushing ahead of GPU
    WaitForFrame(m_frameIndex);

    // Retrieve a reference to data structure tracking the current frame resources
    FrameContext &frame = m_frameContexts[m_frameIndex];

    // Clears the memory allocator allocation pool designated for this specific frame to reuse the underlying gpu memory without alllocating new system resources maximizing performance.
    ThrowIfFailed(frame.CommandAllocator->Reset());

    // Error check if the reset fails
    ThrowIfFailed(m_commandList->Reset(
        frame.CommandAllocator.Get(),
        nullptr));
}

// Close and execute the command list
void CommandContext::ExecuteCommandList()
{
    // close list, throw error if fails
    ThrowIfFailed(m_commandList->Close());

    // Create a list from all the commands
    ID3D12CommandList *commandLists[] =
        {
            m_commandList.Get()};

    // Execute 1 command list, followed by the list
    m_graphicsQueue->ExecuteCommandLists(1, commandLists);
}

// Stops recording Graphics commands, bundling them into an array and hands them over to the hardware graphics queue
void CommandContext::EndFrame()
{
    // Inserts a fence marker into the GPU queue immediately after the submitted command
    SignalFrame(m_frameIndex);

    // increments the frame index counter so the application moves to the next fram buffer slot with modulo ensuring it loops back to 0 once it reaches mac number of allowed frames.
    m_frameIndex = (m_frameIndex + 1) % FrameCount;
}

// Assigns a new tracking number to the GPU timeline tells the gPU to alert the CPU when it reachs that number and forces the CPU to stall until that alert happens and resets the frame tracking counters
void CommandContext::Flush()
{
    // Saftey check that both HPU queue and synchonziation fence exits
    if (!m_graphicsQueue || !m_fence)
    {
        return;
    }

    // generate a unque increasing id number for this synch request
    const uint64_t fenceValue = m_nextFenceValue++;

    // tell GPU queue to update the hardware fence to fencevalue after it finishes processing all commands currently sitting in its queue
    if (FAILED(m_graphicsQueue->Signal(m_fence.Get(), fenceValue)))
    {
        return;
    }

    // aks the gpu for the highest tracking number it has actually completed so far and skips the spu stall entirely if the gpu was fast enough
    if (m_fence->GetCompletedValue() < fenceValue)
    {
        // bind the tracking number to an OS event handle
        if (SUCCEEDED(m_fence->SetEventOnCompletion(fenceValue, m_fenceEvent)))
        {
            // halt CPU thread until GPU triggers OS event
            WaitForSingleObject(m_fenceEvent, INFINITE);
        }
    }

    // Loop through all buffered frame structures and reset synchonization values to 0
    for (FrameContext &frame : m_frameContexts)
    {
        frame.FenceValue = 0;
    }
}

// Get the graphics queue
ID3D12CommandQueue *CommandContext::GetGraphicsQueue() const
{
    return m_graphicsQueue.Get();
}

// Get the command list
ID3D12GraphicsCommandList *CommandContext::GetCommandList() const
{
    return m_commandList.Get();
}

// Get frame index
uint32_t CommandContext::GetFrameIndex() const
{
    return m_frameIndex;
}

// Checks that tracking number halting the CPU only if the GPU is lagging behind and hasn't finished processing that specific frame yet
void CommandContext::WaitForFrame(uint32_t frameIndex)
{
    // Retrieve frame data
    FrameContext &frame = m_frameContexts[frameIndex];

    // Checks if frame has a valid tracking number
    if (frame.FenceValue == 0)
    {
        return;
    }

    // Quieries the GPU to find out the highest tracking number it has finished processing and if lower than this frames value it means the GPU is working on it
    if (m_fence->GetCompletedValue() < frame.FenceValue)
    {
        ThrowIfFailed(m_fence->SetEventOnCompletion(
            frame.FenceValue,
            m_fenceEvent));

        WaitForSingleObject(m_fenceEvent, INFINITE);
    }

    // Resets the frame tracking value to 0, signaling that the frames memory is now safe for the CPU to overwrite
    frame.FenceValue = 0;
}

// Tags a specific frame index with a unique tracking number right after submitting its command to the GPU
void CommandContext::SignalFrame(uint32_t frameIndex)
{
    // Create a unique, higher tracking ID for the frame that was just submitted and increment the global counter for next frame
    const uint64_t fenceValue = m_nextFenceValue++;

    // Insert signal instruction into GPU hardware queue
    // GPU will update its hardware fence object to match fenceValue only after it finishes rendering all the commands submitted right before
    ThrowIfFailed(m_graphicsQueue->Signal(
        m_fence.Get(),
        fenceValue));

    // Saves this specific tracking number inside the frames structure on the CPU side
    m_frameContexts[frameIndex].FenceValue = fenceValue;
}