#pragma once
#include "d3d12_core.h"
#include "d3d12_linkedDevice.h"
#include "d3d12_descriptorHeap.h"

namespace RHI
{
    class D3D12Resource;
    class D3D12Buffer;
    class D3D12ASBuffer;
    class D3D12Texture;
    class CViewSubresourceSubset;

    //CViewSubresourceSubset ResourceToViewSubresourceSubset(D3D12Resource* Resource);
    bool IsResourceIsBuffer(D3D12Resource* Resource);
    bool IsResourceIsTexture(D3D12Resource* Resource);

    class CSubresourceSubset
    {
    public:
        CSubresourceSubset() noexcept = default;
        explicit CSubresourceSubset(UINT8  NumMips,
                                    UINT16 NumArraySlices,
                                    UINT8  NumPlanes,
                                    UINT8  FirstMip        = 0,
                                    UINT16 FirstArraySlice = 0,
                                    UINT8  FirstPlane      = 0) noexcept;
        explicit CSubresourceSubset(const D3D12_SHADER_RESOURCE_VIEW_DESC& Desc) noexcept;
        explicit CSubresourceSubset(const D3D12_UNORDERED_ACCESS_VIEW_DESC& Desc) noexcept;
        explicit CSubresourceSubset(const D3D12_RENDER_TARGET_VIEW_DESC& Desc) noexcept;
        explicit CSubresourceSubset(const D3D12_DEPTH_STENCIL_VIEW_DESC& Desc) noexcept;

        [[nodiscard]] bool DoesNotOverlap(const CSubresourceSubset& CSubresourceSubset) const noexcept;

    protected:
        UINT16 BeginArray = 0; // Also used to store Tex3D slices.
        UINT16 EndArray   = 0; // End - Begin == Array Slices
        UINT8  BeginMip   = 0;
        UINT8  EndMip     = 0; // End - Begin == Mip Levels
        UINT8  BeginPlane = 0;
        UINT8  EndPlane   = 0;
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
        explicit CViewSubresourceSubset(const D3D12_SHADER_RESOURCE_VIEW_DESC& Desc,
                                        UINT8                                  MipLevels,
                                        UINT16                                 ArraySize,
                                        UINT8                                  PlaneCount);
        explicit CViewSubresourceSubset(const D3D12_UNORDERED_ACCESS_VIEW_DESC& Desc,
                                        UINT8                                   MipLevels,
                                        UINT16                                  ArraySize,
                                        UINT8                                   PlaneCount);
        explicit CViewSubresourceSubset(const D3D12_RENDER_TARGET_VIEW_DESC& Desc,
                                        UINT8                                MipLevels,
                                        UINT16                               ArraySize,
                                        UINT8                                PlaneCount);
        explicit CViewSubresourceSubset(const D3D12_DEPTH_STENCIL_VIEW_DESC& Desc,
                                        UINT8                                MipLevels,
                                        UINT16                               ArraySize,
                                        UINT8                                PlaneCount,
                                        DepthStencilMode                     DSMode = ReadOrWrite);

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
        class CViewSubresourceIterator;

    public:
        [[nodiscard]] CViewSubresourceIterator begin() const;
        [[nodiscard]] CViewSubresourceIterator end() const;
        [[nodiscard]] bool                     IsWholeResource() const
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
                UINT startSubresource = D3D12CalcSubresource(0, 0, BeginPlane, MipLevels, ArraySlices);
                UINT endSubresource   = D3D12CalcSubresource(0, 0, EndPlane, MipLevels, ArraySlices);

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

    // This iterator iterates over contiguous ranges of subresources within a
    // subresource subset. eg:
    //
    // // For each contiguous subresource range.
    // for( CViewSubresourceSubset::CViewSubresourceIterator it =
    // ViewSubset.begin(); it != ViewSubset.end(); ++it )
    // {
    //      // StartSubresource and EndSubresource members of the iterator describe
    //      the contiguous range. for( UINT SubresourceIndex =
    //      it.StartSubresource(); SubresourceIndex < it.EndSubresource();
    //      SubresourceIndex++ )
    //      {
    //          // Action for each subresource within the current range.
    //      }
    //  }
    //
    class CViewSubresourceSubset::CViewSubresourceIterator
    {
    public:
        explicit CViewSubresourceIterator(const CViewSubresourceSubset& ViewSubresourceSubset,
                                          UINT16                        ArraySlice,
                                          UINT8                         PlaneSlice) :
            ViewSubresourceSubset(ViewSubresourceSubset),
            CurrentArraySlice(ArraySlice), CurrentPlaneSlice(PlaneSlice)
        {}
        CViewSubresourceIterator& operator++()
        {
            ASSERT(CurrentArraySlice < ViewSubresourceSubset.EndArray);

            if (++CurrentArraySlice >= ViewSubresourceSubset.EndArray)
            {
                ASSERT(CurrentPlaneSlice < ViewSubresourceSubset.EndPlane);
                CurrentArraySlice = ViewSubresourceSubset.BeginArray;
                ++CurrentPlaneSlice;
            }

            return *this;
        }
        CViewSubresourceIterator& operator--()
        {
            if (CurrentArraySlice <= ViewSubresourceSubset.BeginArray)
            {
                CurrentArraySlice = ViewSubresourceSubset.EndArray;

                ASSERT(CurrentPlaneSlice > ViewSubresourceSubset.BeginPlane);
                --CurrentPlaneSlice;
            }

            --CurrentArraySlice;

            return *this;
        }

