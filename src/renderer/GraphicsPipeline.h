#pragma once

#include "renderer/ShaderCompiler.h"
#include "renderer/Swapchain.h"
#include "renderer/DepthBuffer.h"

#include <Windows.h>
#include <wrl/client.h>

#include <d3d12.h>

class GraphicsPipeline
{
public:
    bool Initialize(
        ID3D12Device *device,
        ShaderCompiler *shaderCompiler);

    void Shutdown();

    ID3D12RootSignature *GetRootSignature() const;
    ID3D12PipelineState *GetPipelineState() const;

private:
    bool CreateRootSignature(ID3D12Device *device);
    bool CreatePipelineState(
        ID3D12Device *device,
        ShaderCompiler *shaderCompiler);

private:
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;
};