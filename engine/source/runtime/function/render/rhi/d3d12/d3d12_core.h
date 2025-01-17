#pragma once
#include "d3d12_rhi.h"
#include "../rhi_core.h"

namespace D3D12RHIUtils
{
    // Helper to check for power-of-2
    template<typename T>
    constexpr bool IsPowerOf2(T x) noexcept
    {
        return ((x != 0) && !(x & (x - 1)));
    }

    // Helpers for aligning values by a power of 2
    template<typename T>
    inline T AlignDown(T size, size_t alignment) noexcept
    {
        if (alignment > 0)
        {
            assert(((alignment - 1) & alignment) == 0);
            auto mask = static_cast<T>(alignment - 1);
            return size & ~mask;
        }
        return size;
    }

    template<typename T>
    inline T AlignUp(T size, size_t alignment) noexcept
    {
        if (alignment > 0)
        {
            assert(((alignment - 1) & alignment) == 0);
            auto mask = static_cast<T>(alignment - 1);
            return (size + mask) & ~mask;
        }
        return size;
    }

    // Compute the number of texture levels needed to reduce to 1x1.  This uses
    // _BitScanReverse to find the highest set bit.  Each dimension reduces by
    // half and truncates bits.  The dimension 256 (0x100) has 9 mip levels, same
    // as the dimension 511 (0x1FF).
    static inline uint32_t ComputeNumMips(uint32_t Width, uint32_t Height)
    {
        uint32_t HighBit;
        _BitScanReverse((unsigned long*)&HighBit, Width | Height);
        return HighBit + 1;
    }

    //--------------------------------------------------------------------------------------
    // Return the BPP for a particular format
    //--------------------------------------------------------------------------------------
    constexpr bool IsValid(_In_ DXGI_FORMAT fmt) noexcept;
    bool IsCompressed(DXGI_FORMAT fmt) noexcept;
    bool IsPacked(_In_ DXGI_FORMAT fmt) noexcept;
    bool IsPlanar(_In_ DXGI_FORMAT fmt) noexcept;
    bool IsPalettized(_In_ DXGI_FORMAT fmt) noexcept;
    bool IsDepthStencil(_In_ DXGI_FORMAT fmt) noexcept;
    bool IsSRGB(_In_ DXGI_FORMAT fmt) noexcept;
    bool IsBGR(_In_ DXGI_FORMAT fmt) noexcept;
    bool IsTypeless(_In_ DXGI_FORMAT fmt, _In_ bool partialTypeless = true) noexcept;

    bool HasAlpha(_In_ DXGI_FORMAT fmt) noexcept;

    size_t BitsPerPixel(_In_ DXGI_FORMAT fmt) noexcept;
    inline size_t BytesPerPixel(DXGI_FORMAT Format) { return BitsPerPixel(Format) / 8; };

    size_t BitsPerColor(_In_ DXGI_FORMAT fmt) noexcept;
    
    DXGI_FORMAT MakeSRGB(DXGI_FORMAT fmt) noexcept;
    DXGI_FORMAT MakeLinear(DXGI_FORMAT fmt) noexcept;
    DXGI_FORMAT MakeTypeless(DXGI_FORMAT fmt) noexcept;
    DXGI_FORMAT MakeTypelessUNORM(DXGI_FORMAT fmt) noexcept;
    DXGI_FORMAT MakeTypelessFLOAT(DXGI_FORMAT fmt) noexcept;

    //--------------------------------------------------------------------------------------
    
    //--------------------------------------------------------------------------------------
    inline DXGI_FORMAT GetBaseFormat(DXGI_FORMAT defaultFormat)
    {
        switch (defaultFormat)
        {
            case DXGI_FORMAT_R8G8B8A8_UNORM:
            case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
                return DXGI_FORMAT_R8G8B8A8_TYPELESS;

            case DXGI_FORMAT_B8G8R8A8_UNORM:
            case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
                return DXGI_FORMAT_B8G8R8A8_TYPELESS;

            case DXGI_FORMAT_B8G8R8X8_UNORM:
            case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
                return DXGI_FORMAT_B8G8R8X8_TYPELESS;

            // 32-bit Z w/ Stencil
            case DXGI_FORMAT_R32G8X24_TYPELESS:
            case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
            case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
                return DXGI_FORMAT_R32G8X24_TYPELESS;

            // No Stencil
            case DXGI_FORMAT_R32_TYPELESS:
            case DXGI_FORMAT_D32_FLOAT:
            case DXGI_FORMAT_R32_FLOAT:
                return DXGI_FORMAT_R32_TYPELESS;

            // 24-bit Z
            case DXGI_FORMAT_R24G8_TYPELESS:
            case DXGI_FORMAT_D24_UNORM_S8_UINT:
            case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
            case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
                return DXGI_FORMAT_R24G8_TYPELESS;

            // 16-bit Z w/o Stencil
            case DXGI_FORMAT_R16_TYPELESS:
            case DXGI_FORMAT_D16_UNORM:
            case DXGI_FORMAT_R16_UNORM:
                return DXGI_FORMAT_R16_TYPELESS;

            default:
                return defaultFormat;
        }
    }

