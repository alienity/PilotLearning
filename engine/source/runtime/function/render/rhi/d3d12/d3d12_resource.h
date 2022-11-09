#pragma once
#include "d3d12_core.h"
#include "runtime/core/base/robin_hood.h"

namespace RHI
{
    class D3D12RenderTargetView;
    class D3D12DepthStencilView;
    class D3D12ShaderResourceView;
    class D3D12UnorderedAccessView;

    // https://microsoft.github.io/DirectX-Specs/d3d/CPUEfficiency.html#subresource-state-tracking
    class CResourceState
    {
    public:
        enum class ETrackingMode
        {
            PerResource,
            PerSubresource
        };

        CResourceState() noexcept = default;
        explicit CResourceState(std::uint32_t NumSubresources, D3D12_RESOURCE_STATES InitialResourceState);

        [[nodiscard]] auto begin() const noexcept { return SubresourceStates.begin(); }
        [[nodiscard]] auto end() const noexcept { return SubresourceStates.end(); }

        [[nodiscard]] bool IsUninitialized() const noexcept
        {
            return ResourceState == D3D12_RESOURCE_STATE_UNINITIALIZED;
        }
        [[nodiscard]] bool IsUnknown() const noexcept { return ResourceState == D3D12_RESOURCE_STATE_UNKNOWN; }

        // Returns true if all subresources have the same state
        [[nodiscard]] bool IsUniform() const noexcept { return TrackingMode == ETrackingMode::PerResource; }

        [[nodiscard]] D3D12_RESOURCE_STATES GetSubresourceState(std::uint32_t Subresource) const;

        void SetSubresourceState(std::uint32_t Subresource, D3D12_RESOURCE_STATES State);

    private:
        ETrackingMode                      TrackingMode  = ETrackingMode::PerResource;
        D3D12_RESOURCE_STATES              ResourceState = D3D12_RESOURCE_STATE_UNINITIALIZED;
        std::vector<D3D12_RESOURCE_STATES> SubresourceStates;
    };

    class D3D12Resource : public D3D12LinkedDeviceChild
    {
    public:
        D3D12Resource() noexcept = default;
        D3D12Resource(D3D12LinkedDevice*                       Parent,
                      Microsoft::WRL::ComPtr<ID3D12Resource>&& Resource,
                      CD3DX12_CLEAR_VALUE                      ClearValue,
                      D3D12_RESOURCE_STATES                    InitialResourceState);
        D3D12Resource(D3D12LinkedDevice*                       Parent,
                      Microsoft::WRL::ComPtr<ID3D12Resource>&& Resource,
                      CD3DX12_CLEAR_VALUE                      ClearValue,
                      D3D12_RESOURCE_STATES                    InitialResourceState,
                      std::string                              Name);
        D3D12Resource(D3D12LinkedDevice*                 Parent,
                      CD3DX12_HEAP_PROPERTIES            HeapProperties,
                      CD3DX12_RESOURCE_DESC              Desc,
                      D3D12_RESOURCE_STATES              InitialResourceState,
                      std::optional<CD3DX12_CLEAR_VALUE> ClearValue);
        D3D12Resource(D3D12LinkedDevice*                 Parent,
                      CD3DX12_HEAP_PROPERTIES            HeapProperties,
                      CD3DX12_RESOURCE_DESC              Desc,
                      D3D12_RESOURCE_STATES              InitialResourceState,
                      std::optional<CD3DX12_CLEAR_VALUE> ClearValue,
                      std::string                        Name);

        ~D3D12Resource() { Destroy(); };

        virtual void Destroy();

        D3D12Resource(D3D12Resource&&) noexcept = default;
        D3D12Resource& operator=(D3D12Resource&&) noexcept = default;

        D3D12Resource(const D3D12Resource&) = delete;
        D3D12Resource& operator=(const D3D12Resource&) = delete;

        ID3D12Resource*       operator->() { return m_pResource.Get(); }
        const ID3D12Resource* operator->() const { return m_pResource.Get(); }

