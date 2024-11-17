#pragma once

#include "runtime/function/render/render_pass.h"
#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/rhi/hlsl_data_types.h"
#include "runtime/function/render/renderer/volume_lighting_helper.h"

namespace MoYu
{
	class VolumetriLighting : public RenderPass
	{
	public:

		struct VolumetriLightingInitInfo : public RenderPassInitInfo
		{
			RHI::RgTextureDesc colorTexDesc;

			ShaderCompiler* m_ShaderCompiler;
			std::filesystem::path m_ShaderRootPath;
		};

		struct GenMaxZInputStruct
		{
			RHI::RgResourceHandle perframeBufferHandle;
			RHI::RgResourceHandle depthHandle;
		};

		struct GenMaxZOutputStruct
		{
			RHI::RgResourceHandle maxZ8xBufferHandle;
			RHI::RgResourceHandle maxZBufferHandle;
			RHI::RgResourceHandle dilatedMaxZBufferHandle;
		};

		struct ClearPassInputStruct
		{
			RHI::RgResourceHandle perframeBufferHandle;

		};

		struct ClearPassOutputStruct
		{
			RHI::RgResourceHandle vbufferDensityHandle;
			RHI::RgResourceHandle shaderVariablesVolumetricHandle;
		};

		struct VolumeLightPassInputStruct
		{
			RHI::RgResourceHandle perframeBufferHandle;
			RHI::RgResourceHandle vbufferDensityHandle;
			RHI::RgResourceHandle depthPyramidHandle;
			RHI::RgResourceHandle dilatedMaxZBufferHandle;
			RHI::RgResourceHandle shaderVariablesVolumetricHandle;
		};

		struct VolumeLightPassOutputStruct
		{
			RHI::RgResourceHandle vbufferLightingHandle;
		};

		~VolumetriLighting() { destroy(); }

		void updateShaderVariableslVolumetrics(HLSL::ShaderVariablesVolumetric& cb, glm::vec4 resolution, int maxSliceCount, bool updateVoxelizationFields = false);
		void prepareBuffer(std::shared_ptr<RenderResource> render_resource);

		void initialize(const VolumetriLightingInitInfo& init_info);

		void GenerateMaxZForVolumetricPass(RHI::RenderGraph& graph, GenMaxZInputStruct& passInput, GenMaxZOutputStruct& passOutput);
		void FogVolumeAndVFXVoxelizationPass(RHI::RenderGraph& graph, ClearPassInputStruct& passInput, ClearPassOutputStruct& passOutput);
		void VolumetricLightingPass(RHI::RenderGraph& graph, VolumeLightPassInputStruct& passInput, VolumeLightPassOutputStruct& passOutput);

		void UpdateVolumetricLightingUniform(HLSL::VolumetricLightingUniform& inoutVolumetricLightingUniform);
		void UpdateVolumetricLightingUniform(HLSL::VBufferUniform& inoutVBufferUniform);

		glm::vec4 m_PackedCoeffs[7];
		glm::vec2 m_xySeq[7];
		float m_zSeq[7] = { 7.0f / 14.0f, 3.0f / 14.0f, 11.0f / 14.0f, 5.0f / 14.0f, 9.0f / 14.0f, 1.0f / 14.0f, 13.0f / 14.0f };

		HeightFogVoxelizationPassData fogData;

		HLSL::VolumetricLightingUniform mVolumetricLightingUniform;
		HLSL::VBufferUniform mVBufferUniform;
	private:
		Shader mMaxZCS;
		std::shared_ptr<RHI::D3D12RootSignature> pMaxZSignature;
		std::shared_ptr<RHI::D3D12PipelineState> pMaxZPSO;
		Shader mMaxZDownsampleCS;
		std::shared_ptr<RHI::D3D12RootSignature> pMaxZDownsampleSignature;
		std::shared_ptr<RHI::D3D12PipelineState> pMaxZDownsamplePSO;
		Shader mDilateMaxZCS;
		std::shared_ptr<RHI::D3D12RootSignature> pDilateMaxZSignature;
		std::shared_ptr<RHI::D3D12PipelineState> pDilateMaxZPSO;
		
		Shader mVoxelizationCS;
		std::shared_ptr<RHI::D3D12RootSignature> pVoxelizationSignature;
		std::shared_ptr<RHI::D3D12PipelineState> pVoxelizationPSO;

		Shader mVolumetricLightingCS;
		std::shared_ptr<RHI::D3D12RootSignature> pVolumetricLightingSignature;
		std::shared_ptr<RHI::D3D12PipelineState> pVolumetricLightingPSO;

		Shader mVolumetricLightingFilteringCS;
		std::shared_ptr<RHI::D3D12RootSignature> pVolumetricLightingFilteringSignature;
		std::shared_ptr<RHI::D3D12PipelineState> pVolumetricLightingFilteringPSO;


		RHI::RgTextureDesc colorTexDesc;

		RHI::RgTextureDesc maxZ8xBufferDesc;
		RHI::RgTextureDesc maxZBufferDesc;
		RHI::RgTextureDesc dilatedMaxZBufferDesc;

		glm::ivec3 m_CurrentVolumetricBufferSize;
		VBufferParameters m_CurrentVBufferParams;

		std::shared_ptr<RHI::D3D12Buffer> pShaderVariablesVolumetric;
		std::shared_ptr<RHI::D3D12Texture> mVBufferDensity;
		std::shared_ptr<RHI::D3D12Texture> mLightingBuffer;

		std::shared_ptr<RHI::D3D12Texture> volumetricHistoryBuffers[2];
	};


}