    inline DXGI_FORMAT GetUAVFormat(DXGI_FORMAT defaultFormat)
    {
        switch (defaultFormat)
        {
            case DXGI_FORMAT_R8G8B8A8_TYPELESS:
            case DXGI_FORMAT_R8G8B8A8_UNORM:
            case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
                return DXGI_FORMAT_R8G8B8A8_UNORM;

            case DXGI_FORMAT_B8G8R8A8_TYPELESS:
            case DXGI_FORMAT_B8G8R8A8_UNORM:
            case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
                return DXGI_FORMAT_B8G8R8A8_UNORM;

            case DXGI_FORMAT_B8G8R8X8_TYPELESS:
            case DXGI_FORMAT_B8G8R8X8_UNORM:
            case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
                return DXGI_FORMAT_B8G8R8X8_UNORM;

            case DXGI_FORMAT_R32_TYPELESS:
            case DXGI_FORMAT_R32_FLOAT:
                return DXGI_FORMAT_R32_FLOAT;

#ifdef _DEBUG
            case DXGI_FORMAT_R32G8X24_TYPELESS:
            case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
            case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
            case DXGI_FORMAT_D32_FLOAT:
            case DXGI_FORMAT_R24G8_TYPELESS:
            case DXGI_FORMAT_D24_UNORM_S8_UINT:
            case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
            case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
            case DXGI_FORMAT_D16_UNORM:

                ASSERT(false); //, "Requested a UAV Format for a depth stencil Format.");
#endif

            default:
                return defaultFormat;
        }
    }
    
    inline DXGI_FORMAT GetDSVFormat(DXGI_FORMAT defaultFormat)
    {
        switch (defaultFormat)
        {
            // 32-bit Z w/ Stencil
            case DXGI_FORMAT_R32G8X24_TYPELESS:
            case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
            case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
                return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;

            // No Stencil
            case DXGI_FORMAT_R32_TYPELESS:
            case DXGI_FORMAT_D32_FLOAT:
            case DXGI_FORMAT_R32_FLOAT:
                return DXGI_FORMAT_D32_FLOAT;

            // 24-bit Z
            case DXGI_FORMAT_R24G8_TYPELESS:
            case DXGI_FORMAT_D24_UNORM_S8_UINT:
            case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
            case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
                return DXGI_FORMAT_D24_UNORM_S8_UINT;

            // 16-bit Z w/o Stencil
            case DXGI_FORMAT_R16_TYPELESS:
            case DXGI_FORMAT_D16_UNORM:
            case DXGI_FORMAT_R16_UNORM:
                return DXGI_FORMAT_D16_UNORM;

            default:
                return defaultFormat;
        }
    }

    inline DXGI_FORMAT GetDepthFormat(DXGI_FORMAT defaultFormat)
    {
        switch (defaultFormat)
        {
            // 32-bit Z w/ Stencil
            case DXGI_FORMAT_R32G8X24_TYPELESS:
            case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
            case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
                return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;

            // No Stencil
            case DXGI_FORMAT_R32_TYPELESS:
            case DXGI_FORMAT_D32_FLOAT:
            case DXGI_FORMAT_R32_FLOAT:
                return DXGI_FORMAT_R32_FLOAT;

            // 24-bit Z
            case DXGI_FORMAT_R24G8_TYPELESS:
            case DXGI_FORMAT_D24_UNORM_S8_UINT:
            case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
            case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
                return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;

            // 16-bit Z w/o Stencil
            case DXGI_FORMAT_R16_TYPELESS:
            case DXGI_FORMAT_D16_UNORM:
            case DXGI_FORMAT_R16_UNORM:
                return DXGI_FORMAT_R16_UNORM;

            default:
                return DXGI_FORMAT_UNKNOWN;
        }
    }

