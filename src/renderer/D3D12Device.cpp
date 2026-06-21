#include "renderer/D3D12Device.h"

#include <d3d12sdklayers.h>

#include <cassert>
#include <stdexcept>
#include "D3D12Device.h"

// Namespace for debug utility and if-failed helper functions.
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

// When we initialize the D3D12 device, we want to enable the debug layer in debug builds if requested. In release builds, GPU validation is not available, so we ignore the parameter. We also want to create the DXGI factory, choose an adapter, and create the D3D12 device.
// If any of these steps fail, we return false to indicate initialization failure. If everything succeeds, we return true to indicate successful initialization.
bool D3D12Device::Initialize(bool enableGpuValidation)
{
// Enable the D3D12 debug layer in debug builds if requested. In release builds, GPU validation is not available, so we ignore the parameter.
#if defined(_DEBUG)
    // GPU validation is only available in debug builds, so we only attempt to enable the debug layer if we're in a debug build. In release builds, we ignore the parameter.
    if (!EnableDebugLayer(enableGpuValidation))
    {
        DebugLog(L"Failed to enable D3D12 debug layer.");
        return false;
    }
#else
    // In release builds, GPU validation is not available, so we ignore the parameter.
    (void)enableGpuValidation;
#endif

    // If creation of the factory fails, we cannot proceed, so we return false to indicate initialization failure.
    if (!CreateFactory())
    {
        return false;
    }

    // If choosing an adapter fails, we cannot proceed, so we return false to indicate initialization failure.
    if (!ChooseAdapter())
    {
        return false;
    }
    // If creating the device fails, we cannot proceed, so we return false to indicate initialization failure.
    if (!CreateDevice())
    {
        return false;
    }

    QueryCapabilities();

#if defined(_DEBUG)
    // Setup the info queue to capture debug messages in debug builds. In release builds, we skip this step since the debug layer is not enabled.
    SetupInfoQueue();
#endif

    // If device created we set its name for debugging purposes.
    if (m_device)
    {
        m_device->SetName(L"D3D12 Device");
    }

    DebugLog(L"D3D12 device initialized successfully with adapter: " + m_adapterName);

    return true;
}

// When we shut down the D3D12 device, we want to release all COM objects and clear any stored adapter information.
// This ensures that we clean up resources properly and avoid memory leaks.
void D3D12Device::Shutdown()
{
    m_device.Reset();
    m_adapter.Reset();
    m_factory.Reset();
    m_adapterName.clear();
}

// Getters for the D3D12 device, factory, adapter, and adapter name. These allow other parts of the application to access the D3D12 device and related information as needed.
ID3D12Device *D3D12Device::GetDevice() const
{
    return m_device.Get();
}

IDXGIFactory6 *D3D12Device::GetFactory() const
{
    return m_factory.Get();
}

IDXGIAdapter1 *D3D12Device::GetAdapter() const
{
    return m_adapter.Get();
}

const std::wstring &D3D12Device::GetAdapterName() const
{
    return m_adapterName;
}

const RendererCapabilities &D3D12Device::GetCapabilities() const
{
    return m_capabilities;
}

// Enable the debug layer and gpu validation so that we can catch errors and warnings.
// We have to enable the debug layer manually because because it is not by default because due to performance impact.
bool D3D12Device::EnableDebugLayer(bool enableGpuValidation)
{
    // debugController is a COM pointer to the ID3D12Debug interface, which allows us to enable the D3D12 debug layer.
    Microsoft::WRL::ComPtr<ID3D12Debug> debugController;

    // Attempt to get the D3D12 debug interface. If this fails, we log an error message and return false to indicate that enabling the debug layer failed.
    if (FAILED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
    {
        DebugLog(L"Failed to get D3D12 debug interface.");
        return false;
    }

    debugController->EnableDebugLayer();

    // if GPU validation is requested, we attempt to enable it.
    if (enableGpuValidation)
    {
        // debugController1 is a COM pointer to the ID3D12Debug1 interface, which allows us to enable GPU-based validation.
        Microsoft::WRL::ComPtr<ID3D12Debug1> debugController1;

        // .As() attempts to query the ID3D12Debug interface for the ID3D12Debug1 interface.
        // If succesful, we enable GPU-based validation which provides checks that run on the GPU and can catch issues that the CPU-based debug layer might miss.
        if (SUCCEEDED(debugController.As(&debugController1)))
        {
            debugController1->SetEnableGPUBasedValidation(TRUE);
            DebugLog(L"D3D12 GPU validation enabled.");
        }
        else
        {
            DebugLog(L"Failed to get ID3D12Debug1 interface for GPU validation.");
        }
    }

    DebugLog(L"D3D12 debug layer enabled successfully.");
    return true;
}

// Create Factory which can then enumerate the GPUs installed on system, select optimal hardware adapater and create/manage window swap chain for frames.
bool D3D12Device::CreateFactory()
{
    // Compilation Flags for factory creation
    UINT factoryFlags = 0;

    // if running in debug mode turn on the DXGI flag
#if defined(_DEBUG)
    factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

    // Create a IDXGIFactory2 with the settings and passing pointer to m_factory on success
    HRESULT hr = CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&m_factory));

    // If factory was not successfully created log error
    if (FAILED(hr))
    {
        DebugLog(L"Failed to create DXGI factory.");
        return false;
    }
    return true;
}

