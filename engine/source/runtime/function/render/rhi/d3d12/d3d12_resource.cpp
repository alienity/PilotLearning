#include "d3d12_resource.h"
#include "d3d12_linkedDevice.h"
#include "d3d12_descriptor.h"
#include "platform/system/hash.h"

namespace RHI
{
    // Helper struct used to infer optimal resource state
    struct ResourceStateDeterminer
    {
        ResourceStateDeterminer(const D3D12_RESOURCE_DESC& Desc, D3D12_HEAP_TYPE HeapType) noexcept :
            ShaderResource((Desc.Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) == 0),
            DepthStencil((Desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) != 0),
            RenderTarget((Desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) != 0),
            UnorderedAccess((Desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) != 0),
            Writable(DepthStencil || RenderTarget || UnorderedAccess), ShaderResourceOnly(ShaderResource && !Writable),
            Buffer(Desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER), DeviceLocal(HeapType == D3D12_HEAP_TYPE_DEFAULT),
            ReadbackResource(HeapType == D3D12_HEAP_TYPE_READBACK)
        {}

        [[nodiscard]] D3D12_RESOURCE_STATES InferInitialState() const noexcept
        {
            if (Buffer)
            {
                if (DeviceLocal)
                {
                    return D3D12_RESOURCE_STATE_COMMON;
                }

                return ReadbackResource ? D3D12_RESOURCE_STATE_COPY_DEST : D3D12_RESOURCE_STATE_GENERIC_READ;
            }
            if (ShaderResourceOnly)
            {
                return D3D12_RESOURCE_STATE_COMMON;
            }
            if (Writable)
            {
                if (DepthStencil)
                {
                    return D3D12_RESOURCE_STATE_DEPTH_WRITE;
                }
                if (RenderTarget)
                {
                    return D3D12_RESOURCE_STATE_RENDER_TARGET;
                }
                if (UnorderedAccess)
                {
                    return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
                }
            }

            return D3D12_RESOURCE_STATE_COMMON;
        }

        bool ShaderResource;
        bool DepthStencil;
        bool RenderTarget;
        bool UnorderedAccess;
        bool Writable;
        bool ShaderResourceOnly;
        bool Buffer;
        bool DeviceLocal;
        bool ReadbackResource;
    };

    CResourceState::CResourceState(std::uint32_t NumSubresources, D3D12_RESOURCE_STATES InitialResourceState) :
        ResourceState(InitialResourceState), SubresourceStates(NumSubresources, InitialResourceState)
    {
        if (NumSubresources == 1)
        {
            TrackingMode = ETrackingMode::PerResource;
        }
        else
        {
            TrackingMode = ETrackingMode::PerSubresource;
        }
    }

    D3D12_RESOURCE_STATES CResourceState::GetSubresourceState(std::uint32_t Subresource) const
    {
        if (TrackingMode == ETrackingMode::PerResource)
        {
            return ResourceState;
        }

        return SubresourceStates[Subresource];
    }

    void CResourceState::SetSubresourceState(std::uint32_t Subresource, D3D12_RESOURCE_STATES State)
    {
        // If setting all subresources, or the resource only has a single subresource, set the per-resource state
        if (Subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES || SubresourceStates.size() == 1)
        {
            TrackingMode  = ETrackingMode::PerResource;
            ResourceState = State;
        }
        else
        {
            // If we previous tracked resource per resource level, we need to update all
            // all subresource states before proceeding
            if (TrackingMode == ETrackingMode::PerResource)
            {
                TrackingMode = ETrackingMode::PerSubresource;
                for (auto& SubresourceState : SubresourceStates)
                {
                    SubresourceState = ResourceState;
                }
            }
            SubresourceStates[Subresource] = State;
        }
    }

    D3D12Resource::D3D12Resource(D3D12LinkedDevice*                       Parent,
                                 Microsoft::WRL::ComPtr<ID3D12Resource>&& Resource,
                                 CD3DX12_CLEAR_VALUE                      ClearValue,
                                 D3D12_RESOURCE_STATES                    InitialResourceState) :
        D3D12LinkedDeviceChild(Parent),
        m_pResource(std::move(Resource)), m_ResourceDesc(this->m_pResource->GetDesc()),
        m_PlaneCount(D3D12GetFormatPlaneCount(Parent->GetDevice(), m_ResourceDesc.Format)),
        m_NumSubresources(CalculateNumSubresources()), m_ResourceState(m_NumSubresources, InitialResourceState),
        m_GpuVirtualAddress(m_pResource->GetGPUVirtualAddress())
    {}

