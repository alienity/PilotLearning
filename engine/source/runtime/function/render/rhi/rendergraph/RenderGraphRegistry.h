#pragma once
#include "RenderGraphCommon.h"

namespace RHI
{
    class RenderGraph;

	template<typename T, RgResourceType Type>
	class RgRegistry
	{
	public:
		template<typename... TArgs>
		[[nodiscard]] auto Add(TArgs&&... Args) -> RgResourceHandle
		{
			RgResourceHandle Handle = {};
			Handle.Type				= Type;
			Handle.Version			= 0;
			Handle.Id				= Array.size();
			Array.emplace_back(std::forward<TArgs>(Args)...);
			return Handle;
		}

		[[nodiscard]] auto GetResource(RgResourceHandle Handle) -> T*
		{
			assert(Handle.Type == Type);
			return &Array[Handle.Id];
		}

	protected:
		std::vector<T> Array;
	};

	using RootSignatureRegistry           = RgRegistry<D3D12RootSignature, RgResourceType::RootSignature>;
	using CommandSignatureRegistry        = RgRegistry<D3D12CommandSignature, RgResourceType::RootSignature>;
	using PipelineStateRegistry           = RgRegistry<D3D12PipelineState, RgResourceType::PipelineState>;
	using RaytracingPipelineStateRegistry = RgRegistry<D3D12RaytracingPipelineState, RgResourceType::RaytracingPipelineState>;

	class RenderGraphRegistry
	{
	public:
		[[nodiscard]] auto CreateRootSignature(D3D12RootSignature&& RootSignature) -> RgResourceHandle;
        [[nodiscard]] auto CreateCommandSignature(D3D12CommandSignature&& RootSignature) -> RgResourceHandle;
		[[nodiscard]] auto CreatePipelineState(D3D12PipelineState&& PipelineState) -> RgResourceHandle;
		[[nodiscard]] auto CreateRaytracingPipelineState(D3D12RaytracingPipelineState&& RaytracingPipelineState) -> RgResourceHandle;

		D3D12RootSignature*			  GetRootSignature(RgResourceHandle Handle);
        D3D12CommandSignature*        GetCommandSignature(RgResourceHandle Handle);
		D3D12PipelineState*			  GetPipelineState(RgResourceHandle Handle);
		D3D12RaytracingPipelineState* GetRaytracingPipelineState(RgResourceHandle Handle);

		void RealizeResources(RenderGraph* Graph, D3D12Device* Device);

		/*
		template<typename T>
		[[nodiscard]] auto Get(RgResourceHandle Handle) -> T*
		{
            assert(Handle.IsValid());
            assert(Handle.Type == RgResourceTraits<T>::Enum);
            assert(!Handle.IsImported());
            auto& Container = GetContainer<T>();
            assert(Handle.Id < Container.size());
            return &Container[Handle.Id];
		}
        */

		std::shared_ptr<D3D12Buffer> GetD3D12Buffer(RgResourceHandle Handle);
        std::shared_ptr<D3D12Texture> GetD3D12Texture(RgResourceHandle Handle);

	private:
        template<typename T>
        [[nodiscard]] auto GetContainer() -> std::vector<std::shared_ptr<typename RgResourceTraits<T>::ApiType>>&
        {
            if constexpr (std::is_same_v<T, D3D12Buffer>)
            {
                return pBuffers;
            }
            else if constexpr (std::is_same_v<T, D3D12Texture>)
            {
                return pTextures;
            }
        }

	private:
		RenderGraph*					Graph = nullptr;
		RootSignatureRegistry			RootSignatureRegistry;
		CommandSignatureRegistry		CommandSignatureRegistry;
		PipelineStateRegistry			PipelineStateRegistry;
		RaytracingPipelineStateRegistry RaytracingPipelineStateRegistry;

		// Cached desc table based on resource handle to prevent unnecessary recreation of resources
		robin_hood::unordered_map<RgResourceHandle, RgBufferDesc>  BufferDescTable;
		robin_hood::unordered_map<RgResourceHandle, RgTextureDesc> TextureDescTable;

		std::vector<std::shared_ptr<D3D12Buffer>>  pBuffers;
        std::vector<std::shared_ptr<D3D12Texture>> pTextures;
	};
} // namespace RHI