        ID3D12Resource*       GetResource() { return m_pResource.Get(); }
        const ID3D12Resource* GetResource() const { return m_pResource.Get(); }

        ID3D12Resource** GetAddressOf() { return m_pResource.GetAddressOf(); }

        [[nodiscard]] D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress() const;

        uint32_t GetVersionID() const { return m_VersionID; }

        void SetResourceName(std::string name);

        [[nodiscard]] D3D12_CLEAR_VALUE GetClearValue() const noexcept
        {
            return m_ClearValue.has_value() ? *m_ClearValue : D3D12_CLEAR_VALUE {};
        }

        [[nodiscard]] inline void GetClearColor(FLOAT color[4]) const
        {
            D3D12_CLEAR_VALUE clear_val = GetClearValue();
            memcpy(color, clear_val.Color, sizeof(clear_val.Color));
        }
        [[nodiscard]] inline FLOAT GetClearDepth() const { return GetClearValue().DepthStencil.Depth; }
        [[nodiscard]] inline UINT8 GetClearStencil() const { return GetClearValue().DepthStencil.Stencil; }

        [[nodiscard]] inline const D3D12_RESOURCE_DESC& GetDesc() const noexcept { return m_Desc; }
        [[nodiscard]] inline D3D12_RESOURCE_DIMENSION   GetDimension() const { return m_Desc.Dimension; }
        [[nodiscard]] inline UINT64                     GetWidth() const { return m_Desc.Width; }
        [[nodiscard]] inline UINT64                     GetHeight() const { return m_Desc.Height; }
        [[nodiscard]] inline UINT16                     GetArraySize() const { return m_Desc.ArraySize(); }
        [[nodiscard]] inline UINT16                     GetDepth() const { return m_Desc.Depth(); }
        [[nodiscard]] inline UINT16                     GetMipLevels() const { return m_Desc.MipLevels; }
        [[nodiscard]] inline DXGI_FORMAT                GetFormat() const { return m_Desc.Format; }

        [[nodiscard]] inline UINT8           GetPlaneCount() const noexcept { return m_PlaneCount; }
        [[nodiscard]] inline UINT            GetNumSubresources() const noexcept { return m_NumSubresources; }
        [[nodiscard]] inline CResourceState& GetResourceState() { return m_ResourceState; }

        // https://docs.microsoft.com/en-us/windows/win32/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12#implicit-state-transitions
        // https://devblogs.microsoft.com/directx/a-look-inside-d3d12-resource-state-barriers/
        // Can this resource be promoted to State from common
        [[nodiscard]] bool ImplicitStatePromotion(D3D12_RESOURCE_STATES State) const noexcept;

        // Can this resource decay back to common
        // 4 Cases:
        // 1. Resources being accessed on a Copy queue, or
        // 2. Buffer resources on any queue type, or
        // 3. Texture resources on any queue type that have the D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS flag set,
        // or
        // 4. Any resource implicitly promoted to a read-only state.
        [[nodiscard]] bool ImplicitStateDecay(D3D12_RESOURCE_STATES   State,
                                              D3D12_COMMAND_LIST_TYPE AccessedQueueType) const noexcept;

        const std::shared_ptr<D3D12ShaderResourceView>  CreateSRV(D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc);
        const std::shared_ptr<D3D12UnorderedAccessView> CreateUAV(D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc,
                                                                  D3D12Resource* pCounterResource = nullptr);
        const std::shared_ptr<D3D12RenderTargetView>    CreateRTV(D3D12_RENDER_TARGET_VIEW_DESC rtvDesc);
        const std::shared_ptr<D3D12DepthStencilView>    CreateDSV(D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc);

    private:
        Microsoft::WRL::ComPtr<ID3D12Resource> InitializeResource(CD3DX12_HEAP_PROPERTIES HeapProperties,
                                                                  CD3DX12_RESOURCE_DESC Desc,
                                                                  D3D12_RESOURCE_STATES InitialResourceState,
                                                                  std::optional<CD3DX12_CLEAR_VALUE> ClearValue) const;

