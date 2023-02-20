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
        //uint64_t                               mPendingFence;
        D3D12SyncHandle                        mSyncHandle;
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
        void FenceCommittedPages(_In_ D3D12SyncHandle syncHandle);

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
        Microsoft::WRL::ComPtr<ID3D12Device> m_device;
        D3D12SyncHandle                      m_syncHandle;

        // uint64_t                             m_fenceCount;
        // Microsoft::WRL::ComPtr<ID3D12Fence>  m_fence;


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

} // namespace RHI
