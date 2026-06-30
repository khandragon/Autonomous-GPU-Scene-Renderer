#include "renderer/GraphicsPipeline.h"

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

    // Converts engine's custom CompiledShader struct into the lightweight pointer and size structure.
    D3D12_SHADER_BYTECODE ToShaderBytecode(const CompiledShader &shader)
    {
        D3D12_SHADER_BYTECODE bytecode = {};
        bytecode.pShaderBytecode = shader.Bytecode->GetBufferPointer();
        bytecode.BytecodeLength = shader.Bytecode->GetBufferSize();
        return bytecode;
    }
}

// Main startup function, it takes virtual graphics card handle (device) and your compiler tool, then builds the Root Signature and the Pipeline State.
bool GraphicsPipeline::Initialize(
    ID3D12Device *device,
    ShaderCompiler *shaderCompiler)
{
    if (!device || !shaderCompiler)
    {
        return false;
    }

    try
    {
        if (!CreateRootSignature(device))
        {
            return false;
        }

        if (!CreatePipelineState(device, shaderCompiler))
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

// Clears smart pointers
void GraphicsPipeline::Shutdown()
{
    m_pipelineState.Reset();
    m_rootSignature.Reset();
}

ID3D12RootSignature *GraphicsPipeline::GetRootSignature() const
{
    return m_rootSignature.Get();
}

ID3D12PipelineState *GraphicsPipeline::GetPipelineState() const
{
    return m_pipelineState.Get();
}

// A Root Signature is like a function signature in C++, but for your shaders.
// It tells the GPU exactly what variables (constant buffers, textures) the shader expects to find in memory.
bool GraphicsPipeline::CreateRootSignature(ID3D12Device *device)
{
    // Prepares an array of 2 slots because Forward.hlsl file uses exactly two constant buffers (b0 and b1).
    D3D12_ROOT_PARAMETER rootParameters[2] = {};

    // Configures slot 0 as a Constant Buffer View (CBV). It links specifically to register b0 (ShaderRegister = 0), which holds our FrameConstants
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[0].Descriptor.ShaderRegister = 0;
    rootParameters[0].Descriptor.RegisterSpace = 0;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    // Configures slot 1 as another Constant Buffer View. It links to register b1 (ShaderRegister = 1), which holds our ObjectConstants.
    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[1].Descriptor.ShaderRegister = 1;
    rootParameters[1].Descriptor.RegisterSpace = 0;
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    // Packs the slots into a layout description structure. The flag tells the GPU that we will also be feeding raw 3D model data (vertices) through the Input Assembler stage.
    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
    rootSignatureDesc.NumParameters = 2;
    rootSignatureDesc.pParameters = rootParameters;
    rootSignatureDesc.NumStaticSamplers = 0;
    rootSignatureDesc.pStaticSamplers = nullptr;
    rootSignatureDesc.Flags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    // Raw binary data that the driver understands, printing out error logs if our layout description is invalid.
    Microsoft::WRL::ComPtr<ID3DBlob> serializedRootSignature;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;

    HRESULT hr = D3D12SerializeRootSignature(
        &rootSignatureDesc,
        D3D_ROOT_SIGNATURE_VERSION_1,
        &serializedRootSignature,
        &errorBlob);

    if (FAILED(hr))
    {
        if (errorBlob)
        {
            OutputDebugStringA(
                static_cast<const char *>(errorBlob->GetBufferPointer()));
            OutputDebugStringA("\n");
        }

        return false;
    }

    // Takes that raw binary layout and officially instantiates the Root Signature object on the GPU, giving it a name
    ThrowIfFailed(device->CreateRootSignature(
        0,
        serializedRootSignature->GetBufferPointer(),
        serializedRootSignature->GetBufferSize(),
        IID_PPV_ARGS(&m_rootSignature)));

    m_rootSignature->SetName(L"Forward Root Signature");

    return true;
}

bool GraphicsPipeline::CreatePipelineState(
    ID3D12Device *device,
    ShaderCompiler *shaderCompiler)
{
    // Uses the shader compiler we made earlier to compile Forward.hlsl.
    // It compiles the vertex shader entry (VSMain) and the pixel shader entry (PSMain) using Shader Model 6.0.
    CompiledShader vertexShader = shaderCompiler->CompileFromFile(
        L"shaders/Forward.hlsl",
        L"VSMain",
        L"vs_6_0");

    CompiledShader pixelShader = shaderCompiler->CompileFromFile(
        L"shaders/Forward.hlsl",
        L"PSMain",
        L"ps_6_0");

    // Defines the input layout, matching memory layouts to our vertex shader structure
    D3D12_INPUT_ELEMENT_DESC inputElements[] =
        {
            {"POSITION",
             0,
             DXGI_FORMAT_R32G32B32_FLOAT,
             0,
             0,
             D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
             0},
            {"NORMAL",
             0,
             DXGI_FORMAT_R32G32B32_FLOAT,
             0,
             12,
             D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
             0}};

    // configuration structure and hooks up our Root Signature alongside our newly compiled Vertex and Pixel shader bytecodes.
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = m_rootSignature.Get();

    psoDesc.VS = ToShaderBytecode(vertexShader);
    psoDesc.PS = ToShaderBytecode(pixelShader);

    // Configures Color Blending options. By setting BlendEnable = FALSE,
    // we tell the GPU to simply overwrite the pixel color on screen with our new shader color, rather than trying to blend transparent objects together.
    psoDesc.BlendState.AlphaToCoverageEnable = FALSE;
    psoDesc.BlendState.IndependentBlendEnable = FALSE;

    D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlend = {};
    defaultRenderTargetBlend.BlendEnable = FALSE;
    defaultRenderTargetBlend.LogicOpEnable = FALSE;
    defaultRenderTargetBlend.SrcBlend = D3D12_BLEND_ONE;
    defaultRenderTargetBlend.DestBlend = D3D12_BLEND_ZERO;
    defaultRenderTargetBlend.BlendOp = D3D12_BLEND_OP_ADD;
    defaultRenderTargetBlend.SrcBlendAlpha = D3D12_BLEND_ONE;
    defaultRenderTargetBlend.DestBlendAlpha = D3D12_BLEND_ZERO;
    defaultRenderTargetBlend.BlendOpAlpha = D3D12_BLEND_OP_ADD;
    defaultRenderTargetBlend.LogicOp = D3D12_LOGIC_OP_NOOP;
    defaultRenderTargetBlend.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    for (uint32_t i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
    {
        psoDesc.BlendState.RenderTarget[i] = defaultRenderTargetBlend;
    }

    // Setting it to the maximum integer value (UINT_MAX) leaves all bits enabled.
    psoDesc.SampleMask = UINT_MAX;

    // Configures the Rasterizer State, which dictates how triangles are drawn.
    // D3D12_FILL_MODE_SOLID tells it to draw filled solid shapes instead of wireframes.
    // D3D12_CULL_MODE_BACK tells the GPU to skip drawing triangles facing away from the camera, immediately doubling performance.
    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
    psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
    psoDesc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    psoDesc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    psoDesc.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    psoDesc.RasterizerState.DepthClipEnable = TRUE;
    psoDesc.RasterizerState.MultisampleEnable = FALSE;
    psoDesc.RasterizerState.AntialiasedLineEnable = FALSE;
    psoDesc.RasterizerState.ForcedSampleCount = 0;
    psoDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    // Depth Testing, stops objects in the background from being accidentally drawn on top of objects closer to the camera.
    // 3D12_COMPARISON_FUNC_LESS means a new pixel is only drawn if its depth distance is closer (less) than the pixel already sitting there.
    psoDesc.DepthStencilState.DepthEnable = TRUE;
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.DepthStencilState.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
    psoDesc.DepthStencilState.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;

    // Attaches the vertex map array (POSITION and NORMAL) we declared to the PSO
    psoDesc.InputLayout.pInputElementDescs = inputElements;
    psoDesc.InputLayout.NumElements = static_cast<UINT>(
        sizeof(inputElements) / sizeof(inputElements[0]));

    // Tells the hardware that the raw geometry data is being sent over arranged as regular individual triangles.
    psoDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    // Configures formatting outputs, It states we are outputting to exactly 1 monitor display screen target using our application's default screen format,
    // alongside a dedicated depth buffer format.
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = Swapchain::BackBufferFormat;
    psoDesc.DSVFormat = DepthBuffer::Format;

    // Sets up default Anti-Aliasing parameters (1 sample means standard multi-sampling is turned off).
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleDesc.Quality = 0;

    // Sets advanced options like multi-GPU nodes and pre-compiled file caches to zero/none.
    psoDesc.NodeMask = 0;
    psoDesc.CachedPSO = {};
    psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

    // The graphics card compiles all of these settings down into hardware machine code, creates your ready-to-run Pipeline State Object (m_pipelineState), and names it for debugging.
    ThrowIfFailed(device->CreateGraphicsPipelineState(
        &psoDesc,
        IID_PPV_ARGS(&m_pipelineState)));

    m_pipelineState->SetName(L"Forward Graphics Pipeline State");

    return true;
}