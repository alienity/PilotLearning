#pragma once

#include "runtime/function/render/renderer/VolumetricLighting.h"
#include "runtime/function/render/renderer/renderer_config.h"

namespace MoYu
{
	static glm::uint VolumetricFrameIndex(RHI::D3D12Device* InDevice)
	{
		long frameIndex = InDevice->GetLinkedDevice()->m_FrameIndex;
		// Here we do modulo 14 because we need the enable to detect a change every frame, but the accumulation is done on 7 frames (7x2=14)
		return frameIndex % 14;
	}

	void VolumetriLighting::prepareBuffer(std::shared_ptr<RenderResource> render_resource)
	{
		


	}

	void VolumetriLighting::initialize(const VolumetriLightingInitInfo& init_info)
	{

	}


	void VolumetriLighting::ClearAndHeightFogVoxelizationPass(RHI::RenderGraph& graph, ClearPassInputStruct& passInput, ClearPassOutputStruct& passOutput)
	{
		if (!EngineConfig::g_FogConfig.enableVolumetricFog)
		{
			return;
		}

		int frameIndex = (int)VolumetricFrameIndex(m_Device);
		int currIdx = (frameIndex + 0) & 1;

		RHI::RenderPass& clearPass = graph.AddRenderPass("ClearandHeightFogVoxelization");







	}

}

