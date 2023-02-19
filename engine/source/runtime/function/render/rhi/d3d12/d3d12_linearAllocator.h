#pragma once
#include "d3d12_core.h"

namespace RHI
{
    class LinearAllocatorPage
    {
    public:
        LinearAllocatorPage() noexcept;

        LinearAllocatorPage(LinearAllocatorPage&&)            = delete;
        LinearAllocatorPage& operator=(LinearAllocatorPage&&) = delete;

        LinearAllocatorPage(LinearAllocatorPage const&)            = delete;
        LinearAllocatorPage& operator=(LinearAllocatorPage const&) = delete;

        size_t Suballocate(_In_ size_t size, _In_ size_t alignment);

        void*                     BaseMemory() const noexcept { return mMemory; }
        ID3D12Resource*           UploadResource() const noexcept { return mUploadResource.Get(); }
        D3D12_GPU_VIRTUAL_ADDRESS GpuAddress() const noexcept { return mGpuAddress; }
        size_t                    BytesUsed() const noexcept { return mOffset; }
        size_t                    Size() const noexcept { return mSize; }

        void    AddRef() noexcept { mRefCount.fetch_add(1); }
        int32_t RefCount() const noexcept { return mRefCount.load(); }
        void    Release() noexcept;

    protected:
        friend class LinearAllocator;

        LinearAllocatorPage* pPrevPage;
        LinearAllocatorPage* pNextPage;

        void*                                  mMemory;
        uint64_t                               mPendingFence;
        D3D12_GPU_VIRTUAL_ADDRESS              mGpuAddress;
        size_t                                 mOffset;
        size_t                                 mSize;
        Microsoft::WRL::ComPtr<ID3D12Resource> mUploadResource;

    private:
        std::atomic<int32_t> mRefCount;
    };

    class LinearAllocator
    {
    public:
        // These values will be rounded up to the nearest 64k.
        // You can specify zero for incrementalSizeBytes to increment
        // by 1 page (64k).
        LinearAllocator(_In_ ID3D12Device* pDevice,
                        _In_ size_t        pageSize,
                        _In_ size_t        preallocateBytes = 0) noexcept(false);

        LinearAllocator(LinearAllocator&&)            = default;
        LinearAllocator& operator=(LinearAllocator&&) = default;

        LinearAllocator(LinearAllocator const&)            = delete;
        LinearAllocator& operator=(LinearAllocator const&) = delete;

        ~LinearAllocator();

        LinearAllocatorPage* FindPageForAlloc(_In_ size_t requestedSize, _In_ size_t alignment);

        // Call this at least once a frame to check if pages have become available.
        void RetirePendingPages() noexcept;

        // Call this after you submit your work to the driver.
        // (e.g. immediately before Present.)
        void FenceCommittedPages(_In_ ID3D12CommandQueue* commandQueue);

        // Throws away all currently unused pages
        void Shrink() noexcept;

        // Statistics
        size_t CommittedPageCount() const noexcept { return m_numPending; }
        size_t TotalPageCount() const noexcept { return m_totalPages; }
        size_t CommittedMemoryUsage() const noexcept { return m_numPending * m_increment; }
        size_t TotalMemoryUsage() const noexcept { return m_totalPages * m_increment; }
        size_t PageSize() const noexcept { return m_increment; }

#if defined(_DEBUG) || defined(PROFILE)
        // Debug info
        const wchar_t* GetDebugName() const noexcept { return m_debugName.c_str(); }
        void           SetDebugName(const wchar_t* name);
        void           SetDebugName(const char* name);
#endif

    private:
        LinearAllocatorPage*                 m_pendingPages; // Pages in use by the GPU
        LinearAllocatorPage*                 m_usedPages;    // Pages to be submitted to the GPU
        LinearAllocatorPage*                 m_unusedPages;  // Pages not being used right now
        size_t                               m_increment;
        size_t                               m_numPending;
        size_t                               m_totalPages;
        uint64_t                             m_fenceCount;
        Microsoft::WRL::ComPtr<ID3D12Device> m_device;
        Microsoft::WRL::ComPtr<ID3D12Fence>  m_fence;

