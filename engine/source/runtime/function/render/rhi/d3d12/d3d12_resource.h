#pragma once
#include "d3d12_core.h"

namespace RHI
{
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
                      D3D12_CLEAR_VALUE                        ClearValue,
                      D3D12_RESOURCE_STATES                    InitialResourceState);
        D3D12Resource(D3D12LinkedDevice*               Parent,
                      D3D12_HEAP_PROPERTIES            HeapProperties,
                      D3D12_RESOURCE_DESC              Desc,
                      D3D12_RESOURCE_STATES            InitialResourceState,
                      std::optional<D3D12_CLEAR_VALUE> ClearValue);

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

        uint32_t GetVersionID() const { return VersionID; }

        void SetResourceName(std::string name);

        [[nodiscard]] D3D12_CLEAR_VALUE GetClearValue() const noexcept
        {
            return ClearValue.has_value() ? *ClearValue : D3D12_CLEAR_VALUE {};
        }
        [[nodiscard]] const D3D12_RESOURCE_DESC& GetDesc() const noexcept { return Desc; }
        [[nodiscard]] UINT16                     GetMipLevels() const noexcept { return Desc.MipLevels; }
        [[nodiscard]] UINT16                     GetArraySize() const noexcept { return Desc.DepthOrArraySize; }
        [[nodiscard]] UINT8                      GetPlaneCount() const noexcept { return PlaneCount; }
        [[nodiscard]] UINT                       GetNumSubresources() const noexcept { return NumSubresources; }
        [[nodiscard]] CResourceState&            GetResourceState() { return ResourceState; }

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

    private:
        Microsoft::WRL::ComPtr<ID3D12Resource> InitializeResource(D3D12_HEAP_PROPERTIES            HeapProperties,
                                                                  D3D12_RESOURCE_DESC              Desc,
                                                                  D3D12_RESOURCE_STATES            InitialResourceState,
                                                                  std::optional<D3D12_CLEAR_VALUE> ClearValue) const;

        UINT CalculateNumSubresources() const;

    protected:
        // TODO: Add support for custom heap properties for UMA GPUs

        Microsoft::WRL::ComPtr<ID3D12Resource> m_pResource;
        std::optional<D3D12_CLEAR_VALUE>       ClearValue;
        D3D12_RESOURCE_DESC                    Desc            = {};
        UINT8                                  PlaneCount      = 0;
        UINT                                   NumSubresources = 0;
        CResourceState                         ResourceState;
        std::string                            ResourceName;

        // Used to identify when a resource changes so descriptors can be copied etc.
        uint32_t VersionID = 0;
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
        [[nodiscard]] UINT                      GetStride() const { return Stride; }
        [[nodiscard]] UINT                      GetSizeInBytes() const { return SizeInBytes; }
        template<typename T>
        [[nodiscard]] T* GetCpuVirtualAddress() const
        {
            assert(CpuVirtualAddress && "Invalid CpuVirtualAddress");
            return reinterpret_cast<T*>(CpuVirtualAddress);
        }

        [[nodiscard]] D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const noexcept
        {
            D3D12_VERTEX_BUFFER_VIEW VertexBufferView = {};
            VertexBufferView.BufferLocation           = m_pResource->GetGPUVirtualAddress();
            VertexBufferView.SizeInBytes              = static_cast<UINT>(Desc.Width);
            VertexBufferView.StrideInBytes            = Stride;
            return VertexBufferView;
        }

        [[nodiscard]] D3D12_INDEX_BUFFER_VIEW
        GetIndexBufferView(DXGI_FORMAT Format = DXGI_FORMAT_R32_UINT) const noexcept
        {
            D3D12_INDEX_BUFFER_VIEW IndexBufferView = {};
            IndexBufferView.BufferLocation          = m_pResource->GetGPUVirtualAddress();
            IndexBufferView.SizeInBytes             = static_cast<UINT>(Desc.Width);
            IndexBufferView.Format                  = Format;
            return IndexBufferView;
        }

        template<typename T>
        void CopyData(UINT Index, const T& Data)
        {
            assert(CpuVirtualAddress && "Invalid CpuVirtualAddress");
            memcpy(&CpuVirtualAddress[Index * Stride], &Data, sizeof(T));
        }

    private:
        D3D12_HEAP_TYPE    HeapType = {};
        UINT               SizeInBytes = 0;
        UINT               Stride   = 0;
        D3D12ScopedPointer ScopedPointer; // Upload heap
        BYTE*              CpuVirtualAddress = nullptr;
    };

    class D3D12Texture : public D3D12Resource
    {
    public:
        D3D12Texture() noexcept = default;
        D3D12Texture(D3D12LinkedDevice*                       Parent,
                     Microsoft::WRL::ComPtr<ID3D12Resource>&& Resource,
                     D3D12_CLEAR_VALUE                        ClearValue,
                     D3D12_RESOURCE_STATES                    InitialResourceState);
        D3D12Texture(D3D12LinkedDevice*               Parent,
                     const D3D12_RESOURCE_DESC&       Desc,
                     std::optional<D3D12_CLEAR_VALUE> ClearValue = std::nullopt,
                     bool                             Cubemap    = false);

        [[nodiscard]] UINT GetSubresourceIndex(std::optional<UINT> OptArraySlice = std::nullopt,
                                               std::optional<UINT> OptMipSlice   = std::nullopt,
                                               std::optional<UINT> OptPlaneSlice = std::nullopt) const noexcept;

        [[nodiscard]] bool IsCubemap() const noexcept { return Cubemap; }

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
    class UploadBuffer : public D3D12Resource
    {
    public:
        UploadBuffer(D3D12LinkedDevice* Parent, const std::wstring& name, size_t BufferSize);

        virtual ~UploadBuffer() { Destroy(); }

        void* Map(void);
        void  Unmap(size_t begin = 0, size_t end = -1);

        size_t GetBufferSize() const { return m_BufferSize; }

    protected:
        size_t m_BufferSize;
    };

    class PixelBuffer : public D3D12Resource
    {
    public:
        PixelBuffer() : m_Width(0), m_Height(0), m_ArraySize(0), m_Format(DXGI_FORMAT_UNKNOWN) {}

        uint32_t           GetWidth(void) const { return m_Width; }
        uint32_t           GetHeight(void) const { return m_Height; }
        uint32_t           GetDepth(void) const { return m_ArraySize; }
        const DXGI_FORMAT& GetFormat(void) const { return m_Format; }

        // Write the raw pixel buffer contents to a file
        // Note that data is preceded by a 16-byte header:  { DXGI_FORMAT, Pitch (in pixels), Width (in pixels), Height
        // }
        void ExportToFile(const std::wstring& FilePath);

    protected:
        D3D12_RESOURCE_DESC DescribeTex2D(uint32_t    Width,
                                          uint32_t    Height,
                                          uint32_t    DepthOrArraySize,
                                          uint32_t    NumMips,
                                          DXGI_FORMAT Format,
                                          UINT        Flags);

        void AssociateWithResource(ID3D12Device*         Device,
                                   const std::wstring&   Name,
                                   ID3D12Resource*       Resource,
                                   D3D12_RESOURCE_STATES CurrentState);

        void CreateTextureResource(ID3D12Device*              Device,
                                   const std::wstring&        Name,
                                   const D3D12_RESOURCE_DESC& ResourceDesc,
                                   D3D12_CLEAR_VALUE          ClearValue,
                                   D3D12_GPU_VIRTUAL_ADDRESS  VidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);

        static DXGI_FORMAT GetBaseFormat(DXGI_FORMAT Format);
        static DXGI_FORMAT GetUAVFormat(DXGI_FORMAT Format);
        static DXGI_FORMAT GetDSVFormat(DXGI_FORMAT Format);
        static DXGI_FORMAT GetDepthFormat(DXGI_FORMAT Format);
        static DXGI_FORMAT GetStencilFormat(DXGI_FORMAT Format);
        static size_t      BytesPerPixel(DXGI_FORMAT Format);

        uint32_t    m_Width;
        uint32_t    m_Height;
        uint32_t    m_ArraySize;
        DXGI_FORMAT m_Format;
    };

}