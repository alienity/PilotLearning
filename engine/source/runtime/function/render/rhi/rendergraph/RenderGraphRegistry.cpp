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
		Buffers.resize(Graph->Buffers.size());
		Textures.resize(Graph->Textures.size());
		RenderTargetViews.resize(Graph->RenderTargetViews.size());
		DepthStencilViews.resize(Graph->DepthStencilViews.size());
		ShaderResourceViews.resize(Graph->ShaderResourceViews.size());
		UnorderedAccessViews.resize(Graph->UnorderedAccessViews.size());

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

		for (size_t i = 0; i < Graph->RenderTargetViews.size(); ++i)
		{
			auto& RgView = Graph->RenderTargetViews[i];
			if (!IsViewDirty(RgView) && !TextureDirtyHandles.contains(RgView.Desc.AssociatedResource))
			{
				continue;
			}

			std::optional<UINT> ArraySlice = RgView.Desc.RgRtv.ArraySlice != -1 ? RgView.Desc.RgRtv.ArraySlice : std::optional<UINT>{};
			std::optional<UINT> MipSlice   = RgView.Desc.RgRtv.MipSlice != -1 ? RgView.Desc.RgRtv.MipSlice : std::optional<UINT>{};
			std::optional<UINT> ArraySize  = RgView.Desc.RgRtv.ArraySize != -1 ? RgView.Desc.RgRtv.ArraySize : std::optional<UINT>{};

            RenderTargetViews[i] = D3D12RenderTargetView(Device->GetLinkedDevice(), GetD3D12Texture(RgView.Desc.AssociatedResource), ArraySlice, MipSlice, ArraySize, RgView.Desc.RgRtv.sRGB);
		}

		for (size_t i = 0; i < Graph->DepthStencilViews.size(); ++i)
		{
			auto& RgView = Graph->DepthStencilViews[i];
			if (!IsViewDirty(RgView) && !TextureDirtyHandles.contains(RgView.Desc.AssociatedResource))
			{
				continue;
			}

			std::optional<UINT> ArraySlice = RgView.Desc.RgDsv.ArraySlice != -1 ? RgView.Desc.RgDsv.ArraySlice : std::optional<UINT>{};
			std::optional<UINT> MipSlice   = RgView.Desc.RgDsv.MipSlice != -1 ? RgView.Desc.RgDsv.MipSlice : std::optional<UINT>{};
			std::optional<UINT> ArraySize  = RgView.Desc.RgDsv.ArraySize != -1 ? RgView.Desc.RgDsv.ArraySize : std::optional<UINT>{};

			DepthStencilViews[i] = D3D12DepthStencilView(Device->GetLinkedDevice(), GetD3D12Texture(RgView.Desc.AssociatedResource), ArraySlice, MipSlice, ArraySize);
		}

		for (size_t i = 0; i < Graph->ShaderResourceViews.size(); ++i)
		{
			auto& RgView = Graph->ShaderResourceViews[i];
			if (!IsViewDirty(RgView) && !TextureDirtyHandles.contains(RgView.Desc.AssociatedResource))
			{
				continue;
			}

			switch (RgView.Desc.Type)
			{
			case RgViewType::BufferSrv:
			{
                ShaderResourceViews[i] = D3D12ShaderResourceView(Device->GetLinkedDevice(), GetD3D12Buffer(RgView.Desc.AssociatedResource), RgView.Desc.BufferSrv.Raw, RgView.Desc.BufferSrv.FirstElement, RgView.Desc.BufferSrv.NumElements);
			}
			break;

			case RgViewType::TextureSrv:
			{
				D3D12Texture*		Resource		= GetD3D12Texture(RgView.Desc.AssociatedResource);
				bool				sRGB			= RgView.Desc.TextureSrv.sRGB;
				std::optional<UINT> MostDetailedMip = RgView.Desc.TextureSrv.MostDetailedMip != -1 ? RgView.Desc.TextureSrv.MostDetailedMip : std::optional<UINT>{};
				std::optional<UINT> MipLevels		= RgView.Desc.TextureSrv.MipLevels != -1 ? RgView.Desc.TextureSrv.MipLevels : std::optional<UINT>{};
				ShaderResourceViews[i]				= D3D12ShaderResourceView(Device->GetLinkedDevice(), Resource, sRGB, MostDetailedMip, MipLevels);
			}
			break;

			default:
				assert(false && "Invalid Srv");
			}
		}

		for (size_t i = 0; i < Graph->UnorderedAccessViews.size(); ++i)
		{
			auto& RgView = Graph->UnorderedAccessViews[i];
			if (!IsViewDirty(RgView) && !TextureDirtyHandles.contains(RgView.Desc.AssociatedResource))
			{
				continue;
			}

			switch (RgView.Desc.Type)
			{
			case RgViewType::BufferUav:
			{
				UnorderedAccessViews[i] = D3D12UnorderedAccessView(Device->GetLinkedDevice(), GetD3D12Buffer(RgView.Desc.AssociatedResource), RgView.Desc.BufferUav.NumElements, RgView.Desc.BufferUav.CounterOffsetInBytes);
			}
			break;

			case RgViewType::TextureUav:
			{
                D3D12Texture*       Resource   = GetD3D12Texture(RgView.Desc.AssociatedResource);
				std::optional<UINT> ArraySlice = RgView.Desc.TextureUav.ArraySlice != -1 ? RgView.Desc.TextureUav.ArraySlice : std::optional<UINT>{};
				std::optional<UINT> MipSlice   = RgView.Desc.TextureUav.MipSlice != -1 ? RgView.Desc.TextureUav.MipSlice : std::optional<UINT>{};
				UnorderedAccessViews[i]		   = D3D12UnorderedAccessView(Device->GetLinkedDevice(), Resource, ArraySlice, MipSlice);
			}
			break;

			default:
				assert(false && "Invalid Uav");
			}
		}
	}

	bool RenderGraphRegistry::IsViewDirty(RgView& View)
	{
		RgResourceHandle Handle = View.Handle; // View andle

		bool ViewDirty;
		auto Iter = ViewDescTable.find(Handle);
		if (Iter == ViewDescTable.end())
		{
			ViewDirty = true;
		}
		else
		{
			ViewDirty = Iter->second != View.Desc;
		}
		ViewDescTable[Handle] = View.Desc;
		return ViewDirty;
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

	D3D12RenderTargetView* RenderGraphRegistry::GetD3D12RenderTargetView(RgResourceHandle Handle)
	{
        assert(Handle.IsValid());
        assert(Handle.Type == RgResourceTraits<D3D12RenderTargetView>::Enum);
        if (!Handle.IsImported())
        {
            auto& Container = GetContainer<D3D12RenderTargetView>();
            assert(Handle.Id < Container.size());
            return &Container[Handle.Id];
        }
        else
        {
            auto& ImportedContainer = Graph->GetImportedContainer<D3D12RenderTargetView>();
            assert(Handle.Id < ImportedContainer.size());
            return ImportedContainer[Handle.Id];
        }
    }

    D3D12DepthStencilView* RenderGraphRegistry::GetD3D12DepthStencilView(RgResourceHandle Handle)
	{
        assert(Handle.IsValid());
        assert(Handle.Type == RgResourceTraits<D3D12DepthStencilView>::Enum);
        if (!Handle.IsImported())
        {
            auto& Container = GetContainer<D3D12DepthStencilView>();
            assert(Handle.Id < Container.size());
            return &Container[Handle.Id];
        }
        else
        {
            auto& ImportedContainer = Graph->GetImportedContainer<D3D12DepthStencilView>();
            assert(Handle.Id < ImportedContainer.size());
            return ImportedContainer[Handle.Id];
        }
	}

    D3D12ShaderResourceView* RenderGraphRegistry::GetD3D12ShaderResourceView(RgResourceHandle Handle)
    {
        assert(Handle.IsValid());
        assert(Handle.Type == RgResourceTraits<D3D12ShaderResourceView>::Enum);
        if (!Handle.IsImported())
        {
            auto& Container = GetContainer<D3D12ShaderResourceView>();
            assert(Handle.Id < Container.size());
            return &Container[Handle.Id];
        }
        else
        {
            auto& ImportedContainer = Graph->GetImportedContainer<D3D12ShaderResourceView>();
            assert(Handle.Id < ImportedContainer.size());
            return ImportedContainer[Handle.Id];
        }
    }

    D3D12UnorderedAccessView* RenderGraphRegistry::GetD3D12UnorderedAccessView(RgResourceHandle Handle)
    {
        assert(Handle.IsValid());
        assert(Handle.Type == RgResourceTraits<D3D12UnorderedAccessView>::Enum);
        if (!Handle.IsImported())
        {
            auto& Container = GetContainer<D3D12UnorderedAccessView>();
            assert(Handle.Id < Container.size());
            return &Container[Handle.Id];
        }
        else
        {
            auto& ImportedContainer = Graph->GetImportedContainer<D3D12UnorderedAccessView>();
            assert(Handle.Id < ImportedContainer.size());
            return ImportedContainer[Handle.Id];
        }
    }

} // namespace RHI