        LinearAllocatorPage* GetPageForAlloc(size_t sizeBytes, size_t alignment);
        LinearAllocatorPage* GetCleanPageForAlloc();

        LinearAllocatorPage* FindPageForAlloc(LinearAllocatorPage* list, size_t sizeBytes, size_t alignment) noexcept;

        LinearAllocatorPage* GetNewPage();

        void UnlinkPage(LinearAllocatorPage* page) noexcept;
        void LinkPage(LinearAllocatorPage* page, LinearAllocatorPage*& list) noexcept;
        void LinkPageChain(LinearAllocatorPage* page, LinearAllocatorPage*& list) noexcept;
        void ReleasePage(LinearAllocatorPage* page) noexcept;
        void FreePages(LinearAllocatorPage* list) noexcept;

#if defined(_DEBUG) || defined(PROFILE)
        std::wstring m_debugName;

        static void ValidateList(LinearAllocatorPage* list);
        void        ValidatePageLists();

        void SetPageDebugName(LinearAllocatorPage* list) noexcept;
#endif
    };


























































    /*
    struct D3D12Allocation
    {
        ID3D12Resource*           Resource;
        UINT64                    Offset;
        UINT64                    Size;
        BYTE*                     CpuVirtualAddress;
        D3D12_GPU_VIRTUAL_ADDRESS GpuVirtualAddress;
    };

	class D3D12LinearAllocatorPage
    {
    public:
        explicit D3D12LinearAllocatorPage(Microsoft::WRL::ComPtr<ID3D12Resource> Resource, UINT64 PageSize);
        ~D3D12LinearAllocatorPage();

        std::optional<D3D12Allocation> Suballocate(UINT64 Size, UINT Alignment);

        void Reset();

    private:
        Microsoft::WRL::ComPtr<ID3D12Resource> Resource;
        UINT64                                 Offset;
        UINT64                                 PageSize;
        BYTE*                                  CpuVirtualAddress;
        D3D12_GPU_VIRTUAL_ADDRESS              GpuVirtualAddress;
    };

	class D3D12LinearAllocator : public D3D12LinkedDeviceChild
    {
    public:
        static constexpr UINT64 CpuAllocatorPageSize = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;

        D3D12LinearAllocator() noexcept = default;
        explicit D3D12LinearAllocator(D3D12LinkedDevice* Parent);

        D3D12LinearAllocator(D3D12LinearAllocator&&) noexcept = default;
        D3D12LinearAllocator& operator=(D3D12LinearAllocator&&) noexcept = default;

        D3D12LinearAllocator(const D3D12LinearAllocator&) = delete;
        D3D12LinearAllocator& operator=(const D3D12LinearAllocator&) = delete;

        // Versions all the current constant data with SyncHandle to ensure memory is not overriden when it GPU uses it
        void Version(D3D12SyncHandle SyncHandle);

        [[nodiscard]] D3D12Allocation Allocate(UINT64 Size,
                                               UINT   Alignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

        template<typename T>
        [[nodiscard]] D3D12Allocation Allocate(const T& Data,
                                               UINT     Alignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT)
        {
            D3D12Allocation Allocation = Allocate(sizeof(T), Alignment);
            std::memcpy(Allocation.CpuVirtualAddress, &Data, sizeof(T));
            return Allocation;
        }

    private:
        [[nodiscard]] D3D12LinearAllocatorPage* RequestPage();

        [[nodiscard]] std::unique_ptr<D3D12LinearAllocatorPage> CreateNewPage(UINT64 PageSize) const;

        void DiscardPages(UINT64 FenceValue, const std::vector<D3D12LinearAllocatorPage*>& Pages);

    private:
        std::vector<std::unique_ptr<D3D12LinearAllocatorPage>>   PagePool;
        std::queue<std::pair<UINT64, D3D12LinearAllocatorPage*>> RetiredPages;
        std::queue<D3D12LinearAllocatorPage*>                    AvailablePages;

        D3D12SyncHandle                        SyncHandle;
        D3D12LinearAllocatorPage*              CurrentPage = nullptr;
        std::vector<D3D12LinearAllocatorPage*> RetiredPageList;
    };
    */

} // namespace RHI
