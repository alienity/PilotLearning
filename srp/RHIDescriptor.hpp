#include "RHICore.hpp"

namespace RHI
{
    class RHIResource;
    class RHIBuffer;
    class RHITexture;

    class CSubresourceSubset
    {
    public:
        CSubresourceSubset() noexcept = default;
        explicit CSubresourceSubset(UINT NumMips,
                                    UINT NumArraySlices,
                                    UINT NumPlanes,
                                    UINT FirstMip        = 0,
                                    UINT FirstArraySlice = 0,
                                    UINT FirstPlane      = 0) noexcept;
        explicit CSubresourceSubset(const RHI_SHADER_RESOURCE_VIEW_DESC& Desc) noexcept;
        explicit CSubresourceSubset(const RHI_UNORDERED_ACCESS_VIEW_DESC& Desc) noexcept;
        explicit CSubresourceSubset(const RHI_RENDER_TARGET_VIEW_DESC& Desc) noexcept;
        explicit CSubresourceSubset(const RHI_DEPTH_STENCIL_VIEW_DESC& Desc) noexcept;

        [[nodiscard]] bool DoesNotOverlap(const CSubresourceSubset& CSubresourceSubset) const noexcept;

    protected:
        UINT BeginArray = 0; // Also used to store Tex3D slices.
        UINT EndArray   = 0; // End - Begin == Array Slices
        UINT BeginMip   = 0;
        UINT EndMip     = 0; // End - Begin == Mip Levels
        UINT BeginPlane = 0;
        UINT EndPlane   = 0;
    };

    class CViewSubresourceSubset : public CSubresourceSubset
    {
    public:
        enum DepthStencilMode
        {
            ReadOnly,
            WriteOnly,
            ReadOrWrite
        };

        CViewSubresourceSubset() noexcept = default;
        explicit CViewSubresourceSubset(const CSubresourceSubset& Subresources,
                                        UINT8                     MipLevels,
                                        UINT16                    ArraySize,
                                        UINT8                     PlaneCount);
        explicit CViewSubresourceSubset(const RHI_SHADER_RESOURCE_VIEW_DESC& Desc,
                                        UINT8                                MipLevels,
                                        UINT16                               ArraySize,
                                        UINT8                                PlaneCount);
        explicit CViewSubresourceSubset(const RHI_UNORDERED_ACCESS_VIEW_DESC& Desc,
                                        UINT8                                 MipLevels,
                                        UINT16                                ArraySize,
                                        UINT8                                 PlaneCount);
        explicit CViewSubresourceSubset(const RHI_RENDER_TARGET_VIEW_DESC& Desc,
                                        UINT8                              MipLevels,
                                        UINT16                             ArraySize,
                                        UINT8                              PlaneCount);
        explicit CViewSubresourceSubset(const RHI_DEPTH_STENCIL_VIEW_DESC& Desc,
                                        UINT8                              MipLevels,
                                        UINT16                             ArraySize,
                                        UINT8                              PlaneCount,
                                        DepthStencilMode                   DSMode = ReadOrWrite);

        template<typename T>
        static CViewSubresourceSubset FromView(const T* pView)
        {
            return CViewSubresourceSubset(pView->GetDesc(),
                                          static_cast<UINT8>(pView->GetResource()->GetMipLevels()),
                                          static_cast<UINT16>(pView->GetResource()->GetArraySize()),
                                          static_cast<UINT8>(pView->GetResource()->GetPlaneCount()));
        }

        template<typename D, typename R>
        static CViewSubresourceSubset FromView(const D desc, R* pResource)
        {
            return CViewSubresourceSubset(desc,
                                          static_cast<UINT8>(pResource->GetMipLevels()),
                                          static_cast<UINT16>(pResource->GetArraySize()),
                                          static_cast<UINT8>(pResource->GetPlaneCount()));
        }

