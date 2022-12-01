#include "RenderGraphRegistry.h"
#include "RenderGraph.h"

namespace RHI
{
	auto RenderGraphRegistry::CreateRootSignature(D3D12RootSignature&& RootSignature) -> RgResourceHandle
	{
		return RootSignatureRegistry.Add(std::forward<D3D12RootSignature>(RootSignature));
	}

	auto RenderGraphRegistry::CreateCommandSignature(D3D12CommandSignature&& RootSignature) -> RgResourceHandle
	{
        return CommandSignatureRegistry.Add(std::forward<D3D12CommandSignature>(RootSignature));
	}

	auto RenderGraphRegistry::CreatePipelineState(D3D12PipelineState&& PipelineState) -> RgResourceHandle
	{
		return PipelineStateRegistry.Add(std::forward<D3D12PipelineState>(PipelineState));
	}

	auto RenderGraphRegistry::CreateRaytracingPipelineState(D3D12RaytracingPipelineState&& RaytracingPipelineState) -> RgResourceHandle
	{
		return RaytracingPipelineStateRegistry.Add(std::forward<D3D12RaytracingPipelineState>(RaytracingPipelineState));
	}

	D3D12RootSignature* RenderGraphRegistry::GetRootSignature(RgResourceHandle Handle)
	{
		return RootSignatureRegistry.GetResource(Handle);
	}

	D3D12CommandSignature* RenderGraphRegistry::GetCommandSignature(RgResourceHandle Handle)
	{
        return CommandSignatureRegistry.GetResource(Handle);
	}

	D3D12PipelineState* RenderGraphRegistry::GetPipelineState(RgResourceHandle Handle)
	{
		return PipelineStateRegistry.GetResource(Handle);
	}

	D3D12RaytracingPipelineState* RenderGraphRegistry::GetRaytracingPipelineState(RgResourceHandle Handle)
	{
		return RaytracingPipelineStateRegistry.GetResource(Handle);
	}

	void RenderGraphRegistry::RealizeResources(RenderGraph* Graph, D3D12Device* Device)
	{
		this->Graph = Graph;
		pBuffers.resize(Graph->Buffers.size());
        pTextures.resize(Graph->Textures.size());

		// This is used to check to see if any view associated with the texture needs to be updated in case if texture is dirty
		// The view does not check for this, so do it here manually
		robin_hood::unordered_set<RgResourceHandle> TextureDirtyHandles;

		for (size_t i = 0; i < Graph->Textures.size(); ++i)
		{
			auto&		 RgTexture = Graph->Textures[i];
			RgResourceHandle Handle	   = RgTexture.Handle;
			assert(!Handle.IsImported());

			bool TextureDirty = false;
			auto Iter		  = TextureDescTable.find(Handle);
			if (Iter == TextureDescTable.end())
			{
				TextureDirty = true;
			}
			else
			{
				TextureDirty = Iter->second != RgTexture.Desc;
			}
			TextureDescTable[Handle] = RgTexture.Desc;

			if (!TextureDirty)
			{
				continue;
			}

			TextureDirtyHandles.insert(Handle);
			RgTextureDesc& Desc = RgTexture.Desc;

			D3D12_RESOURCE_DESC				 ResourceDesc  = {};
			D3D12_RESOURCE_FLAGS			 ResourceFlags = D3D12_RESOURCE_FLAG_NONE;
			std::optional<D3D12_CLEAR_VALUE> ClearValue	   = std::nullopt;

			if (Desc.AllowRenderTarget)
			{
				ResourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
				ClearValue = CD3DX12_CLEAR_VALUE(Desc.Format, Desc.ClearValue.Color);
			}
			if (Desc.AllowDepthStencil)
			{
				ResourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
				ClearValue = CD3DX12_CLEAR_VALUE(Desc.Format, Desc.ClearValue.DepthStencil.Depth, Desc.ClearValue.DepthStencil.Stencil);
			}
			if (Desc.AllowUnorderedAccess)
			{
				ResourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
			}

			switch (Desc.Type)
			{
			case RgTextureType::Texture2D:
				ResourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(Desc.Format, Desc.Width, Desc.Height, 1, Desc.MipLevels, 1, 0, ResourceFlags);
				break;
			case RgTextureType::Texture2DArray:
				ResourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(Desc.Format, Desc.Width, Desc.Height, Desc.DepthOrArraySize, Desc.MipLevels, 1, 0, ResourceFlags);
				break;
			case RgTextureType::Texture3D:
				ResourceDesc = CD3DX12_RESOURCE_DESC::Tex3D(Desc.Format, Desc.Width, Desc.Height, Desc.DepthOrArraySize, Desc.MipLevels, ResourceFlags);
				break;
			case RgTextureType::TextureCube:
				ResourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(Desc.Format, Desc.Width, Desc.Height, Desc.DepthOrArraySize, Desc.MipLevels, 1, 0, ResourceFlags);
				break;
			default:
				break;
			}

			Textures[i]		  = D3D12Texture(Device->GetLinkedDevice(), ResourceDesc, ClearValue);
			std::wstring Name = std::wstring(RgTexture.Desc.Name.begin(), RgTexture.Desc.Name.end());
			Textures[i].GetResource()->SetName(Name.data());
		}
	}

	D3D12Buffer* RenderGraphRegistry::GetD3D12Buffer(RgResourceHandle Handle)
	{
        assert(Handle.IsValid());
        assert(Handle.Type == RgResourceTraits<D3D12Buffer>::Enum);
		if (!Handle.IsImported())
		{
            auto& Container = GetContainer<D3D12Buffer>();
            assert(Handle.Id < Container.size());
            return &Container[Handle.Id];
		}
		else
		{
            auto& ImportedContainer = Graph->GetImportedContainer<D3D12Buffer>();
            assert(Handle.Id < ImportedContainer.size());
            return ImportedContainer[Handle.Id];
		}
	}

	D3D12Texture* RenderGraphRegistry::GetD3D12Texture(RgResourceHandle Handle)
	{
        assert(Handle.IsValid());
        assert(Handle.Type == RgResourceTraits<D3D12Texture>::Enum);
        if (!Handle.IsImported())
        {
            auto& Container = GetContainer<D3D12Texture>();
            assert(Handle.Id < Container.size());
            return &Container[Handle.Id];
        }
        else
        {
            auto& ImportedContainer = Graph->GetImportedContainer<D3D12Texture>();
            assert(Handle.Id < ImportedContainer.size());
            return ImportedContainer[Handle.Id];
        }
	}

} // namespace RHI
