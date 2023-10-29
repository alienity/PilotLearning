#pragma once

#include "runtime/function/render/render_pass.h"

#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"

namespace MoYu
{
    class ToolPass : public RenderPass
    {
    public:
        struct ToolPassInitInfo : public RenderPassInitInfo
        {
            ShaderCompiler* m_ShaderCompiler;
        };

        struct ToolInputParameters : public PassInput
        {
        };

        struct ToolOutputParameters : public PassOutput
        {
            //std::shared_ptr<RHI::D3D12Texture> p_Radians;
            //std::shared_ptr<RHI::D3D12Texture> p_DFG;
            //std::shared_ptr<RHI::D3D12Texture> p_LD;
        };

    public:
        ~ToolPass() { destroy(); }

        void initialize(const ToolPassInitInfo& init_info);
        void update(RHI::RenderGraph& graph, ToolInputParameters& passInput, ToolOutputParameters& passOutput);
        void destroy() override final;

        void preUpdate(ToolInputParameters& passInput, ToolOutputParameters& passOutput);
        void preUpdate1(ToolInputParameters& passInput, ToolOutputParameters& passOutput);
        void preUpdate2(ToolInputParameters& passInput, ToolOutputParameters& passOutput);

    private:
        glm::float4 _SH[7];

        std::shared_ptr<RHI::D3D12Texture> p_DFG;
        std::shared_ptr<RHI::D3D12Texture> p_LD;
        std::shared_ptr<RHI::D3D12Texture> p_DiffuseRadians;
        std::shared_ptr<RHI::D3D12Buffer>  p_SH;

        Shader m_DFGCS;
        Shader m_LDCS;
        Shader m_RadiansCS;
        Shader m_Radians2SHCS;

        std::shared_ptr<RHI::D3D12RootSignature> p_DFGRootsignature;
        std::shared_ptr<RHI::D3D12RootSignature> p_LDRootsignature;
        std::shared_ptr<RHI::D3D12RootSignature> p_RadiansRootsignature;
        std::shared_ptr<RHI::D3D12RootSignature> p_Radians2SHRootsignature;
        
        std::shared_ptr<RHI::D3D12PipelineState> p_DFGPSO;
        std::shared_ptr<RHI::D3D12PipelineState> p_LDPSO;
        std::shared_ptr<RHI::D3D12PipelineState> p_RadiansPSO;
        std::shared_ptr<RHI::D3D12PipelineState> p_Radians2SHPSO;

        bool isDFGGenerated = false;
        bool isDFGSaved     = false;

        bool isLDGenerated = false;
        bool isLDSaved     = false;

        bool isRadiansGenerated = false;
        bool isRadiansSaved     = false;

        bool isSHGenerated = false;
        bool isSHSaved     = false;
    };

    namespace Tools
    {
        void ReadCubemapToFile(RHI::D3D12Device* device, RHI::D3D12Texture* cubemap, const wchar_t* filename);
        void ReadBuffer2SH(RHI::D3D12Device* pDevice, RHI::D3D12Buffer* pBuffer, glm::float4 pSH[7]);
    }
}

