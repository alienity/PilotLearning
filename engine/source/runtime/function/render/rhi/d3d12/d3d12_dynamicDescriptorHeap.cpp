#include "d3d12_dynamicDescriptorHeap.h"
#include "d3d12_linkedDevice.h"
#include "d3d12_commandContext.h"
#include "d3d12_rootSignature.h"

namespace RHI
{
    //
    // DynamicDescriptorHeap Implementation
    //

    DynamicDescriptorHeap::DynamicDescriptorHeap(D3D12LinkedDevice*   Parent,
                                                 D3D12CommandContext* POwningContext,
                                                 GPUDescriptorHeap*   PDescriptorHeap) :
        D3D12LinkedDeviceChild(Parent),
        m_OwningContext(POwningContext)
    {
        m_DescriptorSize = PDescriptorHeap->GetDescriptorSize();
        m_DescriptorType = PDescriptorHeap->GetHeapDesc().Type;

        bool isSRVHeap = m_DescriptorType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        int mDynamicChunkSize = isSRVHeap ? 256 : 16;
        std::string mManagerName =
            isSRVHeap ? "CBV_SRV_UAV dynamic descriptor allocator" : "SAMPLER dynamic descriptor allocator";

        m_DynamicSubAllocManager = new DynamicSuballocationsManager(PDescriptorHeap, mDynamicChunkSize, mManagerName);
    }

    DynamicDescriptorHeap::~DynamicDescriptorHeap()
    {
        this->CleanupUsedHeaps();
        delete m_DynamicSubAllocManager;
    }

    void DynamicDescriptorHeap::CleanupUsedHeaps()
    {
        m_DynamicSubAllocManager->RetireAllcations();
        m_GraphicsHandleCache.ClearCache();
        m_ComputeHandleCache.ClearCache();
    }

    uint32_t DynamicDescriptorHeap::DescriptorHandleCache::ComputeStagedSize()
    {
        // Sum the maximum assigned offsets of stale descriptor tables to determine total needed space.
        uint32_t NeededSpace = 0;
        uint32_t RootIndex;
        uint32_t StaleParams = m_StaleRootParamsBitMap;
        while (_BitScanForward((unsigned long*)&RootIndex, StaleParams))
        {
            StaleParams ^= (1 << RootIndex);

            uint32_t MaxSetHandle = 0;
            ASSERT(TRUE == _BitScanReverse((unsigned long*)&MaxSetHandle,
                                           m_RootDescriptorTable[RootIndex].AssignedHandlesBitMap));

            NeededSpace += MaxSetHandle + 1;
        }
        return NeededSpace;
    }