// Looks through every GPU sorting by speed and then checks if it can handle DirectX 12 returning the fastest.
bool D3D12Device::ChooseAdapter()
{

    // Create a smart pointer to a phyical graphics card(IDXGIAdapter1)
    Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;

    // loop through every graphics card installed in the system, sorting by power checking dedicated before integrated, then end loop when it runs out of GPUs
    for (UINT adapterIndex = 0; m_factory->EnumAdapterByGpuPreference(adapterIndex, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)) != DXGI_ERROR_NOT_FOUND; ++adapterIndex)
    {
        // Get information about currently iterated GPU
        DXGI_ADAPTER_DESC1 desc = {};
        adapter->GetDesc1(&desc);

        // Check if GPU is a fake "software" renderer
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            // Empties adapter then goes to next iteration of loop
            adapter.Reset();
            continue;
        }

        // Check if current iteration supports DirectX 12_0 features by initializing the decices into a temp (nullptr)
        HRESULT hr = D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device), nullptr);

        // If Success store the GPU into m_adapter and copy its info to m_adapterName, then exit function
        if (SUCCEEDED(hr))
        {
            m_adapter = adapter;
            m_adapterName = desc.Description;
            return true;
        }
        // otherwise reset pointer and try next GPU in list
        adapter.Reset();
    }

    DebugLog(L"Failed to find a hardware adapter that supports D3D12 feature level 12_0.");

    return false;
}

// Creates logical programming interface from the physical graphics card.
bool D3D12Device::CreateDevice()
{
    // Creates device based on adapter selected from ChooseAdapter, then forces it to initialize using DirectX 12, then assigns pointer to m_device.
    HRESULT hr = D3D12CreateDevice(m_adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_device));

    if (FAILED(hr))
    {
        DebugLog(L"Failed to create D3D12 device.");
        return false;
    }

    return true;
}

void D3D12Device::SetupInfoQueue()
{
    // Create an empty DirectX 12 Info Queue interface
    Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue;

    // Only available in Debug mode so if this is in Release mode this should fail
    if (FAILED(m_device.As(&infoQueue)))
    {
        DebugLog(L"Failed to get device info");
        return;
    }

    // If we leak GPU memory or send invalid instructions to GPU code should stop instantly
    infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
    infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);

    // If we get warning don't stop the program.
    infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, FALSE);

    // array of specific DirectX messages to ignore, these two are usually spammy about background color or depth settings
    D3D12_MESSAGE_ID hiddenMessages[] =
        {
            D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
            D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE};

    // Graphics driver will quitetly supress these specific messages from output window for debugging purposes.
    D3D12_INFO_QUEUE_FILTER filter = {};
    filter.DenyList.NumIDs = static_cast<UINT>(std::size(hiddenMessages));
    filter.DenyList.pIDList = hiddenMessages;

    infoQueue->AddStorageFilterEntries(&filter);
}

// Go through device capabilities
void D3D12Device::QueryCapabilities()
{
    m_capabilities = {};

    m_capabilities.SupportsExecuteIndirect = true;

    m_capabilities.SupportsTimestampQueries = true;

    D3D12_FEATURE_DATA_D3D12_OPTIONS7 options7 = {};

    if (SUCCEEDED(m_device->CheckFeatureSupport(
            D3D12_FEATURE_D3D12_OPTIONS7,
            &options7,
            sizeof(options7))))
    {
        m_capabilities.SupportsMeshShaders =
            options7.MeshShaderTier != D3D12_MESH_SHADER_TIER_NOT_SUPPORTED;
    }

    m_capabilities.SupportsWorkGraphs = false;
}
