#pragma once
#include <compare>
#include "runtime/platform/system/system_core.h"
#include "runtime/function/render/rhi/rhi.h"
#include "runtime/function/render/rhi/d3d12/d3d12_resource.h"
#include "runtime/function/render/rhi/d3d12/d3d12_descriptor.h"
#include "runtime/platform/system/hash.h"

namespace RHI
{
	enum class RgResourceType : std::uint64_t
	{
		Unknown,
		Buffer,
		Texture,
		RenderTargetView,
		DepthStencilView,
		ShaderResourceView,
		UnorderedAccessView,
		RootSignature,
		PipelineState,
		RaytracingPipelineState,
	};

	enum RgResourceFlags : std::uint64_t
	{
		RG_RESOURCE_FLAG_NONE,
		RG_RESOURCE_FLAG_IMPORTED
	};

	// A virtual resource handle, the underlying realization of the resource type is done in RenderGraphRegistry
    struct RgResourceHandle
    {
        [[nodiscard]] bool IsValid() const noexcept { return Type != RgResourceType::Unknown && Id != UINT_MAX; }
        [[nodiscard]] bool IsImported() const noexcept { return Flags & RG_RESOURCE_FLAG_IMPORTED; }

        void Invalidate()
        {
            Type    = RgResourceType::Unknown;
            Flags   = RG_RESOURCE_FLAG_NONE;
            Version = 0;
            Id      = UINT_MAX;
        }

        RgResourceType  Type : 15; // 14 bit to represent type, might remove some bits from this and give it to version
        RgResourceFlags Flags : 1;
        std::uint64_t   Version : 16; // 16 bits to represent version should be more than enough, we can always just
                                      // increase bit used if is not enough
        std::uint64_t   Id : 32;      // 32 bit unsigned int
    };

	static_assert(sizeof(RgResourceHandle) == sizeof(std::uint64_t));

	struct RgBufferDesc
    {
        RgBufferDesc& SetSize(std::uint64_t SizeInBytes)
        {
            this->SizeInBytes = SizeInBytes;
            return *this;
        }

        RgBufferDesc& AllowUnorderedAccess()
        {
            UnorderedAccess = true;
            return *this;
        }

        std::uint64_t SizeInBytes     = 0;
        bool          UnorderedAccess = false;
    };

	enum class RgTextureType
	{
		Texture2D,
		Texture2DArray,
		Texture3D,
		TextureCube
	};

	struct RgDepthStencilValue
	{
        std::float_t Depth;
        std::uint8_t Stencil;
	};

	struct RgClearValue
	{
        RgClearValue() noexcept = default;
        const RgClearValue(const std::float_t Color[4])
        {
            this->Color[0] = Color[0];
            this->Color[1] = Color[1];
            this->Color[2] = Color[2];
            this->Color[3] = Color[3];
        }
        const RgClearValue(std::float_t Depth, std::uint8_t Stencil) : DepthStencil {Depth, Stencil} {}

        union
        {
            std::float_t        Color[4];
            RgDepthStencilValue DepthStencil;
        };
	};

	struct RgTextureDesc
	{
		RgTextureDesc() noexcept = default;
		RgTextureDesc(std::string_view Name)
			: Name(Name)
		{
		}

		[[nodiscard]] bool operator==(const RgTextureDesc& RgTextureDesc) const noexcept
		{
			return memcmp(this, &RgTextureDesc, sizeof(RgTextureDesc)) == 0;
		}
		[[nodiscard]] bool operator!=(const RgTextureDesc& RgTextureDesc) const noexcept
		{
			return !(*this == RgTextureDesc);
		}

		RgTextureDesc& SetFormat(DXGI_FORMAT Format)
		{
			this->Format = Format;
			return *this;
		}

		RgTextureDesc& SetType(RgTextureType Type)
		{
			this->Type = Type;
			return *this;
		}

		RgTextureDesc& SetExtent(std::uint32_t Width, std::uint32_t Height, std::uint32_t DepthOrArraySize = 1)
		{
			this->Width			   = Width;
			this->Height		   = Height;
			this->DepthOrArraySize = DepthOrArraySize;
			return *this;
		}

		RgTextureDesc& SetMipLevels(std::uint16_t MipLevels)
		{
			this->MipLevels = MipLevels;
			return *this;
		}

