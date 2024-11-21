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
			mVBufferDensity = RHI::D3D12Texture::Create3D(
				m_Device->GetLinkedDevice(),
				m_CurrentVolumetricBufferSize.x,
				m_CurrentVolumetricBufferSize.y,
				m_CurrentVolumetricBufferSize.z,
				1,
				DXGI_FORMAT_R16G16B16A16_FLOAT,
				RHI::RHISurfaceCreateRandomWrite,
				1,
				L"ASScattering3D",
				D3D12_RESOURCE_STATE_COMMON);
		}

		if (mLightingBuffer == nullptr)
		{
			mLightingBuffer = RHI::D3D12Texture::Create3D(
				m_Device->GetLinkedDevice(),
				m_CurrentVolumetricBufferSize.x,
				m_CurrentVolumetricBufferSize.y,
				m_CurrentVolumetricBufferSize.z,
				1,
				DXGI_FORMAT_R32G32B32A32_FLOAT,
				RHI::RHISurfaceCreateRandomWrite,
				1,
				L"VolumeLightingBuffer",
				D3D12_RESOURCE_STATE_COMMON);
		}

		HLSL::VolumetricLightingUniform mVolumetricLightingUniform;
		UpdateVolumetricLightingUniform(mVolumetricLightingUniform);

		HLSL::VBufferUniform mVBufferUniform;
		UpdateVolumetricLightingUniform(mVBufferUniform);

		HLSL::FrameUniforms* _frameUniforms = &(render_resource->m_FrameUniforms);

		_frameUniforms->volumetricLightingUniform = mVolumetricLightingUniform;
		_frameUniforms->vBufferUniform = mVBufferUniform;
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

		RHI::RenderPass& volumeLightingPass = graph.AddRenderPass("Volumetric Lighting");

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
		});
	}

	void VolumetriLighting::UpdateVolumetricLightingUniform(HLSL::VolumetricLightingUniform& inoutVolumetricLightingUniform)
	{
		inoutVolumetricLightingUniform._FogEnabled = 1;
		inoutVolumetricLightingUniform._EnableVolumetricFog = 1;

		const FogVolume& fog = GetFogVolume();

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

	void VolumetriLighting::UpdateVolumetricLightingUniform(HLSL::VBufferUniform& inoutVBufferUniform)
	{
		const FogVolume& fog = GetFogVolume();

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

}