    void DynamicDescriptorHeap::DescriptorHandleCache::CopyAndBindStaleTables(
        D3D12LinkedDevice*         PDevice,
        D3D12_DESCRIPTOR_HEAP_TYPE Type,
        uint32_t                   DescriptorSize,
        DescriptorHandle           DestHandleStart,
        ID3D12GraphicsCommandList* CmdList,
        void (STDMETHODCALLTYPE ID3D12GraphicsCommandList::*SetFunc)(UINT, D3D12_GPU_DESCRIPTOR_HANDLE))
    {
        uint32_t StaleParamCount = 0;
        uint32_t TableSize[DescriptorHandleCache::kMaxNumDescriptorTables];
        uint32_t RootIndices[DescriptorHandleCache::kMaxNumDescriptorTables];
        uint32_t NeededSpace = 0;
        uint32_t RootIndex;

        // Sum the maximum assigned offsets of stale descriptor tables to determine total needed space.
        uint32_t StaleParams = m_StaleRootParamsBitMap;
        while (_BitScanForward((unsigned long*)&RootIndex, StaleParams))
        {
            RootIndices[StaleParamCount] = RootIndex;
            StaleParams ^= (1 << RootIndex);

            uint32_t MaxSetHandle = 0;
            ASSERT(TRUE == _BitScanReverse((unsigned long*)&MaxSetHandle,
                                           m_RootDescriptorTable[RootIndex].AssignedHandlesBitMap));

            NeededSpace += MaxSetHandle + 1;
            TableSize[StaleParamCount] = MaxSetHandle + 1;

            ++StaleParamCount;
        }

        ASSERT(StaleParamCount <= DescriptorHandleCache::kMaxNumDescriptorTables);

        m_StaleRootParamsBitMap = 0;

        static const uint32_t       kMaxDescriptorsPerCopy  = 16;
        UINT                        NumDestDescriptorRanges = 0;
        D3D12_CPU_DESCRIPTOR_HANDLE pDestDescriptorRangeStarts[kMaxDescriptorsPerCopy];
        UINT                        pDestDescriptorRangeSizes[kMaxDescriptorsPerCopy];

        UINT                        NumSrcDescriptorRanges = 0;
        D3D12_CPU_DESCRIPTOR_HANDLE pSrcDescriptorRangeStarts[kMaxDescriptorsPerCopy];
        UINT                        pSrcDescriptorRangeSizes[kMaxDescriptorsPerCopy];

        DescriptorHandle DestHandleStartArray[DescriptorHandleCache::kMaxNumDescriptorTables];
        
        for (uint32_t i = 0; i < StaleParamCount; ++i)
        {
            RootIndex = RootIndices[i];
            DestHandleStartArray[i] = DestHandleStart;
            
            DescriptorTableCache& RootDescTable = m_RootDescriptorTable[RootIndex];

            D3D12_CPU_DESCRIPTOR_HANDLE* SrcHandles = RootDescTable.TableStart;
            uint64_t                     SetHandles = (uint64_t)RootDescTable.AssignedHandlesBitMap;
            D3D12_CPU_DESCRIPTOR_HANDLE  CurDest    = DestHandleStart;
            DestHandleStart += TableSize[i] * DescriptorSize;

            unsigned long SkipCount;
            while (_BitScanForward64(&SkipCount, SetHandles))
            {
                // Skip over unset descriptor handles
                SetHandles >>= SkipCount;
                SrcHandles += SkipCount;
                CurDest.ptr += SkipCount * DescriptorSize;

                unsigned long DescriptorCount;
                _BitScanForward64(&DescriptorCount, ~SetHandles);
                SetHandles >>= DescriptorCount;

                // If we run out of temp room, copy what we've got so far
                if (NumSrcDescriptorRanges + DescriptorCount > kMaxDescriptorsPerCopy)
                {
                    // https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_descriptor_heap_flags
                    // Descriptor heaps with the D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE flag 
                    // can't be used as the source heaps in calls to ID3D12Device::CopyDescriptors or 
                    // ID3D12Device::CopyDescriptorsSimple, because they could be resident in 
                    // WRITE_COMBINE memory or GPU-local memory, which is very inefficient to read from.
                    PDevice->GetDevice()->CopyDescriptors(NumDestDescriptorRanges,
                                                          pDestDescriptorRangeStarts,
                                                          pDestDescriptorRangeSizes,
                                                          NumSrcDescriptorRanges,
                                                          pSrcDescriptorRangeStarts,
                                                          pSrcDescriptorRangeSizes,
                                                          Type);

                    NumSrcDescriptorRanges  = 0;
                    NumDestDescriptorRanges = 0;
                }

                // Setup destination range
                pDestDescriptorRangeStarts[NumDestDescriptorRanges] = CurDest;
                pDestDescriptorRangeSizes[NumDestDescriptorRanges]  = DescriptorCount;
                ++NumDestDescriptorRanges;

                // Setup source ranges (one descriptor each because we don't assume they are contiguous)
                for (uint32_t j = 0; j < DescriptorCount; ++j)
                {
                    pSrcDescriptorRangeStarts[NumSrcDescriptorRanges] = SrcHandles[j];
                    pSrcDescriptorRangeSizes[NumSrcDescriptorRanges]  = 1;
                    ++NumSrcDescriptorRanges;
                }

                // Move the destination pointer forward by the number of descriptors we will copy
                SrcHandles += DescriptorCount;
                CurDest.ptr += DescriptorCount * DescriptorSize;
            }
        }

        PDevice->GetDevice()->CopyDescriptors(NumDestDescriptorRanges,
                                              pDestDescriptorRangeStarts,
                                              pDestDescriptorRangeSizes,
                                              NumSrcDescriptorRanges,
                                              pSrcDescriptorRangeStarts,
                                              pSrcDescriptorRangeSizes,
                                              Type);

        for (uint32_t i = 0; i < StaleParamCount; ++i)
        {
            (CmdList->*SetFunc)(RootIndex, DestHandleStartArray[i]);
        }
    }