        bool operator==(const CViewSubresourceIterator& CViewSubresourceIterator) const
        {
            return &CViewSubresourceIterator.ViewSubresourceSubset == &ViewSubresourceSubset &&
                   CViewSubresourceIterator.CurrentArraySlice == CurrentArraySlice &&
                   CViewSubresourceIterator.CurrentPlaneSlice == CurrentPlaneSlice;
        }
        bool operator!=(const CViewSubresourceIterator& CViewSubresourceIterator) const
        {
            return !(CViewSubresourceIterator == *this);
        }

        [[nodiscard]] UINT StartSubresource() const
        {
            return D3D12CalcSubresource(ViewSubresourceSubset.BeginMip,
                                        CurrentArraySlice,
                                        CurrentPlaneSlice,
                                        ViewSubresourceSubset.MipLevels,
                                        ViewSubresourceSubset.ArraySlices);
        }
        [[nodiscard]] UINT EndSubresource() const
        {
            return D3D12CalcSubresource(ViewSubresourceSubset.EndMip,
                                        CurrentArraySlice,
                                        CurrentPlaneSlice,
                                        ViewSubresourceSubset.MipLevels,
                                        ViewSubresourceSubset.ArraySlices);
        }
        [[nodiscard]] std::pair<UINT, UINT> operator*() const
        {
            return std::make_pair(StartSubresource(), EndSubresource());
        }

    private:
        const CViewSubresourceSubset& ViewSubresourceSubset;
        UINT16                        CurrentArraySlice;
        UINT8                         CurrentPlaneSlice;
    };

    template<typename ViewDesc>
    struct D3D12DescriptorTraits
    {};
    template<>
    struct D3D12DescriptorTraits<D3D12_CONSTANT_BUFFER_VIEW_DESC>
    {
        static auto Create() { return &ID3D12Device::CreateConstantBufferView; }
    };
    template<>
    struct D3D12DescriptorTraits<D3D12_RENDER_TARGET_VIEW_DESC>
    {
        static auto Create() { return &ID3D12Device::CreateRenderTargetView; }
    };
    template<>
    struct D3D12DescriptorTraits<D3D12_DEPTH_STENCIL_VIEW_DESC>
    {
        static auto Create() { return &ID3D12Device::CreateDepthStencilView; }
    };
    template<>
    struct D3D12DescriptorTraits<D3D12_SHADER_RESOURCE_VIEW_DESC>
    {
        static auto Create() { return &ID3D12Device::CreateShaderResourceView; }
    };
    template<>
    struct D3D12DescriptorTraits<D3D12_UNORDERED_ACCESS_VIEW_DESC>
    {
        static auto Create() { return &ID3D12Device::CreateUnorderedAccessView; }
    };

