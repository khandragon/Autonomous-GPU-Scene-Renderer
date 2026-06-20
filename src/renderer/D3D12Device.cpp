#include "renderer/D3D12Device.h"

#include <d3d12sdklayers.h>

#include <cassert>
#include <stdexcept>
#include "D3D12Device.h"

namespace
{
    void ThrowIfFailed(HRESULT hr)
    {
        if (FAILED(hr))
        {
            throw std::runtime_error("HRESULT failed.");
        }
    }

    void DebugLog(const std::wstring &message)
    {
        OutputDebugStringW(message.c_str());
        OutputDebugStringW(L"\n");
    }
}

bool D3D12Device::Initialize(bool enableGpuValidation)
{
    return false;
}

void D3D12Device::Shutdown()
{
}

ID3D12Device *D3D12Device::GetDevice() const
{
    return nullptr;
}

IDXGIFactory6 *D3D12Device::GetFactory() const
{
    return nullptr;
}

IDXGIAdapter1 *D3D12Device::GetAdapter() const
{
    return nullptr;
}

const std::wstring &D3D12Device::GetAdapterName() const
{
    // TODO: insert return statement here
}

bool D3D12Device::EnableDebugLayer(bool enableGpuValidation)
{
    return false;
}

bool D3D12Device::CreateFactory()
{
    return false;
}

bool D3D12Device::ChooseAdapter()
{
    return false;
}

bool D3D12Device::CreateDevice()
{
    return false;
}

void D3D12Device::SetupInfoQueue()
{
}
