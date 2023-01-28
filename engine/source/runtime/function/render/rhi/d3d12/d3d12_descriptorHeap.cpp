#include "d3d12_descriptorHeap.h"
#include "d3d12_device.h"
#include "d3d12_linkedDevice.h"

#include "runtime/core/math/moyu_math.h"

namespace RHI
{
    VariableSizeAllocationsManager::Allocation VariableSizeAllocationsManager::Allocate(OffsetType Size,
                                                                                        OffsetType Alignment)
    {
        ASSERT(Size > 0);
        ASSERT(Pilot::IsPowerOfTwo(Alignment), "Alignment (", Alignment, ") must be power of 2");
        Size = Pilot::AlignUp(Size, Alignment);
        if (m_FreeSize < Size)
            return Allocation::InvalidAllocation();

        auto AlignmentReserve = (Alignment > m_CurrAlignment) ? Alignment - m_CurrAlignment : 0;
        // Get the first block that is large enough to encompass Size + AlignmentReserve bytes
        // lower_bound() returns an iterator pointing to the first element that
        // is not less (i.e. >= ) than key
        auto SmallestBlockItIt = m_FreeBlocksBySize.lower_bound(Size + AlignmentReserve);
        if (SmallestBlockItIt == m_FreeBlocksBySize.end())
            return Allocation::InvalidAllocation();

        auto SmallestBlockIt = SmallestBlockItIt->second;
        ASSERT(Size + AlignmentReserve <= SmallestBlockIt->second.Size);
        ASSERT(SmallestBlockIt->second.Size == SmallestBlockItIt->first);

        //     SmallestBlockIt.Offset
        //        |                                  |
        //        |<------SmallestBlockIt.Size------>|
        //        |<------Size------>|<---NewSize--->|
        //        |                  |
        //      Offset              NewOffset
        //
        auto Offset = SmallestBlockIt->first;
        ASSERT(Offset % m_CurrAlignment == 0);
        auto AlignedOffset = Pilot::AlignUp(Offset, Alignment);
        auto AdjustedSize  = Size + (AlignedOffset - Offset);
        ASSERT(AdjustedSize <= Size + AlignmentReserve);
        auto NewOffset = Offset + AdjustedSize;
        auto NewSize   = SmallestBlockIt->second.Size - AdjustedSize;
        ASSERT(SmallestBlockItIt == SmallestBlockIt->second.OrderBySizeIt);
        m_FreeBlocksBySize.erase(SmallestBlockItIt);
        m_FreeBlocksByOffset.erase(SmallestBlockIt);
        if (NewSize > 0)
        {
            AddNewBlock(NewOffset, NewSize);
        }

        m_FreeSize -= AdjustedSize;

        if ((Size & (m_CurrAlignment - 1)) != 0)
        {
            if (Pilot::IsPowerOfTwo(Size))
            {
                ASSERT(Size >= Alignment && Size < m_CurrAlignment);
                m_CurrAlignment = Size;
            }
            else
            {
                m_CurrAlignment = std::min(m_CurrAlignment, Alignment);
            }
        }

        return Allocation {Offset, AdjustedSize};
    }

    void VariableSizeAllocationsManager::Free(Allocation&& allocation)
    {
        ASSERT(allocation.IsValid());
        Free(allocation.UnalignedOffset, allocation.Size);
        allocation = Allocation {};
    }