    inline DXGI_FORMAT GetStencilFormat(DXGI_FORMAT defaultFormat)
    {
        switch (defaultFormat)
        {
            // 32-bit Z w/ Stencil
            case DXGI_FORMAT_R32G8X24_TYPELESS:
            case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
            case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
                return DXGI_FORMAT_X32_TYPELESS_G8X24_UINT;

            // 24-bit Z
            case DXGI_FORMAT_R24G8_TYPELESS:
            case DXGI_FORMAT_D24_UNORM_S8_UINT:
            case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
            case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
                return DXGI_FORMAT_X24_TYPELESS_G8_UINT;

            default:
                return DXGI_FORMAT_UNKNOWN;
        }
    }

    inline bool FormatIsUAVCompatible(_In_ ID3D12Device* device, bool typedUAVLoadAdditionalFormats, DXGI_FORMAT format) noexcept
    {
        switch (format)
        {
            case DXGI_FORMAT_R32_FLOAT:
            case DXGI_FORMAT_R32_UINT:
            case DXGI_FORMAT_R32_SINT:
                // Unconditionally supported.
                return true;

            case DXGI_FORMAT_R32G32B32A32_FLOAT:
            case DXGI_FORMAT_R32G32B32A32_UINT:
            case DXGI_FORMAT_R32G32B32A32_SINT:
            case DXGI_FORMAT_R16G16B16A16_FLOAT:
            case DXGI_FORMAT_R16G16B16A16_UINT:
            case DXGI_FORMAT_R16G16B16A16_SINT:
            case DXGI_FORMAT_R8G8B8A8_UNORM:
            case DXGI_FORMAT_R8G8B8A8_UINT:
            case DXGI_FORMAT_R8G8B8A8_SINT:
            case DXGI_FORMAT_R16_FLOAT:
            case DXGI_FORMAT_R16_UINT:
            case DXGI_FORMAT_R16_SINT:
            case DXGI_FORMAT_R8_UNORM:
            case DXGI_FORMAT_R8_UINT:
            case DXGI_FORMAT_R8_SINT:
                // All these are supported if this optional feature is set.
                return typedUAVLoadAdditionalFormats;

            case DXGI_FORMAT_R16G16B16A16_UNORM:
            case DXGI_FORMAT_R16G16B16A16_SNORM:
            case DXGI_FORMAT_R32G32_FLOAT:
            case DXGI_FORMAT_R32G32_UINT:
            case DXGI_FORMAT_R32G32_SINT:
            case DXGI_FORMAT_R10G10B10A2_UNORM:
            case DXGI_FORMAT_R10G10B10A2_UINT:
            case DXGI_FORMAT_R11G11B10_FLOAT:
            case DXGI_FORMAT_R8G8B8A8_SNORM:
            case DXGI_FORMAT_R16G16_FLOAT:
            case DXGI_FORMAT_R16G16_UNORM:
            case DXGI_FORMAT_R16G16_UINT:
            case DXGI_FORMAT_R16G16_SNORM:
            case DXGI_FORMAT_R16G16_SINT:
            case DXGI_FORMAT_R8G8_UNORM:
            case DXGI_FORMAT_R8G8_UINT:
            case DXGI_FORMAT_R8G8_SNORM:
            case DXGI_FORMAT_R8G8_SINT:
            case DXGI_FORMAT_R16_UNORM:
            case DXGI_FORMAT_R16_SNORM:
            case DXGI_FORMAT_R8_SNORM:
            case DXGI_FORMAT_A8_UNORM:
            case DXGI_FORMAT_B5G6R5_UNORM:
            case DXGI_FORMAT_B5G5R5A1_UNORM:
            case DXGI_FORMAT_B4G4R4A4_UNORM:
                // Conditionally supported by specific devices.
                if (typedUAVLoadAdditionalFormats)
                {
                    D3D12_FEATURE_DATA_FORMAT_SUPPORT formatSupport = {
                        format, D3D12_FORMAT_SUPPORT1_NONE, D3D12_FORMAT_SUPPORT2_NONE};
                    if (SUCCEEDED(device->CheckFeatureSupport(
                            D3D12_FEATURE_FORMAT_SUPPORT, &formatSupport, sizeof(formatSupport))))
                    {
                        const DWORD mask = D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD | D3D12_FORMAT_SUPPORT2_UAV_TYPED_STORE;
                        return ((formatSupport.Support2 & mask) == mask);
                    }
                }
                return false;

            default:
                return false;
        }
    }

