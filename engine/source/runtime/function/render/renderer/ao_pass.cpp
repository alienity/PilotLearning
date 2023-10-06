#include "runtime/function/render/renderer/ao_pass.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/rhi/rhi_core.h"

#include <cassert>

namespace MoYu
{

	void AOPass::initialize(const AOInitInfo& init_info)
	{
        colorTexDesc = init_info.colorTexDesc;

        ShaderCompiler*       m_ShaderCompiler = init_info.m_ShaderCompiler;
        std::filesystem::path m_ShaderRootPath = init_info.m_ShaderRootPath;




    }

    void AOPass::update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput)
    {

    }

    void AOPass::destroy()
    {

    }

    bool AOPass::initializeRenderTarget(RHI::RenderGraph& graph, DrawOutputParameters* drawPassOutput)
    {


        return false;
    }

}