        UINT CalculateNumSubresources() const;

    protected:
        // TODO: Add support for custom heap properties for UMA GPUs

        Microsoft::WRL::ComPtr<ID3D12Resource> m_pResource;
        D3D12_GPU_VIRTUAL_ADDRESS              m_GpuVirtualAddress;
        std::optional<CD3DX12_CLEAR_VALUE>     m_ClearValue;
        CD3DX12_RESOURCE_DESC                  m_Desc            = {};
        UINT8                                  m_PlaneCount      = 0;
        UINT                                   m_NumSubresources = 0;
        CResourceState                         m_ResourceState;
        std::string                            m_ResourceName;

    protected:
        // clang-format off
        robin_hood::unordered_map<D3D12_RENDER_TARGET_VIEW_DESC, std::shared_ptr<D3D12RenderTargetView>> m_RTVHandleMap;
        robin_hood::unordered_map<D3D12_DEPTH_STENCIL_VIEW_DESC, std::shared_ptr<D3D12DepthStencilView>> m_DSVHandleMap;
        robin_hood::unordered_map<D3D12_SHADER_RESOURCE_VIEW_DESC, std::shared_ptr<D3D12ShaderResourceView>> m_SRVHandleMap;
        struct UnorderedAccessViewKey { D3D12_UNORDERED_ACCESS_VIEW_DESC desc; D3D12Resource* pCounterResource; };
        robin_hood::unordered_map<UnorderedAccessViewKey, std::shared_ptr<D3D12UnorderedAccessView>> m_UAVHandleMap;
        // clang-format on

        // Used to identify when a resource changes so descriptors can be copied etc.
        uint32_t m_VersionID = 0;
    };

    class D3D12ASBuffer : public D3D12Resource
    {
    public:
        D3D12ASBuffer() noexcept = default;
        D3D12ASBuffer(D3D12LinkedDevice* Parent, UINT64 SizeInBytes);
    };

    class D3D12Buffer : public D3D12Resource
    {
    public:
        D3D12Buffer() noexcept = default;
        D3D12Buffer(D3D12LinkedDevice*   Parent,
                    UINT64               SizeInBytes,
                    UINT                 Stride,
                    D3D12_HEAP_TYPE      HeapType,
                    D3D12_RESOURCE_FLAGS ResourceFlags);
        D3D12Buffer(D3D12LinkedDevice*    Parent,
                    UINT64                SizeInBytes,
                    UINT                  Stride,
                    D3D12_HEAP_TYPE       HeapType,
                    D3D12_RESOURCE_FLAGS  ResourceFlags,
                    D3D12_RESOURCE_STATES InitialResourceState);

        [[nodiscard]] D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress(UINT Index) const;
        [[nodiscard]] UINT                      GetStride() const { return m_Stride; }
        [[nodiscard]] UINT                      GetSizeInBytes() const { return m_SizeInBytes; }
        template<typename T>
        [[nodiscard]] T* GetCpuVirtualAddress() const
        {
            assert(m_CpuVirtualAddress && "Invalid CpuVirtualAddress");
            return reinterpret_cast<T*>(m_CpuVirtualAddress);
        }

        [[nodiscard]] D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const noexcept
        {
            D3D12_VERTEX_BUFFER_VIEW VertexBufferView = {};
            VertexBufferView.BufferLocation           = m_pResource->GetGPUVirtualAddress();
            VertexBufferView.SizeInBytes              = static_cast<UINT>(m_Desc.Width);
            VertexBufferView.StrideInBytes            = m_Stride;
            return VertexBufferView;
        }

        [[nodiscard]] D3D12_INDEX_BUFFER_VIEW
        GetIndexBufferView(DXGI_FORMAT Format = DXGI_FORMAT_R32_UINT) const noexcept
        {
            D3D12_INDEX_BUFFER_VIEW IndexBufferView = {};
            IndexBufferView.BufferLocation          = m_pResource->GetGPUVirtualAddress();
            IndexBufferView.SizeInBytes             = static_cast<UINT>(m_Desc.Width);
            IndexBufferView.Format                  = Format;
            return IndexBufferView;
        }