    void VariableSizeAllocationsManager::Free(OffsetType Offset, OffsetType Size)
    {
        ASSERT(Offset != Allocation::InvalidOffset && Offset + Size <= m_MaxSize);

        // Find the first element whose offset is greater than the specified offset.
        // upper_bound() returns an iterator pointing to the first element in the
        // container whose key is considered to go after k.
        auto NextBlockIt = m_FreeBlocksByOffset.upper_bound(Offset);
        // Block being deallocated must not overlap with the next block
        ASSERT(NextBlockIt == m_FreeBlocksByOffset.end() || Offset + Size <= NextBlockIt->first);
        auto PrevBlockIt = NextBlockIt;
        if (PrevBlockIt != m_FreeBlocksByOffset.begin())
        {
            --PrevBlockIt;
            // Block being deallocated must not overlap with the previous block
            ASSERT(Offset >= PrevBlockIt->first + PrevBlockIt->second.Size);
        }
        else
            PrevBlockIt = m_FreeBlocksByOffset.end();

        OffsetType NewSize, NewOffset;
        if (PrevBlockIt != m_FreeBlocksByOffset.end() && Offset == PrevBlockIt->first + PrevBlockIt->second.Size)
        {
            //  PrevBlock.Offset             Offset
            //       |                          |
            //       |<-----PrevBlock.Size----->|<------Size-------->|
            //
            NewSize   = PrevBlockIt->second.Size + Size;
            NewOffset = PrevBlockIt->first;

            if (NextBlockIt != m_FreeBlocksByOffset.end() && Offset + Size == NextBlockIt->first)
            {
                //   PrevBlock.Offset           Offset            NextBlock.Offset
                //     |                          |                    |
                //     |<-----PrevBlock.Size----->|<------Size-------->|<-----NextBlock.Size----->|
                //
                NewSize += NextBlockIt->second.Size;
                m_FreeBlocksBySize.erase(PrevBlockIt->second.OrderBySizeIt);
                m_FreeBlocksBySize.erase(NextBlockIt->second.OrderBySizeIt);
                // Delete the range of two blocks
                ++NextBlockIt;
                m_FreeBlocksByOffset.erase(PrevBlockIt, NextBlockIt);
            }
            else
            {
                //   PrevBlock.Offset           Offset                     NextBlock.Offset
                //     |                          |                             |
                //     |<-----PrevBlock.Size----->|<------Size-------->| ~ ~ ~  |<-----NextBlock.Size----->|
                //
                m_FreeBlocksBySize.erase(PrevBlockIt->second.OrderBySizeIt);
                m_FreeBlocksByOffset.erase(PrevBlockIt);
            }
        }
        else if (NextBlockIt != m_FreeBlocksByOffset.end() && Offset + Size == NextBlockIt->first)
        {
            //   PrevBlock.Offset                   Offset            NextBlock.Offset
            //     |                                  |                    |
            //     |<-----PrevBlock.Size----->| ~ ~ ~ |<------Size-------->|<-----NextBlock.Size----->|
            //
            NewSize   = Size + NextBlockIt->second.Size;
            NewOffset = Offset;
            m_FreeBlocksBySize.erase(NextBlockIt->second.OrderBySizeIt);
            m_FreeBlocksByOffset.erase(NextBlockIt);
        }
        else
        {
            //   PrevBlock.Offset                   Offset                     NextBlock.Offset
            //     |                                  |                            |
            //     |<-----PrevBlock.Size----->| ~ ~ ~ |<------Size-------->| ~ ~ ~ |<-----NextBlock.Size----->|
            //
            NewSize   = Size;
            NewOffset = Offset;
        }

        AddNewBlock(NewOffset, NewSize);

        m_FreeSize += Size;
        if (IsEmpty())
        {
            // Reset current alignment
            ASSERT(GetNumFreeBlocks() == 1);
            ResetCurrAlignment();
        }
    }

    size_t VariableSizeAllocationsManager::GetNumFreeBlocks() const { return m_FreeBlocksByOffset.size(); }

    void VariableSizeAllocationsManager::Extend(size_t ExtraSize)
    {
        size_t NewBlockOffset = m_MaxSize;
        size_t NewBlockSize   = ExtraSize;

        if (!m_FreeBlocksByOffset.empty())
        {
            auto LastBlockIt = m_FreeBlocksByOffset.end();
            --LastBlockIt;

            const auto LastBlockOffset = LastBlockIt->first;
            const auto LastBlockSize   = LastBlockIt->second.Size;
            if (LastBlockOffset + LastBlockSize == m_MaxSize)
            {
                // Extend the last block
                NewBlockOffset = LastBlockOffset;
                NewBlockSize += LastBlockSize;

                ASSERT(LastBlockIt->second.OrderBySizeIt->first == LastBlockSize &&
                       LastBlockIt->second.OrderBySizeIt->second == LastBlockIt);
                m_FreeBlocksBySize.erase(LastBlockIt->second.OrderBySizeIt);
                m_FreeBlocksByOffset.erase(LastBlockIt);
            }
        }

        AddNewBlock(NewBlockOffset, NewBlockSize);

        m_MaxSize += ExtraSize;
        m_FreeSize += ExtraSize;
    }

