#include "renderer/ShaderCompiler.h"

#include <stdexcept>
#include <vector>

// Handle DirectX errors
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

//
bool ShaderCompiler::Initialize()
{
    try
    {
        // Create a DXC helper tool
        ThrowIfFailed(DxcCreateInstance(
            CLSID_DxcUtils,
            IID_PPV_ARGS(&m_utils)));

        // Creates the actual shader compiler engine itself
        ThrowIfFailed(DxcCreateInstance(
            CLSID_DxcCompiler,
            IID_PPV_ARGS(&m_compiler)));

        // Creates a system handler to manage #include lines inside your shader files, ensuring files can reference code inside other files.
        ThrowIfFailed(m_utils->CreateDefaultIncludeHandler(
            &m_includeHandler));
    }
    catch (...)
    {
        Shutdown();
        return false;
    }

    return true;
}

// Releases the smart pointers when the engine shuts down
void ShaderCompiler::Shutdown()
{
    m_includeHandler.Reset();
    m_compiler.Reset();
    m_utils.Reset();
}

CompiledShader ShaderCompiler::CompileFromFile(
    const std::wstring &filePath,
    const std::wstring &entryPoint,
    const std::wstring &targetProfile)
{
    if (!m_utils || !m_compiler || !m_includeHandler)
    {
        throw std::runtime_error("ShaderCompiler is not initialized.");
    }

    Microsoft::WRL::ComPtr<IDxcBlobEncoding> sourceBlob;

    // reads file
    ThrowIfFailed(m_utils->LoadFile(
        filePath.c_str(),
        nullptr,
        &sourceBlob));

    // Wraps that memory block into a lightweight DxcBuffer structure so the compiler tool knows exactly where the text begins, how long it is
    DxcBuffer sourceBuffer = {};
    sourceBuffer.Ptr = sourceBlob->GetBufferPointer();
    sourceBuffer.Size = sourceBlob->GetBufferSize();
    sourceBuffer.Encoding = DXC_CP_UTF8;

    // Sets up command-line arguments for the compiler.
    std::vector<LPCWSTR> arguments;

    arguments.push_back(filePath.c_str());

    arguments.push_back(L"-E");
    arguments.push_back(entryPoint.c_str());

    arguments.push_back(L"-T");
    arguments.push_back(targetProfile.c_str());

    // Adjusts builds automatically based on project mode.
#if defined(_DEBUG)
    arguments.push_back(L"-Zi");
    arguments.push_back(L"-Qembed_debug");
    arguments.push_back(L"-Od");
#else
    arguments.push_back(L"-O3");
#endif

    // Enforces HLSL 2021 language rules
    arguments.push_back(L"-HV");
    arguments.push_back(L"2021");

    //-WX treats all compilation warnings as hard errors
    arguments.push_back(L"-WX");

    // Orders the compiler engine to process the shader text buffer with all specified settings.
    Microsoft::WRL::ComPtr<IDxcResult> compileResult;

    ThrowIfFailed(m_compiler->Compile(
        &sourceBuffer,
        arguments.data(),
        static_cast<UINT32>(arguments.size()),
        m_includeHandler.Get(),
        IID_PPV_ARGS(&compileResult)));

    // Asks the result bundle to extract any text logs generated during compilation.
    Microsoft::WRL::ComPtr<IDxcBlobUtf8> errors;

    ThrowIfFailed(compileResult->GetOutput(
        DXC_OUT_ERRORS,
        IID_PPV_ARGS(&errors),
        nullptr));

    // extracts errors as string text and prints them
    if (errors && errors->GetStringLength() > 0)
    {
        std::string errorText(
            errors->GetStringPointer(),
            errors->GetStringPointer() + errors->GetStringLength());

        OutputDebugStringA(errorText.c_str());
        OutputDebugStringA("\n");
    }

    // Checks if the compilation genuinely completed. If it failed, it throws a full runtime error.
    HRESULT status = S_OK;
    ThrowIfFailed(compileResult->GetStatus(&status));

    if (FAILED(status))
    {
        throw std::runtime_error("Shader compilation failed.");
    }

    // Extracts the finalized binary data
    Microsoft::WRL::ComPtr<IDxcBlob> shaderBlob;

    ThrowIfFailed(compileResult->GetOutput(
        DXC_OUT_OBJECT,
        IID_PPV_ARGS(&shaderBlob),
        nullptr));

    // Packs the binary shader data alongside its metadata into a container structure, prints a success log, and passes it back to the engine.
    CompiledShader shader = {};
    shader.Bytecode = shaderBlob;
    shader.FilePath = filePath;
    shader.EntryPoint = entryPoint;
    shader.TargetProfile = targetProfile;

    DebugLog(L"Compiled shader: " + filePath + L" / " + entryPoint + L" / " + targetProfile);

    return shader;
}