    inline DXGI_FORMAT ConvertSRVtoResourceFormat(DXGI_FORMAT format) noexcept
    {
        switch (format)
        {
            case DXGI_FORMAT_R32G32B32A32_FLOAT:
            case DXGI_FORMAT_R32G32B32A32_UINT:
            case DXGI_FORMAT_R32G32B32A32_SINT:
                return DXGI_FORMAT_R32G32B32A32_TYPELESS;

            case DXGI_FORMAT_R16G16B16A16_FLOAT:
            case DXGI_FORMAT_R16G16B16A16_UNORM:
            case DXGI_FORMAT_R16G16B16A16_UINT:
            case DXGI_FORMAT_R16G16B16A16_SNORM:
            case DXGI_FORMAT_R16G16B16A16_SINT:
                return DXGI_FORMAT_R16G16B16A16_TYPELESS;

            case DXGI_FORMAT_R32G32_FLOAT:
            case DXGI_FORMAT_R32G32_UINT:
            case DXGI_FORMAT_R32G32_SINT:
                return DXGI_FORMAT_R32G32_TYPELESS;

            case DXGI_FORMAT_R10G10B10A2_UNORM:
            case DXGI_FORMAT_R10G10B10A2_UINT:
                return DXGI_FORMAT_R10G10B10A2_TYPELESS;

            case DXGI_FORMAT_R8G8B8A8_UNORM:
            case DXGI_FORMAT_R8G8B8A8_UINT:
            case DXGI_FORMAT_R8G8B8A8_SNORM:
            case DXGI_FORMAT_R8G8B8A8_SINT:
                return DXGI_FORMAT_R8G8B8A8_TYPELESS;

            case DXGI_FORMAT_R16G16_FLOAT:
            case DXGI_FORMAT_R16G16_UNORM:
            case DXGI_FORMAT_R16G16_UINT:
            case DXGI_FORMAT_R16G16_SNORM:
            case DXGI_FORMAT_R16G16_SINT:
                return DXGI_FORMAT_R16G16_TYPELESS;

            case DXGI_FORMAT_R32_FLOAT:
            case DXGI_FORMAT_R32_UINT:
            case DXGI_FORMAT_R32_SINT:
                return DXGI_FORMAT_R32_TYPELESS;

            case DXGI_FORMAT_R8G8_UNORM:
            case DXGI_FORMAT_R8G8_UINT:
            case DXGI_FORMAT_R8G8_SNORM:
            case DXGI_FORMAT_R8G8_SINT:
                return DXGI_FORMAT_R8G8_TYPELESS;

            case DXGI_FORMAT_R16_FLOAT:
            case DXGI_FORMAT_R16_UNORM:
            case DXGI_FORMAT_R16_UINT:
            case DXGI_FORMAT_R16_SNORM:
            case DXGI_FORMAT_R16_SINT:
                return DXGI_FORMAT_R16_TYPELESS;

            case DXGI_FORMAT_R8_UNORM:
            case DXGI_FORMAT_R8_UINT:
            case DXGI_FORMAT_R8_SNORM:
            case DXGI_FORMAT_R8_SINT:
                return DXGI_FORMAT_R8_TYPELESS;

            default:
                return format;
        }
    }

} // namespace D3D12RHIUtils

namespace RHI
{

    struct DWParam
    {
        DWParam(FLOAT f) : Float(f) {}
        DWParam(UINT u) : Uint(u) {}
        DWParam(INT i) : Int(i) {}

        void operator=(FLOAT f) { Float = f; }
        void operator=(UINT u) { Uint = u; }
        void operator=(INT i) { Int = i; }

        union
        {
            FLOAT Float;
            UINT  Uint;
            INT   Int;
        };
    };
    
    enum RHIIndexFormat
    {
        RHIIndexFormat16,
        RHIIndexFormat32,
        RHIIndexFormatCount
    };

    enum RHIBufferMode
    {
        RHIBufferModeImmutable = 0,
        RHIBufferModeDynamic   = 1,
        RHIBufferModeScratch   = 3, // do not use
        RHIBufferModeCount
    };

    enum RHIBufferTarget
    {
        RHIBufferTargetNone                            = 0,
        RHIBufferTargetVertex                          = (1 << 0),
        RHIBufferTargetIndex                           = (1 << 1),
        RHIBufferReadBack                              = (1 << 2),
        RHIBufferRandomReadWrite                       = (1 << 3),
        RHIBufferTargetStructured                      = (1 << 4),
        RHIBufferTargetRaw                             = (1 << 5),
        RHIBufferTargetAppend                          = (1 << 6),
        RHIBufferTargetCounter                         = (1 << 7),
        RHIBufferTargetIndirectArgs                    = (1 << 8),
        RHIBufferTargetRayTracingAccelerationStructure = (1 << 10),
        RHIBufferTargetRayTracingShaderTable           = (1 << 11),
        RHIBufferTargetComputeNeeded = RHIBufferTargetStructured | RHIBufferTargetRaw | RHIBufferTargetAppend |
                                       RHIBufferTargetCounter | RHIBufferTargetIndirectArgs |
                                       RHIBufferTargetRayTracingAccelerationStructure,
    };
    DEFINE_ENUM_FLAG_OPERATORS(RHIBufferTarget);