		RgTextureDesc& SetAllowRenderTarget()
		{
			AllowRenderTarget = true;
			return *this;
		}

		RgTextureDesc& SetAllowDepthStencil()
		{
			AllowDepthStencil = true;
			return *this;
		}

		RgTextureDesc& SetAllowUnorderedAccess()
		{
			AllowUnorderedAccess = true;
			return *this;
		}

		RgTextureDesc& SetClearValue(const RgClearValue& ClearValue)
		{
			this->ClearValue = ClearValue;
			return *this;
		}

		std::string_view Name;
		DXGI_FORMAT		 Format				  = DXGI_FORMAT_UNKNOWN;
		RgTextureType	 Type				  = RgTextureType::Texture2D;
        std::uint32_t    Width                = 1;
        std::uint32_t    Height               = 1;
        std::uint32_t    DepthOrArraySize     = 1;
        std::uint16_t    MipLevels            = 1;
		bool			 AllowRenderTarget	  = false;
		bool			 AllowDepthStencil	  = false;
		bool			 AllowUnorderedAccess = false;
		RgClearValue	 ClearValue			  = {};
	};

	enum class RgViewType
	{
		Rtv,
		Dsv,
		BufferSrv,
		BufferUav,
		TextureSrv,
		TextureUav
	};

	struct RgRtv
	{
        bool          sRGB;
        std::uint32_t ArraySlice;
        std::uint32_t MipSlice;
        std::uint32_t ArraySize;
	};

	struct RgDsv
	{
        std::uint32_t ArraySlice;
        std::uint32_t MipSlice;
        std::uint32_t ArraySize;
	};

	struct RgBufferSrv
	{
        bool          Raw;
        std::uint32_t FirstElement;
        std::uint32_t NumElements;
	};

	struct RgBufferUav
	{
        std::uint32_t NumElements;
        std::uint64_t CounterOffsetInBytes;
	};

	struct RgTextureSrv
	{
        bool          sRGB;
        std::uint32_t MostDetailedMip;
        std::uint32_t MipLevels;
	};

	struct RgTextureUav
	{
        std::uint32_t ArraySlice;
        std::uint32_t MipSlice;
	};

	struct RgViewDesc
	{
		[[nodiscard]] bool operator==(const RgViewDesc& RgViewDesc) const noexcept
		{
			return memcmp(this, &RgViewDesc, sizeof(RgViewDesc)) == 0;
		}
		[[nodiscard]] bool operator!=(const RgViewDesc& RgViewDesc) const noexcept
		{
			return !(*this == RgViewDesc);
		}

		RgViewDesc& SetResource(RgResourceHandle Resource)
		{
			this->AssociatedResource = Resource;
			return *this;
		}

		RgViewDesc& AsRtv(bool sRGB, std::optional<UINT> OptArraySlice = std::nullopt, std::optional<UINT> OptMipSlice = std::nullopt, std::optional<UINT> OptArraySize = std::nullopt)
		{
			Type			 = RgViewType::Rtv;
			RgRtv.sRGB		 = sRGB;
			RgRtv.ArraySlice = OptArraySlice.value_or(-1);
			RgRtv.MipSlice	 = OptMipSlice.value_or(-1);
			RgRtv.ArraySize	 = OptArraySize.value_or(-1);
			return *this;
		}

		RgViewDesc& AsDsv(std::optional<UINT> OptArraySlice = std::nullopt, std::optional<UINT> OptMipSlice = std::nullopt, std::optional<UINT> OptArraySize = std::nullopt)
		{
			Type			 = RgViewType::Dsv;
			RgDsv.ArraySlice = OptArraySlice.value_or(-1);
			RgDsv.MipSlice	 = OptMipSlice.value_or(-1);
			RgDsv.ArraySize	 = OptArraySize.value_or(-1);
			return *this;
		}

		RgViewDesc& AsBufferSrv(bool Raw, std::uint32_t FirstElement, std::uint32_t NumElements)
		{
			Type				   = RgViewType::BufferSrv;
			BufferSrv.Raw		   = Raw;
			BufferSrv.FirstElement = FirstElement;
			BufferSrv.NumElements  = NumElements;
			return *this;
		}