    template<typename ViewDesc>
    class D3D12Descriptor : public D3D12LinkedDeviceChild
    {
    public:
        D3D12Descriptor() noexcept = default;
        explicit D3D12Descriptor(D3D12LinkedDevice* Parent, BOOL IsNonShaderVisible = FALSE) :
            D3D12LinkedDeviceChild(Parent), m_IsNonShaderVisible(IsNonShaderVisible)
        {
            if (Parent)
            {
                if (IsNonShaderVisible)
                {
                    RHI::CPUDescriptorHeap* pCPUDescriptorHeap = Parent->GetHeapManager<ViewDesc>();
                    m_DescriptorHeapAllocation = pCPUDescriptorHeap->Allocate(1);
                }
                else
                {
                    RHI::GPUDescriptorHeap* pDescriptorHeap = Parent->GetDescriptorHeap<ViewDesc>();
                    ASSERT(pDescriptorHeap != nullptr);
                    m_DescriptorHeapAllocation = pDescriptorHeap->Allocate(1);
                }
            }
        }
        D3D12Descriptor(D3D12Descriptor&& D3D12Descriptor) noexcept :
            D3D12LinkedDeviceChild(std::move(D3D12Descriptor.Parent)),
            m_DescriptorHeapAllocation(std::move(D3D12Descriptor.m_DescriptorHeapAllocation))
        {}
        D3D12Descriptor& operator=(D3D12Descriptor&& D3D12Descriptor) noexcept
        {
            if (this == &D3D12Descriptor)
            {
                return *this;
            }

            InternalDestroy();
            Parent                     = std::move(D3D12Descriptor.Parent);
            m_DescriptorHeapAllocation = std::move(D3D12Descriptor.m_DescriptorHeapAllocation);

            return *this;
        }
        ~D3D12Descriptor() { InternalDestroy(); }

        D3D12Descriptor(const D3D12Descriptor&) = delete;
        D3D12Descriptor& operator=(const D3D12Descriptor&) = delete;