    void VariableSizeAllocationsManager::AddNewBlock(OffsetType Offset, OffsetType Size)
    {
        auto NewBlockIt = m_FreeBlocksByOffset.emplace(Offset, Size);
        ASSERT(NewBlockIt.second);
        auto OrderIt = m_FreeBlocksBySize.emplace(Size, NewBlockIt.first);
        NewBlockIt.first->second.OrderBySizeIt = OrderIt;
    }

    void VariableSizeAllocationsManager::ResetCurrAlignment()
    {
        for (m_CurrAlignment = 1; m_CurrAlignment * 2 <= m_MaxSize; m_CurrAlignment *= 2)
        {}
    }

    void DescriptorHeapAllocation::ReleaseDescriptorHeapAllocation()
    {
        if (!IsNull() && m_pAllocator)
        {
            m_pAllocator->Free(std::move(*this));
            // Allocation must have been disposed by the allocator
            ASSERT(IsNull(), "Non-null descriptor is being destroyed");
        }
        this->Reset();
    }

	D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeapAllocation::GetCpuHandle(UINT32 Offset) const
    {
        ASSERT(Offset >= 0 && Offset < m_NumHandles);

        D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle = m_FirstCpuHandle;
        CPUHandle.ptr += m_DescriptorSize * Offset;

        return CPUHandle;
    }

    D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeapAllocation::GetGpuHandle(UINT32 Offset) const
    {
        ASSERT(Offset >= 0 && Offset < m_NumHandles);
        D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle = m_FirstGpuHandle;
        GPUHandle.ptr += m_DescriptorSize * Offset;

        return GPUHandle;
    }

    DescriptorHandle DescriptorHeapAllocation::GetDescriptorHandle(UINT32 Offset) const
    {
        ASSERT(Offset >= 0 && Offset < m_NumHandles);
        D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle = GetCpuHandle(Offset);
        D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle = GetGpuHandle(Offset);
        DescriptorHandle Handle = DescriptorHandle(CPUHandle, GPUHandle);
        return Handle;
    }

    UINT DescriptorHeapAllocation::GetDescriptorHeapOffsetIndex(UINT32 Offset) const
    {
        ASSERT(Offset >= 0 && Offset < m_NumHandles);
        ASSERT(m_DescriptorHandleOffsetIndex != InvalidDescriptorHandleOffsetIndex);
        return m_DescriptorHandleOffsetIndex + Offset;
    }

	DescriptorHeapAllocationManager::DescriptorHeapAllocationManager(IDescriptorAllocator*             ParentAllocator,
                                                                     size_t                            ThisManagerId,
                                                                     const D3D12_DESCRIPTOR_HEAP_DESC& HeapDesc) :
        DescriptorHeapAllocationManager(
            ParentAllocator,
            ThisManagerId,
            [&HeapDesc, &ParentAllocator]() -> Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> {
                Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> pd3d12DescriptorHeap;
                ParentAllocator->GetParentLinkedDevice()->GetDevice()->CreateDescriptorHeap(
                    &HeapDesc, IID_PPV_ARGS(&pd3d12DescriptorHeap));
                return pd3d12DescriptorHeap;
            }(),
            0,                      // First descriptor
            HeapDesc.NumDescriptors // Num descriptors
        )
    {}

