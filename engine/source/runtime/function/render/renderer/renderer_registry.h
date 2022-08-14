#pragma once

#include "runtime/function/render/rhi/rhi.h"
#include "runtime/function/render/rhi/rendergraph/RenderGraphCommon.h"
#include "runtime/function/render/rhi/rendergraph/RenderGraphRegistry.h"
#include "runtime/function/render/rhi/rendergraph/RenderGraph.h"

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
    };

	// Compute Shaders
    struct CS
    {
        //inline static Shader IndirectCull;
    };

    static void Compile(ShaderCompiler* Compiler, const std::filesystem::path& ShaderPath)
    {
        // VS
        {
            ShaderCompileOptions Options(g_VSEntryPoint);
            VS::ScreenQuadPresentVS =
                Compiler->CompileShader(RHI_SHADER_TYPE::Vertex, ShaderPath / "hlsl/ScreenQuadPresentVS.hlsl", Options);
        }

		// PS
        {
            ShaderCompileOptions Options(g_PSEntryPoint);
            PS::PresentSDRPS =
                Compiler->CompileShader(RHI_SHADER_TYPE::Pixel, ShaderPath / "hlsl/PresentSDRPS.hlsl", Options);
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

    static void Compile(RHI::D3D12Device* Device, RHI::RenderGraphRegistry& Registry)
    {
        FullScreenPresent =
            Registry.CreateRootSignature(Device->CreateRootSignature(RHI::RootSignatureDesc()
                                                                         .AllowResourceDescriptorHeapIndexing()
                                                                         .AllowSampleDescriptorHeapIndexing()));

		IndirectCull =
            Registry.CreateRootSignature(Device->CreateRootSignature(RHI::RootSignatureDesc()
                                                                         .AddConstantBufferView<0, 0>()
                                                                         .AddShaderResourceView<0, 0>()
                                                                         .AddUnorderedAccessViewWithCounter<0, 0>()
                                                                         .AllowResourceDescriptorHeapIndexing()
                                                                         .AllowSampleDescriptorHeapIndexing()));
    }
};

struct CommandSignatures
{
    inline static RHI::D3D12CommandSignature DrawIndirectCommandSignature;

    static void Compile(RHI::D3D12Device* Device, RHI::RenderGraphRegistry& Registry)
    {
        /*
        {
            RHI::CommandSignatureDesc Builder(4, sizeof(HLSL::CommandSignatureParams));
            Builder.AddConstant(0, 0, 1);
            Builder.AddVertexBufferView(0);
            Builder.AddIndexBufferView();
            Builder.AddDrawIndexed();

            ID3D12RootSignature* indirectCullRootSignature =
                Registry.GetRootSignature(RootSignatures::IndirectCull)->GetApiHandle();
            DrawIndirectCommandSignature = RHI::D3D12CommandSignature(Device, Builder, indirectCullRootSignature);
        }
        */
    }


};

struct PipelineStates
{
    inline static RHI::RgResourceHandle FullScreenPresent;

    static void
    Compile(DXGI_FORMAT RtFormat, DXGI_FORMAT DsFormat, RHI::D3D12Device* Device, RHI::RenderGraphRegistry& Registry)
    {
        {
            RHI::D3D12InputLayout InputLayout;

            RHIDepthStencilState DepthStencilState;
            DepthStencilState.DepthEnable = true;

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

    }
};
