#pragma once

#include "runtime/function/render/rhi/rhi.h"
#include "runtime/function/render/rhi/rendergraph/RenderGraphCommon.h"
#include "runtime/function/render/rhi/rendergraph/RenderGraphRegistry.h"
#include "runtime/function/render/rhi/rendergraph/RenderGraph.h"
#include "runtime/function/render/render_mesh.h"

struct Shaders
{
    static constexpr LPCWSTR g_VSEntryPoint = L"VSMain";
    static constexpr LPCWSTR g_MSEntryPoint = L"MSMain";
    static constexpr LPCWSTR g_PSEntryPoint = L"PSMain";
    static constexpr LPCWSTR g_CSEntryPoint = L"CSMain";

	// Vertex Shaders
    struct VS
    {
        inline static Shader ScreenQuadPresentVS;
        inline static Shader IndirectDrawVS;
        inline static Shader IndirectDrawShadowmapVS;
    };

    // Mesh Shaders
    struct MS
    {
        //inline static Shader Meshlet;
    };

    // Pixel Shaders
    struct PS
    {
        inline static Shader PresentSDRPS;
        inline static Shader IndirectDrawPS;
        inline static Shader IndirectDrawShadowmapPS;
    };

	// Compute Shaders
    struct CS
    {
        inline static Shader IndirectCull;
        inline static Shader IndirectCullShadowmap;
    };

    static void Compile(ShaderCompiler* Compiler, const std::filesystem::path& ShaderPath)
    {
        // VS
        {
            ShaderCompileOptions Options(g_VSEntryPoint);
            VS::ScreenQuadPresentVS =
                Compiler->CompileShader(RHI_SHADER_TYPE::Vertex, ShaderPath / "hlsl/ScreenQuadPresentVS.hlsl", Options);
            VS::IndirectDrawVS =
                Compiler->CompileShader(RHI_SHADER_TYPE::Vertex, ShaderPath / "hlsl/IndirectDraw.hlsl", Options);
            VS::IndirectDrawShadowmapVS =
                Compiler->CompileShader(RHI_SHADER_TYPE::Vertex, ShaderPath / "hlsl/IndirectDrawShadowmap.hlsl", Options);
        }

		// PS
        {
            ShaderCompileOptions Options(g_PSEntryPoint);
            PS::PresentSDRPS =
                Compiler->CompileShader(RHI_SHADER_TYPE::Pixel, ShaderPath / "hlsl/PresentSDRPS.hlsl", Options);
            PS::IndirectDrawPS =
                Compiler->CompileShader(RHI_SHADER_TYPE::Pixel, ShaderPath / "hlsl/IndirectDraw.hlsl", Options);
            PS::IndirectDrawShadowmapPS =
                Compiler->CompileShader(RHI_SHADER_TYPE::Pixel, ShaderPath / "hlsl/IndirectDrawShadowmap.hlsl", Options);
        }

		// CS
        {
            ShaderCompileOptions Options(g_CSEntryPoint);
            CS::IndirectCull =
                Compiler->CompileShader(RHI_SHADER_TYPE::Compute, ShaderPath / "hlsl/IndirectCull.hlsl", Options);
            CS::IndirectCullShadowmap =
                Compiler->CompileShader(RHI_SHADER_TYPE::Compute, ShaderPath / "hlsl/IndirectCullShadowmap.hlsl", Options);
        }
    }

};

struct Libraries
{
    inline static Library PathTrace;

    static void Compile(ShaderCompiler* Compiler)
    { 

    }
};

struct RootSignatures
{
    inline static RHI::RgResourceHandle FullScreenPresent;
    inline static RHI::RgResourceHandle IndirectCull;
    inline static RHI::RgResourceHandle IndirectDraw;
    inline static RHI::RgResourceHandle IndirectDrawShadowmap;

    static void Compile(RHI::D3D12Device* Device, RHI::RenderGraphRegistry& Registry)
    {
        FullScreenPresent = Registry.CreateRootSignature(Device->CreateRootSignature(
            RHI::RootSignatureDesc()
                .Add32BitConstants<0, 0>(1)
                .AddSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP, 8)
                .AllowResourceDescriptorHeapIndexing()
                .AllowSampleDescriptorHeapIndexing()));

		IndirectCull =
            Registry.CreateRootSignature(Device->CreateRootSignature(RHI::RootSignatureDesc()
                                                                         .AddConstantBufferView<0, 0>()
                                                                         .AddShaderResourceView<0, 0>()
                                                                         .AddUnorderedAccessViewWithCounter<0, 0>()
                                                                         .AllowResourceDescriptorHeapIndexing()
                                                                         .AllowSampleDescriptorHeapIndexing()));

