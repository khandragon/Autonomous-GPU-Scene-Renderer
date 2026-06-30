#pragma once

#include <Windows.h>
#include <wrl/client.h>

#include <dxcapi.h>

#include <string>

struct CompiledShader
{
    Microsoft::WRL::ComPtr<IDxcBlob> Bytecode;

    std::wstring FilePath;
    std::wstring EntryPoint;
    std::wstring TargetProfile;

    bool IsValid() const
    {
        return Bytecode != nullptr;
    }
};

class ShaderCompiler
{
public:
    bool Initialize();
    void Shutdown();

    CompiledShader CompileFromFile(
        const std::wstring &filePath,
        const std::wstring &entryPoint,
        const std::wstring &targetProfile);

private:
    Microsoft::WRL::ComPtr<IDxcUtils> m_utils;
    Microsoft::WRL::ComPtr<IDxcCompiler3> m_compiler;
    Microsoft::WRL::ComPtr<IDxcIncludeHandler> m_includeHandler;
};