#pragma once

#include "runtime/function/render/renderer/VolumetricLighting.h"
#include "runtime/function/render/renderer/renderer_config.h"
#include "runtime/function/render/utility/HDUtils.h"
#include "pass_helper.h"

namespace MoYu
{
	static int GetNumTileBigTileX(RenderCamera* pRenderCamera)
	{
		MoYu::LightDefinitions DefaultDefinition;
		return HDUtils::DivRoundUp((int)pRenderCamera->getWidth(), DefaultDefinition.s_TileSizeBigTile);
	}

	static int GetNumTileBigTileY(RenderCamera* pRenderCamera)
	{
		MoYu::LightDefinitions DefaultDefinition;
		return HDUtils::DivRoundUp((int)pRenderCamera->getHeight(), DefaultDefinition.s_TileSizeBigTile);
	}

	void VolumetriLighting::updateShaderVariableslVolumetrics(
		HLSL::ShaderVariablesVolumetric& cb, glm::vec4 resolution, int maxSliceCount, bool updateVoxelizationFields)
	{
		float vFoV = MoYu::f::DEG_TO_RAD * m_render_camera->fovy();
		float gpuAspect = HDUtils::ProjectionMatrixAspect(m_render_camera->getProjMatrix());
		int frameIndex = VolumetricFrameIndex(m_Device->GetLinkedDevice()->m_FrameIndex);


		glm::float4x4 m_PixelCoordToViewDirWS;
		m_render_camera->GetPixelCoordToViewDirWS(resolution, gpuAspect, m_PixelCoordToViewDirWS);

		cb._VBufferCoordToViewDirWS = m_PixelCoordToViewDirWS;
		cb._VBufferUnitDepthTexelSpacing = HDUtils::ComputZPlaneTexelSpacing(1.0f, vFoV, resolution.y);
		cb._NumVisibleLocalVolumetricFog = 1; // TODO: update from local data
		cb._CornetteShanksConstant = CornetteShanksPhasePartConstant(EngineConfig::g_FogConfig.anisotropy);
		cb._VBufferHistoryIsValid = m_render_camera->volumetricHistoryIsValid ? 1u : 0u;

		GetHexagonalClosePackedSpheres7(m_xySeq);
		int sampleIndex = frameIndex % 7;
		// TODO: should we somehow reorder offsets in Z based on the offset in XY? S.t. the samples more evenly cover the domain.
		// Currently, we assume that they are completely uncorrelated, but maybe we should correlate them somehow.
		cb._VBufferSampleOffset = glm::float4(m_xySeq[sampleIndex].x, m_xySeq[sampleIndex].y, m_zSeq[sampleIndex], frameIndex);

		VBufferParameters& currParams = m_CurrentVBufferParams;

		glm::ivec3 pvp = m_CurrentVBufferParams.viewportSize;
		glm::ivec3 historyBufferSize = m_CurrentVolumetricBufferSize;

		cb._VBufferVoxelSize = currParams.voxelSize;
		cb._VBufferPrevViewportSize = glm::vec4(pvp.x, pvp.y, 1.0f / pvp.x, 1.0f / pvp.y);
		cb._VBufferHistoryViewportScale = glm::vec4(currParams.ComputeViewportScale(historyBufferSize), 0);
		cb._VBufferHistoryViewportLimit = glm::vec4(currParams.ComputeViewportLimit(historyBufferSize), 0);
		cb._VBufferPrevDistanceEncodingParams = currParams.depthEncodingParams;
		cb._VBufferPrevDistanceDecodingParams = currParams.depthDecodingParams;
		cb._NumTileBigTileX = (glm::uint)GetNumTileBigTileX(m_render_camera);
		cb._NumTileBigTileY = (glm::uint)GetNumTileBigTileY(m_render_camera);

		cb._MaxSliceCount = (glm::uint)maxSliceCount;
		cb._MaxVolumetricFogDistance = EngineConfig::g_FogConfig.depthExtent;
		cb._VolumeCount = 1;

		if (updateVoxelizationFields)
		{
			// TODO: 
		}

		m_render_camera->volumetricHistoryIsValid = true;
	}