		IndirectDraw = Registry.CreateRootSignature(Device->CreateRootSignature(
            RHI::RootSignatureDesc()
                .Add32BitConstants<0, 0>(1)
                .AddConstantBufferView<1, 0>()
                .AddShaderResourceView<0, 0>()
                .AddShaderResourceView<1, 0>()
                .AddSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_ANISOTROPIC,
                                   D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                                   8)
                .AddSampler<11, 0>(D3D12_FILTER::D3D12_FILTER_COMPARISON_ANISOTROPIC,
                                   D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                                   8,
                                   D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_GREATER_EQUAL,
                                   D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK)
                .AllowInputLayout()
                .AllowResourceDescriptorHeapIndexing()
                .AllowSampleDescriptorHeapIndexing()));

        IndirectDrawShadowmap = Registry.CreateRootSignature(Device->CreateRootSignature(
            RHI::RootSignatureDesc()
                .Add32BitConstants<0, 0>(1)
                .AddConstantBufferView<1, 0>()
                .AddShaderResourceView<0, 0>()
                .AddShaderResourceView<1, 0>()
                .AddSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP, 8)
                .AllowInputLayout()
                .AllowResourceDescriptorHeapIndexing()
                .AllowSampleDescriptorHeapIndexing()));

    }
};

struct CommandSignatures
{
    inline static RHI::RgResourceHandle IndirectDraw;
    inline static RHI::RgResourceHandle IndirectDrawShadowmap;

    static void Compile(RHI::D3D12Device* Device, RHI::RenderGraphRegistry& Registry)
    {
        {
            RHI::CommandSignatureDesc Builder(4, sizeof(HLSL::CommandSignatureParams));
            Builder.AddConstant(0, 0, 1);
            Builder.AddVertexBufferView(0);
            Builder.AddIndexBufferView();
            Builder.AddDrawIndexed();

            ID3D12RootSignature* indirectDrawRootSignature =
                Registry.GetRootSignature(RootSignatures::IndirectDraw)->GetApiHandle();
            IndirectDraw =
                Registry.CreateCommandSignature(RHI::D3D12CommandSignature(Device, Builder, indirectDrawRootSignature));
        }
        {
            RHI::CommandSignatureDesc Builder(4, sizeof(HLSL::CommandSignatureParams));
            Builder.AddConstant(0, 0, 1);
            Builder.AddVertexBufferView(0);
            Builder.AddIndexBufferView();
            Builder.AddDrawIndexed();

            ID3D12RootSignature* indirectDrawShadowmapRootSignature =
                Registry.GetRootSignature(RootSignatures::IndirectDrawShadowmap)->GetApiHandle();
            IndirectDrawShadowmap = Registry.CreateCommandSignature(
                RHI::D3D12CommandSignature(Device, Builder, indirectDrawShadowmapRootSignature));
        }
    }


};

struct PipelineStates
{
    inline static RHI::RgResourceHandle FullScreenPresent;
    inline static RHI::RgResourceHandle IndirectCull;
    inline static RHI::RgResourceHandle IndirectCullShadowmap;
    inline static RHI::RgResourceHandle IndirectDraw;
    inline static RHI::RgResourceHandle IndirectDrawShadowmap;

    static void Compile(DXGI_FORMAT PiplineRtFormat, DXGI_FORMAT PipelineDsFormat, DXGI_FORMAT RtFormat, DXGI_FORMAT DsFormat, RHI::D3D12Device* Device, RHI::RenderGraphRegistry& Registry)
    {
        {
            RHI::D3D12InputLayout InputLayout;

            RHIDepthStencilState DepthStencilState;
            DepthStencilState.DepthEnable = false;

            RHIRenderTargetState RenderTargetState;
            RenderTargetState.RTFormats[0]     = RtFormat; // DXGI_FORMAT_R32G32B32A32_FLOAT;
            RenderTargetState.NumRenderTargets = 1;
            RenderTargetState.DSFormat         = DsFormat; // DXGI_FORMAT_D32_FLOAT;

            struct PsoStream
            {
                PipelineStateStreamRootSignature     RootSignature;
                PipelineStateStreamInputLayout       InputLayout;
                PipelineStateStreamPrimitiveTopology PrimitiveTopologyType;
                PipelineStateStreamVS                VS;
                PipelineStateStreamPS                PS;
                PipelineStateStreamDepthStencilState DepthStencilState;
                PipelineStateStreamRenderTargetState RenderTargetState;
            } Stream;
            Stream.RootSignature         = Registry.GetRootSignature(RootSignatures::FullScreenPresent);
            Stream.InputLayout           = &InputLayout;
            Stream.PrimitiveTopologyType = RHI_PRIMITIVE_TOPOLOGY::Triangle;
            Stream.VS                    = &Shaders::VS::ScreenQuadPresentVS;
            Stream.PS                    = &Shaders::PS::PresentSDRPS;
            Stream.DepthStencilState     = DepthStencilState;
            Stream.RenderTargetState     = RenderTargetState;

            FullScreenPresent = Registry.CreatePipelineState(Device->CreatePipelineState(L"FullScreenPresent", Stream));
        }
        {
            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } Stream;
            Stream.RootSignature = Registry.GetRootSignature(RootSignatures::IndirectCull);
            Stream.CS            = &Shaders::CS::IndirectCull;

            IndirectCull = Registry.CreatePipelineState(Device->CreatePipelineState(L"IndirectCull", Stream));
        }
        {
            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } Stream;
            Stream.RootSignature = Registry.GetRootSignature(RootSignatures::IndirectCull);
            Stream.CS            = &Shaders::CS::IndirectCullShadowmap;

