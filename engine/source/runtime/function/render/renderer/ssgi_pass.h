#pragma once

#include "runtime/function/render/render_pass.h"
#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"

namespace MoYu
{
    class SSGIPass : public RenderPass
	{
    public:
        struct SSGIInitInfo : public RenderPassInitInfo
        {
            RHI::RgTextureDesc    m_ColorTexDesc;
            ShaderCompiler*       m_ShaderCompiler;
            std::filesystem::path m_ShaderRootPath;
        };

        struct DrawInputParameters : public PassInput
        {
            RHI::RgResourceHandle perframeBufferHandle;
            RHI::RgResourceHandle colorPyramidHandle;
            RHI::RgResourceHandle depthPyramidHandle;
            RHI::RgResourceHandle lastDepthPyramidHandle;
            RHI::RgResourceHandle normalBufferHandle;
            RHI::RgResourceHandle cameraMotionVectorHandle;
        };

        struct DrawOutputParameters : public PassOutput
        {
            RHI::RgResourceHandle ssrOutHandle;
        };

    public:
        ~SSGIPass() { destroy(); }

        void initialize(const SSGIInitInfo& init_info);
        void prepareMetaData(std::shared_ptr<RenderResource> render_resource);
        void update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput);
        void destroy() override final;

        std::shared_ptr<RHI::D3D12Texture> getCurTemporalResult();
        std::shared_ptr<RHI::D3D12Texture> getPrevTemporalResult();

    protected:
        struct ScreenSpaceGIStruct
        {
            // Ray marching constants
            int _RayMarchingSteps;
            float _RayMarchingThicknessScale;
            float _RayMarchingThicknessBias;
            int _RayMarchingReflectsSky;

            int _RayMarchingFallbackHierarchy;
            int _IndirectDiffuseProbeFallbackFlag;
            int _IndirectDiffuseProbeFallbackBias;
            int _SsrStencilBit;

            int _IndirectDiffuseFrameIndex;
            int _ObjectMotionStencilBit;
            float _RayMarchingLowResPercentageInv;
            int _SSGIUnused0;

            glm::float4 _ColorPyramidUvScaleAndLimitPrevFrame;
        };

        ScreenSpaceGIStruct mSSGIStruct;

        std::shared_ptr<RHI::D3D12Buffer> pSSGIConsBuffer;

    private:
        int passIndex;

        std::shared_ptr<RHI::D3D12Texture> m_bluenoise;
        std::shared_ptr<RHI::D3D12Texture> m_owenScrambled256Tex;
        std::shared_ptr<RHI::D3D12Texture> m_scramblingTile8SPP;
        std::shared_ptr<RHI::D3D12Texture> m_rankingTile8SPP;
        std::shared_ptr<RHI::D3D12Texture> m_scramblingTex;

        std::shared_ptr<RHI::D3D12Texture> pIndirectDiffuseTexture;

        Shader SSTraceGICS;
        std::shared_ptr<RHI::D3D12RootSignature> pSSTraceGISignature;
        std::shared_ptr<RHI::D3D12PipelineState> pSSTraceGIPSO;

        Shader SSReporjectGICS;
        std::shared_ptr<RHI::D3D12RootSignature> pSSReporjectGISignature;
        std::shared_ptr<RHI::D3D12PipelineState> pSSReporjectGIPSO;

        RHI::RgTextureDesc colorTexDesc;
        RHI::RgTextureDesc indirectDiffuseHitPointDesc;
	};
}