    public:
        [[nodiscard]] bool IsWholeResource() const
        {
            return BeginMip == 0 && BeginArray == 0 && BeginPlane == 0 &&
                   (EndMip * EndArray * EndPlane == MipLevels * ArraySlices * PlaneCount);
        }
        [[nodiscard]] bool IsEmpty() const
        {
            return BeginMip == EndMip || BeginArray == EndArray || BeginPlane == EndPlane;
        }
        [[nodiscard]] UINT ArraySize() const { return ArraySlices; }

        [[nodiscard]] UINT MinSubresource() const;
        [[nodiscard]] UINT MaxSubresource() const;

    private:
        void Reduce()
        {
            if (BeginMip == 0 && EndMip == MipLevels && BeginArray == 0 && EndArray == ArraySlices)
            {
                UINT startSubresource = RHICalcSubresource(0, 0, BeginPlane, MipLevels, ArraySlices);
                UINT endSubresource   = RHICalcSubresource(0, 0, EndPlane, MipLevels, ArraySlices);

                // Only coalesce if the full-resolution UINTs fit in the UINT8s used
                // for storage here
                if (endSubresource < static_cast<UINT8>(-1))
                {
                    BeginArray = 0;
                    EndArray   = 1;
                    BeginPlane = 0;
                    EndPlane   = 1;
                    BeginMip   = static_cast<UINT8>(startSubresource);
                    EndMip     = static_cast<UINT8>(endSubresource);
                }
            }
        }

    protected:
        UINT8  MipLevels   = 0;
        UINT16 ArraySlices = 0;
        UINT8  PlaneCount  = 0;
    };

    template<typename ViewDesc>
    struct RHIDescriptorTraits
    {};
    template<>
    struct RHIDescriptorTraits<RHI_CONSTANT_BUFFER_VIEW_DESC>
    {
        static auto Create() { return &IRHIDevice::CreateConstantBufferView; }
    };
    template<>
    struct RHIDescriptorTraits<RHI_RENDER_TARGET_VIEW_DESC>
    {
        static auto Create() { return &IRHIDevice::CreateRenderTargetView; }
    };
    template<>
    struct RHIDescriptorTraits<RHI_DEPTH_STENCIL_VIEW_DESC>
    {
        static auto Create() { return &IRHIDevice::CreateDepthStencilView; }
    };
    template<>
    struct RHIDescriptorTraits<RHI_SHADER_RESOURCE_VIEW_DESC>
    {
        static auto Create() { return &IRHIDevice::CreateShaderResourceView; }
    };
    template<>
    struct RHIDescriptorTraits<RHI_UNORDERED_ACCESS_VIEW_DESC>
    {
        static auto Create() { return &IRHIDevice::CreateUnorderedAccessView; }
    };

    template<typename ViewDesc>
    class D3D12View
    {
    public:
        D3D12View() noexcept = default;
        explicit D3D12View(const ViewDesc& Desc, D3D12Resource* Resource, CViewSubresourceSubset ViewSubresourceSubset) :
            Descriptor(Device), Desc(Desc), Resource(Resource), ViewSubresourceSubset(ViewSubresourceSubset)
        {}

        D3D12View(D3D12View&&) noexcept = default;
        D3D12View& operator=(D3D12View&&) noexcept = default;

        D3D12View(const D3D12View&) = delete;
        D3D12View& operator=(const D3D12View&) = delete;

        [[nodiscard]] bool                          IsValid() const noexcept { return Descriptor.IsValid(); }
        [[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE   GetCpuHandle() const noexcept { return Descriptor.GetCpuHandle(); }
        [[nodiscard]] ViewDesc                      GetDesc() const noexcept { return Desc; }
        [[nodiscard]] D3D12Resource*                GetResource() const noexcept { return Resource; }
    protected:
        [[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle() const noexcept { return Descriptor.GetGpuHandle(); }
        [[nodiscard]] const CViewSubresourceSubset& GetViewSubresourceSubset() const noexcept
        {
            return ViewSubresourceSubset;
        }

    protected:
        friend class D3D12Resource;

        D3D12Descriptor<ViewDesc> Descriptor;
        ViewDesc                  Desc     = {};
        D3D12Resource*            Resource = nullptr;
        CViewSubresourceSubset    ViewSubresourceSubset;
    };








































}
