#pragma once

#include "subsurface_scattering_pass.h"
#include "pass_helper.h"
#include "diffusion_pfoile_settings.h"

namespace MoYu
{
    
    void SubsurfaceScatteringPass::initialize(const PassInitInfo& init_info)
    {
        colorTexDesc = init_info.colorTexDesc;

        colorTexDesc.SetAllowUnorderedAccess(true);
        colorTexDesc.Name = "VolumeCloudOutput";

		ShaderCompiler*       m_ShaderCompiler = init_info.m_ShaderCompiler;
        std::filesystem::path m_ShaderRootPath = init_info.m_ShaderRootPath;


    }

    void SubsurfaceScatteringPass::prepareMetaData(std::shared_ptr<RenderResource> render_resource)
    {
        RenderResource* real_resource = (RenderResource*)render_resource.get();
     
        HLSL::SSSUniform* sssUniform = &real_resource->m_FrameUniforms.sssUniform;

        sssUniform->_DiffusionProfileCount = 1;
        sssUniform->_EnableSubsurfaceScattering = 1u;
        sssUniform->_TexturingModeFlags = DiffusionProfile::TexturingMode::PreAndPostScatter;
        sssUniform->_TransmissionFlags = DiffusionProfile::TransmissionMode::Regular;

        for (int i = 0; i < sssUniform->_DiffusionProfileCount; i++)
        {
            for (int c = 0; c < 4; ++c) // Vector4 component
            {
                sssUniform->_ShapeParamsAndMaxScatterDists[i * 4 + c] = diffusionProfileSettings.shapeParamAndMaxScatterDist;
                // To disable transmission, we simply nullify the transmissionTint
                sssUniform->_TransmissionTintsAndFresnel0[i * 4 + c] = diffusionProfileSettings.transmissionTintAndFresnel0;
                sssUniform->_WorldScalesAndFilterRadiiAndThicknessRemaps[i * 4 + c] = diffusionProfileSettings.worldScaleAndFilterRadiusAndThicknessRemap;
            }
            sssUniform->_DiffusionProfileHashTable[i * 4] = glm::uvec4(diffusionProfileSettings.profile.hash);
        }
    }

    void SubsurfaceScatteringPass::update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput)
    {
        
    }
     
    void SubsurfaceScatteringPass::destroy()
    {

    }

}