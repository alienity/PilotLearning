#include "runtime/function/render/renderer/diffuseDenoiser_pass.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/rhi/rhi_core.h"
#include "runtime/function/render/jitter_helper.h"
#include "runtime/function/render/render_helper.h"
#include "runtime/function/render/renderer/pass_helper.h"

#include <cassert>

namespace MoYu
{

	void DiffuseDenoisePass::initialize(const DDInitInfo& init_info)
	{
        colorTexDesc = init_info.m_ColorTexDesc;

        ShaderCompiler*       m_ShaderCompiler = init_info.m_ShaderCompiler;
        std::filesystem::path m_ShaderRootPath = init_info.m_ShaderRootPath;

        {
            BilateralFilterSingleCS = m_ShaderCompiler->CompileShader(
                RHI_SHADER_TYPE::Compute, m_ShaderRootPath / "pipeline/Runtime/Lighting/ScreenSpaceLighting/ScreenSpaceTraceGICS.hlsl",
                ShaderCompileOptions(L"TRACE_GLOBAL_ILLUMINATION"));

            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                    .Add32BitConstants<0, 0>(16)
                    .AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 1)
                    .AddStaticSampler<11, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 1)
                    .AllowResourceDescriptorHeapIndexing()
                    .AllowSampleDescriptorHeapIndexing();

            pBilateralFilterSingleSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);

            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature         = PipelineStateStreamRootSignature(pBilateralFilterSingleSignature.get());
            psoStream.CS                    = &BilateralFilterSingleCS;
            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};

            pBilateralFilterSinglePSO = std::make_shared<RHI::D3D12PipelineState>(m_Device, L"BilateralFilterSinglePSO", psoDesc);
        }

        {

            BilateralFilterColorCS = m_ShaderCompiler->CompileShader(
                RHI_SHADER_TYPE::Compute, m_ShaderRootPath / "pipeline/Runtime/Lighting/ScreenSpaceLighting/ScreenSpaceTraceGICS.hlsl",
                ShaderCompileOptions(L"TRACE_GLOBAL_ILLUMINATION"));

            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                .Add32BitConstants<0, 0>(16)
                .AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 1)
                .AddStaticSampler<11, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 1)
                .AllowResourceDescriptorHeapIndexing()
                .AllowSampleDescriptorHeapIndexing();

            pBilateralFilterColorSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);

            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature = PipelineStateStreamRootSignature(pBilateralFilterColorSignature.get());
            psoStream.CS = &BilateralFilterColorCS;
            PipelineStateStreamDesc psoDesc = { sizeof(PsoStream), &psoStream };

            pBilateralFilterColorPSO = std::make_shared<RHI::D3D12PipelineState>(m_Device, L"BilateralFilterColorPSO", psoDesc);

        }

    }

    void DiffuseDenoisePass::prepareMetaData(std::shared_ptr<RenderResource> render_resource)
    {

    }



    void DiffuseDenoisePass::update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput)
    {
        
    }

    void DiffuseDenoisePass::destroy()
    {



    }

}
