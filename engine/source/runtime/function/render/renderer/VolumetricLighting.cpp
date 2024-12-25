#pragma once

#include "runtime/function/render/renderer/VolumetricLighting.h"
#include "runtime/function/render/renderer/renderer_config.h"
#include "runtime/function/render/utility/HDUtils.h"
#include "pass_helper.h"

namespace MoYu
{
	struct VolumeCommandSignatureParams
	{
		uint32_t MeshIndex;
		D3D12_DRAW_ARGUMENTS DrawArguments;
	};

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
		cb._CornetteShanksConstant = CornetteShanksPhasePartConstant(GetFogVolume().anisotropy);
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
		cb._MaxVolumetricFogDistance = GetFogVolume().depthExtent;
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
			GetFogVolume());

		glm::ivec3 cvp = m_CurrentVBufferParams.viewportSize;

		m_CurrentVolumetricBufferSize = glm::ivec3(
			glm::max(m_CurrentVolumetricBufferSize.x, cvp.x),
			glm::max(m_CurrentVolumetricBufferSize.y, cvp.y),
			glm::max(m_CurrentVolumetricBufferSize.z, cvp.z));

		glm::vec4 resolution = glm::vec4(cvp.x, cvp.y, 1.0f / cvp.x, 1.0f / cvp.y);

		int sliceCount;
		float screenFraction;
		ComputeVolumetricFogSliceCountAndScreenFraction(GetFogVolume(), sliceCount, screenFraction);

		HLSL::ShaderVariablesVolumetric m_ShaderVariablesVolumetricCB;
		updateShaderVariableslVolumetrics(m_ShaderVariablesVolumetricCB, resolution, sliceCount);

		memcpy(pShaderVariablesVolumetric->GetCpuVirtualAddress<HLSL::ShaderVariablesVolumetric>(),
			&m_ShaderVariablesVolumetricCB, sizeof(m_ShaderVariablesVolumetricCB));

		fogData.resolution = resolution;
		fogData.volumetricCB = m_ShaderVariablesVolumetricCB;

		if (mVBufferDensity == nullptr)
		{
			constexpr FLOAT renderTargetClearColor[4] = { 0, 0, 0, 0 };
			auto renderTargetClearValue = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R16G16B16A16_FLOAT, renderTargetClearColor);

			mVBufferDensity = RHI::D3D12Texture::Create3D(
				m_Device->GetLinkedDevice(),
				m_CurrentVolumetricBufferSize.x,
				m_CurrentVolumetricBufferSize.y,
				m_CurrentVolumetricBufferSize.z,
				1,
				DXGI_FORMAT_R16G16B16A16_FLOAT,
				RHI::RHISurfaceCreateRenderTarget,
				1,
				L"ASScattering3D",
				D3D12_RESOURCE_STATE_COMMON,
				renderTargetClearValue);
		}

		if (mLightingBuffer == nullptr)
		{
			mLightingBuffer = RHI::D3D12Texture::Create3D(
				m_Device->GetLinkedDevice(),
				m_CurrentVolumetricBufferSize.x,
				m_CurrentVolumetricBufferSize.y,
				m_CurrentVolumetricBufferSize.z,
				1,
				DXGI_FORMAT_R16G16B16A16_FLOAT,
				RHI::RHISurfaceCreateRandomWrite,
				1,
				L"VolumeLightingBuffer",
				D3D12_RESOURCE_STATE_COMMON);
		}

		if (pFogIndirectIndexCommandBuffer == nullptr)
		{
			pFogIndirectIndexCommandBuffer = RHI::D3D12Buffer::Create(
				m_Device->GetLinkedDevice(),
				RHI::RHIBufferRandomReadWrite | RHI::RHIBufferTargetStructured | RHI::RHIBufferTargetCounter,
				MAX_VOLUMETRIC_FOG_COUNT,
				sizeof(HLSL::BitonicSortCommandSigParams),
				L"FogIndexBuffer");
		}

		if (pFogIndirectSortCommandBuffer == nullptr)
		{
			pFogIndirectSortCommandBuffer = RHI::D3D12Buffer::Create(m_Device->GetLinkedDevice(),
				RHI::RHIBufferRandomReadWrite | RHI::RHIBufferTargetStructured | RHI::RHIBufferTargetCounter,
				MAX_VOLUMETRIC_FOG_COUNT,
				sizeof(VolumeCommandSignatureParams),
				L"FogCommandSignatureBuffer");
		}

		if(pUploadVolumesDataBuffer == nullptr)
		{
			pUploadVolumesDataBuffer = RHI::D3D12Buffer::Create(
				m_Device->GetLinkedDevice(),
				RHI::RHIBufferTargetNone,
				MAX_VOLUMETRIC_FOG_COUNT,
				sizeof(HLSL::LocalVolumetricFogDatas),
				L"UploadVolumesDataBuffer",
				RHI::RHIBufferModeDynamic,
				D3D12_RESOURCE_STATE_GENERIC_READ);
		}

		if (pVolumesDataBuffer == nullptr)
		{
			pVolumesDataBuffer = RHI::D3D12Buffer::Create(
				m_Device->GetLinkedDevice(),
				RHI::RHIBufferTargetNone,
				MAX_VOLUMETRIC_FOG_COUNT,
				sizeof(HLSL::LocalVolumetricFogDatas),
				L"VolumesDataBuffer",
				RHI::RHIBufferModeImmutable,
				D3D12_RESOURCE_STATE_GENERIC_READ);
		}

		// prepare global variables
		{
			const FogVolume& fog = GetFogVolume();
			
			HLSL::VolumetricLightingUniform mVolumetricLightingUniform;
			UpdateVolumetricLightingUniform(fog, mVolumetricLightingUniform);

			HLSL::VBufferUniform mVBufferUniform;
			UpdateVolumetricLightingUniform(fog, mVBufferUniform);

			HLSL::FrameUniforms* _frameUniforms = &(render_resource->m_FrameUniforms);

			_frameUniforms->volumetricLightingUniform = mVolumetricLightingUniform;
			_frameUniforms->vBufferUniform = mVBufferUniform;
		}
		
		// prepare volume datas
		{
			HLSL::LocalVolumetricFogDatas* pUploadVolumesDatas =
				pUploadVolumesDataBuffer->GetCpuVirtualAddress<HLSL::LocalVolumetricFogDatas>();

			for (int i = 0; i < m_render_scene->m_volume_renderers.size(); ++i)
			{
				InternalVolumeFogRenderer& internalFog = m_render_scene->m_volume_renderers[i].internalSceneVolumeRenderer;
				
				OrientedBBox bounds = OrientedBBoxFromRTS(internalFog.model_matrix);
				
				HLSL::LocalVolumetricFogDatas localVolemFogData {};

				HLSL::LocalVolumetricTransform& localTransformData = localVolemFogData.localTransformData;
				localTransformData.prevObjectToWorldMatrix = localTransformData.objectToWorldMatrix;
				localTransformData.prevWorldToObjectMatrix = localTransformData.worldToObjectMatrix;
				localTransformData.objectToWorldMatrix = internalFog.model_matrix;
				localTransformData.worldToObjectMatrix = glm::inverse(internalFog.model_matrix);
				localTransformData.volumeSize = glm::float4(internalFog.ref_fog.m_Size, 0); 

				HLSL::LocalVolumetricFogEngineData& localVolumetricFogData = localVolemFogData.localFogEngineData;
				localVolumetricFogData.scattering = internalFog.ref_fog.m_SingleScatteringAlbedo;
				localVolumetricFogData.falloffMode = internalFog.ref_fog.m_FalloffMode;
				localVolumetricFogData.textureTiling = internalFog.ref_fog.m_Tilling;
				localVolumetricFogData.invertFade = internalFog.ref_fog.m_InvertBlend ? 1 : 0;
				localVolumetricFogData.textureScroll = internalFog.ref_fog.m_ScrollSpeed;
				localVolumetricFogData.rcpDistFadeLen =
					1.0f / glm::max(internalFog.ref_fog.m_DistanceFadeEnd - internalFog.ref_fog.m_DistanceFadeStart, 0.00001526f);
				localVolumetricFogData.rcpPosFaceFade = glm::float3(0.1f, 0.1f, 0.1f);
				localVolumetricFogData.endTimesRcpDistFadeLen = internalFog.ref_fog.m_DistanceFadeEnd * localVolumetricFogData.rcpDistFadeLen;
				localVolumetricFogData.rcpNegFaceFade = glm::float3(0.1f, 0.1f, 0.1f);
				localVolumetricFogData.blendingMode = 1;

				HLSL::LocalVolumetricFogTextures& localFogTexturesData = localVolemFogData.localFogTextures;
				localFogTexturesData.noise3DIndex = internalFog.ref_fog.m_NoiseImage->GetDefaultSRV()->GetIndex();
				
				HLSL::VolumetricMaterialRenderingData& volumetricFogData = localVolemFogData.volumetricRenderData;
				volumetricFogData.startSliceIndex = 0;
				volumetricFogData.sliceCount = 18;
				volumetricFogData.viewSpaceBounds = glm::float4(-1, -1, 2, 2);
				for (int i = 0; i < 8; i++)
				{
					volumetricFogData.obbVertexPositionWS[i] = glm::float4(0, 0, 0, 0);
				}

				HLSL::VolumetricMaterialDataCBuffer& volumeMaterialDataCBuffer = localVolemFogData.volumeMaterialDataCBuffer;
				volumeMaterialDataCBuffer._VolumetricMaterialObbRight = glm::float4(bounds.right, 0);
				volumeMaterialDataCBuffer._VolumetricMaterialObbUp = glm::float4(bounds.up, 0);
				volumeMaterialDataCBuffer._VolumetricMaterialObbExtents = glm::float4(bounds.extentX, bounds.extentY, bounds.extentZ, 0);
				volumeMaterialDataCBuffer._VolumetricMaterialObbCenter = glm::float4(bounds.center, 0);
				volumeMaterialDataCBuffer._VolumetricMaterialRcpPosFaceFade = glm::float4(0.1f, 0.1f, 0.1f, 0);
				volumeMaterialDataCBuffer._VolumetricMaterialRcpNegFaceFade = glm::float4(0.1f, 0.1f, 0.1f, 0);
				volumeMaterialDataCBuffer._VolumetricMaterialInvertFade = internalFog.ref_fog.m_InvertBlend ? 1 : 0;
				volumeMaterialDataCBuffer._VolumetricMaterialRcpDistFadeLen =
					1.0f / glm::max(internalFog.ref_fog.m_DistanceFadeEnd - internalFog.ref_fog.m_DistanceFadeStart, 0.00001526f);
				volumeMaterialDataCBuffer._VolumetricMaterialEndTimesRcpDistFadeLen =
					internalFog.ref_fog.m_DistanceFadeEnd * localVolumetricFogData.rcpDistFadeLen;
				volumeMaterialDataCBuffer._VolumetricMaterialFalloffMode = internalFog.ref_fog.m_FalloffMode;

				HLSL::LocalFogCustomData& localFogCustomData = localVolemFogData.localFogCustomData;
				localFogCustomData._FogVolumeBlendMode = LOCALVOLUMETRICFOGBLENDINGMODE_ADDITIVE;
				localFogCustomData._FogVolumeSingleScatteringAlbedo = glm::float4(1,1,1,1);
				localFogCustomData._FogVolumeFogDistanceProperty = 2;
				
				pUploadVolumesDatas[i] = localVolemFogData;
			}

			volumeCounts = m_render_scene->m_volume_renderers.size();
		}

		{

		}
	}

	void VolumetriLighting::initialize(const VolumetriLightingInitInfo& init_info)
	{
		colorTexDesc = init_info.colorTexDesc;

		maxZ8xBufferDesc = RHI::RgTextureDesc("MaxZ mask 8x").
			SetExtent(colorTexDesc.Width * 0.125f, colorTexDesc.Height * 0.125f).
			SetFormat(DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT).
			SetAllowUnorderedAccess();

		maxZBufferDesc = RHI::RgTextureDesc("MaxZ mask").
			SetExtent(colorTexDesc.Width * 0.125f, colorTexDesc.Height * 0.125f).
			SetFormat(DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT).
			SetAllowUnorderedAccess();

		dilatedMaxZBufferDesc = RHI::RgTextureDesc("Dilated MaxZ mask").
			SetExtent(colorTexDesc.Width / 16.0f, colorTexDesc.Height / 16.0f).
			SetFormat(DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT).
			SetAllowUnorderedAccess();

		ShaderCompiler* m_ShaderCompiler = init_info.m_ShaderCompiler;
		std::filesystem::path m_ShaderRootPath = init_info.m_ShaderRootPath;

		{
			ShaderCompileOptions ShaderCompileOptions(L"ComputeMaxZ");
			ShaderCompileOptions.SetDefine(L"MAX_Z_DOWNSAMPLE", L"1");

			mMaxZCS = m_ShaderCompiler->CompileShader(
				RHI_SHADER_TYPE::Compute, m_ShaderRootPath / "pipeline/Runtime/RenderPipeline/RenderPass/GenerateMaxZCS.hlsl", ShaderCompileOptions);

			RHI::RootSignatureDesc rootSigDesc =
				RHI::RootSignatureDesc()
				.Add32BitConstants<0, 0>(16)
				.AllowResourceDescriptorHeapIndexing()
				.AllowSampleDescriptorHeapIndexing();

			pMaxZSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);

			struct PsoStream
			{
				PipelineStateStreamRootSignature RootSignature;
				PipelineStateStreamCS            CS;
			} psoStream;
			psoStream.RootSignature = PipelineStateStreamRootSignature(pMaxZSignature.get());
			psoStream.CS = &mMaxZCS;
			PipelineStateStreamDesc psoDesc = { sizeof(PsoStream), &psoStream };

			pMaxZPSO = std::make_shared<RHI::D3D12PipelineState>(m_Device, L"MaxZPSO", psoDesc);
		}
		{
			ShaderCompileOptions ShaderCompileOptions(L"DilateMask");
			ShaderCompileOptions.SetDefine(L"DILATE_MASK", L"1");

			mDilateMaxZCS = m_ShaderCompiler->CompileShader(
				RHI_SHADER_TYPE::Compute, m_ShaderRootPath / "pipeline/Runtime/RenderPipeline/RenderPass/GenerateMaxZCS.hlsl", ShaderCompileOptions);

			RHI::RootSignatureDesc rootSigDesc =
				RHI::RootSignatureDesc()
				.Add32BitConstants<0, 0>(5)
				.AddDescriptorTable(RHI::D3D12DescriptorTable(1).AddSRVRange<0, 0>(1, D3D12_DESCRIPTOR_RANGE_FLAG_NONE, 0))
				.AddDescriptorTable(RHI::D3D12DescriptorTable(1).AddUAVRange<0, 0>(1, D3D12_DESCRIPTOR_RANGE_FLAG_NONE, 0))
				.AllowResourceDescriptorHeapIndexing()
				.AllowSampleDescriptorHeapIndexing();

			pDilateMaxZSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);

			struct PsoStream
			{
				PipelineStateStreamRootSignature RootSignature;
				PipelineStateStreamCS            CS;
			} psoStream;
			psoStream.RootSignature = PipelineStateStreamRootSignature(pDilateMaxZSignature.get());
			psoStream.CS = &mDilateMaxZCS;
			PipelineStateStreamDesc psoDesc = { sizeof(PsoStream), &psoStream };

			pDilateMaxZPSO = std::make_shared<RHI::D3D12PipelineState>(m_Device, L"DilateMaxZPSO", psoDesc);
		}
		{
			ShaderCompileOptions ShaderCompileOptions(L"ComputeFinalMask");
			ShaderCompileOptions.SetDefine(L"FINAL_MASK", L"1");

			mMaxZDownsampleCS = m_ShaderCompiler->CompileShader(
				RHI_SHADER_TYPE::Compute, m_ShaderRootPath / "pipeline/Runtime/RenderPipeline/RenderPass/GenerateMaxZCS.hlsl", ShaderCompileOptions);

			RHI::RootSignatureDesc rootSigDesc =
				RHI::RootSignatureDesc()
				.Add32BitConstants<0, 0>(5)
				.AddDescriptorTable(RHI::D3D12DescriptorTable(1).AddSRVRange<0, 0>(1, D3D12_DESCRIPTOR_RANGE_FLAG_NONE, 0))
				.AddDescriptorTable(RHI::D3D12DescriptorTable(1).AddUAVRange<0, 0>(1, D3D12_DESCRIPTOR_RANGE_FLAG_NONE, 0))
				.AllowResourceDescriptorHeapIndexing()
				.AllowSampleDescriptorHeapIndexing();

			pMaxZDownsampleSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);

			struct PsoStream
			{
				PipelineStateStreamRootSignature RootSignature;
				PipelineStateStreamCS            CS;
			} psoStream;
			psoStream.RootSignature = PipelineStateStreamRootSignature(pMaxZDownsampleSignature.get());
			psoStream.CS = &mMaxZDownsampleCS;
			PipelineStateStreamDesc psoDesc = { sizeof(PsoStream), &psoStream };

			pMaxZDownsamplePSO = std::make_shared<RHI::D3D12PipelineState>(m_Device, L"MaxZDownsamplePSO", psoDesc);
		}
		
		{
			mVoxelizationCS = m_ShaderCompiler->CompileShader(
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
			psoStream.CS = &mVoxelizationCS;
			PipelineStateStreamDesc psoDesc = { sizeof(PsoStream), &psoStream };

			pVoxelizationPSO = std::make_shared<RHI::D3D12PipelineState>(m_Device, L"VoxelizationPSO", psoDesc);

		}

		{
			mVolumetricLightingCS = m_ShaderCompiler->CompileShader(
				RHI_SHADER_TYPE::Compute, m_ShaderRootPath / "pipeline/Runtime/Tools/VolumeLighting/VolumetricLightingCS.hlsl", ShaderCompileOptions(L"VolumetricLighting"));

			RHI::RootSignatureDesc rootSigDesc =
				RHI::RootSignatureDesc()
				.Add32BitConstants<0, 0>(16)
				.AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 8)
				.AddStaticSampler<11, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 8)
				.AddStaticSampler<12, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP, 8)
				.AddStaticSampler<13, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 8)
				.AddStaticSampler<14, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP, 8)
				.AddStaticSampler<15, 0>(D3D12_FILTER::D3D12_FILTER_COMPARISON_ANISOTROPIC,
					D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
					8,
					D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_GREATER_EQUAL,
					D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK)
				.AllowResourceDescriptorHeapIndexing()
				.AllowSampleDescriptorHeapIndexing();

			pVolumetricLightingSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);

			struct PsoStream
			{
				PipelineStateStreamRootSignature RootSignature;
				PipelineStateStreamCS            CS;
			} psoStream;
			psoStream.RootSignature = PipelineStateStreamRootSignature(pVolumetricLightingSignature.get());
			psoStream.CS = &mVolumetricLightingCS;
			PipelineStateStreamDesc psoDesc = { sizeof(PsoStream), &psoStream };

			pVolumetricLightingPSO = std::make_shared<RHI::D3D12PipelineState>(m_Device, L"VolumetricLightingPSO", psoDesc);
		}

		{
			mVolumetricLightingFilteringCS = m_ShaderCompiler->CompileShader(
				RHI_SHADER_TYPE::Compute, m_ShaderRootPath / "pipeline/Runtime/Tools/VolumeLighting/VolumetricLightingFilteringCS.hlsl", 
				ShaderCompileOptions(L"FilterVolumetricLighting"));

			RHI::RootSignatureDesc rootSigDesc =
				RHI::RootSignatureDesc()
				.AddConstantBufferView<0, 0>()
				.AddDescriptorTable(RHI::D3D12DescriptorTable(1).AddUAVRange<0, 0>(1, D3D12_DESCRIPTOR_RANGE_FLAG_NONE, 0))
				.AllowResourceDescriptorHeapIndexing()
				.AllowSampleDescriptorHeapIndexing();

			pVolumetricLightingFilteringSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);

			struct PsoStream
			{
				PipelineStateStreamRootSignature RootSignature;
				PipelineStateStreamCS            CS;
			} psoStream;
			psoStream.RootSignature = PipelineStateStreamRootSignature(pVolumetricLightingFilteringSignature.get());
			psoStream.CS = &mVolumetricLightingFilteringCS;
			PipelineStateStreamDesc psoDesc = { sizeof(PsoStream), &psoStream };

			pVolumetricLightingFilteringPSO = std::make_shared<RHI::D3D12PipelineState>(m_Device, L"VolumetricLightingFilteringPSO", psoDesc);
		}

		{
			mVolumeIndirectCullForSortCS = m_ShaderCompiler->CompileShader(
				RHI_SHADER_TYPE::Compute, m_ShaderRootPath / "pipeline/Runtime/Tools/Culling/VolumeIndirectCullForSort.hlsl",
				ShaderCompileOptions(L"CSMain"));

			RHI::RootSignatureDesc rootSigDesc =
				RHI::RootSignatureDesc()
				.Add32BitConstants<0, 0>(1)
				.AddConstantBufferView<1, 0>()
				.AddDescriptorTable(RHI::D3D12DescriptorTable(1).AddSRVRange<0, 0>(1, D3D12_DESCRIPTOR_RANGE_FLAG_NONE, 0))
				.AddDescriptorTable(RHI::D3D12DescriptorTable(1).AddUAVRange<0, 0>(1, D3D12_DESCRIPTOR_RANGE_FLAG_NONE, 0))
				.AllowResourceDescriptorHeapIndexing()
				.AllowSampleDescriptorHeapIndexing();

			pVolumeIndirectCullForSortSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);

			struct PsoStream
			{
				PipelineStateStreamRootSignature RootSignature;
				PipelineStateStreamCS            CS;
			} psoStream;
			psoStream.RootSignature = PipelineStateStreamRootSignature(pVolumeIndirectCullForSortSignature.get());
			psoStream.CS = &mVolumeIndirectCullForSortCS;
			PipelineStateStreamDesc psoDesc = { sizeof(PsoStream), &psoStream };

			pVolumeIndirectCullForSortPSO = std::make_shared<RHI::D3D12PipelineState>(m_Device, L"VolumeIndirectCullForSortPSO", psoDesc);
		}

		{
			mVolumeIndirectGrabCS = m_ShaderCompiler->CompileShader(
				RHI_SHADER_TYPE::Compute, m_ShaderRootPath / "pipeline/Runtime/Tools/Culling/VolumeIndirectGrabCS.hlsl",
				ShaderCompileOptions(L"CSMain"));

			RHI::RootSignatureDesc rootSigDesc = RHI::RootSignatureDesc()
				.AddShaderResourceView<0, 0>()
				.AddShaderResourceView<1, 0>()
				.AddShaderResourceView<2, 0>()
				.AddUnorderedAccessViewWithCounter<0, 0>()
				.AllowResourceDescriptorHeapIndexing()
				.AllowSampleDescriptorHeapIndexing();

			pVolumeIndirectGrabSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);

			struct PsoStream
			{
				PipelineStateStreamRootSignature RootSignature;
				PipelineStateStreamCS            CS;
			} psoStream;
			psoStream.RootSignature = PipelineStateStreamRootSignature(pVolumeIndirectGrabSignature.get());
			psoStream.CS = &mVolumeIndirectGrabCS;
			PipelineStateStreamDesc psoDesc = { sizeof(PsoStream), &psoStream };

			pVolumeIndirectGrabPSO = std::make_shared<RHI::D3D12PipelineState>(m_Device, L"VolumeIndirectGrabPSO", psoDesc);
		}

		{
			sortDispatchArgsBufferDesc = RHI::RgBufferDesc("VolumeSortDispatchArgs")
				.SetSize(22 * 23 / 2, sizeof(D3D12_DISPATCH_ARGUMENTS))
				.SetRHIBufferMode(RHI::RHIBufferMode::RHIBufferModeImmutable)
				.SetRHIBufferTarget(
					RHI::RHIBufferTarget::RHIBufferTargetIndirectArgs | 
					RHI::RHIBufferTarget::RHIBufferRandomReadWrite | 
					RHI::RHIBufferTarget::RHIBufferTargetRaw);
		}
		{
			grabDispatchArgsBufferDesc = RHI::RgBufferDesc("VolumeGrabDispatchArgs")
				.SetSize(22 * 23 / 2, sizeof(D3D12_DISPATCH_ARGUMENTS))
				.SetRHIBufferMode(RHI::RHIBufferMode::RHIBufferModeImmutable)
				.SetRHIBufferTarget(
					RHI::RHIBufferTarget::RHIBufferTargetIndirectArgs |
					RHI::RHIBufferTarget::RHIBufferRandomReadWrite |
					RHI::RHIBufferTarget::RHIBufferTargetRaw);
		}

		{
			indirectDrawVolumeVS = m_ShaderCompiler->CompileShader(
				RHI_SHADER_TYPE::Vertex, m_ShaderRootPath / "pipeline/Runtime/Tools/VolumeLighting/VolumeDefaultShader.hlsl", ShaderCompileOptions(L"Vert"));
			indirectDrawVolumePS = m_ShaderCompiler->CompileShader(
				RHI_SHADER_TYPE::Pixel, m_ShaderRootPath / "pipeline/Runtime/Tools/VolumeLighting/VolumeDefaultShader.hlsl", ShaderCompileOptions(L"Frag"));

			{
				RHI::RootSignatureDesc rootSigDesc =
					RHI::RootSignatureDesc()
					.Add32BitConstants<0, 0>(1)
					//.AddConstantBufferView<1, 0>()
					//.AddConstantBufferView<2, 0>()
					.AddDescriptorTable(RHI::D3D12DescriptorTable(1).AddCBVRange<1, 0>(2, D3D12_DESCRIPTOR_RANGE_FLAG_NONE, 0))
					.AddDescriptorTable(RHI::D3D12DescriptorTable(1).AddSRVRange<0, 0>(1, D3D12_DESCRIPTOR_RANGE_FLAG_NONE, 0))
					.AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 8)
					.AddStaticSampler<11, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 8)
					.AddStaticSampler<12, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP, 8)
					.AddStaticSampler<13, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 8)
					.AddStaticSampler<14, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP, 8)
					.AllowInputLayout()
					.AllowResourceDescriptorHeapIndexing()
					.AllowSampleDescriptorHeapIndexing();

				pIndirectDrawVolumeSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);
			}
			{
				RHI::CommandSignatureDesc Builder(4);
				Builder.AddConstant(0, 0, 1);
				Builder.AddDraw();

				pIndirectDrawVolumeCommandSignature = std::make_shared<RHI::D3D12CommandSignature>(
					m_Device, Builder, pIndirectDrawVolumeSignature->GetApiHandle());
			}
			{
				RHI::D3D12InputLayout InputLayout = {};

				RHIDepthStencilState DepthStencilState;
				DepthStencilState.DepthEnable = false;
				DepthStencilState.DepthWrite = false;
				DepthStencilState.DepthFunc = RHI_COMPARISON_FUNC::GreaterEqual;

				RHIRenderTargetState RenderTargetState;
				RenderTargetState.RTFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT; // DXGI_FORMAT_R32G32B32A32_FLOAT;
				RenderTargetState.NumRenderTargets = 1;
				RenderTargetState.DSFormat = DXGI_FORMAT_D32_FLOAT; // DXGI_FORMAT_D32_FLOAT;


				RHIRasterizerState rasterizerState = RHIRasterizerState();
				rasterizerState.CullMode = RHI_CULL_MODE::Back;

				RHISampleState SampleState;
				SampleState.Count = 1;

				struct PsoStream
				{
					PipelineStateStreamRootSignature     RootSignature;
					PipelineStateStreamInputLayout       InputLayout;
					PipelineStateStreamPrimitiveTopology PrimitiveTopologyType;
					PipelineStateStreamRasterizerState   RasterrizerState;
					PipelineStateStreamVS                VS;
					PipelineStateStreamPS                PS;
					PipelineStateStreamDepthStencilState DepthStencilState;
					PipelineStateStreamRenderTargetState RenderTargetState;
					PipelineStateStreamSampleState       SampleState;
				} psoStream;
				psoStream.RootSignature = PipelineStateStreamRootSignature(pIndirectDrawVolumeSignature.get());
				psoStream.InputLayout = &InputLayout;
				psoStream.PrimitiveTopologyType = RHI_PRIMITIVE_TOPOLOGY::Triangle;
				psoStream.RasterrizerState = rasterizerState;
				psoStream.VS = &indirectDrawVolumeVS;
				psoStream.PS = &indirectDrawVolumePS;
				psoStream.DepthStencilState = DepthStencilState;
				psoStream.RenderTargetState = RenderTargetState;
				psoStream.SampleState = SampleState;

				PipelineStateStreamDesc psoDesc = { sizeof(PsoStream), &psoStream };
				pIndirectDrawVolumePSO = std::make_shared<RHI::D3D12PipelineState>(m_Device, L"IndirectVolumeDraw", psoDesc);
			}
		}

	}

	void VolumetriLighting::GenerateMaxZForVolumetricPass(RHI::RenderGraph& graph, GenMaxZInputStruct& passInput, GenMaxZOutputStruct& passOutput)
	{
		struct GenerateMaxZMaskPassData
		{
			glm::ivec2 intermediateMaskSize;
			glm::ivec2 finalMaskSize;
			float dilationWidth;
		};
		GenerateMaxZMaskPassData passData;

		passData.intermediateMaskSize.x = HDUtils::DivRoundUp(colorTexDesc.Width, 8);
		passData.intermediateMaskSize.y = HDUtils::DivRoundUp(colorTexDesc.Height, 8);

		passData.finalMaskSize.x = passData.intermediateMaskSize.x / 2;
		passData.finalMaskSize.y = passData.intermediateMaskSize.y / 2;

		passData.dilationWidth = 1;

		RHI::RgResourceHandle perframeBufferHandle = passInput.perframeBufferHandle;
		RHI::RgResourceHandle depthHandle = passInput.depthHandle;

		RHI::RgResourceHandle maxZ8xBufferHandle = graph.Create<RHI::D3D12Texture>(maxZ8xBufferDesc);
		RHI::RgResourceHandle maxZBufferHandle = graph.Create<RHI::D3D12Texture>(maxZBufferDesc);
		RHI::RgResourceHandle dilatedMaxZBufferHandle = graph.Create<RHI::D3D12Texture>(dilatedMaxZBufferDesc);

		RHI::RenderPass& genMaxZPass = graph.AddRenderPass("Generate Max Z Mask for Volumetric");

		genMaxZPass.Read(perframeBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
		genMaxZPass.Read(depthHandle, false, RHIResourceState::RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		genMaxZPass.Write(maxZ8xBufferHandle, true);
		genMaxZPass.Write(maxZBufferHandle, true);
		genMaxZPass.Write(dilatedMaxZBufferHandle, true);

		genMaxZPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
			RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

			// Downsample 8x8 with max operator

			pContext->TransitionBarrier(RegGetTex(maxZ8xBufferHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			pContext->FlushResourceBarriers();

			int maskW = passData.intermediateMaskSize.x;
			int maskH = passData.intermediateMaskSize.y;

			int dispatchX = maskW;
			int dispatchY = maskH;

			pContext->SetRootSignature(pMaxZSignature.get());
			pContext->SetPipelineState(pMaxZPSO.get());

			struct RootIndexBuffer
			{
				uint32_t perFrameBufferIndex;
				uint32_t _DepthBufferIndex;
				uint32_t _OutputTextureIndex;
			};

			RootIndexBuffer rootIndexBuffer = RootIndexBuffer{ RegGetBufDefCBVIdx(perframeBufferHandle),
															   RegGetTexDefSRVIdx(depthHandle),
															   RegGetTexDefUAVIdx(maxZ8xBufferHandle) };

			pContext->SetConstantArray(0, sizeof(RootIndexBuffer) / sizeof(std::uint32_t), &rootIndexBuffer);

			pContext->Dispatch(dispatchX, dispatchY, 1);

			// Downsample to 16x16

			pContext->TransitionBarrier(RegGetTex(maxZ8xBufferHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			pContext->TransitionBarrier(RegGetTex(maxZBufferHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			pContext->FlushResourceBarriers();

			pContext->SetRootSignature(pMaxZDownsampleSignature.get());
			pContext->SetPipelineState(pMaxZDownsamplePSO.get());

			struct Root0Constants
			{
				glm::float4 _SrcOffsetAndLimit;
				float _DilationWidth;
			};
			Root0Constants root0Const = { glm::float4(maskW, maskH, 0, 0), passData.dilationWidth };

			pContext->SetConstantArray(0, sizeof(root0Const)/sizeof(std::uint32_t), &root0Const);
			pContext->SetDynamicDescriptor(1, 0, RegGetTex(maxZ8xBufferHandle)->GetDefaultSRV(1)->GetCpuHandle());
			pContext->SetDynamicDescriptor(2, 0, RegGetTex(maxZBufferHandle)->GetDefaultUAV(1)->GetCpuHandle());

			int finalMaskW = glm::ceil(maskW / 2.0f);
			int finalMaskH = glm::ceil(maskH / 2.0f);

			dispatchX = HDUtils::DivRoundUp(finalMaskW, 8);
			dispatchY = HDUtils::DivRoundUp(finalMaskH, 8);

			pContext->Dispatch(dispatchX, dispatchY, 1);

			// Dilate max Z

			pContext->TransitionBarrier(RegGetTex(maxZBufferHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			pContext->TransitionBarrier(RegGetTex(dilatedMaxZBufferHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			pContext->FlushResourceBarriers();

			pContext->SetRootSignature(pDilateMaxZSignature.get());
			pContext->SetPipelineState(pDilateMaxZPSO.get());

			root0Const._SrcOffsetAndLimit.x = finalMaskW;
			root0Const._SrcOffsetAndLimit.y = finalMaskW;

			pContext->SetConstantArray(0, sizeof(root0Const) / sizeof(std::uint32_t), &root0Const);
			pContext->SetDynamicDescriptor(1, 0, RegGetTex(maxZBufferHandle)->GetDefaultSRV(1)->GetCpuHandle());
			pContext->SetDynamicDescriptor(2, 0, RegGetTex(dilatedMaxZBufferHandle)->GetDefaultUAV(1)->GetCpuHandle());

			pContext->Dispatch(dispatchX, dispatchY, 1);
		});

		passOutput.maxZ8xBufferHandle = maxZ8xBufferHandle;
		passOutput.maxZBufferHandle = maxZBufferHandle;
		passOutput.dilatedMaxZBufferHandle = dilatedMaxZBufferHandle;
	}
	
	void VolumetriLighting::FogVolumeAndVFXVoxelizationPass(RHI::RenderGraph& graph, ClearPassInputStruct& passInput, ClearPassOutputStruct& passOutput)
	{
		//RHI::RgResourceHandle mShaderVariablesVolumetricHandle = graph.Import<RHI::D3D12Buffer>(pShaderVariablesVolumetric.get());
		RHI::RgResourceHandle mShaderVariablesVolumetricHandle = passInput.shaderVariablesVolumetricHandle;
		RHI::RgResourceHandle mVBufferDensityHandle = passInput.vBufferDensityHandle;

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
		passOutput.shaderVariablesVolumetricHandle = mShaderVariablesVolumetricHandle;
	}

	void VolumetriLighting::VolumetricLightingPass(RHI::RenderGraph& graph, VolumeLightPassInputStruct& passInput, VolumeLightPassOutputStruct& passOutput)
	{
		//RHI::RgResourceHandle mShaderVariablesVolumetricHandle = graph.Import<RHI::D3D12Buffer>(pShaderVariablesVolumetric.get());
		RHI::RgResourceHandle mLightBufferHandle = graph.Import<RHI::D3D12Texture>(mLightingBuffer.get());
		
		RHI::RgResourceHandle perframeBufferHandle = passInput.perframeBufferHandle;
		RHI::RgResourceHandle vbufferDensityHandle = passInput.vbufferDensityHandle;
		RHI::RgResourceHandle depthBufferHandle = passInput.depthBufferHandle;
		RHI::RgResourceHandle shaderVariablesVolumetricHandle = passInput.shaderVariablesVolumetricHandle;
		RHI::RgResourceHandle dilatedMaxZBufferHandle = passInput.dilatedMaxZBufferHandle;

		const RHI::RgResourceHandle& directionalCascadeShadowmapHandle = passInput.directionalCascadeShadowmapHandle;
		const std::vector<RHI::RgResourceHandle>& spotShadowmapHandles = passInput.spotShadowmapHandles;

		RHI::RenderPass& volumeLightingPass = graph.AddRenderPass("Volumetric Lighting");

		volumeLightingPass.Read(directionalCascadeShadowmapHandle, false, RHIResourceState::RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		for (int i = 0; i < spotShadowmapHandles.size(); i++)
		{
			volumeLightingPass.Read(spotShadowmapHandles[i], false, RHIResourceState::RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		}
		volumeLightingPass.Read(perframeBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
		volumeLightingPass.Read(shaderVariablesVolumetricHandle, false, RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
		volumeLightingPass.Read(vbufferDensityHandle, false, RHIResourceState::RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		volumeLightingPass.Read(depthBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		volumeLightingPass.Read(dilatedMaxZBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		volumeLightingPass.Write(mLightBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_UNORDERED_ACCESS);

		volumeLightingPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
			RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

			pContext->SetRootSignature(pVolumetricLightingSignature.get());
			pContext->SetPipelineState(pVolumetricLightingPSO.get());

			struct RootIndexBuffer
			{
				uint32_t perFrameBufferIndex;
				uint32_t shaderVariablesVolumetricIndex;
				uint32_t _VBufferDensityIndex;  // RGB = sqrt(scattering), A = sqrt(extinction)
				uint32_t _DepthBufferIndex;
				uint32_t _MaxZMaskTextureIndex;
				uint32_t _VBufferLightingIndex;
			};

			RootIndexBuffer rootIndexBuffer = RootIndexBuffer{ RegGetBufDefCBVIdx(perframeBufferHandle),
															   RegGetBufDefCBVIdx(shaderVariablesVolumetricHandle),
															   RegGetTexDefSRVIdx(vbufferDensityHandle),
															   RegGetTexDefSRVIdx(depthBufferHandle),
															   RegGetTexDefSRVIdx(dilatedMaxZBufferHandle),
															   RegGetTexDefUAVIdx(mLightBufferHandle) };

			pContext->SetConstantArray(0, sizeof(RootIndexBuffer) / sizeof(UINT), &rootIndexBuffer);

			// The shader defines GROUP_SIZE_1D = 8.
			pContext->Dispatch((fogData.resolution.x + 7) / 8, (fogData.resolution.y + 7) / 8, 1);


			// VolumetricLightingFiltering
			pContext->SetRootSignature(pVolumetricLightingFilteringSignature.get());
			pContext->SetPipelineState(pVolumetricLightingFilteringPSO.get());

			pContext->SetConstantBuffer(0, RegGetBuf(perframeBufferHandle)->GetGpuVirtualAddress());
			pContext->SetDynamicDescriptor(1, 0, RegGetTex(mLightBufferHandle)->GetDefaultUAV(1)->GetCpuHandle());

			pContext->Dispatch((fogData.resolution.x + 7) / 8, (fogData.resolution.y + 7) / 8, 1);
		});

		passOutput.vbufferLightingHandle = mLightBufferHandle;
	}

	void VolumetriLighting::UpdateVolumetricLightingUniform(const FogVolume& fog, HLSL::VolumetricLightingUniform& inoutVolumetricLightingUniform)
	{
		inoutVolumetricLightingUniform._FogEnabled = 1;
		inoutVolumetricLightingUniform._EnableVolumetricFog = 1;

		// const FogVolume& fog = GetFogVolume();

		glm::vec4 fogColor = (fog.colorMode == FogColorMode::ConstantColor) ? fog.color.color : fog.tint.color;

		inoutVolumetricLightingUniform._FogColorMode = (int)fog.colorMode;
		inoutVolumetricLightingUniform._FogColor = fogColor;
		inoutVolumetricLightingUniform._MipFogParameters = glm::vec4(fog.mipFogNear, fog.mipFogFar, fog.mipFogMaxMip, 0.0f);

		LocalVolumetricFogArtistParameters param(fog.albedo.color, fog.meanFreePath, fog.anisotropy);
		HLSL::LocalVolumetricFogEngineData data = param.ConvertToEngineData();

		// When volumetric fog is disabled, we don't want its color to affect the heightfog. So we pass neutral values here.
		float extinction = VolumeRenderingUtils::ExtinctionFromMeanFreePath(param.meanFreePath);
		inoutVolumetricLightingUniform._HeightFogBaseScattering = glm::vec4(data.scattering, 1.0f);
		inoutVolumetricLightingUniform._HeightFogBaseExtinction = extinction;

		float crBaseHeight = fog.baseHeight;

		float layerDepth = glm::max(0.01f, fog.maximumHeight - fog.baseHeight);
		float H = ScaleHeightFromLayerDepth(layerDepth);
		inoutVolumetricLightingUniform._HeightFogExponents = glm::vec2(1.0f / H, H);
		inoutVolumetricLightingUniform._HeightFogBaseHeight = crBaseHeight;
		inoutVolumetricLightingUniform._GlobalFogAnisotropy = fog.anisotropy;
		inoutVolumetricLightingUniform._VolumetricFilteringEnabled = ((int)fog.denoisingMode & (int)FogDenoisingMode::Gaussian) != 0 ? 1 : 0;
		inoutVolumetricLightingUniform._FogDirectionalOnly = fog.directionalLightsOnly ? 1 : 0;
	}

	void VolumetriLighting::UpdateVolumetricLightingUniform(const FogVolume& fog, HLSL::VBufferUniform& inoutVBufferUniform)
	{
		// const FogVolume& fog = GetFogVolume();

		VBufferParameters& currParams = m_CurrentVBufferParams;

		// The lighting & density buffers are shared by all cameras.
		// The history & feedback buffers are specific to the camera.
		// These 2 types of buffers can have different sizes.
		// Additionally, history buffers can have different sizes, since they are not resized at the same time.
		glm::ivec3 cvp = currParams.viewportSize;

		// Adjust slices for XR rendering: VBuffer is shared for all single-pass views
		glm::uint sliceCount = (glm::uint)cvp.z;

		inoutVBufferUniform._VBufferViewportSize = glm::vec4(cvp.x, cvp.y, 1.0f / cvp.x, 1.0f / cvp.y);
		inoutVBufferUniform._VBufferSliceCount = sliceCount;
		inoutVBufferUniform._VBufferRcpSliceCount = 1.0f / sliceCount;
		inoutVBufferUniform._VBufferLightingViewportScale = glm::vec4(currParams.ComputeViewportScale(m_CurrentVolumetricBufferSize), 1.0f);
		inoutVBufferUniform._VBufferLightingViewportLimit = glm::vec4(currParams.ComputeViewportLimit(m_CurrentVolumetricBufferSize), 1.0f);
		inoutVBufferUniform._VBufferDistanceEncodingParams = currParams.depthEncodingParams;
		inoutVBufferUniform._VBufferDistanceDecodingParams = currParams.depthDecodingParams;
		inoutVBufferUniform._VBufferLastSliceDist = currParams.ComputeLastSliceDistance(sliceCount);
		inoutVBufferUniform._VBufferRcpInstancedViewCount = 1.0f;
	}

	void VolumetriLighting::prepareForVolumes(RHI::RenderGraph& graph, VolumeCullingPassInputStruct& passInput, VolumeCullingPassOutputStruct& passOutput)
	{
		RHI::RgResourceHandle perframeBufferHandle = passInput.perframeBufferHandle;

		RHI::RgResourceHandle mShaderVariablesVolumetricHandle = graph.Import<RHI::D3D12Buffer>(pShaderVariablesVolumetric.get());
		
		RHI::RgResourceHandle indirectFogIndexBufferHandle = GImport(graph, pFogIndirectIndexCommandBuffer.get());
		RHI::RgResourceHandle indirectFogSortCommandBufferHandle = GImport(graph, pFogIndirectSortCommandBuffer.get());
		
		RHI::RgResourceHandle uploadVolumesDataBufferHandle = GImport(graph, pUploadVolumesDataBuffer.get());
		RHI::RgResourceHandle volumesDataBufferHandle = GImport(graph, pVolumesDataBuffer.get());

		RHI::RenderPass& resetPass = graph.AddRenderPass("ResetVolumeDataPass");

		resetPass.Read(uploadVolumesDataBufferHandle, true);
		resetPass.Write(volumesDataBufferHandle, true);
		resetPass.Write(indirectFogIndexBufferHandle, true);

		resetPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
			RHI::D3D12ComputeContext* pCopyContext = context->GetComputeContext();

			pCopyContext->TransitionBarrier(RegGetBuf(volumesDataBufferHandle), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);
			pCopyContext->TransitionBarrier(RegGetBufCounter(indirectFogIndexBufferHandle), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);
			pCopyContext->ResetCounter(RegGetBufCounter(indirectFogIndexBufferHandle));

			pCopyContext->FlushResourceBarriers();

			pCopyContext->CopyBuffer(RegGetBuf(volumesDataBufferHandle), RegGetBuf(uploadVolumesDataBufferHandle));
		});

		RHI::RenderPass& cullingPass = graph.AddRenderPass("VolumesCullingPass");

		cullingPass.Read(perframeBufferHandle, true);
		cullingPass.Read(volumesDataBufferHandle, true);
		
		cullingPass.Write(indirectFogIndexBufferHandle, true);

		cullingPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
			RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

			pContext->TransitionBarrier(RegGetBuf(perframeBufferHandle), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
			pContext->TransitionBarrier(RegGetBuf(volumesDataBufferHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			pContext->TransitionBarrier(RegGetBufCounter(indirectFogIndexBufferHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			pContext->TransitionBarrier(RegGetBuf(indirectFogIndexBufferHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			pContext->FlushResourceBarriers();

			pContext->SetRootSignature(pVolumeIndirectCullForSortSignature.get());
			pContext->SetPipelineState(pVolumeIndirectCullForSortPSO.get());

			pContext->SetConstant(0, 0, volumeCounts);
			pContext->SetConstantBuffer(1, RegGetBuf(perframeBufferHandle)->GetGpuVirtualAddress());
			pContext->SetDynamicDescriptor(2, 0, RegGetBuf(volumesDataBufferHandle)->GetDefaultSRV(1)->GetCpuHandle());
			pContext->SetDynamicDescriptor(3, 0, RegGetBuf(indirectFogIndexBufferHandle)->GetDefaultUAV(1)->GetCpuHandle());

			pContext->Dispatch1D(volumeCounts, 128);
		});

		RHI::RgResourceHandle sortDispatchArgsHandle = graph.Create<RHI::D3D12Buffer>(sortDispatchArgsBufferDesc);		
		{
			RHI::RenderPass& volumeSortPass = graph.AddRenderPass("VolumeBitonicSortPass");

			volumeSortPass.Read(indirectFogIndexBufferHandle, true);

			volumeSortPass.Write(sortDispatchArgsHandle, true);
			volumeSortPass.Write(indirectFogIndexBufferHandle, true);

			volumeSortPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
				RHI::D3D12ComputeContext* pAsyncCompute = context->GetComputeContext();

				RHI::D3D12Buffer* bufferPtr = RegGetBuf(indirectFogIndexBufferHandle);
				RHI::D3D12Buffer* bufferCouterPtr = bufferPtr->GetCounterBuffer().get();
				RHI::D3D12Buffer* sortDispatchArgsPtr = RegGetBuf(sortDispatchArgsHandle);

				bitonicSort(pAsyncCompute, bufferPtr, bufferCouterPtr, sortDispatchArgsPtr, false, false);
			});
		}

		RHI::RgResourceHandle grabDispatchArgsHandle = graph.Create<RHI::D3D12Buffer>(grabDispatchArgsBufferDesc);
		{
			RHI::RenderPass& grabVolumePass = graph.AddRenderPass("GrabVolumePass");

			grabVolumePass.Read(indirectFogIndexBufferHandle, true);
			grabVolumePass.Write(indirectFogSortCommandBufferHandle, true);

			grabVolumePass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
				RHI::D3D12ComputeContext* pCompute = context->GetComputeContext();

				RHI::D3D12Buffer* indirectIndexBufferPtr = RegGetBuf(indirectFogIndexBufferHandle);
				RHI::D3D12Buffer* indirectSortBufferPtr = RegGetBuf(indirectFogSortCommandBufferHandle);
				RHI::D3D12Buffer* grabDispatchArgsPtr = RegGetBuf(grabDispatchArgsHandle);

				{
					pCompute->TransitionBarrier(indirectIndexBufferPtr->GetCounterBuffer().get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
					pCompute->TransitionBarrier(indirectSortBufferPtr->GetCounterBuffer().get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
					pCompute->TransitionBarrier(grabDispatchArgsPtr, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
					//context->InsertUAVBarrier(grabDispatchArgBuffer);
					pCompute->FlushResourceBarriers();

					pCompute->SetPipelineState(PipelineStates::pIndirectCullArgs.get());
					pCompute->SetRootSignature(RootSignatures::pIndirectCullArgs.get());

					pCompute->SetBufferSRV(0, indirectIndexBufferPtr->GetCounterBuffer().get());
					pCompute->SetBufferUAV(1, grabDispatchArgsPtr);

					pCompute->Dispatch(1, 1, 1);
				}
				{
					pCompute->TransitionBarrier(indirectIndexBufferPtr->GetCounterBuffer().get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
					pCompute->TransitionBarrier(indirectIndexBufferPtr, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
					pCompute->TransitionBarrier(grabDispatchArgsPtr, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
					pCompute->TransitionBarrier(indirectSortBufferPtr, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
					//context->InsertUAVBarrier(indirectSortBuffer);
					pCompute->FlushResourceBarriers();

					pCompute->SetPipelineState(pVolumeIndirectGrabPSO.get());
					pCompute->SetRootSignature(pVolumeIndirectGrabSignature.get());

					pCompute->SetBufferSRV(1, indirectIndexBufferPtr->GetCounterBuffer().get());
					pCompute->SetBufferSRV(2, indirectIndexBufferPtr);
					pCompute->SetDescriptorTable(3, indirectSortBufferPtr->GetDefaultUAV()->GetGpuHandle());

					pCompute->DispatchIndirect(grabDispatchArgsPtr, 0);

					// Transition to indirect argument state
					pCompute->TransitionBarrier(indirectSortBufferPtr, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
					pCompute->FlushResourceBarriers();
				}

			});
		}

		RHI::RgResourceHandle mVBufferDensityHandle = graph.Import<RHI::D3D12Texture>(mVBufferDensity.get());
		{
			RHI::RenderPass& volumeDrawPass = graph.AddRenderPass("VolumeDrawPass");

			volumeDrawPass.Read(indirectFogSortCommandBufferHandle, true);
			volumeDrawPass.Read(perframeBufferHandle, true);
			volumeDrawPass.Read(mShaderVariablesVolumetricHandle, true);
			volumeDrawPass.Read(volumesDataBufferHandle, true);
			volumeDrawPass.Write(mVBufferDensityHandle, true);

			volumeDrawPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
				RHI::D3D12GraphicsContext* graphicContext = context->GetGraphicsContext();

				graphicContext->TransitionBarrier(RegGetBuf(indirectFogSortCommandBufferHandle), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
				graphicContext->TransitionBarrier(RegGetBufCounter(indirectFogSortCommandBufferHandle), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
				graphicContext->TransitionBarrier(RegGetBuf(perframeBufferHandle), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
				graphicContext->TransitionBarrier(RegGetBuf(mShaderVariablesVolumetricHandle), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
				graphicContext->TransitionBarrier(RegGetBuf(volumesDataBufferHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
				graphicContext->TransitionBarrier(RegGetTex(mVBufferDensityHandle), D3D12_RESOURCE_STATE_RENDER_TARGET);
				graphicContext->FlushResourceBarriers();

				RHI::D3D12Texture* pRenderTargetColor = registry->GetD3D12Texture(mVBufferDensityHandle);
				
				RHI::D3D12RenderTargetView* renderTargetView = pRenderTargetColor->GetDefaultRTV().get();
				
				int32_t vWidth = m_CurrentVolumetricBufferSize.x;
				int32_t vHeight = m_CurrentVolumetricBufferSize.y;

				graphicContext->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				graphicContext->SetViewport(RHIViewport{ 0.0f, 0.0f, (float)vWidth, (float)vHeight, 0.0f, 1.0f });
				graphicContext->SetScissorRect(RHIRect{ 0, 0, (int)vWidth, (int)vHeight });

				graphicContext->SetRenderTarget(renderTargetView, nullptr);
				graphicContext->ClearRenderTarget(renderTargetView, nullptr);

				graphicContext->SetRootSignature(pIndirectDrawVolumeSignature.get());
				graphicContext->SetPipelineState(pIndirectDrawVolumePSO.get());

				//graphicContext->SetConstantBuffer(1, RegGetBuf(perframeBufferHandle)->GetGpuVirtualAddress());
				//graphicContext->SetConstantBuffer(2, RegGetBuf(mShaderVariablesVolumetricHandle)->GetGpuVirtualAddress());
				graphicContext->SetDynamicDescriptor(1, 0, RegGetBuf(perframeBufferHandle)->GetDefaultCBV(1)->GetCpuHandle());
				graphicContext->SetDynamicDescriptor(1, 1, RegGetBuf(mShaderVariablesVolumetricHandle)->GetDefaultCBV(1)->GetCpuHandle());
				graphicContext->SetDynamicDescriptor(2, 0, RegGetBuf(volumesDataBufferHandle)->GetDefaultSRV(1)->GetCpuHandle());

				RHI::D3D12Buffer* pIndirectCommandBuffer = registry->GetD3D12Buffer(indirectFogSortCommandBufferHandle);

				graphicContext->ExecuteIndirect(
					pIndirectDrawVolumeCommandSignature.get(),
					pIndirectCommandBuffer,
					0,
					MAX_VOLUMETRIC_FOG_COUNT,
					pIndirectCommandBuffer->GetCounterBuffer().get(),
					0);
			});
		}

		passOutput.vBufferDensityHandle = mVBufferDensityHandle;
		passOutput.shaderVariablesVolumetricHandle = mShaderVariablesVolumetricHandle;
	}

	void VolumetriLighting::bitonicSort(RHI::D3D12ComputeContext* context,
		RHI::D3D12Buffer* keyIndexList,
		RHI::D3D12Buffer* countBuffer,
		RHI::D3D12Buffer* sortDispatchArgBuffer, // pSortDispatchArgs
		bool isPartiallyPreSorted,
		bool sortAscending)
	{
		const uint32_t ElementSizeBytes = keyIndexList->GetStride();
		const uint32_t MaxNumElements = 1024; //keyIndexList->GetSizeInBytes() / ElementSizeBytes;
		const uint32_t AlignedMaxNumElements = MoYu::AlignPowerOfTwo(MaxNumElements);
		const uint32_t MaxIterations = MoYu::Log2(std::max(2048u, AlignedMaxNumElements)) - 10;

		ASSERT(ElementSizeBytes == 4 || ElementSizeBytes == 8); // , "Invalid key-index list for bitonic sort"

		context->TransitionBarrier(countBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		context->TransitionBarrier(sortDispatchArgBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		context->FlushResourceBarriers();

		context->SetRootSignature(RootSignatures::pBitonicSortRootSignature.get());
		// Generate execute indirect arguments
		context->SetPipelineState(PipelineStates::pBitonicIndirectArgsPSO.get());

		// This controls two things.  It is a key that will sort to the end, and it is a mask used to
		// determine whether the current group should sort ascending or descending.
		//context.SetConstants(3, counterOffset, sortAscending ? 0xffffffff : 0);
		context->SetConstants(0, MaxIterations);
		context->SetDescriptorTable(1, countBuffer->GetDefaultSRV()->GetGpuHandle());
		context->SetDescriptorTable(2, sortDispatchArgBuffer->GetDefaultUAV()->GetGpuHandle());
		context->SetConstants(3, 0, sortAscending ? 0x7F7FFFFF : 0);
		context->Dispatch(1, 1, 1);

		// Pre-Sort the buffer up to k = 2048.  This also pads the list with invalid indices
		// that will drift to the end of the sorted list.
		context->TransitionBarrier(sortDispatchArgBuffer, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
		context->TransitionBarrier(keyIndexList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		context->InsertUAVBarrier(keyIndexList);
		context->FlushResourceBarriers();

		//context->SetComputeRootUnorderedAccessView(2, keyIndexList->GetGpuVirtualAddress());
		context->SetDescriptorTable(2, keyIndexList->GetDefaultRawUAV()->GetGpuHandle());

		if (!isPartiallyPreSorted)
		{
			context->FlushResourceBarriers();
			context->SetPipelineState(ElementSizeBytes == 4 ? PipelineStates::pBitonic32PreSortPSO.get() :
				PipelineStates::pBitonic64PreSortPSO.get());

			context->DispatchIndirect(sortDispatchArgBuffer, 0);
			context->InsertUAVBarrier(keyIndexList);
		}

		uint32_t IndirectArgsOffset = 12;

		// We have already pre-sorted up through k = 2048 when first writing our list, so
		// we continue sorting with k = 4096.  For unnecessarily large values of k, these
		// indirect dispatches will be skipped over with thread counts of 0.

		for (uint32_t k = 4096; k <= AlignedMaxNumElements; k *= 2)
		{
			context->SetPipelineState(ElementSizeBytes == 4 ? PipelineStates::pBitonic32OuterSortPSO.get() :
				PipelineStates::pBitonic64OuterSortPSO.get());

			for (uint32_t j = k / 2; j >= 2048; j /= 2)
			{
				context->SetConstant(0, j, k);
				context->DispatchIndirect(sortDispatchArgBuffer, IndirectArgsOffset);
				context->InsertUAVBarrier(keyIndexList);
				IndirectArgsOffset += 12;
			}

			context->SetPipelineState(ElementSizeBytes == 4 ? PipelineStates::pBitonic32InnerSortPSO.get() :
				PipelineStates::pBitonic64InnerSortPSO.get());
			context->DispatchIndirect(sortDispatchArgBuffer, IndirectArgsOffset);
			context->InsertUAVBarrier(keyIndexList);
			IndirectArgsOffset += 12;
		}
	}

}


