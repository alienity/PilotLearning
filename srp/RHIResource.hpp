#include "RHICore.hpp"

namespace RHI
{
    class RHIRenderTargetView;
    class RHIDepthStencilView;
    class RHIConstantBufferView;
    class RHIShaderResourceView;
    class RHIUnorderedAccessView;
    class RHICommandBuffer;

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

        void SetResourceName(std::wstring name);

    protected:
        void* m_PRealResource = nullptr;
        std::string m_Name;
    };

    class RHIBuffer : public RHIResource
    {
    public:
        RHIBuffer() noexcept = default;
        ~RHIBuffer() { Destroy(); };

        virtual void Destroy() override;

        [[nodiscard]] RHI_GPU_VIRTUAL_ADDRESS   GetGpuVirtualAddress(UINT Index = 0) const;
        [[nodiscard]] UINT                      GetStride() const { return m_Desc.stride; }
        [[nodiscard]] UINT                      GetSizeInBytes() const { return m_Desc.size; }
        template<typename T>
        [[nodiscard]] T* GetCpuVirtualAddress() const
        {
            return reinterpret_cast<T*>(GetCpuVirtualAddress());
        }

        BYTE* GetCpuVirtualAddress() const { return m_CpuVirtualAddress; }

        [[nodiscard]] RHI_VERTEX_BUFFER_VIEW GetVertexBufferView() const noexcept;

        [[nodiscard]] RHI_INDEX_BUFFER_VIEW
        GetIndexBufferView(RHI_FORMAT Format = RHI_FORMAT_R32_UINT) const noexcept;

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
        void ResetCounterBuffer(RHICommandBuffer* pCommandContext);

        std::shared_ptr<RHIConstantBufferView>  CreateCBV(RHI_CONSTANT_BUFFER_VIEW_DESC cbvDesc, BOOL isShaderVisable = TRUE);
        std::shared_ptr<RHIShaderResourceView>  CreateSRV(RHI_SHADER_RESOURCE_VIEW_DESC srvDesc, BOOL isShaderVisable = TRUE);
        std::shared_ptr<RHIUnorderedAccessView> CreateUAV(RHI_UNORDERED_ACCESS_VIEW_DESC uavDesc, BOOL isShaderVisable = TRUE);
        std::shared_ptr<RHIUnorderedAccessView> CreateUAV(RHI_UNORDERED_ACCESS_VIEW_DESC uavDesc, RHIResource* pCounterRes, BOOL isShaderVisable = TRUE);
        
        std::shared_ptr<RHIConstantBufferView>  GetDefaultCBV(BOOL isShaderVisable = TRUE);
        std::shared_ptr<RHIShaderResourceView>  GetDefaultSRV(BOOL isShaderVisable = TRUE);
        std::shared_ptr<RHIUnorderedAccessView> GetDefaultUAV(BOOL isShaderVisable = TRUE);

        std::shared_ptr<RHIUnorderedAccessView> GetDefaultStructureUAV(bool hasCounter = false);
        std::shared_ptr<RHIUnorderedAccessView> GetDefaultRawUAV(bool hasCounter = false);

        RHIBufferDesc& GetBufferDesc();

    protected:
        RHIBufferDesc m_Desc;
        RHIBufferData m_Data;
        
        std::shared_ptr<RHIBuffer> p_CounterBufferRHI;

        BYTE*                     m_CpuVirtualAddress;
        RHI_GPU_VIRTUAL_ADDRESS   m_GpuVirtualAddress;
    };

    class RHITexture : public RHIResource
    {
    public:
        RHITexture() noexcept = default;
        ~RHITexture() { Destroy(); };

        virtual void Destroy() override;

        [[nodiscard]] UINT GetSubresourceIndex(UINT OptArraySlice = 0, UINT OptMipSlice   = 0, UINT OptPlaneSlice = 0) const noexcept;
        [[nodiscard]] bool IsCubemap() const noexcept { return m_Desc.dimension == RHITextureDimension::RHITexDimCube; }

        [[nodiscard]] const RHIClearValue GetClearValue() const noexcept { return m_Desc.clearValue; };
        [[nodiscard]] inline void  GetClearColor(FLOAT color[4]) const { for (size_t i = 0; i < 4; i++) color[i] = m_Desc.clearValue.Color[i]; };
        [[nodiscard]] inline FLOAT GetClearDepth() const { return GetClearValue().DepthStencil.Depth; }
        [[nodiscard]] inline UINT8 GetClearStencil() const { return GetClearValue().DepthStencil.Stencil; }

        [[nodiscard]] inline const RHIRenderSurfaceBaseDesc& GetDesc() const noexcept { return m_Desc; }
        [[nodiscard]] inline RHITextureDimension GetDimension() const { return m_Desc.dimension; }
        [[nodiscard]] inline UINT64              GetWidth() const { return m_Desc.width; }
        [[nodiscard]] inline UINT64              GetHeight() const { return m_Desc.height; }
        [[nodiscard]] inline UINT16              GetArraySize() const { return m_Desc.depthOrArray; }
        [[nodiscard]] inline UINT16              GetDepth() const { return m_Desc.depthOrArray; }
        [[nodiscard]] inline UINT16              GetMipLevels() const { return m_Desc.mipLevels; }
        [[nodiscard]] inline RHI_FORMAT          GetFormat() const { return m_Desc.graphicsFormat; }
        // [[nodiscard]] inline UINT8               GetPlaneCount() const noexcept { return m_PlaneCount; }

        void AssociateWithResource(const std::wstring& Name, RHITexture* Resource, RHI_RESOURCE_STATES CurrentState);

        static void GenerateMipMaps(RHITexture* pTexture);

    public:
        static std::shared_ptr<RHITexture> Create(RHIRenderSurfaceBaseDesc desc,
                                                  const std::wstring       name = L"Texture",
                                                  RHI_RESOURCE_STATES    initState = RHI_RESOURCE_STATE_COMMON,
                                                  std::optional<RHIClearValue> clearValue = std::nullopt,
                                                  std::vector<RHI_SUBRESOURCE_DATA> initDatas  = {});

        static std::shared_ptr<RHITexture> Create2D(UINT32                 width,
                                                      UINT32                 height,
                                                      INT32                  numMips,
                                                      RHI_FORMAT            format,
                                                      RHISurfaceCreateFlags  flags,
                                                      UINT32                 sampleCount = 1,
                                                      const std::wstring     name        = L"Texture2D",
                                                      RHI_RESOURCE_STATES  initState   = RHI_RESOURCE_STATE_COMMON,
                                                      std::optional<RHIClearValue> clearValue = std::nullopt,
                                                      RHI_SUBRESOURCE_DATA initData    = {});

        static std::shared_ptr<RHITexture> Create2DArray(UINT32                              width,
                                                        UINT32                              height,
                                                        UINT32                              arraySize,
                                                        INT32                               numMips,
                                                        RHI_FORMAT                         format,
                                                        RHISurfaceCreateFlags               flags,
                                                        UINT32                              sampleCount = 1,
                                                        const std::wstring                  name        = L"Texture2DArray",
                                                        RHI_RESOURCE_STATES               initState   = RHI_RESOURCE_STATE_COMMON,
                                                        std::optional<RHIClearValue>  clearValue  = std::nullopt,
                                                        std::vector<RHI_SUBRESOURCE_DATA> initDatas   = {});

        static std::shared_ptr<RHITexture> CreateCubeMap(UINT32                              width,
                                                        UINT32                              height,
                                                        INT32                               numMips,
                                                        RHI_FORMAT                         format,
                                                        RHISurfaceCreateFlags               flags,
                                                        UINT32                              sampleCount = 1,
                                                        const std::wstring                  name        = L"TextureCubeMap",
                                                        RHI_RESOURCE_STATES               initState   = RHI_RESOURCE_STATE_COMMON,
                                                        std::optional<RHIClearValue>  clearValue  = std::nullopt,
                                                        std::vector<RHI_SUBRESOURCE_DATA> initDatas   = {});

        static std::shared_ptr<RHITexture> CreateCubeMapArray(UINT32                              width,
                                                            UINT32                              height,
                                                            UINT32                              arraySize,
                                                            INT32                               numMips,
                                                            RHI_FORMAT                         format,
                                                            RHISurfaceCreateFlags               flags,
                                                            UINT32                              sampleCount = 1,
                                                            const std::wstring                  name        = L"TextureCubeMapArray",
                                                            RHI_RESOURCE_STATES               initState   = RHI_RESOURCE_STATE_COMMON,
                                                            std::optional<RHIClearValue>  clearValue  = std::nullopt,
                                                            std::vector<RHI_SUBRESOURCE_DATA> initDatas   = {});

        static std::shared_ptr<RHITexture> Create3D(UINT32                 width,
                                                      UINT32                 height,
                                                      UINT32                 depth,
                                                      INT32                  numMips,
                                                      RHI_FORMAT            format,
                                                      RHISurfaceCreateFlags  flags,
                                                      UINT32                 sampleCount = 1,
                                                      const std::wstring     name        = L"Texture3D",
                                                      RHI_RESOURCE_STATES  initState   = RHI_RESOURCE_STATE_COMMON,
                                                      std::optional<RHIClearValue> clearValue = std::nullopt,
                                                      RHI_SUBRESOURCE_DATA initData    = {});

        static std::shared_ptr<RHITexture> CreateFromSwapchain(
            RHITexture* pResource, RHI_RESOURCE_STATES initState = RHI_RESOURCE_STATE_COMMON, std::wstring name = L"Backbuffer");

        bool InflateTexture(std::vector<RHI_SUBRESOURCE_DATA> initDatas);

        std::shared_ptr<RHIShaderResourceView>  CreateSRV(RHI_SHADER_RESOURCE_VIEW_DESC srvDesc, BOOL isShaderVisable = TRUE);
        std::shared_ptr<RHIUnorderedAccessView> CreateUAV(RHI_UNORDERED_ACCESS_VIEW_DESC uavDesc, BOOL isShaderVisable = TRUE);
        std::shared_ptr<RHIRenderTargetView>    CreateRTV(RHI_RENDER_TARGET_VIEW_DESC rtvDesc);
        std::shared_ptr<RHIDepthStencilView>    CreateDSV(RHI_DEPTH_STENCIL_VIEW_DESC dsvDesc);

        std::shared_ptr<RHIShaderResourceView>  GetDefaultSRV(BOOL isShaderVisable = TRUE);
        std::shared_ptr<RHIUnorderedAccessView> GetDefaultUAV(BOOL isShaderVisable = TRUE);
        std::shared_ptr<RHIRenderTargetView>    GetDefaultRTV();
        std::shared_ptr<RHIDepthStencilView>    GetDefaultDSV();

    protected:
        static INT GetMipLevels(UINT width, UINT height, INT32 numMips, RHISurfaceCreateFlags flags);

        RHIRenderSurfaceBaseDesc m_Desc;
    };


}