		RgViewDesc& AsBufferUav(std::uint32_t NumElements, std::uint64_t CounterOffsetInBytes)
		{
			Type						   = RgViewType::BufferUav;
			BufferUav.NumElements		   = NumElements;
			BufferUav.CounterOffsetInBytes = CounterOffsetInBytes;
			return *this;
		}

		RgViewDesc& AsTextureSrv(bool                         sRGB            = false,
                                 std::optional<std::uint32_t> MostDetailedMip = std::nullopt,
                                 std::optional<std::uint32_t> MipLevels       = std::nullopt)
		{
			Type					   = RgViewType::TextureSrv;
			TextureSrv.sRGB			   = sRGB;
			TextureSrv.MostDetailedMip = MostDetailedMip.value_or(-1);
			TextureSrv.MipLevels	   = MipLevels.value_or(-1);
			return *this;
		}

		RgViewDesc& AsTextureUav(std::optional<std::uint32_t> ArraySlice = std::nullopt,
                                 std::optional<std::uint32_t> MipSlice   = std::nullopt)
		{
			Type				  = RgViewType::TextureUav;
			TextureUav.ArraySlice = ArraySlice.value_or(-1);
			TextureUav.MipSlice	  = MipSlice.value_or(-1);
			return *this;
		}

		RgResourceHandle AssociatedResource;
		RgViewType		 Type;
		union
		{
			RgRtv		 RgRtv;
			RgDsv		 RgDsv;
			RgBufferSrv	 BufferSrv;
			RgBufferUav	 BufferUav;
			RgTextureSrv TextureSrv;
			RgTextureUav TextureUav;
		};
	};

	struct RgResource
	{
		RgResourceHandle Handle;
	};

	struct RgBuffer : RgResource
	{
		RgBufferDesc Desc;
	};

	struct RgTexture : RgResource
	{
		RgTextureDesc Desc;
	};

	struct RgView : RgResource
	{
		RgViewDesc Desc;
	};

	template<typename T>
	struct RgResourceTraits
	{
		static constexpr auto Enum = RgResourceType::Unknown;
		using Desc				   = void;
		using Type				   = void;
	};

	template<>
	struct RgResourceTraits<D3D12Buffer>
	{
		static constexpr auto Enum = RgResourceType::Buffer;
		using ApiType			   = D3D12Buffer;
		using Desc				   = RgBufferDesc;
		using Type				   = RgBuffer;
	};

	template<>
	struct RgResourceTraits<D3D12Texture>
	{
		static constexpr auto Enum = RgResourceType::Texture;
		using ApiType			   = D3D12Texture;
		using Desc				   = RgTextureDesc;
		using Type				   = RgTexture;
	};

	template<>
	struct RgResourceTraits<D3D12RenderTargetView>
	{
		static constexpr auto Enum = RgResourceType::RenderTargetView;
		using ApiType			   = D3D12RenderTargetView;
		using Desc				   = RgViewDesc;
		using Type				   = RgView;
	};

	template<>
	struct RgResourceTraits<D3D12DepthStencilView>
	{
		static constexpr auto Enum = RgResourceType::DepthStencilView;
		using ApiType			   = D3D12DepthStencilView;
		using Desc				   = RgViewDesc;
		using Type				   = RgView;
	};

	template<>
	struct RgResourceTraits<D3D12ShaderResourceView>
	{
		static constexpr auto Enum = RgResourceType::ShaderResourceView;
		using ApiType			   = D3D12ShaderResourceView;
		using Desc				   = RgViewDesc;
		using Type				   = RgView;
	};

	template<>
	struct RgResourceTraits<D3D12UnorderedAccessView>
	{
		static constexpr auto Enum = RgResourceType::UnorderedAccessView;
		using ApiType			   = D3D12UnorderedAccessView;
		using Desc				   = RgViewDesc;
		using Type				   = RgView;
	};
} // namespace RHI

template<>
struct std::hash<RHI::RgResourceHandle>
{
	size_t operator()(const RHI::RgResourceHandle& RenderResourceHandle) const noexcept
	{
		return Hash::Hash64(&RenderResourceHandle, sizeof(RenderResourceHandle));
	}
};

template<>
struct robin_hood::hash<RHI::RgResourceHandle>
{
	size_t operator()(const RHI::RgResourceHandle& RenderResourceHandle) const noexcept
	{
		return Hash::Hash64(&RenderResourceHandle, sizeof(RenderResourceHandle));
	}
};
