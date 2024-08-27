#pragma once

#include "runtime/function/render/render_pass.h"
#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"

namespace MoYu
{
    class DiffuseFilter
    {
    public:
        struct DiffuseFilterInitInfo
        {
            RHI::D3D12Device* m_Device;
            RHI::RgTextureDesc m_ColorTexDesc;
            ShaderCompiler* m_ShaderCompiler;
            std::filesystem::path m_ShaderRootPath;
        };

        struct ShaderSignaturePSO
        {
            Shader m_Kernal;
            std::shared_ptr<RHI::D3D12RootSignature> pKernalSignature;
            std::shared_ptr<RHI::D3D12PipelineState> pKernalPSO;
        };

        struct GeneratePointDistributionData
        {
            RHI::RgResourceHandle pointDistributionRWHandle;
        };

        struct BilateralFilterData
        {
            RHI::RgResourceHandle perFrameBufferHandle;
            RHI::RgResourceHandle depthStencilBuffer;
            RHI::RgResourceHandle normalBuffer;
            RHI::RgResourceHandle noisyBuffer;
            RHI::RgResourceHandle outputBuffer;
        };

        struct BilateralFilterParameter
        {
            // Radius of the filter (world space)
            glm::float4 _DenoiserResolutionMultiplierVals;
            float _DenoiserFilterRadius;
            float _PixelSpreadAngleTangent;
            int _JitterFramePeriod;
            // Flag used to do a half resolution filter
            int _HalfResolutionFilter;
        };


        void Init(DiffuseFilterInitInfo InitInfo);

        void PrepareUniforms(RenderScene* render_scene, RenderCamera* camera);

        void GeneratePointDistribution(RHI::RenderGraph& graph, GeneratePointDistributionData& passData);
        void BilateralFilter(RHI::RenderGraph& graph, BilateralFilterData& passData);

        BilateralFilterParameter mBilateralFilterParameter;
        std::shared_ptr<RHI::D3D12Buffer> mBilateralFilterParameterBuffer;
        RHI::RgResourceHandle mBilateralFilterParameterBufferHandle;

        std::shared_ptr<RHI::D3D12Texture> pointDistributionTexture;

        ShaderSignaturePSO mGeneratePointDistribution;
        ShaderSignaturePSO mBilateralFilter;

        RHI::RgTextureDesc colorTexDesc;
        RHI::D3D12Device* m_Device;
    };

};

