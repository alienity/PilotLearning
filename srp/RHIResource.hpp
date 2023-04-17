#include "RHICore.hpp"

namespace RHI
{
    class RHIRenderTargetView;
    class RHIDepthStencilView;
    class RHIConstantBufferView;
    class RHIShaderResourceView;
    class RHIUnorderedAccessView;

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
        explicit CResourceState(UINT NumSubresources, RHI_RESOURCE_STATES InitialResourceState);

        [[nodiscard]] auto begin() const noexcept { return SubresourceStates.begin(); }
        [[nodiscard]] auto end() const noexcept { return SubresourceStates.end(); }

        [[nodiscard]] bool IsUninitialized() const noexcept { return ResourceState == RHI_RESOURCE_STATE_UNINITIALIZED; }
        [[nodiscard]] bool IsUnknown() const noexcept { return ResourceState == RHI_RESOURCE_STATE_UNKNOWN; }

        // Returns true if all subresources have the same state
        [[nodiscard]] bool IsUniform() const noexcept { return TrackingMode == ETrackingMode::PerResource; }

        [[nodiscard]] RHI_RESOURCE_STATES GetSubresourceState(UINT Subresource) const;

        void SetSubresourceState(UINT Subresource, RHI_RESOURCE_STATES State);

    private:
        ETrackingMode                    TrackingMode  = ETrackingMode::PerResource;
        RHI_RESOURCE_STATES              ResourceState = RHI_RESOURCE_STATE_UNINITIALIZED;
        std::vector<RHI_RESOURCE_STATES> SubresourceStates;
    };

    class RHIResource
    {
    public:
        RHIResource() noexcept = default;
        ~RHIResource() { Destroy(); };

        virtual void Destroy();

        RHIResource(RHIResource&&) noexcept = default;
        RHIResource& operator=(RHIResource&&) noexcept = default;

        RHIResource(const RHIResource&) = delete;
        RHIResource& operator=(const RHIResource&) = delete;

        UINT64 GetUniqueId() const { return g_GlobalUniqueId; }

        void SetResourceName(std::wstring name);

        [[nodiscard]] const RHI_CLEAR_VALUE GetClearValue() const noexcept;
        [[nodiscard]] inline void  GetClearColor(FLOAT color[4]) const;
        [[nodiscard]] inline FLOAT GetClearDepth() const { return GetClearValue().DepthStencil.Depth; }
        [[nodiscard]] inline UINT8 GetClearStencil() const { return GetClearValue().DepthStencil.Stencil; }

        [[nodiscard]] inline const RHI_RESOURCE_DESC& GetDesc() const noexcept { return m_ResourceDesc; }
        [[nodiscard]] inline RHI_RESOURCE_DIMENSION     GetDimension() const { return m_ResourceDesc.Dimension; }
        [[nodiscard]] inline UINT64                       GetWidth() const { return m_ResourceDesc.Width; }
        [[nodiscard]] inline UINT64                       GetHeight() const { return m_ResourceDesc.Height; }
        [[nodiscard]] inline UINT16                       GetArraySize() const { return m_ResourceDesc.ArraySize(); }
        [[nodiscard]] inline UINT16                       GetDepth() const { return m_ResourceDesc.Depth(); }
        [[nodiscard]] inline UINT16                       GetMipLevels() const { return m_ResourceDesc.MipLevels; }
        [[nodiscard]] inline RHI_FORMAT                  GetFormat() const { return m_ResourceDesc.Format; }

        [[nodiscard]] inline UINT8           GetPlaneCount() const noexcept { return m_PlaneCount; }
        [[nodiscard]] inline UINT            GetNumSubresources() const noexcept { return m_NumSubresources; }
        [[nodiscard]] inline CResourceState& GetResourceState() { return m_ResourceState; }

        // https://docs.microsoft.com/en-us/windows/win32/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12#implicit-state-transitions
        // https://devblogs.microsoft.com/directx/a-look-inside-d3d12-resource-state-barriers/
        // Can this resource be promoted to State from common
        [[nodiscard]] bool ImplicitStatePromotion(RHI_RESOURCE_STATES State) const noexcept;

        // Can this resource decay back to common
        // 4 Cases:
        // 1. Resources being accessed on a Copy queue, or
        // 2. Buffer resources on any queue type, or
        // 3. Texture resources on any queue type that have the RHI_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS flag set,
        // or
        // 4. Any resource implicitly promoted to a read-only state.
        [[nodiscard]] bool ImplicitStateDecay(RHI_RESOURCE_STATES State, RHI_COMMAND_LIST_TYPE AccessedQueueType) const noexcept;
    };

    class RHIBuffer : public RHIResource
    {
    public:
        RHIBuffer() noexcept = default;
        RHIBuffer(UINT64               SizeInBytes,
                    UINT                 Stride,
                    RHI_HEAP_TYPE      HeapType,
                    RHI_RESOURCE_FLAGS ResourceFlags);
        RHIBuffer(UINT64                SizeInBytes,
                    UINT                  Stride,
                    RHI_HEAP_TYPE       HeapType,
                    RHI_RESOURCE_FLAGS  ResourceFlags,
                    RHI_RESOURCE_STATES InitialResourceState);

        ~RHIBuffer() { Destroy(); };

        virtual void Destroy() override;

        [[nodiscard]] RHI_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress(UINT Index = 0) const;
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

        [[nodiscard]] RHI_VERTEX_BUFFER_VIEW GetVertexBufferView() const noexcept
        {
            RHI_VERTEX_BUFFER_VIEW VertexBufferView = {};
            VertexBufferView.BufferLocation           = m_pResource->GetGPUVirtualAddress();
            VertexBufferView.SizeInBytes              = static_cast<UINT>(m_ResourceDesc.Width);
            VertexBufferView.StrideInBytes            = m_Stride;
            return VertexBufferView;
        }

        [[nodiscard]] RHI_INDEX_BUFFER_VIEW
        GetIndexBufferView(DXGI_FORMAT Format = DXGI_FORMAT_R32_UINT) const noexcept
        {
            RHI_INDEX_BUFFER_VIEW IndexBufferView = {};
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
        static std::shared_ptr<RHIBuffer> Create(RHIBufferTarget    bufferTarget,
                                                 UINT32             numElements,
                                                 UINT32             elementSize,
                                                 const std::wstring name     = L"Buffer",
                                                 RHIBufferMode mapplableMode = RHIBufferMode::RHIBufferModeImmutable,
                                                 RHI_RESOURCE_STATES initState = RHI_RESOURCE_STATE_GENERIC_READ,
                                                 BYTE*                 initialData = nullptr,
                                                 UINT                  dataLen     = 0);

        std::shared_ptr<RHIBuffer> GetCounterBuffer();

        bool InflateBuffer(BYTE* initialData, UINT dataLen);
        void ResetCounterBuffer(RHICommandContext* pCommandContext);

        std::shared_ptr<RHIConstantBufferView>  CreateCBV(RHI_CONSTANT_BUFFER_VIEW_DESC cbvDesc);
        std::shared_ptr<RHIShaderResourceView>  CreateSRV(RHI_SHADER_RESOURCE_VIEW_DESC srvDesc);
        std::shared_ptr<RHIUnorderedAccessView> CreateUAV(RHI_UNORDERED_ACCESS_VIEW_DESC uavDesc);
        std::shared_ptr<RHIUnorderedAccessView> CreateUAV(RHI_UNORDERED_ACCESS_VIEW_DESC uavDesc, RHIResource* pCounterRes);

        std::shared_ptr<RHIConstantBufferView> CreateNoneVisualCBV(RHI_CONSTANT_BUFFER_VIEW_DESC cbvDesc);
        std::shared_ptr<RHIShaderResourceView> CreateNoneVisualSRV(RHI_SHADER_RESOURCE_VIEW_DESC srvDesc);
        std::shared_ptr<RHIUnorderedAccessView> CreateNoneVisualUAV(RHI_UNORDERED_ACCESS_VIEW_DESC uavDesc);
        std::shared_ptr<RHIUnorderedAccessView> CreateNoneVisualUAV(RHI_UNORDERED_ACCESS_VIEW_DESC uavDesc, RHIResource* pCounterRes);
        
        std::shared_ptr<RHIConstantBufferView>  GetDefaultCBV();
        std::shared_ptr<RHIShaderResourceView>  GetDefaultSRV();
        std::shared_ptr<RHIUnorderedAccessView> GetDefaultUAV();

        std::shared_ptr<RHIConstantBufferView> GetDefaultNoneVisualCBV();
        std::shared_ptr<RHIShaderResourceView> GetDefaultNoneVisualSRV();
        std::shared_ptr<RHIUnorderedAccessView> GetDefaultNoneVisualUAV();
        
        std::shared_ptr<RHIUnorderedAccessView> GetDefaultStructureUAV(bool hasCounter = false);
        std::shared_ptr<RHIUnorderedAccessView> GetDefaultRawUAV(bool hasCounter = false);

        RHIBufferDesc& GetBufferDesc();

    protected:
        RHIBufferDesc m_Desc;
        RHIBufferData m_Data;
        
        std::shared_ptr<RHIBuffer> p_CounterBufferRHI;

    private:
        RHI_HEAP_TYPE           m_HeapType    = {};
        UINT                      m_SizeInBytes = 0;
        UINT                      m_Stride      = 0;
        BYTE*                     m_CpuVirtualAddress;
        RHI_GPU_VIRTUAL_ADDRESS m_GpuVirtualAddress;
        RHIScopedPointer        m_ScopedPointer; // Upload heap
    };

    class RHITexture : public RHIResource
    {
    public:
        RHITexture() noexcept = default;
        RHITexture(const CD3DX12_RESOURCE_DESC&       Desc,
                   const CD3DX12_HEAP_PROPERTIES&     HeapProperty,
                   std::optional<CD3DX12_CLEAR_VALUE> ClearValue,
                   RHI_RESOURCE_STATES              InitialResourceState,
                   bool                               Cubemap = false);
        RHITexture(const CD3DX12_RESOURCE_DESC&       Desc,
                     std::optional<CD3DX12_CLEAR_VALUE> ClearValue,
                     RHI_RESOURCE_STATES              InitialResourceState,
                     bool                               Cubemap = false);
        RHITexture(const CD3DX12_RESOURCE_DESC&       Desc,
                     std::optional<CD3DX12_CLEAR_VALUE> ClearValue,
                     bool                               Cubemap = false);

        ~RHITexture() { Destroy(); };

        virtual void Destroy() override;

        [[nodiscard]] UINT GetSubresourceIndex(std::optional<UINT> OptArraySlice = std::nullopt,
                                               std::optional<UINT> OptMipSlice   = std::nullopt,
                                               std::optional<UINT> OptPlaneSlice = std::nullopt) const noexcept;

        [[nodiscard]] bool IsCubemap() const noexcept { return m_IsCubemap; }

    public:

        void AssociateWithResource(RHILinkedDevice*                     Parent,
                                   const std::wstring&                    Name,
                                   Microsoft::WRL::ComPtr<IRHIResource> Resource,
                                   RHI_RESOURCE_STATES                  CurrentState);

        static void GenerateMipMaps(RHICommandContext* context, RHITexture* pTexture);

    public:
        static std::shared_ptr<RHITexture> Create(RHILinkedDevice*       Parent,
                                                    RHIRenderSurfaceBaseDesc desc,
                                                    const std::wstring       name      = L"Texture",
                                                    RHI_RESOURCE_STATES    initState = RHI_RESOURCE_STATE_COMMON,
                                                    std::optional<CD3DX12_CLEAR_VALUE>  clearValue = std::nullopt,
                                                    std::vector<RHI_SUBRESOURCE_DATA> initDatas  = {});

        static std::shared_ptr<RHITexture> Create2D(RHILinkedDevice*     Parent,
                                                      UINT32                 width,
                                                      UINT32                 height,
                                                      INT32                  numMips,
                                                      DXGI_FORMAT            format,
                                                      RHISurfaceCreateFlags  flags,
                                                      UINT32                 sampleCount = 1,
                                                      const std::wstring     name        = L"Texture2D",
                                                      RHI_RESOURCE_STATES  initState   = RHI_RESOURCE_STATE_COMMON,
                                                      std::optional<CD3DX12_CLEAR_VALUE> clearValue = std::nullopt,
                                                      RHI_SUBRESOURCE_DATA initData    = {});

        static std::shared_ptr<RHITexture>
        Create2DArray(RHILinkedDevice*                  Parent,
                      UINT32                              width,
                      UINT32                              height,
                      UINT32                              arraySize,
                      INT32                               numMips,
                      DXGI_FORMAT                         format,
                      RHISurfaceCreateFlags               flags,
                      UINT32                              sampleCount = 1,
                      const std::wstring                  name        = L"Texture2DArray",
                      RHI_RESOURCE_STATES               initState   = RHI_RESOURCE_STATE_COMMON,
                      std::optional<CD3DX12_CLEAR_VALUE>  clearValue  = std::nullopt,
                      std::vector<RHI_SUBRESOURCE_DATA> initDatas   = {});

        static std::shared_ptr<RHITexture>
        CreateCubeMap(RHILinkedDevice*                  Parent,
                      UINT32                              width,
                      UINT32                              height,
                      INT32                               numMips,
                      DXGI_FORMAT                         format,
                      RHISurfaceCreateFlags               flags,
                      UINT32                              sampleCount = 1,
                      const std::wstring                  name        = L"TextureCubeMap",
                      RHI_RESOURCE_STATES               initState   = RHI_RESOURCE_STATE_COMMON,
                      std::optional<CD3DX12_CLEAR_VALUE>  clearValue  = std::nullopt,
                      std::vector<RHI_SUBRESOURCE_DATA> initDatas   = {});

        static std::shared_ptr<RHITexture>
        CreateCubeMapArray(RHILinkedDevice*                  Parent,
                           UINT32                              width,
                           UINT32                              height,
                           UINT32                              arraySize,
                           INT32                               numMips,
                           DXGI_FORMAT                         format,
                           RHISurfaceCreateFlags               flags,
                           UINT32                              sampleCount = 1,
                           const std::wstring                  name        = L"TextureCubeMapArray",
                           RHI_RESOURCE_STATES               initState   = RHI_RESOURCE_STATE_COMMON,
                           std::optional<CD3DX12_CLEAR_VALUE>  clearValue  = std::nullopt,
                           std::vector<RHI_SUBRESOURCE_DATA> initDatas   = {});

        static std::shared_ptr<RHITexture> Create3D(RHILinkedDevice*     Parent,
                                                      UINT32                 width,
                                                      UINT32                 height,
                                                      UINT32                 depth,
                                                      INT32                  numMips,
                                                      DXGI_FORMAT            format,
                                                      RHISurfaceCreateFlags  flags,
                                                      UINT32                 sampleCount = 1,
                                                      const std::wstring     name        = L"Texture3D",
                                                      RHI_RESOURCE_STATES  initState   = RHI_RESOURCE_STATE_COMMON,
                                                      std::optional<CD3DX12_CLEAR_VALUE> clearValue = std::nullopt,
                                                      RHI_SUBRESOURCE_DATA initData    = {});

        static std::shared_ptr<RHITexture>
        CreateFromSwapchain(RHILinkedDevice*                     Parent,
                            Microsoft::WRL::ComPtr<IRHIResource> pResource,
                            RHI_RESOURCE_STATES                  initState = RHI_RESOURCE_STATE_COMMON,
                            std::wstring                           name      = L"Backbuffer");

        bool InflateTexture(std::vector<RHI_SUBRESOURCE_DATA> initDatas);

        std::shared_ptr<RHIShaderResourceView>  CreateSRV(RHI_SHADER_RESOURCE_VIEW_DESC srvDesc);
        std::shared_ptr<RHIUnorderedAccessView> CreateUAV(RHI_UNORDERED_ACCESS_VIEW_DESC uavDesc);
        
        std::shared_ptr<RHIRenderTargetView>    CreateRTV(RHI_RENDER_TARGET_VIEW_DESC rtvDesc);
        std::shared_ptr<RHIDepthStencilView>    CreateDSV(RHI_DEPTH_STENCIL_VIEW_DESC dsvDesc);
        std::shared_ptr<RHINoneVisualSRV>       CreateNoneVisualSRV(RHI_SHADER_RESOURCE_VIEW_DESC srvDesc);
        std::shared_ptr<RHINoneVisualUAV>       CreateNoneVisualUAV(RHI_UNORDERED_ACCESS_VIEW_DESC uavDesc);

        std::shared_ptr<RHIShaderResourceView>  GetDefaultSRV();
        std::shared_ptr<RHIUnorderedAccessView> GetDefaultUAV();

        std::shared_ptr<RHIRenderTargetView>    GetDefaultRTV();
        std::shared_ptr<RHIDepthStencilView>    GetDefaultDSV();
        std::shared_ptr<RHINoneVisualSRV>       GetDefaultNoneVisualSRV();
        std::shared_ptr<RHINoneVisualUAV>       GetDefaultNoneVisualUAV();

        // Write the raw pixel buffer contents to a file
        // Note that data is preceded by a 16-byte header:  { DXGI_FORMAT, Pitch (in pixels), Width (in pixels), Height
        // }
        void ExportToFile(const std::wstring& FilePath);

    protected:
        static INT GetMipLevels(UINT width, UINT height, INT32 numMips, RHISurfaceCreateFlags flags);

    protected:
        RHIRenderSurfaceBaseDesc m_Desc;
        
        robin_hood::unordered_map<UINT64, std::shared_ptr<RHIRenderTargetView>> m_RTVHandleMap;
        robin_hood::unordered_map<UINT64, std::shared_ptr<RHIDepthStencilView>> m_DSVHandleMap;
        robin_hood::unordered_map<UINT64, std::shared_ptr<RHINoneVisualSRV>>    m_NoneVisual_SRVHandleMap;
        robin_hood::unordered_map<UINT64, std::shared_ptr<RHINoneVisualUAV>>    m_NoneVisual_UAVHandleMap;

        robin_hood::unordered_map<UINT64, std::shared_ptr<RHIShaderResourceView>> m_SRVHandleMap;
        robin_hood::unordered_map<UINT64, std::shared_ptr<RHIUnorderedAccessView>> m_UAVHandleMap;

    protected:
        bool m_IsCubemap = false;
    };


}