        [[nodiscard]] bool IsValid() const noexcept { return !m_DescriptorHeapAllocation.IsNull(); }
        [[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle() const noexcept
        {
            ASSERT(IsValid());
            return m_DescriptorHeapAllocation.GetCpuHandle(0);
        }
        [[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle() const noexcept
        {
            ASSERT(IsValid());
            return m_DescriptorHeapAllocation.GetGpuHandle(0);
        }
        [[nodiscard]] UINT GetIndex() const noexcept
        {
            ASSERT(IsValid());
            return m_DescriptorHeapAllocation.GetDescriptorHeapOffsetIndex(0);
        }

        void CreateDefaultView(ID3D12Resource* Resource)
        {
            (GetParentLinkedDevice()->GetDevice()->*D3D12DescriptorTraits<ViewDesc>::Create())(
                Resource, nullptr, m_DescriptorHeapAllocation.GetCpuHandle(0));
        }

        void CreateDefaultView(ID3D12Resource* Resource, ID3D12Resource* CounterResource)
        {
            (GetParentLinkedDevice()->GetDevice()->*D3D12DescriptorTraits<ViewDesc>::Create())(
                Resource, CounterResource, nullptr, m_DescriptorHeapAllocation.GetCpuHandle(0));
        }

        void CreateView(const ViewDesc& Desc)
        {
            (GetParentLinkedDevice()->GetDevice()->*D3D12DescriptorTraits<ViewDesc>::Create())(
                &Desc, m_DescriptorHeapAllocation.GetCpuHandle());
        }

        void CreateView(const ViewDesc& Desc, ID3D12Resource* Resource)
        {
            (GetParentLinkedDevice()->GetDevice()->*D3D12DescriptorTraits<ViewDesc>::Create())(
                Resource, &Desc, m_DescriptorHeapAllocation.GetCpuHandle(0));
        }

        void CreateView(const ViewDesc& Desc, ID3D12Resource* Resource, ID3D12Resource* CounterResource)
        {
            (GetParentLinkedDevice()->GetDevice()->*D3D12DescriptorTraits<ViewDesc>::Create())(
                Resource, CounterResource, &Desc, m_DescriptorHeapAllocation.GetCpuHandle(0));
        }

    private:
        void InternalDestroy()
        {
            if (Parent && IsValid())
            {
                Parent->Retire(std::move(m_DescriptorHeapAllocation));
                Parent = nullptr;
            }
        }

    protected:
        friend class D3D12Resource;
        DescriptorHeapAllocation m_DescriptorHeapAllocation;
        BOOL                     m_IsNonShaderVisible;
    };

    //-------------------------------------------------------------------------------------------------------------------------------

    template<typename ViewDesc>
    class D3D12View
    {
    public:
        D3D12View() noexcept = default;
        explicit D3D12View(D3D12LinkedDevice* Device, const ViewDesc& Desc, D3D12Resource* Resource, CViewSubresourceSubset ViewSubresourceSubset, BOOL IsNonShaderVisible = FALSE) :
            Descriptor(Device, IsNonShaderVisible), Desc(Desc), Resource(Resource), ViewSubresourceSubset(ViewSubresourceSubset)
        {}

        D3D12View(D3D12View&&) noexcept = default;
        D3D12View& operator=(D3D12View&&) noexcept = default;

        D3D12View(const D3D12View&) = delete;
        D3D12View& operator=(const D3D12View&) = delete;

        [[nodiscard]] bool                        IsValid() const noexcept { return Descriptor.IsValid(); }
        [[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle() const noexcept { return Descriptor.GetCpuHandle(); }
        [[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle() const noexcept { return Descriptor.GetGpuHandle(); }
        [[nodiscard]] UINT                        GetIndex() const noexcept { return Descriptor.GetIndex(); }
        [[nodiscard]] ViewDesc                    GetDesc() const noexcept { return Desc; }
        [[nodiscard]] D3D12Resource*              GetResource() const noexcept { return Resource; }

        [[nodiscard]] const CViewSubresourceSubset& GetViewSubresourceSubset() const noexcept { return ViewSubresourceSubset; }

    protected:
        friend class D3D12Resource;

        D3D12Descriptor<ViewDesc> Descriptor;
        ViewDesc                  Desc     = {};
        D3D12Resource*            Resource = nullptr;
        CViewSubresourceSubset    ViewSubresourceSubset;
    };

    class D3D12RenderTargetView : public D3D12View<D3D12_RENDER_TARGET_VIEW_DESC>
    {
    public:
        D3D12RenderTargetView() noexcept = default;
        explicit D3D12RenderTargetView(D3D12LinkedDevice* Device, const D3D12_RENDER_TARGET_VIEW_DESC& Desc, D3D12Resource* Resource);
        explicit D3D12RenderTargetView(D3D12LinkedDevice* Device, D3D12Texture* Texture, BOOL sRGB = FALSE, 
            INT OptArraySlice = -1, INT OptMipSlice = -1, INT OptArraySize = -1);

        void RecreateView();

        [[nodiscard]] const CViewSubresourceSubset& GetViewSubresourceSubset() const noexcept
        {
            return D3D12View::GetViewSubresourceSubset();
        }
    public:
        static D3D12_RENDER_TARGET_VIEW_DESC GetDesc(D3D12Texture* Texture, bool sRGB = FALSE, 
            INT OptArraySlice = -1, INT OptMipSlice = -1, INT OptArraySize = -1);
    };

    class D3D12DepthStencilView : public D3D12View<D3D12_DEPTH_STENCIL_VIEW_DESC>
    {
    public:
        D3D12DepthStencilView() noexcept = default;
        explicit D3D12DepthStencilView(D3D12LinkedDevice* Device, const D3D12_DEPTH_STENCIL_VIEW_DESC& Desc, D3D12Resource* Resource);
        explicit D3D12DepthStencilView(D3D12LinkedDevice* Device, D3D12Texture* Texture, 
            INT OptArraySlice = -1, INT OptMipSlice = -1, INT OptArraySize = -1);

        void RecreateView();

        [[nodiscard]] const CViewSubresourceSubset& GetViewSubresourceSubset() const noexcept
        {
            return D3D12View::GetViewSubresourceSubset();
        }
    public:
        static D3D12_DEPTH_STENCIL_VIEW_DESC GetDesc(D3D12Texture* Texture, INT OptArraySlice = -1, INT OptMipSlice = -1, INT OptArraySize = -1);
    };


    class D3D12ConstantBufferView : public D3D12View<D3D12_CONSTANT_BUFFER_VIEW_DESC>
    {
    public:
        D3D12ConstantBufferView() noexcept = default;
        D3D12ConstantBufferView(D3D12LinkedDevice* Device, const D3D12_CONSTANT_BUFFER_VIEW_DESC& Desc, D3D12Resource* Resource, BOOL IsNonShaderVisible = FALSE);
        D3D12ConstantBufferView(D3D12LinkedDevice* Device, D3D12Buffer* Buffer, UINT32 Offset, UINT32 Size, BOOL IsNonShaderVisible = FALSE);

        void RecreateView();

        [[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle() const noexcept { return D3D12View::GetGpuHandle(); }

    public:
        static D3D12_CONSTANT_BUFFER_VIEW_DESC GetDesc(D3D12Buffer* Buffer, UINT Offset, UINT Size);
    };

    enum BufferResourceType
    {
        Buffer,
        ByteAddressBuffer,
        StructuredBuffer
    };

    class D3D12ShaderResourceView : public D3D12View<D3D12_SHADER_RESOURCE_VIEW_DESC>
    {
    public:
        D3D12ShaderResourceView() noexcept = default;
        D3D12ShaderResourceView(D3D12LinkedDevice* Device, const D3D12_SHADER_RESOURCE_VIEW_DESC& Desc, D3D12Resource* Resource, BOOL IsNonShaderVisible = FALSE);
        D3D12ShaderResourceView(D3D12LinkedDevice* Device, D3D12ASBuffer* ASBuffer, BOOL IsNonShaderVisible = FALSE);
        D3D12ShaderResourceView(D3D12LinkedDevice* Device, D3D12Buffer* Buffer, bool Raw, UINT FirstElement, UINT NumElements, BOOL IsNonShaderVisible = FALSE);
        D3D12ShaderResourceView(D3D12LinkedDevice* Device, D3D12Buffer* Buffer, UINT FirstElement, UINT NumElements, BOOL IsNonShaderVisible = FALSE);
        D3D12ShaderResourceView(D3D12LinkedDevice* Device, D3D12Buffer* Buffer, BOOL IsNonShaderVisible = FALSE);
        D3D12ShaderResourceView(D3D12LinkedDevice* Device, D3D12Texture* Texture, bool sRGB,  INT OptMostDetailedMip = -1, INT OptMipLevels = -1, BOOL IsNonShaderVisible = FALSE);

        void RecreateView();

        [[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle() const noexcept { return D3D12View::GetGpuHandle(); }
        [[nodiscard]] const CViewSubresourceSubset& GetViewSubresourceSubset() const noexcept
        {
            return D3D12View::GetViewSubresourceSubset();
        }

    public:
        static D3D12_SHADER_RESOURCE_VIEW_DESC GetDesc(D3D12ASBuffer* ASBuffer);
        static D3D12_SHADER_RESOURCE_VIEW_DESC GetDesc(D3D12Buffer* Buffer, bool Raw, UINT FirstElement, UINT NumElements);
        static D3D12_SHADER_RESOURCE_VIEW_DESC GetDesc(D3D12Buffer* Buffer, BufferResourceType ResType, UINT FirstElement, UINT NumElements, DXGI_FORMAT BufferFormat = DXGI_FORMAT_R32_TYPELESS);

        static D3D12_SHADER_RESOURCE_VIEW_DESC GetDesc(D3D12Texture* Texture, bool sRGB, INT OptMostDetailedMip, INT OptMipLevels);
    };

    class D3D12UnorderedAccessView : public D3D12View<D3D12_UNORDERED_ACCESS_VIEW_DESC>
    {
    public:
        D3D12UnorderedAccessView() noexcept = default;
        D3D12UnorderedAccessView(D3D12LinkedDevice* Device, const D3D12_UNORDERED_ACCESS_VIEW_DESC& Desc, D3D12Resource* Resource, D3D12Resource* CounterResource = nullptr, BOOL IsNonShaderVisible = FALSE);
        D3D12UnorderedAccessView(D3D12LinkedDevice* Device, D3D12Buffer* Buffer, bool Raw, UINT FirstElement, UINT NumElements, UINT64 CounterOffsetInBytes, BOOL IsNonShaderVisible = FALSE);
        D3D12UnorderedAccessView(D3D12LinkedDevice* Device, D3D12Buffer* Buffer, UINT FirstElement, UINT NumElements, UINT64 CounterOffsetInBytes, BOOL IsNonShaderVisible = FALSE);
        D3D12UnorderedAccessView(D3D12LinkedDevice* Device, D3D12Buffer* Buffer, BOOL IsNonShaderVisible = FALSE);
        D3D12UnorderedAccessView(D3D12LinkedDevice* Device, D3D12Texture* Texture, INT OptArraySlice = -1, INT OptMipSlice = -1, BOOL IsNonShaderVisible = FALSE);

        void RecreateView();

        [[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE   GetGpuHandle() const noexcept { return D3D12View::GetGpuHandle(); }
        [[nodiscard]] const CViewSubresourceSubset& GetViewSubresourceSubset() const noexcept
        {
            return D3D12View::GetViewSubresourceSubset();
        }
    public:
        static D3D12_UNORDERED_ACCESS_VIEW_DESC GetDesc(D3D12Buffer* Buffer, bool Raw, UINT FirstElement, UINT NumElements, UINT64 CounterOffsetInBytes);
        static D3D12_UNORDERED_ACCESS_VIEW_DESC GetDesc(D3D12Buffer* Buffer, BufferResourceType ResType, UINT FirstElement, UINT NumElements, UINT64 CounterOffsetInBytes, DXGI_FORMAT BufferFormat = DXGI_FORMAT_R32_TYPELESS);
        static D3D12_UNORDERED_ACCESS_VIEW_DESC GetDesc(D3D12Texture* Texture, INT OptArraySlice = -1, INT OptMipSlice = -1);
        
    private:
        D3D12Resource* CounterResource = nullptr;
    };
}