	void VolumetriLighting::prepareBuffer(std::shared_ptr<RenderResource> render_resource)
	{
		if (pShaderVariablesVolumetric == nullptr)
		{
			pShaderVariablesVolumetric = RHI::D3D12Buffer::Create(m_Device->GetLinkedDevice(),
				RHI::RHIBufferTargetNone,
				1,
				MoYu::AlignUp(sizeof(HLSL::ShaderVariablesVolumetric), 256),
				L"ShaderVariablesVolumetricCB",
				RHI::RHIBufferModeDynamic,
				D3D12_RESOURCE_STATE_GENERIC_READ);
		}

		m_CurrentVBufferParams = ComputeVolumetricBufferParameters(
			m_render_camera->getWidth(), 
			m_render_camera->getHeight(),
			m_render_camera->nearZ(),
			m_render_camera->farZ(),
			m_render_camera->fovy(),
			EngineConfig::g_FogConfig);

		glm::ivec3 cvp = m_CurrentVBufferParams.viewportSize;

		m_CurrentVolumetricBufferSize = glm::ivec3(
			glm::max(m_CurrentVolumetricBufferSize.x, cvp.x),
			glm::max(m_CurrentVolumetricBufferSize.y, cvp.y),
			glm::max(m_CurrentVolumetricBufferSize.z, cvp.z));

		glm::vec4 resolution = glm::vec4(cvp.x, cvp.y, 1.0f / cvp.x, 1.0f / cvp.y);

		int sliceCount;
		float screenFraction;
		ComputeVolumetricFogSliceCountAndScreenFraction(EngineConfig::g_FogConfig, sliceCount, screenFraction);

		fogData.resolution = resolution;

		HLSL::ShaderVariablesVolumetric m_ShaderVariablesVolumetricCB;
		updateShaderVariableslVolumetrics(m_ShaderVariablesVolumetricCB, resolution, sliceCount);

		fogData.volumetricCB = m_ShaderVariablesVolumetricCB;

		memcpy(pShaderVariablesVolumetric->GetCpuVirtualAddress<HLSL::ShaderVariablesVolumetric>(), 
			&m_ShaderVariablesVolumetricCB, sizeof(m_ShaderVariablesVolumetricCB));


		if (mVBufferDensity == nullptr)
		{
			mVBufferDensity = RHI::D3D12Texture::Create3D(
				m_Device->GetLinkedDevice(),
				m_CurrentVolumetricBufferSize.x,
				m_CurrentVolumetricBufferSize.y,
				m_CurrentVolumetricBufferSize.z,
				1,
				DXGI_FORMAT_R32G32B32A32_FLOAT,
				RHI::RHISurfaceCreateRandomWrite,
				1,
				L"ASScattering3D",
				D3D12_RESOURCE_STATE_COMMON);
		}



	}

	void VolumetriLighting::initialize(const VolumetriLightingInitInfo& init_info)
	{
		colorTexDesc = init_info.colorTexDesc;

		ShaderCompiler* m_ShaderCompiler = init_info.m_ShaderCompiler;
		std::filesystem::path m_ShaderRootPath = init_info.m_ShaderRootPath;

		{
			VoxelizationCS = m_ShaderCompiler->CompileShader(
				RHI_SHADER_TYPE::Compute, m_ShaderRootPath / "pipeline/Runtime/Tools/VolumeLighting/VolumeVoxelizationCS.hlsl", ShaderCompileOptions(L"VolumeVoxelization"));

			RHI::RootSignatureDesc rootSigDesc =
				RHI::RootSignatureDesc()
				.Add32BitConstants<0, 0>(16)
				.AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 1)
				.AddStaticSampler<11, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 1)
				.AllowResourceDescriptorHeapIndexing()
				.AllowSampleDescriptorHeapIndexing();

			pVoxelizationSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);

			struct PsoStream
			{
				PipelineStateStreamRootSignature RootSignature;
				PipelineStateStreamCS            CS;
			} psoStream;
			psoStream.RootSignature = PipelineStateStreamRootSignature(pVoxelizationSignature.get());
			psoStream.CS = &VoxelizationCS;
			PipelineStateStreamDesc psoDesc = { sizeof(PsoStream), &psoStream };

			pVoxelizationPSO = std::make_shared<RHI::D3D12PipelineState>(m_Device, L"VoxelizationPSO", psoDesc);

		}


	}

	void VolumetriLighting::clearAndHeightFogVoxelizationPass(RHI::RenderGraph& graph, ClearPassInputStruct& passInput, ClearPassOutputStruct& passOutput)
	{
		if (!EngineConfig::g_FogConfig.enableVolumetricFog)
		{
			return;
		}

		RHI::RgResourceHandle mShaderVariablesVolumetricHandle = graph.Import<RHI::D3D12Buffer>(pShaderVariablesVolumetric.get());
		RHI::RgResourceHandle mVBufferDensityHandle = graph.Import<RHI::D3D12Texture>(mVBufferDensity.get());

		RHI::RgResourceHandle perframeBufferHandle = passInput.perframeBufferHandle;

		RHI::RenderPass& clearPass = graph.AddRenderPass("ClearandHeightFogVoxelization");

		clearPass.Read(perframeBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
		clearPass.Read(mShaderVariablesVolumetricHandle, false, RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
		clearPass.Write(mVBufferDensityHandle, false, RHIResourceState::RHI_RESOURCE_STATE_UNORDERED_ACCESS);

		clearPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
            RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

            pContext->SetRootSignature(pVoxelizationSignature.get());
            pContext->SetPipelineState(pVoxelizationPSO.get());

            struct RootIndexBuffer
            {
				uint32_t perFrameBufferIndex;
				uint32_t shaderVariablesVolumetricIndex;
				uint32_t _VBufferDensityIndex;  // RGB = sqrt(scattering), A = sqrt(extinction)
            };

            RootIndexBuffer rootIndexBuffer = RootIndexBuffer{ RegGetBufDefCBVIdx(perframeBufferHandle),
															   RegGetBufDefCBVIdx(mShaderVariablesVolumetricHandle),
                                                               RegGetTexDefUAVIdx(mVBufferDensityHandle) };

            pContext->SetConstantArray(0, sizeof(RootIndexBuffer) / sizeof(UINT), &rootIndexBuffer);

			// The shader defines GROUP_SIZE_1D = 8.
			pContext->Dispatch((fogData.resolution.x + 7) / 8, (fogData.resolution.x + 7) / 8, 1);
        });

		passOutput.vbufferDensityHandle = mVBufferDensityHandle;
	}

	void VolumetriLighting::UpdateShaderVariablesGlobalCBFogParameters(
		HLSL::VolumetricLightingUniform& inoutVolumetricLightingUniform, HLSL::VBufferUniform inoutVBufferUniform)
	{
		inoutVolumetricLightingUniform._FogEnabled = 1;
		inoutVolumetricLightingUniform._EnableVolumetricFog = 1;

		EngineConfig::FogConfig& fog = EngineConfig::g_FogConfig;

		glm::vec4 fogColor = (fog.colorMode == EngineConfig::FogColorMode::ConstantColor) ? fog.color.color : fog.tint.color;

		inoutVolumetricLightingUniform._FogColorMode = fog.colorMode;
		inoutVolumetricLightingUniform._FogColor = fogColor;
		inoutVolumetricLightingUniform._MipFogParameters = glm::vec4(fog.mipFogNear, fog.mipFogFar, fog.mipFogMaxMip, 0.0f);




	}

}