    struct RHIBufferDesc
    {
        bool operator==(const RHIBufferDesc& o) const
        {
            return size == o.size && number == o.number && stride == o.stride && target == o.target && mode == o.mode;
        }
        bool operator!=(const RHIBufferDesc& o) const { return !(*this == o); }

        UINT            size   = 0;                   // buffer的字节数
        UINT            number = 0;                   // element的数量
        UINT32          stride = 0;                   // index/vertex/struct的大小
        RHIBufferTarget target = RHIBufferTargetNone; // buffer使用来干啥
        RHIBufferMode mode = RHIBufferModeImmutable;  // 更新模式，是静态的，动态的，或者是可读回的
    };

    struct RHIBufferData
    {
        BYTE*  m_Data = nullptr;
        UINT64 m_DataLen = 0;
    };

    enum RHISurfaceCreateFlags
    {
        RHISurfaceCreateFlagNone            = 0,
        RHISurfaceCreateRenderTarget        = (1 << 0),
        RHISurfaceCreateDepthStencil        = (1 << 1),
        RHISurfaceCreateRandomWrite         = (1 << 3),
        RHISurfaceCreateMipmap              = (1 << 4),
        RHISurfaceCreateAutoGenMips         = (1 << 5),
        RHISurfaceCreateSRGB                = (1 << 6),
        RHISurfaceCreateShadowmap           = (1 << 7),
        RHISurfaceCreateSampleOnly          = (1 << 8),
        RHISurfaceRenderTextureAsBackBuffer = (1 << 9),
        RHISurfaceCreateBindMS              = (1 << 10)
    };
    DEFINE_ENUM_FLAG_OPERATORS(RHISurfaceCreateFlags);

    // 最大允许的mipmap数是12
    #define MAXMIPLEVELS 12

    enum RHITextureDimension
    {
        RHITexDimUnknown = -1, // unknown
        RHITexDimNone    = 0,  // no texture
        RHITexDim2D,
        RHITexDim3D,
        RHITexDimCube,
        RHITexDim2DArray,
        RHITexDimCubeArray,
        RHITexDimCount
    };

    enum RHIRenderTextureSubElement
    {
        RHIRenderTextureSubElementColor,
        RHIRenderTextureSubElementDepth,
        RHIRenderTextureSubElementStencil,
        RHIRenderTextureSubElementDefault
    };

    struct RHIRenderSurfaceBaseDesc
    {
        //RHIResourceID         textureID;
        UINT64                width;
        UINT64                height;
        UINT64                depthOrArray;
        UINT32                samples;
        INT32                 mipCount;
        RHISurfaceCreateFlags flags;
        RHITextureDimension   dim;
        DXGI_FORMAT           graphicsFormat;
        DXGI_FORMAT           clearFormat;
        bool                  colorSurface;
        bool                  backBuffer;
    };

}

namespace RHI
{
    enum RHID3D12CommandQueueType
    {
        Direct,
        AsyncCompute,
        Copy1, // High frequency copies from upload to default heap
        Copy2, // Data initialization during resource creation
    };

	class D3D12Exception : public std::exception
    {
    public:
        D3D12Exception(int Line, HRESULT ErrorCode);

        const char* GetErrorType() const noexcept;
        std::string GetError() const;

    private:
        const HRESULT ErrorCode;
    };

	struct D3D12NodeMask
    {
        D3D12NodeMask() noexcept : NodeMask(1) {}
        constexpr D3D12NodeMask(UINT NodeMask) : NodeMask(NodeMask) {}

        operator UINT() const noexcept { return NodeMask; }

        static D3D12NodeMask FromIndex(std::uint32_t GpuIndex) { return {1u << GpuIndex}; }

        UINT NodeMask;
    };

    class D3D12Device;

	class D3D12DeviceChild
    {
    public:
        D3D12DeviceChild() noexcept = default;
        explicit D3D12DeviceChild(D3D12Device* Parent) noexcept : Parent(Parent) {}

        [[nodiscard]] auto GetParentDevice() const noexcept -> D3D12Device* { return Parent; }

    protected:
        D3D12Device* Parent = nullptr;
    };

    class D3D12LinkedDevice;

