#pragma once
#include "runtime/platform/system/system_core.h"
#include "runtime/function/render/rhi/rhi.h"
#include "runtime/function/render/rhi/d3d12/d3d12_resource.h"
#include "runtime/function/render/rhi/d3d12/d3d12_descriptor.h"
#include "runtime/core/base/utility.h"

namespace RHI
{
#define PassHandle(pn, d) pn##_##d
#define GImport(g, b) g.Import(b)
#define PassHandleDeclare(p, d, b) auto p##_##d = b

#define HandleOps(h)\
    inline bool operator!=(const h& lhs, const h& rhs) { return !operator==(lhs, rhs); } \
    inline bool operator>(const h& lhs, const h& rhs) { return operator<(rhs, lhs); } \
    inline bool operator<=(const h& lhs, const h& rhs) { return !operator>(lhs, rhs); } \
    inline bool operator>=(const h& lhs, const h& rhs) { return !operator<(lhs, rhs); } \

	enum class RgResourceType : std::uint64_t
	{
		Unknown,
		Buffer,
		Texture,
		RootSignature,
		PipelineState,
		RaytracingPipelineState,
	};

    typedef RHIResourceState RgResourceState;

	enum RgResourceFlags : std::uint64_t
	{
		RG_RESOURCE_FLAG_NONE,
		RG_RESOURCE_FLAG_IMPORTED
	};

	enum class RgBarrierFlag : std::uint64_t
    {
		AutoBarrier,
        NoneBarrier
	};

	// A virtual resource handle, the underlying realization of the resource type is done in RenderGraphRegistry
    struct RgResourceHandle
    {
        [[nodiscard]] bool IsValid() const noexcept { return Type != RgResourceType::Unknown && Id != UINT_MAX; }
        [[nodiscard]] bool IsImported() const noexcept { return Flags & RG_RESOURCE_FLAG_IMPORTED; }

        void Invalidate()
        {
            Type        = RgResourceType::Unknown;
            Flags       = RG_RESOURCE_FLAG_NONE;
            Version     = 0;
            Id          = UINT_MAX;
        }

        RgResourceType    Type : 14; // 14 bit to represent type, might remove some bits from this and give it to version
        RgResourceFlags   Flags : 2;
        std::uint64_t     Version : 16; // 16 bits to represent version should be more than enough, we can always just increase bit used if is not enough
        std::uint64_t     Id : 32; // 32 bit unsigned int

        friend bool operator==(const RgResourceHandle& l, const RgResourceHandle& r)
        {
            return l.Type == r.Type && l.Flags == r.Flags && l.Version == r.Version && l.Id == r.Id;
        }
        friend bool operator<(const RgResourceHandle& l, const RgResourceHandle& r)
        {
            return std::tie(l.Type, l.Flags, l.Version, l.Id) <
                   std::tie(r.Type, r.Flags, r.Version, r.Id); // keep the same order
        }
    };
    HandleOps(RgResourceHandle)

	static_assert(sizeof(RgResourceHandle) == sizeof(std::uint64_t));

    extern RgResourceHandle _DefaultRgResourceHandle;
    #define DefaultRgResourceHandle _DefaultRgResourceHandle

    struct RgResourceHandleExt
    {
        RgResourceHandle rgHandle;
        RgResourceState  rgSubType : 64;
        RgResourceState  rgCounterType : 64;
        RgBarrierFlag    rgTransFlag : 64;

        friend bool operator==(const RgResourceHandleExt& l, const RgResourceHandleExt& r)
        {
            return l.rgHandle == r.rgHandle && l.rgSubType == r.rgSubType && l.rgCounterType == r.rgCounterType &&
                   l.rgTransFlag == r.rgTransFlag;
        }
        friend bool operator<(const RgResourceHandleExt& l, const RgResourceHandleExt& r)
        {
            return std::tie(l.rgHandle, l.rgSubType, l.rgCounterType, l.rgTransFlag) <
                   std::tie(r.rgHandle, r.rgSubType, r.rgCounterType, r.rgTransFlag); // keep the same order
        }
    };
    HandleOps(RgResourceHandleExt)

    static_assert(sizeof(RgResourceHandleExt) == sizeof(std::uint64_t) * 4);

    extern RgResourceHandleExt _DefaultRgResourceHandleExt;
    #define DefaultRgResourceHandleExt _DefaultRgResourceHandleExt

