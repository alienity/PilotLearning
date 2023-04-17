#include "RHICore.hpp"

namespace RHI
{
    class RHIResource;
    class RHIBuffer;
    class RHITexture;


    template<typename ViewDesc>
    class RHIDescriptorView
    {
    public:
        RHIDescriptorView() noexcept = default;
        explicit RHIDescriptorView(const ViewDesc& Desc, RHIResource* Resource, BOOL IsGPUDesc = TRUE) : 
            Desc(Desc), Resource(Resource), IsGPUDesc(IsGPUDesc)  {}

        RHIDescriptorView(RHIDescriptorView&&) noexcept = default;
        RHIDescriptorView& operator=(RHIDescriptorView&&) noexcept = default;

        RHIDescriptorView(const RHIDescriptorView&) = delete;
        RHIDescriptorView& operator=(const RHIDescriptorView&) = delete;

        [[nodiscard]] BOOL                      IsGPUView() const noexcept { return IsGPUDesc; }
        [[nodiscard]] BOOL                      IsValid() const noexcept { return TRUE; }
        [[nodiscard]] RHI_CPU_DESCRIPTOR_HANDLE GetCpuHandle() const noexcept { return RHI_CPU_DESCRIPTOR_HANDLE{0}; }
        [[nodiscard]] RHI_GPU_DESCRIPTOR_HANDLE GetGpuHandle() const noexcept { return RHI_GPU_DESCRIPTOR_HANDLE{0}; }
        [[nodiscard]] ViewDesc                  GetDesc() const noexcept { return Desc; }
        [[nodiscard]] RHIResource*              GetResource() const noexcept { return Resource; }
    protected:
        friend class RHIResource;

        BOOL IsGPUDesc = false;
        ViewDesc Desc = {};
        RHIResource* Resource = nullptr;
    };

    class RHIRenderTargetView : public RHIDescriptorView<RHI_RENDER_TARGET_VIEW_DESC>
    {
    public:
        RHIRenderTargetView() noexcept = default;
        explicit RHIRenderTargetView(const RHI_RENDER_TARGET_VIEW_DESC& Desc, RHIResource* Resource);

        void RecreateView();
    public:
        static RHI_RENDER_TARGET_VIEW_DESC GetDesc(RHITexture* Texture, BOOL SRGB = false, UINT ArraySlice = 0, UINT MipSlice = 0, UINT ArraySize = 0);
    };

    class RHIDepthStencilView : public RHIDescriptorView<RHI_DEPTH_STENCIL_VIEW_DESC>
    {
    public:
        RHIDepthStencilView() noexcept = default;
        explicit RHIDepthStencilView(const RHI_DEPTH_STENCIL_VIEW_DESC& Desc, RHIResource* Resource);
        explicit RHIDepthStencilView(RHITexture* Texture, UINT ArraySlice = 0, UINT MipSlice = 0, UINT ArraySize = 0);

        void RecreateView();

    public:
        static RHI_DEPTH_STENCIL_VIEW_DESC GetDesc(RHITexture* Texture, UINT ArraySlice = 0, UINT MipSlice = 0, UINT ArraySize = 0);
    };

    class RHIConstantBufferView : public RHIDescriptorView<RHI_CONSTANT_BUFFER_VIEW_DESC>
    {
    public:
        RHIConstantBufferView() noexcept = default;
        RHIConstantBufferView(const RHI_CONSTANT_BUFFER_VIEW_DESC& Desc, RHIResource* Resource, BOOL IsGPUDesc = TRUE);
        RHIConstantBufferView(RHIBuffer* Buffer, UINT32 Offset, UINT32 Size, BOOL IsGPUDesc = TRUE);

        void RecreateView();

        [[nodiscard]] RHI_GPU_DESCRIPTOR_HANDLE GetGpuHandle() const noexcept { return RHIDescriptorView::GetGpuHandle(); }

    public:
        static RHI_CONSTANT_BUFFER_VIEW_DESC GetDesc(RHIBuffer* Buffer, UINT Offset, UINT Size);
    };

    class RHIShaderResourceView : public RHIDescriptorView<RHI_SHADER_RESOURCE_VIEW_DESC>
    {
    public:
        RHIShaderResourceView() noexcept = default;
        RHIShaderResourceView(const RHI_SHADER_RESOURCE_VIEW_DESC& Desc, RHIResource* Resource, BOOL IsGPUDesc = TRUE);
        RHIShaderResourceView(RHIBuffer* Buffer, BOOL Raw, UINT FirstElement, UINT NumElements, BOOL IsGPUDesc = TRUE);
        RHIShaderResourceView(RHIBuffer* Buffer, UINT FirstElement, UINT NumElements, BOOL IsGPUDesc = TRUE);
        RHIShaderResourceView(RHIBuffer* Buffer, BOOL IsGPUDesc = TRUE);
        RHIShaderResourceView(RHITexture* Texture, BOOL SRGB, BOOL IsGPUDesc = TRUE, UINT MostDetailedMip = 0, UINT OptMipLevels = 0);

        void RecreateView();

        [[nodiscard]] RHI_GPU_DESCRIPTOR_HANDLE GetGpuHandle() const noexcept { return RHIDescriptorView::GetGpuHandle(); }

    public:
        static RHI_SHADER_RESOURCE_VIEW_DESC GetDesc(RHIBuffer* Buffer, BOOL Raw, UINT FirstElement, UINT NumElements);
        static RHI_SHADER_RESOURCE_VIEW_DESC GetDesc(RHITexture* Texture, BOOL SRGB, UINT MostDetailedMip = 0, UINT MipLevels = 0);
    };

    class RHIUnorderedAccessView : public RHIDescriptorView<RHI_UNORDERED_ACCESS_VIEW_DESC>
    {
    public:
        RHIUnorderedAccessView() noexcept = default;
        RHIUnorderedAccessView(const RHI_UNORDERED_ACCESS_VIEW_DESC& Desc, RHIResource* Resource, RHIResource* CounterResource = nullptr);
        RHIUnorderedAccessView(RHIBuffer* Buffer, BOOL Raw, UINT FirstElement, UINT NumElements, UINT64 CounterOffsetInBytes);
        RHIUnorderedAccessView(RHIBuffer* Buffer, UINT FirstElement, UINT NumElements, UINT64 CounterOffsetInBytes);
        RHIUnorderedAccessView(RHIBuffer* Buffer);
        RHIUnorderedAccessView(RHITexture* Texture, UINT ArraySlice, UINT MipSlice);

        void RecreateView();

        [[nodiscard]] RHI_GPU_DESCRIPTOR_HANDLE   GetGpuHandle() const noexcept { return RHIDescriptorView::GetGpuHandle(); }
    public:
        static RHI_UNORDERED_ACCESS_VIEW_DESC GetDesc(RHIBuffer* Buffer, BOOL Raw, UINT FirstElement, UINT NumElements, UINT64 CounterOffsetInBytes);
        static RHI_UNORDERED_ACCESS_VIEW_DESC GetDesc(RHITexture* Texture, UINT ArraySlice, UINT OptMipSlice = 0);
        
    private:
        RHIResource* CounterResource = nullptr;
    };






































}
