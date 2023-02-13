#pragma once
#include "runtime/platform/system/system_core.h"
#include "runtime/function/render/rhi/rhi.h"
#include "runtime/function/render/rhi/d3d12/d3d12_resource.h"
#include "runtime/function/render/rhi/d3d12/d3d12_descriptor.h"
#include "runtime/core/base/utility.h"

namespace RHI
{
	enum class RgResourceType : std::uint64_t
	{
		Unknown,
		Buffer,
		Texture,
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

		[[nodiscard]] bool operator==(const RgResourceHandle& rhs) const noexcept
        {
            return Utility::Hash64(this, sizeof(RgResourceHandle)) == Utility::Hash64(&rhs, sizeof(RgResourceHandle));
        }
        [[nodiscard]] bool operator!=(const RgResourceHandle& rhs) const noexcept
        {
            return Utility::Hash64(this, sizeof(RgResourceHandle)) != Utility::Hash64(&rhs, sizeof(RgResourceHandle));
        }

        void Invalidate()
        {
            Type    = RgResourceType::Unknown;
            Flags   = RG_RESOURCE_FLAG_NONE;
            Version = 0;
            Id      = UINT_MAX;
        }

        RgResourceType Type : 15; // 14 bit to represent type, might remove some bits from this and give it to version
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
        const RgClearValue(const std::float_t Color[4], DXGI_FORMAT Format = DXGI_FORMAT_UNKNOWN)
        {
            this->Color[0] = Color[0];
            this->Color[1] = Color[1];
            this->Color[2] = Color[2];
            this->Color[3] = Color[3];
            this->ClearFormat = Format;
        }
        const RgClearValue(float r, float g, float b, float a, DXGI_FORMAT Format = DXGI_FORMAT_UNKNOWN)
        {
            this->Color[0] = r;
            this->Color[1] = g;
            this->Color[2] = b;
            this->Color[3] = a;
            this->ClearFormat = Format;
        }
        const RgClearValue(std::float_t Depth, std::uint8_t Stencil, DXGI_FORMAT Format = DXGI_FORMAT_UNKNOWN) :
            DepthStencil {Depth, Stencil}, ClearFormat(Format)
        {}

        std::float_t        Color[4];
        RgDepthStencilValue DepthStencil;
        DXGI_FORMAT ClearFormat;
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

		RgTextureDesc& SetSampleCount(std::uint32_t SampleCount = 1)
        {
            this->SampleCount = SampleCount;
            return *this;
        }

		RgTextureDesc& SetMipLevels(std::uint16_t MipLevels)
		{
			this->MipLevels = MipLevels;
			return *this;
		}

		RgTextureDesc& SetAllowRenderTarget(bool allow = true)
		{
            AllowRenderTarget = allow;
			return *this;
		}

		RgTextureDesc& SetAllowDepthStencil(bool allow = true)
		{
			AllowDepthStencil = allow;
			return *this;
		}

		RgTextureDesc& SetAllowUnorderedAccess(bool allow = true)
		{
            AllowUnorderedAccess = allow;
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
        std::uint32_t    SampleCount          = 1;
		bool			 AllowRenderTarget	  = false;
		bool			 AllowDepthStencil	  = false;
		bool			 AllowUnorderedAccess = false;
		RgClearValue	 ClearValue			  = {};
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

} // namespace RHI

#define GetRealVal(realType, renderGraphRegister, rgHandle)\
    

template<>
struct std::hash<RHI::RgResourceHandle>
{
	size_t operator()(const RHI::RgResourceHandle& RenderResourceHandle) const noexcept
	{
        return Utility::Hash64(&RenderResourceHandle, sizeof(RenderResourceHandle));
	}
};

template<>
struct robin_hood::hash<RHI::RgResourceHandle>
{
	size_t operator()(const RHI::RgResourceHandle& RenderResourceHandle) const noexcept
	{
        return Utility::Hash64(&RenderResourceHandle, sizeof(RenderResourceHandle));
	}
};
