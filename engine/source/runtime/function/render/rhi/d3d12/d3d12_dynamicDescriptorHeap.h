#pragma once
#include "d3d12_core.h"
#include "d3d12_rootSignature.h"
#include "d3d12_descriptorHeap.h"

#include <vector>
#include <queue>

namespace RHI
{

    class D3D12CommandContext;

	// This class is a linear allocation system for dynamically generated descriptor tables.  It internally caches
    // CPU descriptor handles so that when not enough space is available in the current heap, necessary descriptors
    // can be re-copied to the new heap.
    class DynamicDescriptorHeap : public D3D12LinkedDeviceChild
    {
    public:
        DynamicDescriptorHeap(D3D12LinkedDevice*         Parent,
                              D3D12CommandContext&       OwningContext,
                              D3D12_DESCRIPTOR_HEAP_TYPE HeapType);
        ~DynamicDescriptorHeap();

        void CleanupUsedHeaps();

        // Copy multiple handles into the cache area reserved for the specified root parameter.
        void SetGraphicsDescriptorHandles(UINT                              RootIndex,
                                          UINT                              Offset,
                                          UINT                              NumHandles,
                                          const D3D12_CPU_DESCRIPTOR_HANDLE Handles[])
        {
            m_GraphicsHandleCache.StageDescriptorHandles(RootIndex, Offset, NumHandles, Handles);
        }

        void SetComputeDescriptorHandles(UINT                              RootIndex,
                                         UINT                              Offset,
                                         UINT                              NumHandles,
                                         const D3D12_CPU_DESCRIPTOR_HANDLE Handles[])
        {
            m_ComputeHandleCache.StageDescriptorHandles(RootIndex, Offset, NumHandles, Handles);
        }

        // Bypass the cache and upload directly to the shader-visible heap
        D3D12_GPU_DESCRIPTOR_HANDLE UploadDirect(D3D12_CPU_DESCRIPTOR_HANDLE Handles);

        // Deduce cache layout needed to support the descriptor tables needed by the root signature.
        void ParseGraphicsRootSignature(const D3D12RootSignature& RootSig)
        {
            m_GraphicsHandleCache.ParseRootSignature(m_DescriptorType, RootSig);
        }

        void ParseComputeRootSignature(const D3D12RootSignature& RootSig)
        {
            m_ComputeHandleCache.ParseRootSignature(m_DescriptorType, RootSig);
        }

        // Upload any new descriptors in the cache to the shader-visible heap.
        inline void CommitGraphicsRootDescriptorTables(ID3D12GraphicsCommandList* CmdList)
        {
            if (m_GraphicsHandleCache.m_StaleRootParamsBitMap != 0)
                CopyAndBindStagedTables(
                    m_GraphicsHandleCache, CmdList, &ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable);
        }

        inline void CommitComputeRootDescriptorTables(ID3D12GraphicsCommandList* CmdList)
        {
            if (m_ComputeHandleCache.m_StaleRootParamsBitMap != 0)
                CopyAndBindStagedTables(
                    m_ComputeHandleCache, CmdList, &ID3D12GraphicsCommandList::SetComputeRootDescriptorTable);
        }

    private:
        // Static members
        static const UINT32 kNumDescriptorsPerHeap = 1024;
        static std::mutex     sm_Mutex;

        // Non-static members
        D3D12CommandContext*               m_OwningContext;
        ID3D12DescriptorHeap*              m_CurrentHeapPtr;
        const D3D12_DESCRIPTOR_HEAP_TYPE   m_DescriptorType;
        UINT32                             m_DescriptorSize;
        UINT32                             m_CurrentOffset;
        DescriptorHandle                   m_FirstDescriptor;

        // Describes a descriptor table entry:  a region of the handle cache and which handles have been set
        struct DescriptorTableCache
        {
            DescriptorTableCache() : AssignedHandlesBitMap(0), TableStart {0}, TableSize {0} {}
            UINT32                       AssignedHandlesBitMap;
            D3D12_CPU_DESCRIPTOR_HANDLE* TableStart;
            UINT32                       TableSize;
        };

        struct DescriptorHandleCache
        {
            DescriptorHandleCache() { ClearCache(); }

            void ClearCache()
            {
                m_RootDescriptorTablesBitMap = 0;
                m_StaleRootParamsBitMap      = 0;
                m_MaxCachedDescriptors       = 0;
            }

            UINT32 m_RootDescriptorTablesBitMap;
            UINT32 m_StaleRootParamsBitMap;
            UINT32 m_MaxCachedDescriptors;

            static const UINT32 kMaxNumDescriptors      = 256;
            static const UINT32 kMaxNumDescriptorTables = 16;

            UINT32 ComputeStagedSize();
            void   CopyAndBindStaleTables(
                  D3D12_DESCRIPTOR_HEAP_TYPE Type,
                  UINT32                     DescriptorSize,
                  DescriptorHandle           DestHandleStart,
                  ID3D12GraphicsCommandList* CmdList,
                  void (STDMETHODCALLTYPE ID3D12GraphicsCommandList::*SetFunc)(UINT, D3D12_GPU_DESCRIPTOR_HANDLE));

            DescriptorTableCache        m_RootDescriptorTable[kMaxNumDescriptorTables];
            D3D12_CPU_DESCRIPTOR_HANDLE m_HandleCache[kMaxNumDescriptors];

            void UnbindAllValid();
            void StageDescriptorHandles(UINT                              RootIndex,
                                        UINT                              Offset,
                                        UINT                              NumHandles,
                                        const D3D12_CPU_DESCRIPTOR_HANDLE Handles[]);
            void ParseRootSignature(D3D12_DESCRIPTOR_HEAP_TYPE Type, const D3D12RootSignature& RootSig);
        };

        DescriptorHandleCache m_GraphicsHandleCache;
        DescriptorHandleCache m_ComputeHandleCache;

        bool HasSpace(UINT32 Count)
        {
            return (m_CurrentHeapPtr != nullptr && m_CurrentOffset + Count <= kNumDescriptorsPerHeap);
        }

        ID3D12DescriptorHeap* GetHeapPointer();

        DescriptorHandle Allocate(UINT Count)
        {
            DescriptorHandle ret = m_FirstDescriptor + m_CurrentOffset * m_DescriptorSize;
            m_CurrentOffset += Count;
            return ret;
        }

        void CopyAndBindStagedTables(
            DescriptorHandleCache&     HandleCache,
            ID3D12GraphicsCommandList* CmdList,
            void (STDMETHODCALLTYPE ID3D12GraphicsCommandList::*SetFunc)(UINT, D3D12_GPU_DESCRIPTOR_HANDLE));

        // Mark all descriptors in the cache as stale and in need of re-uploading.
        void UnbindAllValid(void);
    };


}