	class D3D12LinkedDeviceChild
    {
    public:
        D3D12LinkedDeviceChild() noexcept = default;
        explicit D3D12LinkedDeviceChild(D3D12LinkedDevice* Parent) noexcept : Parent(Parent) {}

        [[nodiscard]] auto GetParentLinkedDevice() const noexcept -> D3D12LinkedDevice* { return Parent; }

    protected:
        D3D12LinkedDevice* Parent = nullptr;
    };

    class D3D12Fence;
    // Represents a Fence and Value pair, similar to that of a coroutine handle
    // you can query the status of a command execution point and wait for it
    class D3D12SyncHandle
    {
    public:
        D3D12SyncHandle() noexcept = default;
        explicit D3D12SyncHandle(D3D12Fence* Fence, UINT64 Value) noexcept : Fence(Fence), Value(Value) {}

        D3D12SyncHandle(std::nullptr_t) noexcept : D3D12SyncHandle() {}

        D3D12SyncHandle& operator=(std::nullptr_t) noexcept
        {
            Fence = nullptr;
            Value = 0;
            return *this;
        }

        // 根据Fence是否为空，来判断当前SyncHandle是否是可用的，不可用的时候可以不做检查直接把对象给释放掉
        explicit operator bool() const noexcept;

        [[nodiscard]] auto GetValue() const noexcept -> UINT64;
        [[nodiscard]] auto IsComplete() const -> bool;
        auto               WaitForCompletion() const -> void;

    private:
        friend class D3D12CommandQueue;

        D3D12Fence* Fence = nullptr;
        UINT64      Value = 0;
    };

    template<typename TResourceType>
    class CFencePool
    {
    public:
        CFencePool(bool ThreadSafe = false) noexcept : Mutex(ThreadSafe ? std::make_unique<std::mutex>() : nullptr) {}
        CFencePool(CFencePool&& CFencePool) noexcept :
            Pool(std::exchange(CFencePool.Pool, {})), Mutex(std::exchange(CFencePool.Mutex, {}))
        {}
        CFencePool& operator=(CFencePool&& CFencePool) noexcept
        {
            if (this == &CFencePool)
            {
                return *this;
            }

            Pool  = std::exchange(CFencePool.Pool, {});
            Mutex = std::exchange(CFencePool.Mutex, {});
            return *this;
        }

        CFencePool(const CFencePool&) = delete;
        CFencePool& operator=(const CFencePool&) = delete;

        void ReturnToPool(TResourceType&& Resource, D3D12SyncHandle SyncHandle) noexcept
        {
            try
            {
                auto Lock = Mutex ? std::unique_lock(*Mutex) : std::unique_lock<std::mutex>();
                Pool.emplace_back(SyncHandle, std::move(Resource)); // throw( bad_alloc )
            }
            catch (std::bad_alloc&)
            {
                // Just drop the error
                // All uses of this pool use Arc, which will release the resource
            }
        }

        template<typename PFNCreateNew, typename... TArgs>
        TResourceType RetrieveFromPool(PFNCreateNew CreateNew, TArgs&&... Args) noexcept(false)
        {
            auto Lock = Mutex ? std::unique_lock(*Mutex) : std::unique_lock<std::mutex>();
            auto Head = Pool.begin();
            if (Head == Pool.end() || !Head->first.IsComplete())
            {
                return std::move(CreateNew(std::forward<TArgs>(Args)...));
            }

            assert(Head->second);
            TResourceType Resource = std::move(Head->second);
            Pool.erase(Head);
            return std::move(Resource);
        }

    protected:
        using TPoolEntry = std::pair<D3D12SyncHandle, TResourceType>;
        using TPool      = std::list<TPoolEntry>;

        TPool                       Pool;
        std::unique_ptr<std::mutex> Mutex;
    };

    class D3D12InputLayout
    {
    public:
        D3D12InputLayout() noexcept = default;
        D3D12InputLayout(size_t NumElements) { InputElements.reserve(NumElements); }
        D3D12InputLayout(const D3D12_INPUT_ELEMENT_DESC _InputElements[], uint32_t _InputElementCount)
        {
            std::copy(&_InputElements[0], &_InputElements[_InputElementCount], back_inserter(InputElements));
        }

        void
        AddVertexLayoutElement(std::string_view SemanticName, UINT SemanticIndex, DXGI_FORMAT Format, UINT InputSlot)
        {
            D3D12_INPUT_ELEMENT_DESC& Desc = InputElements.emplace_back();
            Desc.SemanticName              = SemanticName.data();
            Desc.SemanticIndex             = SemanticIndex;
            Desc.Format                    = Format;
            Desc.InputSlot                 = InputSlot;
            Desc.AlignedByteOffset         = D3D12_APPEND_ALIGNED_ELEMENT;
            Desc.InputSlotClass            = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
            Desc.InstanceDataStepRate      = 0;
        }

