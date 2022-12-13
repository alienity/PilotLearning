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
            auto Iter         = TextureDescTable.find(Handle);
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

			RHITextureDimension textureDim;
            switch (Desc.Type)
            {
                case RgTextureType::Texture2D:
                    textureDim = RHITexDim2D;
                    break;
                case RgTextureType::Texture2DArray:
                    textureDim = RHITexDim2DArray;
                    break;
                case RgTextureType::Texture3D:
                    textureDim = RHITexDim3D;
                    break;
                case RgTextureType::TextureCube:
                    textureDim = RHITexDimCube;
                    break;
                default:
                    break;
            }

			RHISurfaceCreateFlags textureFlags;
			CD3DX12_CLEAR_VALUE clearValue;

            if (Desc.AllowRenderTarget)
            {
                textureFlags |= RHISurfaceCreateRenderTarget;
                clearValue = CD3DX12_CLEAR_VALUE(Desc.Format, Desc.ClearValue.Color);
            }
            if (Desc.AllowDepthStencil)
            {
                textureFlags |= RHISurfaceCreateDepthStencil;
                clearValue = CD3DX12_CLEAR_VALUE(
                    Desc.Format, Desc.ClearValue.DepthStencil.Depth, Desc.ClearValue.DepthStencil.Stencil);
            }
            if (Desc.AllowUnorderedAccess)
            {
                textureFlags |= RHISurfaceCreateRandomWrite;
            }

			RHIRenderSurfaceBaseDesc textureDesc = {Desc.Width,
                                                    Desc.Height,
                                                    Desc.DepthOrArraySize,
                                                    1,
                                                    Desc.MipLevels,
                                                    textureFlags,
                                                    textureDim,
                                                    Desc.Format,
                                                    true,
                                                    false};

			std::wstring textureName = std::wstring(RgTexture.Desc.Name.begin(), RgTexture.Desc.Name.end());

			 pTextures[i] = D3D12Texture::Create(
                Device->GetLinkedDevice(), textureDesc, textureName, clearValue, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON);
		}
	}

	std::shared_ptr<D3D12Buffer> RenderGraphRegistry::GetD3D12Buffer(RgResourceHandle Handle)
	{
        assert(Handle.IsValid());
        assert(Handle.Type == RgResourceTraits<D3D12Buffer>::Enum);
		if (!Handle.IsImported())
		{
            auto& Container = GetContainer<D3D12Buffer>();
            assert(Handle.Id < Container.size());
            return Container[Handle.Id];
		}
		else
		{
            auto& ImportedContainer = Graph->GetImportedContainer<D3D12Buffer>();
            assert(Handle.Id < ImportedContainer.size());
            return ImportedContainer[Handle.Id];
		}
	}

	std::shared_ptr<D3D12Texture> RenderGraphRegistry::GetD3D12Texture(RgResourceHandle Handle)
	{
        assert(Handle.IsValid());
        assert(Handle.Type == RgResourceTraits<D3D12Texture>::Enum);
        if (!Handle.IsImported())
        {
            auto& Container = GetContainer<D3D12Texture>();
            assert(Handle.Id < Container.size());
            return Container[Handle.Id];
        }
        else
        {
            auto& ImportedContainer = Graph->GetImportedContainer<D3D12Texture>();
            assert(Handle.Id < ImportedContainer.size());
            return ImportedContainer[Handle.Id];
        }
	}

} // namespace RHI
