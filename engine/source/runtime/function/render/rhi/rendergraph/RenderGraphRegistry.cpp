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
			ASSERT(!Handle.IsImported());

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

			RHISurfaceCreateFlags textureFlags = RHISurfaceCreateFlagNone;
            std::optional<CD3DX12_CLEAR_VALUE> pClearValue;
            //CD3DX12_CLEAR_VALUE clearValue = CD3DX12_CLEAR_VALUE();

            if (Desc.AllowRenderTarget)
            {
                textureFlags |= RHISurfaceCreateRenderTarget;
                DXGI_FORMAT format = Desc.ClearValue.ClearFormat;
                if (format == DXGI_FORMAT_UNKNOWN)
                    format = Desc.Format;
                pClearValue = CD3DX12_CLEAR_VALUE(format, Desc.ClearValue.Color);
            }
            if (Desc.AllowDepthStencil)
            {
                textureFlags |= RHISurfaceCreateDepthStencil;
                DXGI_FORMAT format = Desc.ClearValue.ClearFormat;
                if (format == DXGI_FORMAT_UNKNOWN)
                    format = Desc.Format;
                pClearValue = CD3DX12_CLEAR_VALUE(format, Desc.ClearValue.DepthStencil.Depth, Desc.ClearValue.DepthStencil.Stencil);
            }
            if (Desc.AllowUnorderedAccess)
            {
                textureFlags |= RHISurfaceCreateRandomWrite;
            }

			DXGI_FORMAT clearFormat = pClearValue.has_value() ? pClearValue->Format : Desc.Format;

			RHIRenderSurfaceBaseDesc textureDesc = {Desc.Width,
                                                    Desc.Height,
                                                    Desc.DepthOrArraySize,
                                                    Desc.SampleCount,
                                                    Desc.MipLevels,
                                                    textureFlags,
                                                    textureDim,
                                                    Desc.Format,
                                                    clearFormat,
                                                    true,
                                                    false};

			std::wstring textureName = std::wstring(RgTexture.Desc.Name.begin(), RgTexture.Desc.Name.end());

			 pTextures[i] = D3D12Texture::Create(
                Device->GetLinkedDevice(), textureDesc, textureName, D3D12_RESOURCE_STATE_COMMON, pClearValue);
		}

		robin_hood::unordered_set<RgResourceHandle> BufferDirtyHandles;

		for (size_t i = 0; i < Graph->Buffers.size(); ++i)
        {
             auto&            RgBuffer = Graph->Buffers[i];
             RgResourceHandle Handle   = RgBuffer.Handle;
             ASSERT(!Handle.IsImported());

             bool BufferDirty = false;
             auto Iter        = BufferDescTable.find(Handle);
             if (Iter == BufferDescTable.end())
             {
                BufferDirty = true;
             }
             else
             {
                BufferDirty = Iter->second != RgBuffer.Desc;
             }
             BufferDescTable[Handle] = RgBuffer.Desc;

             if (!BufferDirty)
             {
                continue;
             }

             BufferDirtyHandles.insert(Handle);
             RgBufferDesc& Desc = RgBuffer.Desc;

             std::wstring bufferName = std::wstring(RgBuffer.Desc.mName.begin(), RgBuffer.Desc.mName.end());

             pBuffers[i] = D3D12Buffer::Create(Device->GetLinkedDevice(),
                                               Desc.mRHIBufferTarget,
                                               Desc.mNumElements,
                                               Desc.mElementSize,
                                               bufferName,
                                               Desc.mRHIBufferMode,
                                               D3D12_RESOURCE_STATE_COMMON);
        }

	}

	D3D12Buffer* RenderGraphRegistry::GetD3D12Buffer(RgResourceHandle Handle)
	{
        ASSERT(Handle.IsValid());
        //ASSERT(Handle.Type == RgResourceTraits<D3D12Buffer>::Enum);
        ASSERT(Handle.Type == RgResourceType::Buffer);
		if (!Handle.IsImported())
		{
            auto& Container = GetContainer<D3D12Buffer>();
            ASSERT(Handle.Id < Container.size());
            return Container[Handle.Id].get();
		}
		else
		{
            auto& ImportedContainer = Graph->GetImportedContainer<D3D12Buffer>();
            ASSERT(Handle.Id < ImportedContainer.size());
            return ImportedContainer[Handle.Id];
		}
	}

	D3D12Texture* RenderGraphRegistry::GetD3D12Texture(RgResourceHandle Handle)
	{
        ASSERT(Handle.IsValid());
        //ASSERT(Handle.Type == RgResourceTraits<D3D12Texture>::Enum);
        ASSERT(Handle.Type == RgResourceType::Texture);
        if (!Handle.IsImported())
        {
            auto& Container = GetContainer<D3D12Texture>();
            ASSERT(Handle.Id < Container.size());
            return Container[Handle.Id].get();
        }
        else
        {
            auto& ImportedContainer = Graph->GetImportedContainer<D3D12Texture>();
            ASSERT(Handle.Id < ImportedContainer.size());
            return ImportedContainer[Handle.Id];
        }
	}

} // namespace RHI