        std::vector<D3D12_INPUT_ELEMENT_DESC> InputElements;
    };

	template<typename TFunc>
    struct D3D12ScopedMap
    {
        D3D12ScopedMap(ID3D12Resource* Resource, UINT Subresource, D3D12_RANGE ReadRange, TFunc Func) :
            Resource(Resource)
        {
            void* Data = nullptr;
            if (SUCCEEDED(Resource->Map(Subresource, &ReadRange, &Data)))
            {
                Func(Data);
            }
            else
            {
                Resource = nullptr;
            }
        }
        ~D3D12ScopedMap()
        {
            if (Resource)
            {
                Resource->Unmap(0, nullptr);
            }
        }

        ID3D12Resource* Resource = nullptr;
    };

    struct D3D12ScopedPointer
    {
        D3D12ScopedPointer() noexcept = default;
        D3D12ScopedPointer(ID3D12Resource* Resource) : Resource(Resource)
        {
            if (Resource)
            {
                if (FAILED(Resource->Map(0, nullptr, reinterpret_cast<void**>(&Address))))
                {
                    Resource = nullptr;
                }
            }
        }
        D3D12ScopedPointer(D3D12ScopedPointer&& D3D12ScopedPointer) noexcept :
            Resource(std::exchange(D3D12ScopedPointer.Resource, {})),
            Address(std::exchange(D3D12ScopedPointer.Address, nullptr))
        {}
        D3D12ScopedPointer& operator=(D3D12ScopedPointer&& D3D12ScopedPointer) noexcept
        {
            if (this != &D3D12ScopedPointer)
            {
                Resource = std::exchange(D3D12ScopedPointer.Resource, {});
                Address  = std::exchange(D3D12ScopedPointer.Address, nullptr);
            }
            return *this;
        }
        void Release()
        {
            if (Resource)
            {
                Resource->Unmap(0, nullptr);
            }
            Resource = nullptr;
            Address  = nullptr;
        }
        ~D3D12ScopedPointer() { Release(); }

        ID3D12Resource* Resource = nullptr;
        BYTE*           Address  = nullptr;
    };

    template<RHI_PIPELINE_STATE_TYPE PsoType>
    struct D3D12DescriptorTableTraits
    {};
    template<>
    struct D3D12DescriptorTableTraits<RHI_PIPELINE_STATE_TYPE::Graphics>
    {
        static auto Bind() { return &ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable; }
    };
    template<>
    struct D3D12DescriptorTableTraits<RHI_PIPELINE_STATE_TYPE::Compute>
    {
        static auto Bind() { return &ID3D12GraphicsCommandList::SetComputeRootDescriptorTable; }
    };

    // Enum->String mappings
    inline D3D12_COMMAND_LIST_TYPE RHITranslateD3D12(RHID3D12CommandQueueType Type)
    {
        // clang-format off
		switch (Type)
		{
		case RHI::RHID3D12CommandQueueType::Direct:		    return D3D12_COMMAND_LIST_TYPE_DIRECT;
		case RHI::RHID3D12CommandQueueType::AsyncCompute:	return D3D12_COMMAND_LIST_TYPE_COMPUTE;
		case RHI::RHID3D12CommandQueueType::Copy1:			[[fallthrough]];
		case RHI::RHID3D12CommandQueueType::Copy2:			return D3D12_COMMAND_LIST_TYPE_COPY;
		}
        // clang-format on
        return D3D12_COMMAND_LIST_TYPE();
    }

    inline LPCWSTR GetCommandQueueTypeString(RHID3D12CommandQueueType Type)
    {
        // clang-format off
		switch (Type)
		{
		case RHI::RHID3D12CommandQueueType::Direct:		    return L"3D";
		case RHI::RHID3D12CommandQueueType::AsyncCompute:	return L"Async Compute";
		case RHI::RHID3D12CommandQueueType::Copy1:			return L"Copy 1";
		case RHI::RHID3D12CommandQueueType::Copy2:			return L"Copy 2";
		}
        // clang-format on
        return L"<unknown>";
    }

