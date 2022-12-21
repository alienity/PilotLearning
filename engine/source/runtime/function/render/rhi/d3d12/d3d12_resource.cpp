#include "d3d12_resource.h"
#include "d3d12_linkedDevice.h"
#include "d3d12_descriptor.h"
#include "d3d12_commandContext.h"
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
                                 D3D12_RESOURCE_STATES                    InitialResourceState) :
        D3D12LinkedDeviceChild(Parent),
        m_pResource(std::move(Resource)), m_ClearValue(std::nullopt), m_ResourceDesc(this->m_pResource->GetDesc()),
        m_PlaneCount(D3D12GetFormatPlaneCount(Parent->GetDevice(), m_ResourceDesc.Format)),
        m_NumSubresources(CalculateNumSubresources()), m_ResourceState(m_NumSubresources, InitialResourceState)
    {}

    D3D12Resource::D3D12Resource(D3D12LinkedDevice*                 Parent,
                                 CD3DX12_HEAP_PROPERTIES            HeapProperties,
                                 CD3DX12_RESOURCE_DESC              Desc,
                                 D3D12_RESOURCE_STATES              InitialResourceState,
                                 std::optional<CD3DX12_CLEAR_VALUE> ClearValue) :
        D3D12LinkedDeviceChild(Parent),
        m_pResource(InitializeResource(HeapProperties, Desc, InitialResourceState, ClearValue)),
        m_ClearValue(ClearValue), m_ResourceDesc(m_pResource->GetDesc()),
        m_PlaneCount(D3D12GetFormatPlaneCount(Parent->GetDevice(), Desc.Format)),
        m_NumSubresources(CalculateNumSubresources()), m_ResourceState(m_NumSubresources, InitialResourceState)
    {}

    D3D12Resource::D3D12Resource(D3D12LinkedDevice*                       Parent,
                                 Microsoft::WRL::ComPtr<ID3D12Resource>&& Resource,
                                 D3D12_RESOURCE_STATES                    InitialResourceState,
                                 std::optional<CD3DX12_CLEAR_VALUE>       ClearValue,
                                 std::wstring                             Name) :
        D3D12LinkedDeviceChild(Parent),
        m_pResource(std::move(Resource)), m_ClearValue(ClearValue), m_ResourceDesc(this->m_pResource->GetDesc()),
        m_PlaneCount(D3D12GetFormatPlaneCount(Parent->GetDevice(), m_ResourceDesc.Format)),
        m_NumSubresources(CalculateNumSubresources()), m_ResourceState(m_NumSubresources, InitialResourceState)
    {
        this->SetResourceName(Name);
    }

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
        m_NumSubresources(CalculateNumSubresources()), m_ResourceState(m_NumSubresources, InitialResourceState)
    {
        this->SetResourceName(Name);
    }

    void D3D12Resource ::Destroy()
    {
        m_pResource = nullptr;
        ++m_VersionID;
    }

    void D3D12Resource::SetResourceName(std::wstring name)
    {
        this->m_ResourceName = name;
        m_pResource->SetName(name.c_str());
    }

    const CD3DX12_CLEAR_VALUE D3D12Resource::GetClearValue() const noexcept
    {
        return m_ClearValue.has_value() ? m_ClearValue.value() : CD3DX12_CLEAR_VALUE {};
    }

    inline void D3D12Resource::GetClearColor(FLOAT color[4]) const
    {
        CD3DX12_CLEAR_VALUE clear_val = GetClearValue();
        memcpy(color, clear_val.Color, sizeof(clear_val.Color));
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
                      std::nullopt),
        m_GpuVirtualAddress(m_pResource->GetGPUVirtualAddress())
    {}

    D3D12_GPU_VIRTUAL_ADDRESS D3D12ASBuffer::GetGpuVirtualAddress() const { return m_GpuVirtualAddress; }

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
        m_GpuVirtualAddress(m_pResource->GetGPUVirtualAddress()),
        m_ScopedPointer(HeapType == D3D12_HEAP_TYPE_UPLOAD ? m_pResource.Get() : nullptr)
    {
        // We do not need to unmap until we are done with the resource.  However, we must not write to
        // the resource while it is in use by the GPU (so we must use synchronization techniques).
        m_CpuVirtualAddress = m_ScopedPointer.Address;
    }

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
        m_GpuVirtualAddress(m_pResource->GetGPUVirtualAddress()),
        m_ScopedPointer(HeapType == D3D12_HEAP_TYPE_UPLOAD ? m_pResource.Get() : nullptr)
    {
        // We do not need to unmap until we are done with the resource.  However, we must not write to
        // the resource while it is in use by the GPU (so we must use synchronization techniques).
        m_CpuVirtualAddress = m_ScopedPointer.Address;
    }

    void D3D12Buffer::Destroy()
    {
        m_ScopedPointer.Release();
        m_pResource = nullptr;
        ++m_VersionID;

        m_CpuVirtualAddress = nullptr;
        m_GpuVirtualAddress = D3D12_GPU_VIRTUAL_ADDRESS_NULL;

        if (m_Data.m_Data != nullptr)
        {
            free(m_Data.m_Data);
            m_Data.m_Data = nullptr;
        }
        p_CounterBufferD3D12 = nullptr;

        m_CBVHandleMap.clear();
        m_SRVHandleMap.clear();
        m_UAVHandleMap.clear();
    }

    D3D12_GPU_VIRTUAL_ADDRESS D3D12Buffer::GetGpuVirtualAddress(UINT Index) const
    {
        //return m_pResource->GetGPUVirtualAddress() + static_cast<UINT64>(Index) * m_Stride;
        return m_GpuVirtualAddress + static_cast<UINT64>(Index) * m_Stride;
    }

    std::shared_ptr<D3D12Buffer> D3D12Buffer::Create(D3D12LinkedDevice*    Parent,
                                                     RHIBufferTarget       bufferTarget,
                                                     UINT32                numElements,
                                                     UINT32                elementSize,
                                                     const std::wstring    name,
                                                     RHIBufferMode         mapplableMode,
                                                     D3D12_RESOURCE_STATES initState,
                                                     BYTE*                 initialData,
                                                     UINT                  dataLen)
    {

        ASSERT(mapplableMode == RHIBufferMode::RHIBufferModeDynamic ||
               mapplableMode == RHIBufferMode::RHIBufferModeImmutable);

        ASSERT(!((mapplableMode == RHIBufferMode::RHIBufferModeDynamic) &&
                 (bufferTarget & RHIBufferTarget::RHIBufferRandomReadWrite)));

        bool allowUAV = bufferTarget & RHIBufferTarget::RHIBufferRandomReadWrite;

        D3D12_RESOURCE_FLAGS resourceFlag =
            allowUAV ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE;
        
        bool mapplable = mapplableMode == RHIBufferMode::RHIBufferModeDynamic;

        D3D12_HEAP_TYPE heapType =
            mapplable ? D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD : D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;

        UINT sizeInBytes = numElements * elementSize;

        std::shared_ptr<D3D12Buffer> pBufferD3D12 =
            std::make_shared<D3D12Buffer>(Parent, sizeInBytes, elementSize, heapType, resourceFlag, initState);
        pBufferD3D12->SetResourceName(name);

        pBufferD3D12->m_Desc = {sizeInBytes, numElements, elementSize, bufferTarget, mapplableMode};
        pBufferD3D12->m_Data = {initialData, dataLen};

        if (bufferTarget & RHIBufferTarget::RHIBufferTargetCounter)
        {
            RHIBufferTarget counterBufferTarget = bufferTarget | RHIBufferTarget::RHIBufferTargetRaw;
            counterBufferTarget &= ~RHIBufferTarget::RHIBufferTargetCounter;

            pBufferD3D12->p_CounterBufferD3D12 = Create(Parent,
                                                        counterBufferTarget,
                                                        1,
                                                        sizeof(UINT32),
                                                        name + L"_Counter",
                                                        RHIBufferMode::RHIBufferModeImmutable,
                                                        D3D12_RESOURCE_STATE_GENERIC_READ,
                                                        nullptr,
                                                        0);
        }
        else
        {
            pBufferD3D12->p_CounterBufferD3D12 = nullptr;
        }
        /*
        bool uploadNeedFinished = false;

        // Inflate Buffer
        if (initialData != nullptr)
        {
            pBufferD3D12->InflateBuffer(initialData, dataLen);
            uploadNeedFinished = true;
        }

        // Reset CounterBuffer
        if (bufferTarget & RHIBufferTarget::RHIBufferTargetCounter)
        {
            D3D12CommandContext& InitContext = Parent->BeginResourceUpload();
            pBufferD3D12->ResetCounterBuffer(&InitContext);
            uploadNeedFinished = true;
        }

        if (uploadNeedFinished)
        {
            Parent->EndResourceUpload(true);
        }
        */
        return pBufferD3D12;
    }

    std::shared_ptr<D3D12Buffer> D3D12Buffer::GetCounterBuffer() { return this->p_CounterBufferD3D12; }

    bool D3D12Buffer::InflateBuffer(BYTE* initialData, UINT dataLen)
    {
        ASSERT(this->m_Data.m_DataLen == dataLen);
        if (!memcmp(this->m_Data.m_Data, initialData, dataLen))
            return false;

        free(this->m_Data.m_Data);
        this->m_Data = {initialData, dataLen};

        if (this->m_Desc.mode == RHIBufferMode::RHIBufferModeDynamic)
        {
            memcpy(this->GetCpuVirtualAddress(), initialData, dataLen);
        }
        else
        {
            D3D12CommandContext::InitializeBuffer(Parent, this, m_Data.m_Data, m_Data.m_DataLen);
        }

        return true;
    }

    void D3D12Buffer::ResetCounterBuffer(D3D12CommandContext* pCommandContext)
    {
        ASSERT(p_CounterBufferD3D12 != nullptr);
        pCommandContext->Open();
        pCommandContext->ResetCounter(p_CounterBufferD3D12.get(), 0, 0);
    }

    std::shared_ptr<D3D12ConstantBufferView> D3D12Buffer::CreateCBV(D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc)
    {
        uint64 descHash = CityHash64((const char*)&cbvDesc, sizeof(D3D12_CONSTANT_BUFFER_VIEW_DESC));

        std::shared_ptr<D3D12ConstantBufferView> cbv = nullptr;
        auto cbvHandleIter = m_CBVHandleMap.find(descHash);
        if (cbvHandleIter == m_CBVHandleMap.end())
        {
            cbv = std::make_shared<D3D12ConstantBufferView>(Parent, cbvDesc, this);
            m_CBVHandleMap[descHash] = cbv;
        }
        else
        {
            cbv = cbvHandleIter->second;
        }
        return cbv;
    }

    std::shared_ptr<D3D12ShaderResourceView> D3D12Buffer::CreateSRV(D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc)
    {
        uint64 descHash = CityHash64((const char*)&srvDesc, sizeof(D3D12_SHADER_RESOURCE_VIEW_DESC));

        std::shared_ptr<D3D12ShaderResourceView> srv = nullptr;
        auto srvHandleIter = m_SRVHandleMap.find(descHash);
        if (srvHandleIter == m_SRVHandleMap.end())
        {
            srv = std::make_shared<D3D12ShaderResourceView>(Parent, srvDesc, this);
            m_SRVHandleMap[descHash] = srv;
        }
        else
        {
            srv = srvHandleIter->second;
        }
        return srv;
    }

    std::shared_ptr<D3D12UnorderedAccessView> D3D12Buffer::CreateUAV(D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc)
    {
        D3D12Resource* pCounterResource = p_CounterBufferD3D12 != nullptr ? p_CounterBufferD3D12.get() : nullptr;
        BUFFER_UNORDERED_ACCESS_VIEW_KEY uavKey = {uavDesc, pCounterResource};

        uint64 descHash = CityHash64((const char*)&uavKey, sizeof(BUFFER_UNORDERED_ACCESS_VIEW_KEY));

        std::shared_ptr<D3D12UnorderedAccessView> uav = nullptr;
        auto uavHandleIter = m_UAVHandleMap.find(descHash);
        if (uavHandleIter == m_UAVHandleMap.end())
        {
            uav = std::make_shared<D3D12UnorderedAccessView>(
                Parent, uavDesc, (D3D12Resource*)this, (D3D12Resource*)pCounterResource);
            m_UAVHandleMap[descHash] = uav;
        }
        else
        {
            uav = uavHandleIter->second;
        }
        return uav;
    }

    std::shared_ptr<D3D12ConstantBufferView> D3D12Buffer::GetDefaultCBV()
    {
        return CreateCBV(D3D12ConstantBufferView::GetDesc(this, 0, m_Desc.size));
    }

    std::shared_ptr<D3D12ShaderResourceView> D3D12Buffer::GetDefaultSRV()
    {
        ASSERT(m_Desc.size % m_Desc.stride == 0);
        return CreateSRV(D3D12ShaderResourceView::GetDesc(this, m_Desc.target & RHIBufferTargetRaw, 0, m_Desc.number));
    }

    std::shared_ptr<D3D12UnorderedAccessView> D3D12Buffer::GetDefaultUAV()
    {
        return CreateUAV(D3D12UnorderedAccessView::GetDesc(this, m_Desc.target & RHIBufferTargetRaw, 0, m_Desc.number, 0));
    }

    RHIBufferDesc& D3D12Buffer::GetBufferDesc() { return m_Desc; }

    D3D12Texture::D3D12Texture(D3D12LinkedDevice*                       Parent,
                               Microsoft::WRL::ComPtr<ID3D12Resource>&& Resource,
                               D3D12_RESOURCE_STATES                    InitialResourceState) :
        D3D12Resource(Parent, std::move(Resource), InitialResourceState)
    {}

    D3D12Texture::D3D12Texture(D3D12LinkedDevice*                 Parent,
                               const CD3DX12_RESOURCE_DESC&       Desc,
                               const CD3DX12_HEAP_PROPERTIES&     HeapProperty,
                               std::optional<CD3DX12_CLEAR_VALUE> ClearValue,
                               D3D12_RESOURCE_STATES              InitialResourceState,
                               bool                               Cubemap) :
        D3D12Resource(Parent, HeapProperty, Desc, InitialResourceState, ClearValue),
        m_IsCubemap(Cubemap)
    {}

    D3D12Texture::D3D12Texture(D3D12LinkedDevice*                 Parent,
                               const CD3DX12_RESOURCE_DESC&       Desc,
                               std::optional<CD3DX12_CLEAR_VALUE> ClearValue,
                               D3D12_RESOURCE_STATES              InitialResourceState,
                               bool                               Cubemap) :
        D3D12Resource(Parent,
                      CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT, Parent->GetNodeMask(), Parent->GetNodeMask()),
                      Desc,
                      InitialResourceState,
                      ClearValue),
        m_IsCubemap(Cubemap)
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
        m_IsCubemap(Cubemap)
    {
        // Textures can only be in device local heap (for discrete GPUs) UMA case is not handled
    }

    void D3D12Texture::Destroy()
    {
        m_pResource = nullptr;
        ++m_VersionID;

        m_RTVHandleMap.clear();
        m_DSVHandleMap.clear();
        m_SRVHandleMap.clear();
        m_UAVHandleMap.clear();
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

    void D3D12Texture::AssociateWithResource(D3D12LinkedDevice*                     Parent,
                                             const std::wstring&                    Name,
                                             Microsoft::WRL::ComPtr<ID3D12Resource> Resource,
                                             D3D12_RESOURCE_STATES                  CurrentState)
    {
        ASSERT(Resource != nullptr);

        this->Parent              = Parent;
        this->m_pResource         = Resource;
        this->m_ClearValue        = CD3DX12_CLEAR_VALUE();
        this->m_ResourceDesc      = CD3DX12_RESOURCE_DESC(Resource->GetDesc());

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

    std::shared_ptr<D3D12Texture> D3D12Texture::Create(D3D12LinkedDevice*                  Parent,
                                                       RHIRenderSurfaceBaseDesc            desc,
                                                       const std::wstring                  name,
                                                       D3D12_RESOURCE_STATES               initState,
                                                       std::optional<CD3DX12_CLEAR_VALUE>  clearValue,
                                                       std::vector<D3D12_SUBRESOURCE_DATA> initDatas)
    {
        D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_NONE;
        if (desc.flags & RHISurfaceCreateRenderTarget)
        {
            ASSERT(!(desc.flags & RHISurfaceCreateDepthStencil));
            resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        }
        else if (desc.flags & (RHISurfaceCreateDepthStencil | RHISurfaceCreateShadowmap))
        {
            ASSERT(!(desc.flags & RHISurfaceCreateRenderTarget));
            resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        }

        if (desc.flags & RHISurfaceCreateRandomWrite)
        {
            resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        }

        desc.mipCount = D3D12Texture::GetMipLevels(desc.width, desc.height, desc.mipCount, desc.flags);

        D3D12_RESOURCE_DIMENSION dimension;
        CD3DX12_RESOURCE_DESC resourceDesc;
        if (desc.dim == RHITexDim2D || desc.dim == RHITexDim2DArray || desc.dim == RHITexDimCube || desc.dim == RHITexDimCubeArray)
        {
            dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        }
        else if (desc.dim == RHITexDim3D)
        {
            dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
        }
        resourceDesc = CD3DX12_RESOURCE_DESC(dimension,
                                             0,
                                             desc.width,
                                             desc.height,
                                             desc.depthOrArray,
                                             desc.mipCount,
                                             desc.graphicsFormat,
                                             desc.samples,
                                             0,
                                             D3D12_TEXTURE_LAYOUT_UNKNOWN,
                                             resourceFlags);

        CD3DX12_HEAP_PROPERTIES resourceHeapProperties =
            CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT, Parent->GetNodeMask(), Parent->GetNodeMask());

        std::shared_ptr<D3D12Texture> pSurfaceD3D12 =
            std::make_shared<D3D12Texture>(Parent, resourceDesc, resourceHeapProperties, clearValue, initState, false);
        pSurfaceD3D12->SetResourceName(name);

        pSurfaceD3D12->m_Desc = desc;
        
        return pSurfaceD3D12;
    }

    std::shared_ptr<D3D12Texture> D3D12Texture::Create2D(D3D12LinkedDevice*                 Parent,
                                                         UINT32                             width,
                                                         UINT32                             height,
                                                         INT32                              numMips,
                                                         DXGI_FORMAT                        format,
                                                         RHISurfaceCreateFlags              flags,
                                                         UINT32                             sampleCount,
                                                         const std::wstring                 name,
                                                         D3D12_RESOURCE_STATES              initState,
                                                         std::optional<CD3DX12_CLEAR_VALUE> clearValue,
                                                         D3D12_SUBRESOURCE_DATA             initData)
    {
        RHIRenderSurfaceBaseDesc desc = {
            width, height, 1, sampleCount, numMips, flags, RHITexDim2D, format, true, false};
        return Create(Parent, desc, name, initState, clearValue, {initData});
    }

    std::shared_ptr<D3D12Texture> D3D12Texture::Create2DArray(D3D12LinkedDevice*                  Parent,
                                                              UINT32                              width,
                                                              UINT32                              height,
                                                              UINT32                              arraySize,
                                                              INT32                               numMips,
                                                              DXGI_FORMAT                         format,
                                                              RHISurfaceCreateFlags               flags,
                                                              UINT32                              sampleCount,
                                                              const std::wstring                  name,
                                                              D3D12_RESOURCE_STATES               initState,
                                                              std::optional<CD3DX12_CLEAR_VALUE>  clearValue,
                                                              std::vector<D3D12_SUBRESOURCE_DATA> initDatas)
    {
        RHIRenderSurfaceBaseDesc desc = {
            width, height, arraySize, sampleCount, numMips, flags, RHITexDim2DArray, format, true, false};
        return Create(Parent, desc, name, initState, clearValue, initDatas);
    }

    std::shared_ptr<D3D12Texture> D3D12Texture::CreateCubeMap(D3D12LinkedDevice*                  Parent,
                                                              UINT32                              width,
                                                              UINT32                              height,
                                                              INT32                               numMips,
                                                              DXGI_FORMAT                         format,
                                                              RHISurfaceCreateFlags               flags,
                                                              UINT32                              sampleCount,
                                                              const std::wstring                  name,
                                                              D3D12_RESOURCE_STATES               initState,
                                                              std::optional<CD3DX12_CLEAR_VALUE>  clearValue,
                                                              std::vector<D3D12_SUBRESOURCE_DATA> initDatas)
    {
        UINT cubeFaces = 6;
        RHIRenderSurfaceBaseDesc desc = {
            width, height, cubeFaces, sampleCount, numMips, flags, RHITexDimCube, format, true, false};
        return Create(Parent, desc, name, initState, clearValue, initDatas);
    }

    std::shared_ptr<D3D12Texture> D3D12Texture::CreateCubeMapArray(D3D12LinkedDevice*                  Parent,
                                                                   UINT32                              width,
                                                                   UINT32                              height,
                                                                   UINT32                              arraySize,
                                                                   INT32                               numMips,
                                                                   DXGI_FORMAT                         format,
                                                                   RHISurfaceCreateFlags               flags,
                                                                   UINT32                              sampleCount,
                                                                   const std::wstring                  name,
                                                                   D3D12_RESOURCE_STATES               initState,
                                                                   std::optional<CD3DX12_CLEAR_VALUE>  clearValue,
                                                                   std::vector<D3D12_SUBRESOURCE_DATA> initDatas)
    {
        UINT cubeFaces = 6;
        UINT cubeArrayFaces = cubeFaces * arraySize;
        RHIRenderSurfaceBaseDesc desc = {
            width, height, cubeArrayFaces, sampleCount, numMips, flags, RHITexDimCubeArray, format, true, false};
        return Create(Parent, desc, name, initState, clearValue, initDatas);
    }

    std::shared_ptr<D3D12Texture> D3D12Texture::Create3D(D3D12LinkedDevice*                 Parent,
                                                         UINT32                             width,
                                                         UINT32                             height,
                                                         UINT32                             depth,
                                                         INT32                              numMips,
                                                         DXGI_FORMAT                        format,
                                                         RHISurfaceCreateFlags              flags,
                                                         UINT32                             sampleCount,
                                                         const std::wstring                 name,
                                                         D3D12_RESOURCE_STATES              initState,
                                                         std::optional<CD3DX12_CLEAR_VALUE> clearValue,
                                                         D3D12_SUBRESOURCE_DATA             initData)
    {
        RHIRenderSurfaceBaseDesc desc = {
            width, height, depth, sampleCount, numMips, flags, RHITexDim3D, format, true, false};
        return Create(Parent, desc, name, initState, clearValue, {initData});
    }

    std::shared_ptr<D3D12Texture> D3D12Texture::CreateFromSwapchain(D3D12LinkedDevice*                     Parent,
                                                                    Microsoft::WRL::ComPtr<ID3D12Resource> pResource,
                                                                    D3D12_RESOURCE_STATES                  initState,
                                                                    std::wstring                           name)
    {
        std::shared_ptr<D3D12Texture> pSurfaceD3D12 = std::make_shared<D3D12Texture>();
        pSurfaceD3D12->AssociateWithResource(Parent, name, pResource, initState);
        pSurfaceD3D12->SetResourceName(name);

        D3D12_RESOURCE_DESC resourceDesc = pResource->GetDesc();

        pSurfaceD3D12->m_Desc = {resourceDesc.Width,
                                 resourceDesc.Height,
                                 resourceDesc.DepthOrArraySize,
                                 resourceDesc.SampleDesc.Count,
                                 1,
                                 RHISurfaceCreateRenderTarget,
                                 RHITexDim2D,
                                 resourceDesc.Format,
                                 true,
                                 true};
        
        return pSurfaceD3D12;
    }

    bool D3D12Texture::InflateTexture(std::vector<D3D12_SUBRESOURCE_DATA> initDatas)
    {
        D3D12CommandContext::InitializeTexture(Parent, this, 0, initDatas);
        return true;
    }

    std::shared_ptr<D3D12ShaderResourceView> D3D12Texture::CreateSRV(D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc)
    {
        uint64 descHash = CityHash64((const char*)&srvDesc, sizeof(D3D12_SHADER_RESOURCE_VIEW_DESC));

        std::shared_ptr<D3D12ShaderResourceView> srv = nullptr;
        auto srvHandleIter = m_SRVHandleMap.find(descHash);
        if (srvHandleIter == m_SRVHandleMap.end())
        {
            srv = std::make_shared<D3D12ShaderResourceView>(Parent, srvDesc, this);
            m_SRVHandleMap[descHash] = srv;
        }
        else
        {
            srv = srvHandleIter->second;
        }
        return srv;
    }

    std::shared_ptr<D3D12UnorderedAccessView> D3D12Texture::CreateUAV(D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc)
    {
        uint64 descHash = CityHash64((const char*)&uavDesc, sizeof(D3D12_UNORDERED_ACCESS_VIEW_DESC));

        std::shared_ptr<D3D12UnorderedAccessView> uav = nullptr;
        auto uavHandleIter = m_UAVHandleMap.find(descHash);
        if (uavHandleIter == m_UAVHandleMap.end())
        {
            uav = std::make_shared<D3D12UnorderedAccessView>(
                Parent, uavDesc, (D3D12Resource*)this, (D3D12Resource*)nullptr);
            m_UAVHandleMap[descHash] = uav;
        }
        else
        {
            uav = uavHandleIter->second;
        }
        return uav;
    }

    std::shared_ptr<D3D12RenderTargetView> D3D12Texture::CreateRTV(D3D12_RENDER_TARGET_VIEW_DESC rtvDesc)
    {
        uint64 descHash = CityHash64((const char*)&rtvDesc, sizeof(D3D12_RENDER_TARGET_VIEW_DESC));

        std::shared_ptr<D3D12RenderTargetView> rtv = nullptr;
        auto rtvHandleIter = m_RTVHandleMap.find(descHash);
        if (rtvHandleIter == m_RTVHandleMap.end())
        {
            rtv = std::make_shared<D3D12RenderTargetView>(Parent, rtvDesc, this);
            m_RTVHandleMap[descHash] = rtv;
        }
        else
        {
            rtv = rtvHandleIter->second;
        }
        return rtv;
    }

    std::shared_ptr<D3D12DepthStencilView> D3D12Texture::CreateDSV(D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc)
    {
        uint64 descHash = CityHash64((const char*)&dsvDesc, sizeof(D3D12_DEPTH_STENCIL_VIEW_DESC));

        std::shared_ptr<D3D12DepthStencilView> dsv = nullptr;
        auto dsvHandleIter = m_DSVHandleMap.find(descHash);
        if (dsvHandleIter == m_DSVHandleMap.end())
        {
            dsv = std::make_shared<D3D12DepthStencilView>(Parent, dsvDesc, this);
            m_DSVHandleMap[descHash] = dsv;
        }
        else
        {
            dsv = dsvHandleIter->second;
        }
        return dsv;
    }

    std::shared_ptr<D3D12ShaderResourceView> D3D12Texture::GetDefaultSRV()
    {
        return CreateSRV(
            D3D12ShaderResourceView::GetDesc(this, m_Desc.flags & RHISurfaceCreateSRGB, 0, m_Desc.mipCount));
    }

    std::shared_ptr<D3D12UnorderedAccessView> D3D12Texture::GetDefaultUAV()
    {
        return CreateUAV(D3D12UnorderedAccessView::GetDesc(this, m_Desc.depthOrArray, m_Desc.mipCount));
    }

    std::shared_ptr<D3D12RenderTargetView> D3D12Texture::GetDefaultRTV()
    {
        ASSERT(m_Desc.flags & RHISurfaceCreateRenderTarget);
        return CreateRTV(D3D12RenderTargetView::GetDesc(this, m_Desc.flags & RHISurfaceCreateSRGB));
    }

    std::shared_ptr<D3D12DepthStencilView> D3D12Texture::GetDefaultDSV()
    {
        ASSERT(m_Desc.flags & (RHISurfaceCreateDepthStencil | RHISurfaceCreateShadowmap));
        return CreateDSV(D3D12DepthStencilView::GetDesc(this));
    }

    void D3D12Texture::ExportToFile(const std::wstring& FilePath)
    {

    }

    INT D3D12Texture::GetMipLevels(UINT width, UINT height, INT32 numMips, RHISurfaceCreateFlags flags)
    {
        INT mipLevels = MOYU_MIN(numMips, MAXMIPLEVELS);
        if (flags & RHISurfaceCreateMipmap)
        {
            if (flags & RHISurfaceCreateAutoGenMips)
            {
                mipLevels = D3D12RHIUtils::ComputeNumMips(width, height);
                mipLevels = MOYU_MIN(mipLevels, MAXMIPLEVELS);
            }
            else
            {
                if (numMips == -1)
                {
                    mipLevels = D3D12RHIUtils::ComputeNumMips(width, height);
                    mipLevels = MOYU_MIN(mipLevels, MAXMIPLEVELS);
                }
                else
                {
                    mipLevels = numMips;
                }
            }
        }
        return mipLevels;
    }

}

