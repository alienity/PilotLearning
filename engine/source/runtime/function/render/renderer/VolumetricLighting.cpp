#pragma once

#include "runtime/function/render/renderer/VolumetricLighting.h"
#include "runtime/function/render/renderer/renderer_config.h"
#include "runtime/function/render/utility/HDUtils.h"

namespace MoYu
{
	float CornetteShanksPhasePartConstant(float anisotropy)
	{
		float g = anisotropy;
		return (3.0f / (8.0f * MoYu::f::PI)) * (1.0f - g * g) / (2.0f + g * g);
	}

	void GetHexagonalClosePackedSpheres7(glm::float2 (&coords)[7])
	{
		float r = 0.17054068870105443882f;
		float d = 2 * r;
		float s = r * glm::sqrt(3);

		// Try to keep the weighted average as close to the center (0.5) as possible.
		//  (7)(5)    ( )( )    ( )( )    ( )( )    ( )( )    ( )(o)    ( )(x)    (o)(x)    (x)(x)
		// (2)(1)(3) ( )(o)( ) (o)(x)( ) (x)(x)(o) (x)(x)(x) (x)(x)(x) (x)(x)(x) (x)(x)(x) (x)(x)(x)
		//  (4)(6)    ( )( )    ( )( )    ( )( )    (o)( )    (x)( )    (x)(o)    (x)(x)    (x)(x)
		coords[0] = glm::float2(0, 0);
		coords[1] = glm::float2(-d, 0);
		coords[2] = glm::float2(d, 0);
		coords[3] = glm::float2(-r, -s);
		coords[4] = glm::float2(r, s);
		coords[5] = glm::float2(r, -s);
		coords[6] = glm::float2(-r, s);

		// Rotate the sampling pattern by 15 degrees.
		const float cos15 = 0.96592582628906828675f;
		const float sin15 = 0.25881904510252076235f;

		for (int i = 0; i < 7; i++)
		{
			glm::float2 coord = coords[i];

			coords[i].x = coord.x * cos15 - coord.y * sin15;
			coords[i].y = coord.x * sin15 + coord.y * cos15;
		}
	}

	static glm::uint VolumetricFrameIndex(RHI::D3D12Device* InDevice)
	{
		long frameIndex = InDevice->GetLinkedDevice()->m_FrameIndex;
		// Here we do modulo 14 because we need the enable to detect a change every frame, but the accumulation is done on 7 frames (7x2=14)
		return frameIndex % 14;
	}

	void VolumetriLighting::updateShaderVariableslVolumetrics(
		HLSL::ShaderVariablesVolumetric& cb, std::shared_ptr<RenderResource> render_resource)
	{
		float vFoV = MoYu::f::DEG_TO_RAD * m_render_camera->fovy();
		float gpuAspect = HDUtils::ProjectionMatrixAspect(m_render_camera->getProjMatrix());
		int frameIndex = VolumetricFrameIndex(m_Device);

		glm::float4 resolution = glm::float4(
			m_render_camera->getWidth(), 
			m_render_camera->getHeight(), 
			1.0f / m_render_camera->getWidth(), 
			1.0f / m_render_camera->getHeight());
		float aspect = m_render_camera->asepct();
		glm::float4x4 mTramsform;
		m_render_camera->GetPixelCoordToViewDirWS(resolution, aspect, mTramsform);


		cb._VBufferCoordToViewDirWS = mTramsform;
		cb._VBufferUnitDepthTexelSpacing = HDUtils::ComputZPlaneTexelSpacing(1.0f, vFoV, resolution.y);
		cb._NumVisibleLocalVolumetricFog = 1; // TODO: update from local data
		cb._CornetteShanksConstant = CornetteShanksPhasePartConstant(EngineConfig::g_FogConfig.anisotropy);
		cb._VBufferHistoryIsValid = m_render_camera->volumetricHistoryIsValid ? 1u : 0u;

		GetHexagonalClosePackedSpheres7(m_xySeq);
		int sampleIndex = frameIndex % 7;
		// TODO: should we somehow reorder offsets in Z based on the offset in XY? S.t. the samples more evenly cover the domain.
		// Currently, we assume that they are completely uncorrelated, but maybe we should correlate them somehow.
		cb._VBufferSampleOffset = glm::float4(m_xySeq[sampleIndex].x, m_xySeq[sampleIndex].y, m_zSeq[sampleIndex], frameIndex);

		int currIdx = (frameIndex + 0) & 1;
		int prevIdx = (frameIndex + 1) & 1;

		//var currParams = hdCamera.vBufferParams[currIdx];
		//var prevParams = hdCamera.vBufferParams[prevIdx];


		m_render_camera->volumetricHistoryIsValid = true;
	}

	void VolumetriLighting::prepareBuffer(std::shared_ptr<RenderResource> render_resource)
	{
		


	}

	void VolumetriLighting::initialize(const VolumetriLightingInitInfo& init_info)
	{

	}


	void VolumetriLighting::clearAndHeightFogVoxelizationPass(RHI::RenderGraph& graph, ClearPassInputStruct& passInput, ClearPassOutputStruct& passOutput)
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

