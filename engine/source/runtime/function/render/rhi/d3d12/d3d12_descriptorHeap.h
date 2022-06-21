#pragma once
#include "d3d12_core.h"

#include <mutex>

namespace RHI
{
    struct DescriptorIndexPool
    {
        // Removes the first element from the free list and returns its index
        UINT Allocate()
        {
            UINT NewIndex;
            if (!IndexQueue.empty())
            {
                NewIndex = IndexQueue.front();
                IndexQueue.pop();
            }
            else
            {
                NewIndex = Index++;
            }
            return NewIndex;
        }

        void Release(UINT Index) { IndexQueue.push(Index); }

        std::queue<UINT> IndexQueue;
        UINT             Index = 0;
    };

    // Dynamic resource binding heap, descriptor index are maintained in a free list
    class D3D12DescriptorHeap : public D3D12LinkedDeviceChild
    {
    public:
        explicit D3D12DescriptorHeap(D3D12LinkedDevice* Parent, D3D12_DESCRIPTOR_HEAP_TYPE Type, UINT NumDescriptors);

        void SetName(LPCWSTR Name) const { DescriptorHeap->SetName(Name); }

        operator ID3D12DescriptorHeap*() const noexcept { return DescriptorHeap.Get(); }

        void Allocate(D3D12_CPU_DESCRIPTOR_HANDLE& CpuDescriptorHandle,
                      D3D12_GPU_DESCRIPTOR_HANDLE& GpuDescriptorHandle,
                      UINT&                        Index);

        void Release(UINT Index);

        [[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE GetCpuDescriptorHandle(UINT Index) const noexcept;
        [[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE GetGpuDescriptorHandle(UINT Index) const noexcept;

    private:
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DescriptorHeap;
        D3D12_DESCRIPTOR_HEAP_DESC                   Desc;
        D3D12_CPU_DESCRIPTOR_HANDLE                  CpuBaseAddress = {};
        D3D12_GPU_DESCRIPTOR_HANDLE                  GpuBaseAddress = {};
        UINT                                         DescriptorSize = 0;

        DescriptorIndexPool IndexPool;
        std::mutex          Mutex;
    };

	// Used for RTV/DSV Heaps
    class CDescriptorHeapManager : public D3D12LinkedDeviceChild
    {
    public: // Types
        using HeapOffsetRaw = decltype(D3D12_CPU_DESCRIPTOR_HANDLE::ptr);

    private: // Types
        struct SFreeRange
        {
            HeapOffsetRaw Start;
            HeapOffsetRaw End;
        };
        struct SHeapEntry
        {
            Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DescriptorHeap;
            std::list<SFreeRange>                        FreeList;
        };

        // Note: This data structure relies on the pointer validity guarantee of std::deque,
        // that as long as inserts/deletes are only on either end of the container, pointers
        // to elements continue to be valid. If trimming becomes an option, the free heap
        // list must be re-generated at that time.
        using THeapMap = std::deque<SHeapEntry>;

    public: // Methods
        explicit CDescriptorHeapManager(D3D12LinkedDevice* Parent, D3D12_DESCRIPTOR_HEAP_TYPE Type, UINT PageSize);

        D3D12_CPU_DESCRIPTOR_HANDLE AllocateHeapSlot(UINT& OutDescriptorHeapIndex);

        void FreeHeapSlot(D3D12_CPU_DESCRIPTOR_HANDLE Offset, UINT DescriptorHeapIndex) noexcept;

    private: // Methods
        void AllocateHeap();

    private: // Members
        const D3D12_DESCRIPTOR_HEAP_DESC Desc;
        const UINT                       DescriptorSize;
        std::mutex                       Mutex;

        THeapMap        Heaps;
        std::list<UINT> FreeHeaps;
    };

}