	DescriptorHeapAllocationManager::DescriptorHeapAllocationManager(
        IDescriptorAllocator*                        ParentAllocator,
        size_t                                       ThisManagerId,
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> pd3d12DescriptorHeap,
        UINT32                                       FirstDescriptor,
        UINT32                                       NumDescriptors) :
        m_ParentAllocator {ParentAllocator}, m_ThisManagerId {ThisManagerId},
        m_HeapDesc {pd3d12DescriptorHeap->GetDesc()},
        m_DescriptorSize {
            ParentAllocator->GetParentLinkedDevice()->GetDevice()->GetDescriptorHandleIncrementSize(m_HeapDesc.Type)},
        m_NumDescriptorsInAllocation {NumDescriptors}, m_FreeBlockManager {NumDescriptors}, m_pd3d12DescriptorHeap {pd3d12DescriptorHeap}
    {
        m_FirstCPUHandle = pd3d12DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
        m_FirstCPUHandle.ptr += m_DescriptorSize * FirstDescriptor;

        if (m_HeapDesc.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
        {
            m_FirstGPUHandle = pd3d12DescriptorHeap->GetGPUDescriptorHandleForHeapStart();
            m_FirstGPUHandle.ptr += m_DescriptorSize * FirstDescriptor;
        }
    }

    DescriptorHeapAllocation DescriptorHeapAllocationManager::Allocate(uint32_t Count)
    {
        ASSERT(Count > 0);

        std::lock_guard<std::mutex> LockGuard(m_FreeBlockManagerMutex);
        // Methods of VariableSizeAllocationsManager class are not thread safe!

        // Use variable-size GPU allocations manager to allocate the requested number of descriptors
        auto Allocation = m_FreeBlockManager.Allocate(Count, 1);
        if (!Allocation.IsValid())
            return DescriptorHeapAllocation {};

        ASSERT(Allocation.Size == Count);

        // Compute the first CPU and GPU descriptor handles in the allocation by
        // offsetting the first CPU and GPU descriptor handle in the range
        auto CPUHandle = m_FirstCPUHandle;
        CPUHandle.ptr += Allocation.UnalignedOffset * m_DescriptorSize;

        auto GPUHandle = m_FirstGPUHandle; // Will be null if the heap is not GPU-visible
        if (m_HeapDesc.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
            GPUHandle.ptr += Allocation.UnalignedOffset * m_DescriptorSize;

        m_MaxAllocatedSize = std::max(m_MaxAllocatedSize, m_FreeBlockManager.GetUsedSize());

        ASSERT(m_ThisManagerId < std::numeric_limits<UINT16>::max(), "ManagerID exceeds 16-bit range");
        return DescriptorHeapAllocation(m_ParentAllocator,
                                        m_pd3d12DescriptorHeap.Get(),
                                        CPUHandle,
                                        GPUHandle,
                                        Count,
                                        static_cast<UINT16>(m_ThisManagerId));
    }

    void DescriptorHeapAllocationManager::FreeAllocation(DescriptorHeapAllocation&& Allocation)
    {
        ASSERT(Allocation.GetAllocationManagerId() == m_ThisManagerId, "Invalid descriptor heap manager Id");

        if (Allocation.IsNull())
            return;

        std::lock_guard<std::mutex> LockGuard(m_FreeBlockManagerMutex);
        auto DescriptorOffset = (Allocation.GetCpuHandle().ptr - m_FirstCPUHandle.ptr) / m_DescriptorSize;
        // Methods of VariableSizeAllocationsManager class are not thread safe!
        m_FreeBlockManager.Free(DescriptorOffset, Allocation.GetNumHandles());

        // Clear the allocation
        Allocation.Reset();
    }

	CPUDescriptorHeap::CPUDescriptorHeap(D3D12LinkedDevice*          LinkedDevice,
                                         UINT32                      NumDescriptorsInHeap,
                                         D3D12_DESCRIPTOR_HEAP_TYPE  Type,
                                         D3D12_DESCRIPTOR_HEAP_FLAGS Flags) :
        IDescriptorAllocator(LinkedDevice),
        m_HeapDesc {Type, NumDescriptorsInHeap, Flags, 1 /*NodeMask*/},
        m_DescriptorSize {LinkedDevice->GetDevice()->GetDescriptorHandleIncrementSize(Type)}
    {
        // Create one pool
        m_HeapPool.emplace_back(std::move(DescriptorHeapAllocationManager(this, 0, m_HeapDesc)));
        m_AvailableHeaps.insert(0);
    }

    DescriptorHeapAllocation CPUDescriptorHeap::Allocate(uint32_t Count)
    {
        std::lock_guard<std::mutex> LockGuard(m_HeapPoolMutex);
        // Note that every DescriptorHeapAllocationManager object instance is itself
        // thread-safe. Nested mutexes cannot cause a deadlock

        DescriptorHeapAllocation Allocation;
        // Go through all descriptor heap managers that have free descriptors
        auto AvailableHeapIt = m_AvailableHeaps.begin();
        while (AvailableHeapIt != m_AvailableHeaps.end())
        {
            auto NextIt = AvailableHeapIt;
            ++NextIt;
            // Try to allocate descriptor using the current descriptor heap manager
            Allocation = m_HeapPool[*AvailableHeapIt].Allocate(Count);
            // Remove the manager from the pool if it has no more available descriptors
            if (m_HeapPool[*AvailableHeapIt].GetNumAvailableDescriptors() == 0)
                m_AvailableHeaps.erase(*AvailableHeapIt);

            // Terminate the loop if descriptor was successfully allocated, otherwise
            // go to the next manager
            if (!Allocation.IsNull())
                break;
            AvailableHeapIt = NextIt;
        }

        // If there were no available descriptor heap managers or no manager was able
        // to suffice the allocation request, create a new manager
        if (Allocation.IsNull())
        {
            // Make sure the heap is large enough to accommodate the requested number of descriptors
            if (Count > m_HeapDesc.NumDescriptors)
            {
                std::string _logInfo = "Number of requested CPU descriptors handles ({}) exceeds the descriptor heap "
                                       "size ({}). Increasing the number of descriptors in the heap";
                LOG_INFO(_logInfo, Count, m_HeapDesc.NumDescriptors);

                //LOG_INFO("Number of requested CPU descriptors handles (",
                //         Count,
                //         ") exceeds the descriptor heap size (",
                //         m_HeapDesc.NumDescriptors,
                //         "). Increasing the number of descriptors in the heap");
            }
            m_HeapDesc.NumDescriptors = std::max(m_HeapDesc.NumDescriptors, static_cast<UINT>(Count));
            // Create a new descriptor heap manager. Note that this constructor creates a new D3D12 descriptor
            // heap and references the entire heap. Pool index is used as manager ID
            m_HeapPool.emplace_back(std::move(DescriptorHeapAllocationManager(this, m_HeapPool.size(), m_HeapDesc)));
            auto NewHeapIt = m_AvailableHeaps.insert(m_HeapPool.size() - 1);
            ASSERT(NewHeapIt.second);

            // Use the new manager to allocate descriptor handles
            Allocation = m_HeapPool[*NewHeapIt.first].Allocate(Count);
        }

        m_CurrentSize += static_cast<UINT32>(Allocation.GetNumHandles());
        m_MaxSize = std::max(m_MaxSize, m_CurrentSize);

        return Allocation;
    }

    void CPUDescriptorHeap::Free(DescriptorHeapAllocation&& Allocation)
    {
        this->FreeAllocation(std::move(Allocation));
    }

    void CPUDescriptorHeap::FreeAllocation(DescriptorHeapAllocation&& Allocation)
    {
        std::lock_guard<std::mutex> LockGuard(m_HeapPoolMutex);
        auto ManagerId = Allocation.GetAllocationManagerId();
        m_CurrentSize -= static_cast<UINT32>(Allocation.GetNumHandles());
        m_HeapPool[ManagerId].FreeAllocation(std::move(Allocation));
        // Return the manager to the pool of available managers
        ASSERT(m_HeapPool[ManagerId].GetNumAvailableDescriptors() > 0);
        m_AvailableHeaps.insert(ManagerId);
    }

	GPUDescriptorHeap::GPUDescriptorHeap(D3D12LinkedDevice*          LinkedDevice,
                                         UINT32                      NumDescriptorsInHeap,
                                         UINT32                      NumDynamicDescriptors,
                                         D3D12_DESCRIPTOR_HEAP_TYPE  Type,
                                         D3D12_DESCRIPTOR_HEAP_FLAGS Flags) :
        IDescriptorAllocator(LinkedDevice),
        m_HeapDesc {Type, NumDescriptorsInHeap + NumDynamicDescriptors, Flags, 1},
        m_pd3d12DescriptorHeap {[&]() -> Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> {
            Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> pd3d12DescriptorHeap;
            LinkedDevice->GetDevice()->CreateDescriptorHeap(&m_HeapDesc, IID_PPV_ARGS(&pd3d12DescriptorHeap));
            return pd3d12DescriptorHeap;
        }()},
        m_DescriptorSize {LinkedDevice->GetDevice()->GetDescriptorHandleIncrementSize(Type)},
        m_HeapAllocationManager(this, 0, m_pd3d12DescriptorHeap.Get(), 0, NumDescriptorsInHeap),
        m_DynamicAllocationsManager(this, 1, m_pd3d12DescriptorHeap.Get(), NumDescriptorsInHeap, NumDynamicDescriptors)
    {}

	DescriptorHeapAllocation GPUDescriptorHeap::Allocate(uint32_t Count)
    {
        return m_HeapAllocationManager.Allocate(Count);
    }

	void GPUDescriptorHeap::Free(DescriptorHeapAllocation&& Allocation)
    {
        auto MgrId = Allocation.GetAllocationManagerId();
        ASSERT(MgrId == 0 || MgrId == 1, "Unexpected allocation manager ID");

        if (MgrId == 0)
        {
            m_HeapAllocationManager.FreeAllocation(std::move(Allocation));
        }
        else
        {
            m_DynamicAllocationsManager.FreeAllocation(std::move(Allocation));
        }
    }

	UINT32 GPUDescriptorHeap::GetDescriptorSize() const { return m_DescriptorSize; }

    DescriptorHeapAllocation GPUDescriptorHeap::AllocateDynamic(uint32_t Count)
    {
        return m_DynamicAllocationsManager.Allocate(Count);
    }

    /*
	void DynamicSuballocationsManager::ReleaseAllocations(UINT32 FenceValue)
    {
        // Clear the list and dispose all allocated chunks of GPU descriptor heap.
        // The chunks will be added to release queues and eventually returned to the
        // parent GPU heap.
        for (auto& Allocation : m_Suballocations)
        {
            m_ParentGPUHeap->Free(std::move(Allocation), FenceValue);
        }
        m_Suballocations.clear();
        m_CurrDescriptorCount         = 0;
        m_CurrSuballocationsTotalSize = 0;
    }
    */

    void DynamicSuballocationsManager::RetireAllcations()
    {
        // Clear the list and dispose all allocated chunks of GPU descriptor heap.
        // The chunks will be added to release queues and eventually returned to the
        // parent GPU heap.
        for (auto& Allocation : m_Suballocations)
        {
            m_ParentGPUHeap->Free(std::move(Allocation));
        }
        m_Suballocations.clear();
        m_CurrDescriptorCount         = 0;
        m_CurrSuballocationsTotalSize = 0;
    }

    DescriptorHeapAllocation DynamicSuballocationsManager::Allocate(UINT32 Count)
    {
        // This method is intentionally lock-free as it is expected to
        // be called through device context from single thread only

        // Check if there are no chunks or the last chunk does not have enough space
        if (m_Suballocations.empty() || m_CurrentSuballocationOffset + Count > m_Suballocations.back().GetNumHandles())
        {
            // Request a new chunk from the parent GPU descriptor heap
            auto SuballocationSize       = std::max(m_DynamicChunkSize, Count);
            auto NewDynamicSubAllocation = m_ParentGPUHeap->AllocateDynamic(SuballocationSize);
            if (NewDynamicSubAllocation.IsNull())
            {
                return DescriptorHeapAllocation();
            }
            m_Suballocations.emplace_back(std::move(NewDynamicSubAllocation));
            m_CurrentSuballocationOffset = 0;

            m_CurrSuballocationsTotalSize += SuballocationSize;
            m_PeakSuballocationsTotalSize = std::max(m_PeakSuballocationsTotalSize, m_CurrSuballocationsTotalSize);
        }

        // Perform suballocation from the last chunk
        auto& CurrentSuballocation = m_Suballocations.back();

        auto ManagerId = CurrentSuballocation.GetAllocationManagerId();
        ASSERT(ManagerId < std::numeric_limits<UINT16>::max(), "ManagerID exceed allowed limit");
        DescriptorHeapAllocation Allocation(this,
                                            CurrentSuballocation.GetDescriptorHeap(),
                                            CurrentSuballocation.GetCpuHandle(m_CurrentSuballocationOffset),
                                            CurrentSuballocation.GetGpuHandle(m_CurrentSuballocationOffset),
                                            Count,
                                            static_cast<UINT16>(ManagerId));
        m_CurrentSuballocationOffset += Count;
        m_CurrDescriptorCount += Count;
        m_PeakDescriptorCount = std::max(m_PeakDescriptorCount, m_CurrDescriptorCount);

        return Allocation;
    }

    void DynamicSuballocationsManager::Free(DescriptorHeapAllocation&& Allocation)
    {
        // Do nothing. Dynamic allocations are not disposed individually, but as whole chunks
        // at the end of the frame by ReleaseAllocations()
        Allocation.Reset();
    }

    UINT32 DynamicSuballocationsManager::GetDescriptorSize() const { return m_ParentGPUHeap->GetDescriptorSize(); }

    DescriptorHandle DynamicSuballocationsManager::AllocateDescHandle()
    {
        DescriptorHeapAllocation curAllocation = this->Allocate(1);
        return curAllocation.GetDescriptorHandle();
    }

}