	inline RgResourceHandleExt ToRgResourceHandle(RgResourceHandle& rgHandle, RgResourceState subType, RgResourceState counterType, bool ignoreBarrier)
	{
        RgResourceHandleExt rgResourceHandle = {};
        rgResourceHandle.rgHandle      = rgHandle;
        rgResourceHandle.rgSubType     = subType;
        rgResourceHandle.rgCounterType = counterType;
        rgResourceHandle.rgTransFlag   = ignoreBarrier ? RgBarrierFlag::NoneBarrier : RgBarrierFlag::AutoBarrier;
        return rgResourceHandle;
	}

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
		RgClearValue(const std::float_t Color[4], DXGI_FORMAT Format = DXGI_FORMAT_UNKNOWN)
		{
			this->Color[0] = Color[0];
			this->Color[1] = Color[1];
			this->Color[2] = Color[2];
			this->Color[3] = Color[3];
			this->ClearFormat = Format;
		}
        RgClearValue(float r, float g, float b, float a, DXGI_FORMAT Format = DXGI_FORMAT_UNKNOWN)
        {
            this->Color[0] = r;
            this->Color[1] = g;
            this->Color[2] = b;
            this->Color[3] = a;
            this->ClearFormat = Format;
        }
        RgClearValue(std::float_t Depth, std::uint8_t Stencil, DXGI_FORMAT Format = DXGI_FORMAT_UNKNOWN) :
            DepthStencil {Depth, Stencil}, ClearFormat(Format)
        {}

        std::float_t        Color[4];
        RgDepthStencilValue DepthStencil;
        DXGI_FORMAT ClearFormat;
	};

    struct RgBufferDesc
    {
        RgBufferDesc() noexcept = default;
        RgBufferDesc(std::string_view Name) : mName(Name) {}

        [[nodiscard]] bool operator==(const RgBufferDesc& RgBufferDesc) const noexcept
        {
            return memcmp(this, &RgBufferDesc, sizeof(RgBufferDesc)) == 0;
        }
        [[nodiscard]] bool operator!=(const RgBufferDesc& RgBufferDesc) const noexcept
        {
            return !(*this == RgBufferDesc);
        }

        RgBufferDesc& SetSize(std::uint64_t numElements, std::uint64_t elementSize)
        {
            this->mNumElements = numElements;
            this->mElementSize = elementSize;
            return *this;
        }

        RgBufferDesc& SetRHIBufferMode(RHIBufferMode rhiBufferMode)
        {
            mRHIBufferMode = rhiBufferMode;
            return *this;
        }

        RgBufferDesc& SetRHIBufferTarget(RHIBufferTarget rhiBufferTarget)
        {
            mRHIBufferTarget = rhiBufferTarget;
            return *this;
        }

        RgBufferDesc& AllowUnorderedAccess(bool allow = true)
        {
			if (allow)
			{
                mRHIBufferTarget |= RHIBufferTarget::RHIBufferRandomReadWrite;
			}
			else
			{
                mRHIBufferTarget &= ~RHIBufferTarget::RHIBufferRandomReadWrite;
			}
            return *this;
        }

		RgBufferDesc& AsRowBuffer(bool raw = true)
        {
            if (raw)
            {
                mRHIBufferTarget |= RHIBufferTarget::RHIBufferTargetRaw;
            }
            else
            {
                mRHIBufferTarget &= ~RHIBufferTarget::RHIBufferTargetRaw;
            }
            return *this;
        }

		std::string_view mName;
        std::uint64_t    mNumElements     = 1;
        std::uint64_t    mElementSize     = 4;
        RHIBufferMode    mRHIBufferMode   = RHIBufferMode::RHIBufferModeImmutable;
        RHIBufferTarget  mRHIBufferTarget = RHIBufferTarget::RHIBufferTargetNone;
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

template<>
struct std::hash<RHI::RgResourceHandle>
{
    size_t operator()(const RHI::RgResourceHandle& RenderResourceHandle) const noexcept
    {
        RHI::RgResourceHandle tmpHandle = RenderResourceHandle;
        tmpHandle.Version = 0;
        return Utility::Hash64(&tmpHandle, sizeof(tmpHandle));
    }
};

template<>
struct robin_hood::hash<RHI::RgResourceHandle>
{
    size_t operator()(const RHI::RgResourceHandle& RenderResourceHandle) const noexcept
    {
        RHI::RgResourceHandle tmpHandle = RenderResourceHandle;
        tmpHandle.Version = 0;
        return Utility::Hash64(&tmpHandle, sizeof(tmpHandle));
    }
};

template<>
struct std::hash<RHI::RgResourceHandleExt>
{
    size_t operator()(const RHI::RgResourceHandleExt& RenderResourceHandleExt) const noexcept
    {
        RHI::RgResourceHandleExt tmpHandleExt = RenderResourceHandleExt;
        tmpHandleExt.rgHandle.Version = 0;
        return Utility::Hash64(&tmpHandleExt, sizeof(tmpHandleExt));
    }
};

template<>
struct robin_hood::hash<RHI::RgResourceHandleExt>
{
    size_t operator()(const RHI::RgResourceHandleExt& RenderResourceHandleExt) const noexcept
    {
        RHI::RgResourceHandleExt tmpHandleExt = RenderResourceHandleExt;
        tmpHandleExt.rgHandle.Version = 0;
        return Utility::Hash64(&tmpHandleExt, sizeof(tmpHandleExt));
    }
};