    D3D12Resource::D3D12Resource(D3D12LinkedDevice*                       Parent,
                                 Microsoft::WRL::ComPtr<ID3D12Resource>&& Resource,
                                 CD3DX12_CLEAR_VALUE                      ClearValue,
                                 D3D12_RESOURCE_STATES                    InitialResourceState,
                                 std::wstring                             Name) :
        D3D12LinkedDeviceChild(Parent),
        m_pResource(std::move(Resource)), m_ClearValue(ClearValue), m_ResourceDesc(this->m_pResource->GetDesc()),
        m_PlaneCount(D3D12GetFormatPlaneCount(Parent->GetDevice(), m_ResourceDesc.Format)),
        m_NumSubresources(CalculateNumSubresources()), m_ResourceState(m_NumSubresources, InitialResourceState),
        m_GpuVirtualAddress(m_pResource->GetGPUVirtualAddress())
    {
        this->SetResourceName(Name);
    }

    D3D12Resource::D3D12Resource(D3D12LinkedDevice*                 Parent,
                                 CD3DX12_HEAP_PROPERTIES            HeapProperties,
                                 CD3DX12_RESOURCE_DESC              Desc,
                                 D3D12_RESOURCE_STATES              InitialResourceState,
                                 std::optional<CD3DX12_CLEAR_VALUE> ClearValue) :
        D3D12LinkedDeviceChild(Parent),
        m_pResource(InitializeResource(HeapProperties, Desc, InitialResourceState, ClearValue)),
        m_ClearValue(ClearValue), m_ResourceDesc(m_pResource->GetDesc()),
        m_PlaneCount(D3D12GetFormatPlaneCount(Parent->GetDevice(), Desc.Format)),
        m_NumSubresources(CalculateNumSubresources()), m_ResourceState(m_NumSubresources, InitialResourceState),
        m_GpuVirtualAddress(m_pResource->GetGPUVirtualAddress())
    {}

    D3D12Resource::D3D12Resource(D3D12LinkedDevice*                 Parent,
                                 CD3DX12_HEAP_PROPERTIES            HeapProperties,
                                 CD3DX12_RESOURCE_DESC              Desc,
                                 D3D12_RESOURCE_STATES              InitialResourceState,
                                 std::optional<CD3DX12_CLEAR_VALUE> ClearValue,
                                 std::wstring                       Name) :
        D3D12LinkedDeviceChild(Parent),
        m_pResource(InitializeResource(HeapProperties, Desc, InitialResourceState, ClearValue)),
        m_ClearValue(ClearValue), m_ResourceDesc(m_pResource->GetDesc()),
        m_PlaneCount(D3D12GetFormatPlaneCount(Parent->GetDevice(), Desc.Format)),
        m_NumSubresources(CalculateNumSubresources()), m_ResourceState(m_NumSubresources, InitialResourceState),
        m_GpuVirtualAddress(m_pResource->GetGPUVirtualAddress())
    {
        this->SetResourceName(Name);
    }

    void D3D12Resource ::Destroy()
    {
        m_pResource = nullptr;
        m_GpuVirtualAddress = D3D12_GPU_VIRTUAL_ADDRESS_NULL;
        ++m_VersionID;
    }

    void D3D12Resource::SetResourceName(std::wstring name)
    {
        this->m_ResourceName = name;
        m_pResource->SetName(name.c_str());
    }

