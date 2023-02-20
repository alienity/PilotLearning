#pragma once
#include "d3d12_core.h"
#include "runtime/core/base/robin_hood.h"

namespace RHI
{
    class D3D12RenderTargetView;
    class D3D12DepthStencilView;
    class D3D12ConstantBufferView;
    class D3D12ShaderResourceView;
    class D3D12UnorderedAccessView;
    class D3D12CommandContext;

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
                      D3D12_RESOURCE_STATES                    InitialResourceState);
        D3D12Resource(D3D12LinkedDevice*                 Parent,
                      CD3DX12_HEAP_PROPERTIES            HeapProperties,
                      CD3DX12_RESOURCE_DESC              Desc,
                      D3D12_RESOURCE_STATES              InitialResourceState,
                      std::optional<CD3DX12_CLEAR_VALUE> ClearValue);
        D3D12Resource(D3D12LinkedDevice*                       Parent,
                      Microsoft::WRL::ComPtr<ID3D12Resource>&& Resource,
                      D3D12_RESOURCE_STATES                    InitialResourceState,
                      std::optional<CD3DX12_CLEAR_VALUE>       ClearValue,
                      std::wstring                             Name);
        D3D12Resource(D3D12LinkedDevice*                 Parent,
                      CD3DX12_HEAP_PROPERTIES            HeapProperties,
                      CD3DX12_RESOURCE_DESC              Desc,
                      D3D12_RESOURCE_STATES              InitialResourceState,
                      std::optional<CD3DX12_CLEAR_VALUE> ClearValue,
                      std::wstring                       Name);

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

        UINT   GetVersionID() const { return m_VersionID; }
        UINT64 GetUniqueId() const { return g_GlobalUniqueId; }

        void SetResourceName(std::wstring name);

        [[nodiscard]] const CD3DX12_CLEAR_VALUE GetClearValue() const noexcept;
        [[nodiscard]] inline void  GetClearColor(FLOAT color[4]) const;
        [[nodiscard]] inline FLOAT GetClearDepth() const { return GetClearValue().DepthStencil.Depth; }
        [[nodiscard]] inline UINT8 GetClearStencil() const { return GetClearValue().DepthStencil.Stencil; }

        [[nodiscard]] inline const CD3DX12_RESOURCE_DESC& GetDesc() const noexcept { return m_ResourceDesc; }
        [[nodiscard]] inline D3D12_RESOURCE_DIMENSION     GetDimension() const { return m_ResourceDesc.Dimension; }
        [[nodiscard]] inline UINT64                       GetWidth() const { return m_ResourceDesc.Width; }
        [[nodiscard]] inline UINT64                       GetHeight() const { return m_ResourceDesc.Height; }
        [[nodiscard]] inline UINT16                       GetArraySize() const { return m_ResourceDesc.ArraySize(); }
        [[nodiscard]] inline UINT16                       GetDepth() const { return m_ResourceDesc.Depth(); }
        [[nodiscard]] inline UINT16                       GetMipLevels() const { return m_ResourceDesc.MipLevels; }
        [[nodiscard]] inline DXGI_FORMAT                  GetFormat() const { return m_ResourceDesc.Format; }

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

    protected:
        Microsoft::WRL::ComPtr<ID3D12Resource> InitializeResource(CD3DX12_HEAP_PROPERTIES HeapProperties,
                                                                  CD3DX12_RESOURCE_DESC Desc,
                                                                  D3D12_RESOURCE_STATES InitialResourceState,
                                                                  std::optional<CD3DX12_CLEAR_VALUE> ClearValue) const;

        UINT CalculateNumSubresources() const;

    protected:
        // TODO: Add support for custom heap properties for UMA GPUs

        Microsoft::WRL::ComPtr<ID3D12Resource> m_pResource;
        std::optional<CD3DX12_CLEAR_VALUE>     m_ClearValue;
        CD3DX12_RESOURCE_DESC                  m_ResourceDesc    = {};
        UINT8                                  m_PlaneCount      = 0;
        UINT                                   m_NumSubresources = 0;
        CResourceState                         m_ResourceState;
        std::wstring                           m_ResourceName;

        static UINT64                          g_GlobalUniqueId;

        // Used to identify when a resource changes so descriptors can be copied etc.
        UINT m_VersionID = 0;
    };

    class D3D12ASBuffer : public D3D12Resource
    {
    public:
        D3D12ASBuffer() noexcept = default;
        D3D12ASBuffer(D3D12LinkedDevice* Parent, UINT64 SizeInBytes);

        D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress() const;

    private:
        D3D12_GPU_VIRTUAL_ADDRESS m_GpuVirtualAddress;
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

        ~D3D12Buffer() { Destroy(); };

        virtual void Destroy() override;

        [[nodiscard]] D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress(UINT Index = 0) const;
        [[nodiscard]] UINT                      GetStride() const { return m_Stride; }
        [[nodiscard]] UINT                      GetSizeInBytes() const { return m_SizeInBytes; }
        template<typename T>
        [[nodiscard]] T* GetCpuVirtualAddress() const
        {
            ASSERT(m_CpuVirtualAddress && "Invalid CpuVirtualAddress");
            return reinterpret_cast<T*>(m_CpuVirtualAddress);
        }

        BYTE* GetCpuVirtualAddress() const
        {
            ASSERT(m_CpuVirtualAddress && "Invalid CpuVirtualAddress");
            return m_CpuVirtualAddress;
        }

        [[nodiscard]] D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const noexcept
        {
            D3D12_VERTEX_BUFFER_VIEW VertexBufferView = {};
            VertexBufferView.BufferLocation           = m_pResource->GetGPUVirtualAddress();
            VertexBufferView.SizeInBytes              = static_cast<UINT>(m_ResourceDesc.Width);
            VertexBufferView.StrideInBytes            = m_Stride;
            return VertexBufferView;
        }

        [[nodiscard]] D3D12_INDEX_BUFFER_VIEW
        GetIndexBufferView(DXGI_FORMAT Format = DXGI_FORMAT_R32_UINT) const noexcept
        {
            D3D12_INDEX_BUFFER_VIEW IndexBufferView = {};
            IndexBufferView.BufferLocation          = m_pResource->GetGPUVirtualAddress();
            IndexBufferView.SizeInBytes             = static_cast<UINT>(m_ResourceDesc.Width);
            IndexBufferView.Format                  = Format;
            return IndexBufferView;
        }

        template<typename T>
        void CopyData(UINT Index, const T& Data)
        {
            assert(m_CpuVirtualAddress && "Invalid CpuVirtualAddress");
            memcpy(&m_CpuVirtualAddress[Index * m_Stride], &Data, sizeof(T));
        }

    public:
        static std::shared_ptr<D3D12Buffer> Create(D3D12LinkedDevice* Parent,
                                                   RHIBufferTarget    bufferTarget,
                                                   UINT32             numElements,
                                                   UINT32             elementSize,
                                                   const std::wstring name     = L"Buffer",
                                                   RHIBufferMode mapplableMode = RHIBufferMode::RHIBufferModeImmutable,
                                                   D3D12_RESOURCE_STATES initState = D3D12_RESOURCE_STATE_GENERIC_READ,
                                                   BYTE*                 initialData = nullptr,
                                                   UINT                  dataLen     = 0);

        std::shared_ptr<D3D12Buffer> GetCounterBuffer();

        bool InflateBuffer(BYTE* initialData, UINT dataLen);
        void ResetCounterBuffer(D3D12CommandContext* pCommandContext);

        std::shared_ptr<D3D12ConstantBufferView>  CreateCBV(D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc);
        std::shared_ptr<D3D12ShaderResourceView>  CreateSRV(D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc);
        std::shared_ptr<D3D12UnorderedAccessView> CreateUAV(D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc);
        std::shared_ptr<D3D12UnorderedAccessView> CreateUAV(D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc, D3D12Resource* pCounterRes);
        
        std::shared_ptr<D3D12ConstantBufferView>  GetDefaultCBV();
        std::shared_ptr<D3D12ShaderResourceView>  GetDefaultSRV();
        std::shared_ptr<D3D12UnorderedAccessView> GetDefaultUAV();

        std::shared_ptr<D3D12UnorderedAccessView> GetDefaultStructureUAV(bool hasCounter = false);
        std::shared_ptr<D3D12UnorderedAccessView> GetDefaultRawUAV(bool hasCounter = false);

        RHIBufferDesc& GetBufferDesc();

    protected:
        RHIBufferDesc m_Desc;
        RHIBufferData m_Data;
        
        std::shared_ptr<D3D12Buffer> p_CounterBufferD3D12;

        struct BUFFER_UNORDERED_ACCESS_VIEW_KEY { D3D12_UNORDERED_ACCESS_VIEW_DESC Desc; D3D12Resource* PCounterResource; };

        robin_hood::unordered_map<UINT64, std::shared_ptr<D3D12ConstantBufferView>>    m_CBVHandleMap;
        robin_hood::unordered_map<UINT64, std::shared_ptr<D3D12ShaderResourceView>>    m_SRVHandleMap;
        robin_hood::unordered_map<UINT64, std::shared_ptr<D3D12UnorderedAccessView>>   m_UAVHandleMap;

    private:
        D3D12_HEAP_TYPE           m_HeapType    = {};
        UINT                      m_SizeInBytes = 0;
        UINT                      m_Stride      = 0;
        BYTE*                     m_CpuVirtualAddress;
        D3D12_GPU_VIRTUAL_ADDRESS m_GpuVirtualAddress;
        D3D12ScopedPointer        m_ScopedPointer; // Upload heap
    };

    class D3D12Texture : public D3D12Resource
    {
    public:
        D3D12Texture() noexcept = default;
        D3D12Texture(D3D12LinkedDevice*                       Parent,
                     Microsoft::WRL::ComPtr<ID3D12Resource>&& Resource,
                     D3D12_RESOURCE_STATES                    InitialResourceState);
        D3D12Texture(D3D12LinkedDevice*                 Parent,
                     const CD3DX12_RESOURCE_DESC&       Desc,
                     const CD3DX12_HEAP_PROPERTIES&     HeapProperty,
                     std::optional<CD3DX12_CLEAR_VALUE> ClearValue,
                     D3D12_RESOURCE_STATES              InitialResourceState,
                     bool                               Cubemap = false);
        D3D12Texture(D3D12LinkedDevice*                 Parent,
                     const CD3DX12_RESOURCE_DESC&       Desc,
                     std::optional<CD3DX12_CLEAR_VALUE> ClearValue,
                     D3D12_RESOURCE_STATES              InitialResourceState,
                     bool                               Cubemap = false);
        D3D12Texture(D3D12LinkedDevice*                 Parent,
                     const CD3DX12_RESOURCE_DESC&       Desc,
                     std::optional<CD3DX12_CLEAR_VALUE> ClearValue,
                     bool                               Cubemap = false);

        ~D3D12Texture() { Destroy(); };

        virtual void Destroy() override;

        [[nodiscard]] UINT GetSubresourceIndex(std::optional<UINT> OptArraySlice = std::nullopt,
                                               std::optional<UINT> OptMipSlice   = std::nullopt,
                                               std::optional<UINT> OptPlaneSlice = std::nullopt) const noexcept;

        [[nodiscard]] bool IsCubemap() const noexcept { return m_IsCubemap; }

    public:

        void AssociateWithResource(D3D12LinkedDevice*                     Parent,
                                   const std::wstring&                    Name,
                                   Microsoft::WRL::ComPtr<ID3D12Resource> Resource,
                                   D3D12_RESOURCE_STATES                  CurrentState);

        static void GenerateMipMaps(D3D12CommandContext* context, D3D12Texture* pTexture);

    public:
        static std::shared_ptr<D3D12Texture> Create(D3D12LinkedDevice*       Parent,
                                                    RHIRenderSurfaceBaseDesc desc,
                                                    const std::wstring       name      = L"Texture",
                                                    D3D12_RESOURCE_STATES    initState = D3D12_RESOURCE_STATE_COMMON,
                                                    std::optional<CD3DX12_CLEAR_VALUE>  clearValue = std::nullopt,
                                                    std::vector<D3D12_SUBRESOURCE_DATA> initDatas  = {});

        static std::shared_ptr<D3D12Texture> Create2D(D3D12LinkedDevice*     Parent,
                                                      UINT32                 width,
                                                      UINT32                 height,
                                                      INT32                  numMips,
                                                      DXGI_FORMAT            format,
                                                      RHISurfaceCreateFlags  flags,
                                                      UINT32                 sampleCount = 1,
                                                      const std::wstring     name        = L"Texture2D",
                                                      D3D12_RESOURCE_STATES  initState   = D3D12_RESOURCE_STATE_COMMON,
                                                      std::optional<CD3DX12_CLEAR_VALUE> clearValue = std::nullopt,
                                                      D3D12_SUBRESOURCE_DATA initData    = {});

        static std::shared_ptr<D3D12Texture>
        Create2DArray(D3D12LinkedDevice*                  Parent,
                      UINT32                              width,
                      UINT32                              height,
                      UINT32                              arraySize,
                      INT32                               numMips,
                      DXGI_FORMAT                         format,
                      RHISurfaceCreateFlags               flags,
                      UINT32                              sampleCount = 1,
                      const std::wstring                  name        = L"Texture2DArray",
                      D3D12_RESOURCE_STATES               initState   = D3D12_RESOURCE_STATE_COMMON,
                      std::optional<CD3DX12_CLEAR_VALUE>  clearValue  = std::nullopt,
                      std::vector<D3D12_SUBRESOURCE_DATA> initDatas   = {});

        static std::shared_ptr<D3D12Texture>
        CreateCubeMap(D3D12LinkedDevice*                  Parent,
                      UINT32                              width,
                      UINT32                              height,
                      INT32                               numMips,
                      DXGI_FORMAT                         format,
                      RHISurfaceCreateFlags               flags,
                      UINT32                              sampleCount = 1,
                      const std::wstring                  name        = L"TextureCubeMap",
                      D3D12_RESOURCE_STATES               initState   = D3D12_RESOURCE_STATE_COMMON,
                      std::optional<CD3DX12_CLEAR_VALUE>  clearValue  = std::nullopt,
                      std::vector<D3D12_SUBRESOURCE_DATA> initDatas   = {});

        static std::shared_ptr<D3D12Texture>
        CreateCubeMapArray(D3D12LinkedDevice*                  Parent,
                           UINT32                              width,
                           UINT32                              height,
                           UINT32                              arraySize,
                           INT32                               numMips,
                           DXGI_FORMAT                         format,
                           RHISurfaceCreateFlags               flags,
                           UINT32                              sampleCount = 1,
                           const std::wstring                  name        = L"TextureCubeMapArray",
                           D3D12_RESOURCE_STATES               initState   = D3D12_RESOURCE_STATE_COMMON,
                           std::optional<CD3DX12_CLEAR_VALUE>  clearValue  = std::nullopt,
                           std::vector<D3D12_SUBRESOURCE_DATA> initDatas   = {});

        static std::shared_ptr<D3D12Texture> Create3D(D3D12LinkedDevice*     Parent,
                                                      UINT32                 width,
                                                      UINT32                 height,
                                                      UINT32                 depth,
                                                      INT32                  numMips,
                                                      DXGI_FORMAT            format,
                                                      RHISurfaceCreateFlags  flags,
                                                      UINT32                 sampleCount = 1,
                                                      const std::wstring     name        = L"Texture3D",
                                                      D3D12_RESOURCE_STATES  initState   = D3D12_RESOURCE_STATE_COMMON,
                                                      std::optional<CD3DX12_CLEAR_VALUE> clearValue = std::nullopt,
                                                      D3D12_SUBRESOURCE_DATA initData    = {});

        static std::shared_ptr<D3D12Texture>
        CreateFromSwapchain(D3D12LinkedDevice*                     Parent,
                            Microsoft::WRL::ComPtr<ID3D12Resource> pResource,
                            D3D12_RESOURCE_STATES                  initState = D3D12_RESOURCE_STATE_COMMON,
                            std::wstring                           name      = L"Backbuffer");

        bool InflateTexture(std::vector<D3D12_SUBRESOURCE_DATA> initDatas);

        std::shared_ptr<D3D12ShaderResourceView>  CreateSRV(D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc);
        std::shared_ptr<D3D12UnorderedAccessView> CreateUAV(D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc);
        std::shared_ptr<D3D12RenderTargetView>    CreateRTV(D3D12_RENDER_TARGET_VIEW_DESC rtvDesc);
        std::shared_ptr<D3D12DepthStencilView>    CreateDSV(D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc);

        std::shared_ptr<D3D12ShaderResourceView>  GetDefaultSRV();
        std::shared_ptr<D3D12UnorderedAccessView> GetDefaultUAV();
        std::shared_ptr<D3D12RenderTargetView>    GetDefaultRTV();
        std::shared_ptr<D3D12DepthStencilView>    GetDefaultDSV();

        // Write the raw pixel buffer contents to a file
        // Note that data is preceded by a 16-byte header:  { DXGI_FORMAT, Pitch (in pixels), Width (in pixels), Height
        // }
        void ExportToFile(const std::wstring& FilePath);

    protected:
        static INT GetMipLevels(UINT width, UINT height, INT32 numMips, RHISurfaceCreateFlags flags);

    protected:
        RHIRenderSurfaceBaseDesc m_Desc;
        
        std::unordered_map<UINT64, std::shared_ptr<D3D12RenderTargetView>>      m_RTVHandleMap;
        std::unordered_map<UINT64, std::shared_ptr<D3D12DepthStencilView>>      m_DSVHandleMap;
        std::unordered_map<UINT64, std::shared_ptr<D3D12ShaderResourceView>>    m_SRVHandleMap;
        std::unordered_map<UINT64, std::shared_ptr<D3D12UnorderedAccessView>>   m_UAVHandleMap;

    protected:
        bool m_IsCubemap = false;
    };

}
