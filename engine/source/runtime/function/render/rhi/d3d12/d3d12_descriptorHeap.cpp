#include "d3d12_descriptorHeap.h"
#include "d3d12_device.h"
#include "d3d12_linkedDevice.h"

namespace RHI
{
    static Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(ID3D12Device*               Device,
                                                                             D3D12_DESCRIPTOR_HEAP_TYPE  Type,
                                                                             std::uint32_t               NumDescriptors,
                                                                             D3D12_DESCRIPTOR_HEAP_FLAGS Flags,
                                                                             std::uint32_t               NodeMask)
    {
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DescriptorHeap;
        D3D12_DESCRIPTOR_HEAP_DESC                   Desc = {Type, NumDescriptors, Flags, NodeMask};
        VERIFY_D3D12_API(Device->CreateDescriptorHeap(&Desc, IID_PPV_ARGS(&DescriptorHeap)));
        return DescriptorHeap;
    }

    // clang-format off
    D3D12DescriptorHeap::D3D12DescriptorHeap(D3D12LinkedDevice*         Parent,
                                             D3D12_DESCRIPTOR_HEAP_TYPE Type,
                                             UINT                       NumDescriptors)
        : D3D12LinkedDeviceChild(Parent)
        , DescriptorHeap(CreateDescriptorHeap(Parent->GetDevice(),
                                            Type,
                                            NumDescriptors,
                                            D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
                                            Parent->GetNodeMask()))
        , Desc(DescriptorHeap->GetDesc())
        , CpuBaseAddress(DescriptorHeap->GetCPUDescriptorHandleForHeapStart())
        , GpuBaseAddress(DescriptorHeap->GetGPUDescriptorHandleForHeapStart())
        , DescriptorSize(Parent->GetParentDevice()->GetSizeOfDescriptor(Type))
    {}
    // clang-format on

	void D3D12DescriptorHeap::Allocate(D3D12_CPU_DESCRIPTOR_HANDLE& CpuDescriptorHandle,
                                       D3D12_GPU_DESCRIPTOR_HANDLE& GpuDescriptorHandle,
                                       UINT&                        Index)
    {
        std::lock_guard<std::mutex> Guard(Mutex);
        Index               = IndexPool.Allocate();
        CpuDescriptorHandle = this->GetCpuDescriptorHandle(Index);
        GpuDescriptorHandle = this->GetGpuDescriptorHandle(Index);
    }

	void D3D12DescriptorHeap::Release(UINT Index)
    {
        std::lock_guard<std::mutex> Guard(Mutex);
        IndexPool.Release(Index);
    }

	D3D12_CPU_DESCRIPTOR_HANDLE D3D12DescriptorHeap::GetCpuDescriptorHandle(UINT Index) const noexcept
    {
        return CD3DX12_CPU_DESCRIPTOR_HANDLE(CpuBaseAddress, static_cast<INT>(Index), DescriptorSize);
    }

    D3D12_GPU_DESCRIPTOR_HANDLE D3D12DescriptorHeap::GetGpuDescriptorHandle(UINT Index) const noexcept
    {
        return CD3DX12_GPU_DESCRIPTOR_HANDLE(GpuBaseAddress, static_cast<INT>(Index), DescriptorSize);
    }

    // clang-format off
	CDescriptorHeapManager::CDescriptorHeapManager(D3D12LinkedDevice*         Parent,
                                                   D3D12_DESCRIPTOR_HEAP_TYPE Type,
                                                   UINT                       PageSize)
        : D3D12LinkedDeviceChild(Parent)
        , Desc({Type, PageSize, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, Parent->GetNodeMask()})
        , DescriptorSize(Parent->GetParentDevice()->GetSizeOfDescriptor(Type))
    {}
    // clang-format on

    D3D12_CPU_DESCRIPTOR_HANDLE CDescriptorHeapManager::AllocateHeapSlot(UINT& OutDescriptorHeapIndex)
    {
        std::lock_guard<std::mutex> Guard(Mutex);

        if (FreeHeaps.empty())
        {
            AllocateHeap(); // throw( _com_error )
        }
        assert(!FreeHeaps.empty());
        UINT        Index     = FreeHeaps.front();
        SHeapEntry& HeapEntry = Heaps[Index];
        assert(!HeapEntry.FreeList.empty());
        SFreeRange&                 Range = *HeapEntry.FreeList.begin();
        D3D12_CPU_DESCRIPTOR_HANDLE Ret   = {Range.Start};
        Range.Start += DescriptorSize;

        if (Range.Start == Range.End)
        {
            HeapEntry.FreeList.pop_front();
            if (HeapEntry.FreeList.empty())
            {
                FreeHeaps.pop_front();
            }
        }
        OutDescriptorHeapIndex = Index;
        return Ret;
    }

void CDescriptorHeapManager::FreeHeapSlot(D3D12_CPU_DESCRIPTOR_HANDLE Offset, UINT DescriptorHeapIndex) noexcept
    {
        std::lock_guard<std::mutex> Guard(Mutex);
        try
        {
            assert(DescriptorHeapIndex < Heaps.size());
            SHeapEntry& HeapEntry = Heaps[DescriptorHeapIndex];

            SFreeRange NewRange = {Offset.ptr, Offset.ptr + DescriptorSize};

            bool bFound = false;
            for (auto Iter = HeapEntry.FreeList.begin(), End = HeapEntry.FreeList.end(); Iter != End && !bFound; ++Iter)
            {
                SFreeRange& Range = *Iter;
                assert(Range.Start <= Range.End);
                if (Range.Start == Offset.ptr + DescriptorSize)
                {
                    Range.Start = Offset.ptr;
                    bFound      = true;
                }
                else if (Range.End == Offset.ptr)
                {
                    Range.End += DescriptorSize;
                    bFound = true;
                }
                else
                {
                    assert(Range.End < Offset.ptr || Range.Start > Offset.ptr);
                    if (Range.Start > Offset.ptr)
                    {
                        HeapEntry.FreeList.insert(Iter, NewRange); // throw( bad_alloc )
                        bFound = true;
                    }
                }
            }

            if (!bFound)
            {
                if (HeapEntry.FreeList.empty())
                {
                    FreeHeaps.push_back(DescriptorHeapIndex); // throw( bad_alloc )
                }
                HeapEntry.FreeList.push_back(NewRange); // throw( bad_alloc )
            }
        }
        catch (std::bad_alloc&)
        {
            // Do nothing - there will be slots that can no longer be reclaimed.
        }
    }

	void CDescriptorHeapManager::AllocateHeap()
    {
        SHeapEntry NewEntry;
        VERIFY_D3D12_API(Parent->GetDevice()->CreateDescriptorHeap(&Desc, IID_PPV_ARGS(&NewEntry.DescriptorHeap)));
        D3D12_CPU_DESCRIPTOR_HANDLE HeapBase = NewEntry.DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
        NewEntry.FreeList.push_back({HeapBase.ptr,
                                     HeapBase.ptr + static_cast<SIZE_T>(Desc.NumDescriptors) *
                                                        static_cast<SIZE_T>(DescriptorSize)}); // throw( bad_alloc )

        Heaps.emplace_back(std::move(NewEntry));                  // throw( bad_alloc )
        FreeHeaps.push_back(static_cast<UINT>(Heaps.size() - 1)); // throw( bad_alloc )
    }

}