            IndirectCullShadowmap = Registry.CreatePipelineState(Device->CreatePipelineState(L"IndirectCullShadowmap", Stream));
        }
        {
            RHI::D3D12InputLayout InputLayout =
                Pilot::MeshVertex::D3D12MeshVertexPositionNormalTangentTexture::InputLayout;
            
            RHIDepthStencilState DepthStencilState;
            DepthStencilState.DepthEnable = true;
            DepthStencilState.DepthFunc   = RHI_COMPARISON_FUNC::GreaterEqual;

            RHIRenderTargetState RenderTargetState;
            RenderTargetState.RTFormats[0]     = PiplineRtFormat; // DXGI_FORMAT_R32G32B32A32_FLOAT;
            RenderTargetState.NumRenderTargets = 1;
            RenderTargetState.DSFormat         = PipelineDsFormat; // DXGI_FORMAT_D32_FLOAT;

            struct PsoStream
            {
                PipelineStateStreamRootSignature     RootSignature;
                PipelineStateStreamInputLayout       InputLayout;
                PipelineStateStreamPrimitiveTopology PrimitiveTopologyType;
                PipelineStateStreamVS                VS;
                PipelineStateStreamPS                PS;
                PipelineStateStreamDepthStencilState DepthStencilState;
                PipelineStateStreamRenderTargetState RenderTargetState;
            } Stream;
            Stream.RootSignature         = Registry.GetRootSignature(RootSignatures::IndirectDraw);
            Stream.InputLayout           = &InputLayout;
            Stream.PrimitiveTopologyType = RHI_PRIMITIVE_TOPOLOGY::Triangle;
            Stream.VS                    = &Shaders::VS::IndirectDrawVS;
            Stream.PS                    = &Shaders::PS::IndirectDrawPS;
            Stream.DepthStencilState     = DepthStencilState;
            Stream.RenderTargetState     = RenderTargetState;

            IndirectDraw = Registry.CreatePipelineState(Device->CreatePipelineState(L"IndirectDraw", Stream));
        }
        {
            RHI::D3D12InputLayout InputLayout =
                Pilot::MeshVertex::D3D12MeshVertexPositionNormalTangentTexture::InputLayout;

            RHIDepthStencilState DepthStencilState;
            DepthStencilState.DepthEnable = true;
            DepthStencilState.DepthFunc   = RHI_COMPARISON_FUNC::GreaterEqual;

            RHIRenderTargetState RenderTargetState;
            RenderTargetState.NumRenderTargets = 0;
            RenderTargetState.DSFormat         = PipelineDsFormat; // DXGI_FORMAT_D32_FLOAT;

            struct PsoStream
            {
                PipelineStateStreamRootSignature     RootSignature;
                PipelineStateStreamInputLayout       InputLayout;
                PipelineStateStreamPrimitiveTopology PrimitiveTopologyType;
                PipelineStateStreamVS                VS;
                PipelineStateStreamPS                PS;
                PipelineStateStreamDepthStencilState DepthStencilState;
                PipelineStateStreamRenderTargetState RenderTargetState;
            } Stream;
            Stream.RootSignature         = Registry.GetRootSignature(RootSignatures::IndirectDrawShadowmap);
            Stream.InputLayout           = &InputLayout;
            Stream.PrimitiveTopologyType = RHI_PRIMITIVE_TOPOLOGY::Triangle;
            Stream.VS                    = &Shaders::VS::IndirectDrawShadowmapVS;
            Stream.DepthStencilState     = DepthStencilState;
            Stream.RenderTargetState     = RenderTargetState;

            IndirectDrawShadowmap = Registry.CreatePipelineState(Device->CreatePipelineState(L"IndirectDrawShadowmap", Stream));
        }
    }
};