    inline LPCWSTR GetCommandQueueTypeFenceString(RHID3D12CommandQueueType Type)
    {
        // clang-format off
		switch (Type)
		{
		case RHI::RHID3D12CommandQueueType::Direct:		    return L"3D Fence";
		case RHI::RHID3D12CommandQueueType::AsyncCompute:	return L"Async Compute Fence";
		case RHI::RHID3D12CommandQueueType::Copy1:			return L"Copy 1 Fence";
		case RHI::RHID3D12CommandQueueType::Copy2:			return L"Copy 2 Fence";
		}
        // clang-format on
        return L"<unknown>";
    }

    inline LPCWSTR GetAutoBreadcrumbOpString(D3D12_AUTO_BREADCRUMB_OP Op)
    {
#define STRINGIFY(x) \
    case x: \
        return L#x

        switch (Op)
        {
            STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_SETMARKER);
            STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_BEGINEVENT);
            STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_ENDEVENT);
            STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_DRAWINSTANCED);
            STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_DRAWINDEXEDINSTANCED);
            STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_EXECUTEINDIRECT);
            STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_DISPATCH);
            STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_COPYBUFFERREGION);
            STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_COPYTEXTUREREGION);
            STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_COPYRESOURCE);
            STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_COPYTILES);
            STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_RESOLVESUBRESOURCE);
            STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_CLEARRENDERTARGETVIEW);
            STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_CLEARUNORDEREDACCESSVIEW);
            STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_CLEARDEPTHSTENCILVIEW);
            STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_RESOURCEBARRIER);
            STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_EXECUTEBUNDLE);
            STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_PRESENT);
            STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_RESOLVEQUERYDATA);
            STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_BEGINSUBMISSION);
            STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_ENDSUBMISSION);
            STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME);
            STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_PROCESSFRAMES);
            STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_ATOMICCOPYBUFFERUINT);
            STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_ATOMICCOPYBUFFERUINT64);
            STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_RESOLVESUBRESOURCEREGION);
            STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_WRITEBUFFERIMMEDIATE);
            STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME1);
            STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_SETPROTECTEDRESOURCESESSION);
            STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME2);
            STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_PROCESSFRAMES1);
            STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_BUILDRAYTRACINGACCELERATIONSTRUCTURE);
            STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_EMITRAYTRACINGACCELERATIONSTRUCTUREPOSTBUILDINFO);
            STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_COPYRAYTRACINGACCELERATIONSTRUCTURE);
            STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_DISPATCHRAYS);
            STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_INITIALIZEMETACOMMAND);
            STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_EXECUTEMETACOMMAND);
            STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_ESTIMATEMOTION);
            STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_RESOLVEMOTIONVECTORHEAP);
            STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_SETPIPELINESTATE1);
            STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_INITIALIZEEXTENSIONCOMMAND);
            STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_EXECUTEEXTENSIONCOMMAND);
            STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_DISPATCHMESH);
            STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_ENCODEFRAME);
            STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_RESOLVEENCODEROUTPUTMETADATA);
        }
        return L"<unknown>";
#undef STRINGIFY
    }

    inline LPCWSTR GetDredAllocationTypeString(D3D12_DRED_ALLOCATION_TYPE Type)
    {
#define STRINGIFY(x) \
    case x: \
        return L#x

        switch (Type)
        {
            STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_COMMAND_QUEUE);
            STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_COMMAND_ALLOCATOR);
            STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_PIPELINE_STATE);
            STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_COMMAND_LIST);
            STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_FENCE);
            STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_DESCRIPTOR_HEAP);
            STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_HEAP);
            STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_QUERY_HEAP);
            STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_COMMAND_SIGNATURE);
            STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_PIPELINE_LIBRARY);
            STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_VIDEO_DECODER);
            STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_VIDEO_PROCESSOR);
            STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_RESOURCE);
            STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_PASS);
            STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_CRYPTOSESSION);
            STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_CRYPTOSESSIONPOLICY);
            STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_PROTECTEDRESOURCESESSION);
            STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_VIDEO_DECODER_HEAP);
            STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_COMMAND_POOL);
            STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_COMMAND_RECORDER);
            STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_STATE_OBJECT);
            STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_METACOMMAND);
            STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_SCHEDULINGGROUP);
            STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_VIDEO_MOTION_ESTIMATOR);
            STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_VIDEO_MOTION_VECTOR_HEAP);
            STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_VIDEO_EXTENSION_COMMAND);
            STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_VIDEO_ENCODER);
            STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_VIDEO_ENCODER_HEAP);
            STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_INVALID);
        }
        return L"<unknown>";
#undef STRINGIFY
    }
} // namespace RHI