    bool D3D12Resource::ImplicitStatePromotion(D3D12_RESOURCE_STATES State) const noexcept
    {
        // All buffer resources as well as textures with the D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS flag set are
        // implicitly promoted from D3D12_RESOURCE_STATE_COMMON to the relevant state on first GPU access, including
        // GENERIC_READ to cover any read scenario.

        // When this access occurs the promotion acts like an implicit resource barrier. For subsequent accesses,
        // resource barriers will be required to change the resource state if necessary. Note that promotion from one
        // promoted read state into multiple read state is valid, but this is not the case for write states.
        if (m_ResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
        {
            return true;
        }
        else
        {
            // Simultaneous-Access Textures
            if (m_ResourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS)
            {
                // *Depth-stencil resources must be non-simultaneous-access textures and thus can never be implicitly
                // promoted.
                constexpr D3D12_RESOURCE_STATES NonPromotableStates =
                    D3D12_RESOURCE_STATE_DEPTH_WRITE | D3D12_RESOURCE_STATE_DEPTH_READ;
                if (State & NonPromotableStates)
                {
                    return false;
                }
                return true;
            }
            // Non-Simultaneous-Access Textures
            else
            {
                constexpr D3D12_RESOURCE_STATES NonPromotableStates =
                    D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER | D3D12_RESOURCE_STATE_INDEX_BUFFER |
                    D3D12_RESOURCE_STATE_RENDER_TARGET | D3D12_RESOURCE_STATE_UNORDERED_ACCESS |
                    D3D12_RESOURCE_STATE_DEPTH_WRITE | D3D12_RESOURCE_STATE_DEPTH_READ |
                    D3D12_RESOURCE_STATE_STREAM_OUT | D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT |
                    D3D12_RESOURCE_STATE_RESOLVE_DEST | D3D12_RESOURCE_STATE_RESOLVE_SOURCE |
                    D3D12_RESOURCE_STATE_PREDICATION;

                constexpr D3D12_RESOURCE_STATES PromotableStates =
                    D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
                    D3D12_RESOURCE_STATE_COPY_DEST | D3D12_RESOURCE_STATE_COPY_SOURCE;

                if (State & NonPromotableStates)
                {
                    return false;
                }
                else
                {
                    UNREFERENCED_PARAMETER(PromotableStates);
                    return true;
                }
            }
        }
    }

    bool D3D12Resource::ImplicitStateDecay(D3D12_RESOURCE_STATES   State,
                                           D3D12_COMMAND_LIST_TYPE AccessedQueueType) const noexcept
    {
        // 1. Resources being accessed on a Copy queue
        if (AccessedQueueType == D3D12_COMMAND_LIST_TYPE_COPY)
        {
            return true;
        }

        // 2. Buffer resources on any queue type
        if (m_ResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
        {
            return true;
        }

        // 3. Texture resources on any queue type that have the D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS flag set
        if (m_ResourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS)
        {
            return true;
        }

        // 4. Any resource implicitly promoted to a read-only state
        // NOTE: We dont care about buffers here because buffer will decay due to Case 2, only textures, so any read
        // state that is supported by Non-Simultaneous-Access Textures can decay to common
        constexpr D3D12_RESOURCE_STATES ReadOnlyStates = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
                                                         D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
                                                         D3D12_RESOURCE_STATE_COPY_SOURCE;
        if (State & ReadOnlyStates)
        {
            return true;
        }

        return false;
    }

    Microsoft::WRL::ComPtr<ID3D12Resource>
    D3D12Resource::InitializeResource(CD3DX12_HEAP_PROPERTIES            HeapProperties,
                                      CD3DX12_RESOURCE_DESC              Desc,
                                      D3D12_RESOURCE_STATES              InitialResourceState,
                                      std::optional<CD3DX12_CLEAR_VALUE> ClearValue) const
    {
        D3D12_CLEAR_VALUE* OptimizedClearValue = ClearValue.has_value() ? &(*ClearValue) : nullptr;

        Microsoft::WRL::ComPtr<ID3D12Resource> Resource;
        VERIFY_D3D12_API(Parent->GetDevice()->CreateCommittedResource(&HeapProperties,
                                                                      D3D12_HEAP_FLAG_NONE,
                                                                      &Desc,
                                                                      InitialResourceState,
                                                                      OptimizedClearValue,
                                                                      IID_PPV_ARGS(Resource.ReleaseAndGetAddressOf())));
        return Resource;
    }

    UINT D3D12Resource::CalculateNumSubresources() const
    {
        if (m_ResourceDesc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER)
        {
            return m_ResourceDesc.MipLevels * m_ResourceDesc.DepthOrArraySize * m_PlaneCount;
        }
        // Buffer only contains 1 subresource
        return 1;
    }

    D3D12ASBuffer::D3D12ASBuffer(D3D12LinkedDevice* Parent, UINT64 SizeInBytes) :
        D3D12Resource(Parent,
                      CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT, Parent->GetNodeMask(), Parent->GetNodeMask()),
                      CD3DX12_RESOURCE_DESC::Buffer(SizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
                      D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
                      std::nullopt)
    {}

    D3D12Buffer::D3D12Buffer(D3D12LinkedDevice*   Parent,
                             UINT64               SizeInBytes,
                             UINT                 Stride,
                             D3D12_HEAP_TYPE      HeapType,
                             D3D12_RESOURCE_FLAGS ResourceFlags) :
        D3D12Resource(Parent,
                      CD3DX12_HEAP_PROPERTIES(HeapType, Parent->GetNodeMask(), Parent->GetNodeMask()),
                      CD3DX12_RESOURCE_DESC::Buffer(SizeInBytes, ResourceFlags),
                      ResourceStateDeterminer(CD3DX12_RESOURCE_DESC::Buffer(SizeInBytes, ResourceFlags), HeapType)
                          .InferInitialState(),
                      std::nullopt),
        m_HeapType(HeapType), m_SizeInBytes(SizeInBytes), m_Stride(Stride),
        m_ScopedPointer(HeapType == D3D12_HEAP_TYPE_UPLOAD ? m_pResource.Get() : nullptr),
        // We do not need to unmap until we are done with the resource.  However, we must not write to
        // the resource while it is in use by the GPU (so we must use synchronization techniques).
        m_CpuVirtualAddress(m_ScopedPointer.Address)
    {}

    D3D12Buffer::D3D12Buffer(D3D12LinkedDevice*    Parent,
                             UINT64                SizeInBytes,
                             UINT                  Stride,
                             D3D12_HEAP_TYPE       HeapType,
                             D3D12_RESOURCE_FLAGS  ResourceFlags,
                             D3D12_RESOURCE_STATES InitialResourceState) :
        D3D12Resource(Parent,
                      CD3DX12_HEAP_PROPERTIES(HeapType, Parent->GetNodeMask(), Parent->GetNodeMask()),
                      CD3DX12_RESOURCE_DESC::Buffer(SizeInBytes, ResourceFlags),
                      InitialResourceState,
                      std::nullopt),
        m_HeapType(HeapType), m_SizeInBytes(SizeInBytes), m_Stride(Stride),
        m_ScopedPointer(HeapType == D3D12_HEAP_TYPE_UPLOAD ? m_pResource.Get() : nullptr),
        // We do not need to unmap until we are done with the resource.  However, we must not write to
        // the resource while it is in use by the GPU (so we must use synchronization techniques).
        m_CpuVirtualAddress(m_ScopedPointer.Address)
    {}

    D3D12_GPU_VIRTUAL_ADDRESS D3D12Buffer::GetGpuVirtualAddress(UINT Index) const
    {
        return m_pResource->GetGPUVirtualAddress() + static_cast<UINT64>(Index) * m_Stride;
    }

    D3D12Texture::D3D12Texture(D3D12LinkedDevice*                       Parent,
                               Microsoft::WRL::ComPtr<ID3D12Resource>&& Resource,
                               CD3DX12_CLEAR_VALUE                      ClearValue,
                               D3D12_RESOURCE_STATES                    InitialResourceState) :
        D3D12Resource(Parent, std::move(Resource), ClearValue, InitialResourceState)
    {}

    D3D12Texture::D3D12Texture(D3D12LinkedDevice*                 Parent,
                               const CD3DX12_RESOURCE_DESC&       Desc,
                               std::optional<CD3DX12_CLEAR_VALUE> ClearValue/* = std::nullopt*/,
                               bool                               Cubemap/*    = false*/) :
        D3D12Resource(Parent,
                      CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT, Parent->GetNodeMask(), Parent->GetNodeMask()),
                      Desc,
                      ResourceStateDeterminer(Desc, D3D12_HEAP_TYPE_DEFAULT).InferInitialState(),
                      ClearValue),
        Cubemap(Cubemap)
    {
        // Textures can only be in device local heap (for discrete GPUs) UMA case is not handled
    }

    UINT D3D12Texture::GetSubresourceIndex(std::optional<UINT> OptArraySlice /*= std::nullopt*/,
                                           std::optional<UINT> OptMipSlice /*= std::nullopt*/,
                                           std::optional<UINT> OptPlaneSlice /*= std::nullopt*/) const noexcept
    {
        UINT ArraySlice = OptArraySlice.value_or(0);
        UINT MipSlice   = OptMipSlice.value_or(0);
        UINT PlaneSlice = OptPlaneSlice.value_or(0);
        return D3D12CalcSubresource(MipSlice, ArraySlice, PlaneSlice, m_ResourceDesc.MipLevels, m_ResourceDesc.DepthOrArraySize);
    }

    void D3D12Texture::AssociateWithResource(D3D12LinkedDevice*                       Parent,
                                             const std::wstring&                      Name,
                                             Microsoft::WRL::ComPtr<ID3D12Resource>&& Resource,
                                             D3D12_RESOURCE_STATES                    CurrentState)
    {
        assert(Resource != nullptr);

        this->Parent              = Parent;
        this->m_pResource         = Resource;
        this->m_GpuVirtualAddress = Resource->GetGPUVirtualAddress();
        this->m_ClearValue        = CD3DX12_CLEAR_VALUE();
        this->m_ResourceDesc              = CD3DX12_RESOURCE_DESC(Resource->GetDesc());

        this->m_PlaneCount      = D3D12GetFormatPlaneCount(Parent->GetDevice(), m_ResourceDesc.Format);
        this->m_NumSubresources = this->CalculateNumSubresources();
        this->m_ResourceState   = CResourceState(m_NumSubresources, CurrentState);
        this->m_ResourceName    = Name;

#ifndef RELEASE
        m_pResource->SetName(Name.c_str());
#else
        (Name);
#endif
    }

    //=========================================================================================================

    std::shared_ptr<BufferD3D12> BufferD3D12::CreateBuffer(D3D12LinkedDevice* Parent,
                                                           RHIBufferTarget    bufferTarget,
                                                           UINT32             numElements,
                                                           UINT32             elementSize,
                                                           bool               mapplable,
                                                           std::wstring&      name,
                                                           BYTE*              initialData,
                                                           UINT               dataLen)
    {
        std::shared_ptr<BufferD3D12> pBufferD3D12 = std::make_shared<BufferD3D12>(Parent);

        UINT sizeInBytes = numElements * elementSize;
        D3D12_HEAP_TYPE heapType = mapplable ? D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD : D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
        
        D3D12_RESOURCE_FLAGS resourceFlag = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

        D3D12_RESOURCE_STATES initState = D3D12_RESOURCE_STATE_GENERIC_READ;

        RHIBufferMode bufferMode = mapplable ? RHIBufferMode::RHIBufferModeDynamic : RHIBufferMode::RHIBufferModeImmutable;

        pBufferD3D12->m_Desc = {sizeInBytes, numElements, elementSize, bufferTarget, bufferMode};
        pBufferD3D12->m_Data = {initialData, dataLen};
        pBufferD3D12->m_Name = name;

        pBufferD3D12->p_ResourceD3D12Buffer =
            std::make_shared<D3D12Buffer>(Parent, sizeInBytes, elementSize, heapType, resourceFlag, initState);

        if (bufferTarget & RHIBufferTarget::RHIBufferTargetRaw)
        {
            pBufferD3D12->p_CounterBufferD3D12 = nullptr;
        }
        else
        {
            std::wstring counterName = L"Counter";
            pBufferD3D12->p_CounterBufferD3D12 =
                CreateBuffer(Parent, RHIBufferTarget::RHIBufferTargetCounter, 1, sizeof(UINT32), false, counterName, nullptr, 0);
        }

        // Inflate Buffer
        if (initialData != nullptr)
        {

        }

        // Reset CounterBuffer
        if (bufferTarget & RHIBufferTarget::RHIBufferTargetCounter)
        {

        }

        return pBufferD3D12;
    }

    std::shared_ptr<BufferD3D12> BufferD3D12::GetCounterBuffer() { return this->p_CounterBufferD3D12; }

    bool BufferD3D12::InflateBuffer(BYTE* initialData, UINT dataLen)
    {

    }

    void BufferD3D12::ResetCounterBuffer()
    {

    }

    std::shared_ptr<D3D12ConstantBufferView> BufferD3D12::CreateCBV(D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc)
    {

    }

    std::shared_ptr<D3D12ShaderResourceView> BufferD3D12::CreateSRV(D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc)
    {

    }

    std::shared_ptr<D3D12UnorderedAccessView> BufferD3D12::CreateUAV(D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc)
    {

    }

    std::shared_ptr<D3D12ConstantBufferView> BufferD3D12::GetDefaultCBV()
    {

    }

    std::shared_ptr<D3D12ShaderResourceView> BufferD3D12::GetDefaultSRV()
    {

    }

    std::shared_ptr<D3D12UnorderedAccessView> BufferD3D12::GetDefaultUAV()
    {

    }




















    /*
    const std::shared_ptr<D3D12ShaderResourceView> D3D12Resource::CreateSRV(D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc)
    {
        std::shared_ptr<D3D12ShaderResourceView> srv           = nullptr;
        auto                                     srvHandleIter = m_SRVHandleMap.find(srvDesc);
        if (srvHandleIter != m_SRVHandleMap.end())
        {
            srv                     = std::make_shared<D3D12ShaderResourceView>(Parent, srvDesc, this);
            m_SRVHandleMap[srvDesc] = srv;
        }
        else
        {
            srv = srvHandleIter->second;
        }
        return srv;
    }

    const std::shared_ptr<D3D12UnorderedAccessView> D3D12Resource::CreateUAV(D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc,
                                                                             D3D12Resource* pCounterResource)
    {
        UnorderedAccessViewKey                    uavInfo       = {uavDesc, pCounterResource};
        std::shared_ptr<D3D12UnorderedAccessView> uav           = nullptr;
        auto                                      uavHandleIter = m_UAVHandleMap.find(uavInfo);
        if (uavHandleIter != m_UAVHandleMap.end())
        {
            uav = std::make_shared<D3D12UnorderedAccessView>(Parent, uavDesc, this, pCounterResource);
            m_UAVHandleMap[uavInfo] = uav;
        }
        else
        {
            uav = uavHandleIter->second;
        }
        return uav;
    }

    const std::shared_ptr<D3D12RenderTargetView> D3D12Resource::CreateRTV(D3D12_RENDER_TARGET_VIEW_DESC rtvDesc)
    {
        std::shared_ptr<D3D12RenderTargetView> rtv           = nullptr;
        auto                                   rtvHandleIter = m_RTVHandleMap.find(rtvDesc);
        if (rtvHandleIter != m_RTVHandleMap.end())
        {
            rtv                     = std::make_shared<D3D12RenderTargetView>(Parent, rtvDesc, this);
            m_RTVHandleMap[rtvDesc] = rtv;
        }
        else
        {
            rtv = rtvHandleIter->second;
        }
        return rtv;
    }

    const std::shared_ptr<D3D12DepthStencilView> D3D12Resource::CreateDSV(D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc)
    {
        std::shared_ptr<D3D12DepthStencilView> dsv = nullptr;

        auto dsvHandleIter = m_DSVHandleMap.find(dsvDesc);
        if (dsvHandleIter != m_DSVHandleMap.end())
        {
            dsv                     = std::make_shared<D3D12DepthStencilView>(Parent, dsvDesc, this);
            m_DSVHandleMap[dsvDesc] = dsv;
        }
        else
        {
            dsv = dsvHandleIter->second;
        }
        return dsv;
    }

    void D3D12Resource::CacheSRV(std::string srvName, std::shared_ptr<D3D12ShaderResourceView>) {}

    void D3D12Resource::CacheUAV(std::string srvName, std::shared_ptr<D3D12UnorderedAccessView>) {}


    UploadBuffer::UploadBuffer(D3D12LinkedDevice* Parent, UINT BufferSize, const std::wstring& name) :
        D3D12Buffer(Parent,
                    BufferSize,
                    BufferSize,
                    D3D12_HEAP_TYPE_UPLOAD,
                    D3D12_RESOURCE_FLAG_NONE,
                    D3D12_RESOURCE_STATE_GENERIC_READ)
    {
        this->m_pResource->SetName(name.c_str());
    }

    UINT UploadBuffer::GetBufferSize() const { return D3D12Buffer::GetSizeInBytes(); }

    ReadbackBuffer::ReadbackBuffer(UINT NumElements, UINT ElementSize, const std::wstring& name) {}
    */

}