    void DynamicDescriptorHeap::CopyAndBindStagedTables(
        DescriptorHandleCache&     HandleCache,
        ID3D12GraphicsCommandList* CmdList,
        void (STDMETHODCALLTYPE ID3D12GraphicsCommandList::*SetFunc)(UINT, D3D12_GPU_DESCRIPTOR_HANDLE))
    {
        uint32_t NeededSize = HandleCache.ComputeStagedSize();
        HandleCache.CopyAndBindStaleTables(
            this->GetParentLinkedDevice(), m_DescriptorType, m_DescriptorSize, Allocate(NeededSize), CmdList, SetFunc);
    }

    void DynamicDescriptorHeap::UnbindAllValid(void)
    {
        m_GraphicsHandleCache.UnbindAllValid();
        m_ComputeHandleCache.UnbindAllValid();
    }

    void DynamicDescriptorHeap::DescriptorHandleCache::UnbindAllValid()
    {
        m_StaleRootParamsBitMap = 0;

        unsigned long TableParams = m_RootDescriptorTablesBitMap;
        unsigned long RootIndex;
        while (_BitScanForward(&RootIndex, TableParams))
        {
            TableParams ^= (1 << RootIndex);
            if (m_RootDescriptorTable[RootIndex].AssignedHandlesBitMap != 0)
                m_StaleRootParamsBitMap |= (1 << RootIndex);
        }
    }

    void
    DynamicDescriptorHeap::DescriptorHandleCache::StageDescriptorHandles(UINT                              RootIndex,
                                                                         UINT                              Offset,
                                                                         UINT                              NumHandles,
                                                                         const D3D12_CPU_DESCRIPTOR_HANDLE Handles[])
    {
        ASSERT(((1 << RootIndex) & m_RootDescriptorTablesBitMap) != 0);
        ASSERT(Offset + NumHandles <= m_RootDescriptorTable[RootIndex].TableSize);

        DescriptorTableCache&        TableCache = m_RootDescriptorTable[RootIndex];
        D3D12_CPU_DESCRIPTOR_HANDLE* CopyDest   = TableCache.TableStart + Offset;
        for (UINT i = 0; i < NumHandles; ++i)
            CopyDest[i] = Handles[i];
        TableCache.AssignedHandlesBitMap |= ((1 << NumHandles) - 1) << Offset;
        m_StaleRootParamsBitMap |= (1 << RootIndex);
    }

    void DynamicDescriptorHeap::DescriptorHandleCache::ParseRootSignature(D3D12_DESCRIPTOR_HEAP_TYPE Type,
                                                                          const D3D12RootSignature*  RootSig)
    {
        UINT CurrentOffset = 0;

        ASSERT(RootSig->GetNumParameters() <= 16);

        m_StaleRootParamsBitMap = 0;

        m_RootDescriptorTablesBitMap = RootSig->GetDescriptorTableBitMask(Type);

        unsigned long TableParams = m_RootDescriptorTablesBitMap;
        unsigned long RootIndex;
        while (_BitScanForward(&RootIndex, TableParams))
        {
            TableParams ^= (1 << RootIndex);

            UINT TableSize = RootSig->GetDescriptorTableSize(RootIndex);
            ASSERT(TableSize > 0);

            DescriptorTableCache& RootDescriptorTable = m_RootDescriptorTable[RootIndex];
            RootDescriptorTable.AssignedHandlesBitMap = 0;
            RootDescriptorTable.TableStart            = m_HandleCache + CurrentOffset;
            RootDescriptorTable.TableSize             = TableSize;

            CurrentOffset += TableSize;
        }

        m_MaxCachedDescriptors = CurrentOffset;

        ASSERT(m_MaxCachedDescriptors <= kMaxNumDescriptors);
    }

}

