#include "runtime/function/render/renderer/exposure_pass.h"

#include "runtime/resource/config_manager/config_manager.h"

#include "runtime/function/render/rhi/rhi_core.h"

#include <cassert>

namespace Pilot
{

	void ExposurePass::initialize(const ExposureInitInfo& init_info)
	{
        const float kInitialMinLog = -12.0f;
        const float kInitialMaxLog = 4.0f;

        __declspec(align(16)) float initExposure[] = {EngineConfig::g_HDRConfig.m_Exposure,
                                                      1.0f / EngineConfig::g_HDRConfig.m_Exposure,
                                                      EngineConfig::g_HDRConfig.m_Exposure,
                                                      0.0f,
                                                      kInitialMinLog,
                                                      kInitialMaxLog,
                                                      kInitialMaxLog - kInitialMinLog,
                                                      1.0f / (kInitialMaxLog - kInitialMinLog)};
        
        p_ExposureBuffer = RHI::D3D12Buffer::Create(m_Device->GetLinkedDevice(),
                                                    RHI::RHIBufferTarget::RHIBufferTargetNone,
                                                    8,
                                                    4,
                                                    L"Exposure",
                                                    RHI::RHIBufferMode::RHIBufferModeDynamic,
                                                    D3D12_RESOURCE_STATE_GENERIC_READ,
                                                    (BYTE*)initExposure,
                                                    sizeof(initExposure));
        
































        mTmpColorTexDesc = init_info.m_ColorTexDesc;
        mTmpColorTexDesc.SetAllowUnorderedAccess();
        mTmpColorTexDesc.SetAllowRenderTarget(false);

        ShaderCompiler*       m_ShaderCompiler = init_info.m_ShaderCompiler;
        std::filesystem::path m_ShaderRootPath = init_info.m_ShaderRootPath;

    }

    void ExposurePass::update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput)
    {
        DrawInputParameters  drawPassInput  = passInput;
        DrawOutputParameters drawPassOutput = passOutput;

    }

    void ExposurePass::destroy()
    {

    }

}