        template<typename T>
        void CopyData(UINT Index, const T& Data)
        {
            assert(m_CpuVirtualAddress && "Invalid CpuVirtualAddress");
            memcpy(&m_CpuVirtualAddress[Index * Stride], &Data, sizeof(T));
        }

        const std::shared_ptr<D3D12ShaderResourceView> GetStructuredBufferSRV(UINT FirstElement, UINT NumElements);
        const std::shared_ptr<D3D12UnorderedAccessView>
        GetStructuredBufferUAV(UINT FirstElement, UINT NumElements, UINT64 CounterOffsetInBytes);

        const std::shared_ptr<D3D12ShaderResourceView>  GetByteAddressBufferSRV();
        const std::shared_ptr<D3D12UnorderedAccessView> GetByteAddressBufferUAV();

    private:
        D3D12_HEAP_TYPE    m_HeapType  = {};
        UINT               m_SizeInBytes = 0;
        UINT               m_Stride      = 0;
        D3D12ScopedPointer m_ScopedPointer; // Upload heap
        BYTE*              m_CpuVirtualAddress = nullptr;
    };

    class D3D12Texture : public D3D12Resource
    {
    public:
        D3D12Texture() noexcept = default;
        D3D12Texture(D3D12LinkedDevice*                       Parent,
                     Microsoft::WRL::ComPtr<ID3D12Resource>&& Resource,
                     CD3DX12_CLEAR_VALUE                      ClearValue,
                     D3D12_RESOURCE_STATES                    InitialResourceState);
        D3D12Texture(D3D12LinkedDevice*                 Parent,
                     const CD3DX12_RESOURCE_DESC&       Desc,
                     std::optional<CD3DX12_CLEAR_VALUE> ClearValue = std::nullopt,
                     bool                               Cubemap    = false);

        const std::shared_ptr<D3D12RenderTargetView> CreateRTV(D3D12_RENDER_TARGET_VIEW_DESC rtvDesc);
        const std::shared_ptr<D3D12DepthStencilView> CreateDSV(D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc);

        [[nodiscard]] UINT GetSubresourceIndex(std::optional<UINT> OptArraySlice = std::nullopt,
                                               std::optional<UINT> OptMipSlice   = std::nullopt,
                                               std::optional<UINT> OptPlaneSlice = std::nullopt) const noexcept;

        [[nodiscard]] bool IsCubemap() const noexcept { return Cubemap; }

        const std::shared_ptr<D3D12ShaderResourceView>  GetDefaultSRV();
        const std::shared_ptr<D3D12UnorderedAccessView> GetDefaultUAV();
        const std::shared_ptr<D3D12RenderTargetView>    GetDefaultRTV();
        const std::shared_ptr<D3D12DepthStencilView>    GetDefaultDSV();

        // Write the raw pixel buffer contents to a file
        // Note that data is preceded by a 16-byte header:  { DXGI_FORMAT, Pitch (in pixels), Width (in pixels), Height }
        void ExportToFile(const std::wstring& FilePath);

    private:
        bool Cubemap = false;
    };

    /**
    // Description:  An upload buffer is visible to both the CPU and the GPU, but
    // because the memory is write combined, you should avoid reading data with the CPU.
    // An upload buffer is intended for moving data to a default GPU buffer.  You can
    // read from a file directly into an upload buffer, rather than reading into regular
    // cached memory, copying that to an upload buffer, and copying that to the GPU.
    */
    class UploadBuffer : public D3D12Buffer
    {
    public:
        UploadBuffer(D3D12LinkedDevice* Parent, const std::wstring& name, size_t BufferSize);

        virtual ~UploadBuffer() { Destroy(); }

        size_t GetBufferSize() const;
    };

}