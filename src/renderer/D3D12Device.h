#pragma once

#include <Windows.h>
#include <wrl/client.h>

#include <d3d12.h>
#include <dxgi1_6.h>

#include <cstdint>
#include <string>

class D3D12Device
{
public:
    bool Initialize(bool enableGpuValidation = false);
    void Shutdown();

    ID3D12Device *GetDevice() const;
    IDXGIFactory6 *GetFactory() const;
    IDXGIAdapter1 *GetAdapter() const;

    const std::wstring &GetAdapterName() const;

private:
    bool EnableDebugLayer(bool enableGpuValidation);
    bool CreateFactory();
    bool ChooseAdapter();
    bool CreateDevice();
    void SetupInfoQueue();

private:
    Microsoft::WRL::ComPtr<IDXGIFactory6> m_factory;
    Microsoft::WRL::ComPtr<IDXGIAdapter1> m_adapter;
    Microsoft::WRL::ComPtr<ID3D12Device> m_device;

    std::wstring m_adapterName;
};