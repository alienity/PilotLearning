#pragma once
#include "runtime/platform/system/system_core.h"
#include "runtime/function/render/rhi/rhi.h"
#include "runtime/function/render/rhi/d3d12/d3d12_resource.h"
#include "runtime/function/render/rhi/d3d12/d3d12_descriptor.h"
#include "runtime/core/base/utility.h"

namespace RHI
{
#define GImport(g, b) g.Import(b)
#define PassRead(p, b) p.Read(b)
#define PassWrite(p, b) p.Write(b)
#define PassReadIg(p, b) p.Read(b, true)
#define PassWriteIg(p, b) p.Write(b, true)

	enum class RgResourceType : std::uint64_t
	{
		Unknown,
		Buffer,
		Texture,
		RootSignature,
		PipelineState,
		RaytracingPipelineState,
	};

	enum RgResourceSubType : std::uint64_t
    {
		None,
        VertexAndConstantBuffer,
        IndirectArgBuffer,
        
	};

	enum RgResourceFlags : std::uint64_t
	{
		RG_RESOURCE_FLAG_NONE,
		RG_RESOURCE_FLAG_IMPORTED
	};

	enum RgBarrierFlag : std::uint64_t
    {
		Auto,
        None
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
    };

	inline bool operator==(const RgResourceHandle& lhs, const RgResourceHandle& rhs)
    {
        return lhs.Type == rhs.Type && lhs.Flags == rhs.Flags && lhs.Id == rhs.Id;
    }
    inline bool operator!=(const RgResourceHandle& lhs, const RgResourceHandle& rhs) { return !operator==(lhs, rhs); }
    inline bool operator<(const RgResourceHandle& lhs, const RgResourceHandle& rhs)
    {
        return lhs.Type < rhs.Type || (lhs.Type == rhs.Type && lhs.Flags < rhs.Flags) ||
               (lhs.Type == rhs.Type && lhs.Flags == rhs.Flags && lhs.Id < rhs.Id);
    }
    inline bool operator>(const RgResourceHandle& lhs, const RgResourceHandle& rhs) { return operator<(rhs, lhs); }
    inline bool operator<=(const RgResourceHandle& lhs, const RgResourceHandle& rhs) { return !operator>(lhs, rhs); }
    inline bool operator>=(const RgResourceHandle& lhs, const RgResourceHandle& rhs) { return !operator<(lhs, rhs); }

	static_assert(sizeof(RgResourceHandle) == sizeof(std::uint64_t));

    struct RgResourceHandleExt
    {
        RgResourceHandle  rgHandle;
        RgResourceSubType rgSubType : 32;
        RgBarrierFlag     rgTransFlag : 32;
    };
    inline bool operator==(const RgResourceHandleExt& lhs, const RgResourceHandleExt& rhs)
    {
        return lhs.rgHandle == rhs.rgHandle && lhs.rgSubType == rhs.rgSubType && lhs.rgTransFlag == rhs.rgTransFlag;
    }
    inline bool operator!=(const RgResourceHandleExt& lhs, const RgResourceHandleExt& rhs) { return !operator==(lhs, rhs); }
    inline bool operator<(const RgResourceHandleExt& lhs, const RgResourceHandleExt& rhs)
    {
        return lhs.rgHandle < rhs.rgHandle || (lhs.rgHandle == rhs.rgHandle && lhs.rgSubType < rhs.rgSubType) ||
               (lhs.rgHandle == rhs.rgHandle && lhs.rgSubType == rhs.rgSubType && lhs.rgTransFlag < rhs.rgTransFlag);
    }
    inline bool operator>(const RgResourceHandleExt& lhs, const RgResourceHandleExt& rhs) { return operator<(rhs, lhs); }
    inline bool operator<=(const RgResourceHandleExt& lhs, const RgResourceHandleExt& rhs) { return !operator>(lhs, rhs); }
    inline bool operator>=(const RgResourceHandleExt& lhs, const RgResourceHandleExt& rhs) { return !operator<(lhs, rhs); }

    static_assert(sizeof(RgResourceHandleExt) == sizeof(std::uint64_t) * 2);

	inline RgResourceHandleExt ToRgResourceHandle(RgResourceHandle& rgHandle, RgResourceSubType subType)
	{
        RgResourceHandleExt rgResourceHandle = {};
        rgResourceHandle.rgHandle = rgHandle;
        rgResourceHandle.rgSubType = subType;
        return rgResourceHandle;
	}

	inline std::vector<RgResourceHandleExt> ToRgResourceHandle(std::vector<RgResourceHandle>& rgHandles, RgResourceSubType subType)
    {
        std::vector<RgResourceHandleExt> rgResourceHandles;
        for (size_t i = 0; i < rgHandles.size(); i++)
        {
            RgResourceHandleExt rgResourceHandle = {};
            rgResourceHandle.rgHandle = rgHandles[i];
            rgResourceHandle.rgSubType = subType;
            rgResourceHandles.push_back(rgResourceHandle);
		}
        return rgResourceHandles;
    }

	inline RgResourceHandleExt ToRgResourceHandle(RgResourceHandle& rgHandle, bool ignoreBarrier)
	{
        RgResourceHandleExt rgResourceHandle = {};
        rgResourceHandle.rgHandle  = rgHandle;
        rgResourceHandle.rgTransFlag = ignoreBarrier ? RgBarrierFlag::None : RgBarrierFlag::Auto;
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
