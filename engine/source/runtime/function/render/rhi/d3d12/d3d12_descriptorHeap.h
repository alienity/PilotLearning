#pragma once
#include "d3d12_core.h"
#include "d3d12_linkedDevice.h"

#include <mutex>
#include <vector>
#include <queue>
#include <string>
#include <unordered_set>
#include <atomic>
#include <map>

namespace RHI
{
    class DescriptorHeapAllocation;
    class DescriptorHeapAllocationManager;
    class CPUDescriptorHeap;
    class GPUDescriptorHeap;
    class DynamicSuballocationsManager;
    class VariableSizeAllocationsManager;


    // This handle refers to a descriptor or a descriptor table (contiguous descriptors) that is shader visible.
    class DescriptorHandle
    {
    public:
        DescriptorHandle()
        {
            m_CpuHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
            m_GpuHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
        }

        DescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle) :
            m_CpuHandle(CpuHandle), m_GpuHandle(GpuHandle)
        {}

        DescriptorHandle operator+(INT OffsetScaledByDescriptorSize) const
        {
            DescriptorHandle ret = *this;
            ret += OffsetScaledByDescriptorSize;
            return ret;
        }

        void operator+=(INT OffsetScaledByDescriptorSize)
        {
            if (m_CpuHandle.ptr != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
                m_CpuHandle.ptr += OffsetScaledByDescriptorSize;
            if (m_GpuHandle.ptr != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
                m_GpuHandle.ptr += OffsetScaledByDescriptorSize;
        }

        inline const D3D12_CPU_DESCRIPTOR_HANDLE* operator&() const { return &m_CpuHandle; }
        operator D3D12_CPU_DESCRIPTOR_HANDLE() const { return m_CpuHandle; }
        operator D3D12_GPU_DESCRIPTOR_HANDLE() const { return m_GpuHandle; }

        inline size_t   GetCpuPtr() const { return m_CpuHandle.ptr; }
        inline uint64_t GetGpuPtr() const { return m_GpuHandle.ptr; }
        inline bool     IsNull() const { return m_CpuHandle.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN; }
        inline bool     IsShaderVisible() const { return m_GpuHandle.ptr != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN; }

    private:
        D3D12_CPU_DESCRIPTOR_HANDLE m_CpuHandle;
        D3D12_GPU_DESCRIPTOR_HANDLE m_GpuHandle;
    };

    // The class handles free memory block management to accommodate variable-size allocation requests.
    // It keeps track of free blocks only and does not record allocation sizes. The class uses two ordered maps
    // to facilitate operations. The first map keeps blocks sorted by their offsets. The second multimap keeps blocks
    // sorted by their sizes. The elements of the two maps reference each other, which enables efficient block
    // insertion, removal and merging.
    //
    //   8                 32                       64                           104
    //   |<---16--->|       |<-----24------>|        |<---16--->|                 |<-----32----->|
    //
    //
    //        m_FreeBlocksBySize      m_FreeBlocksByOffset
    //           size->offset            offset->size
    //
    //                16 ------------------>  8  ---------->  {size = 16, &m_FreeBlocksBySize[0]}
    //
    //                16 ------.   .-------> 32  ---------->  {size = 24, &m_FreeBlocksBySize[2]}
    //                          '.'
    //                24 -------' '--------> 64  ---------->  {size = 16, &m_FreeBlocksBySize[1]}
    //
    //                32 ------------------> 104 ---------->  {size = 32, &m_FreeBlocksBySize[3]}
    //
    class VariableSizeAllocationsManager
    {
    public:
        using OffsetType = size_t;

    private:
        struct FreeBlockInfo;

        // Type of the map that keeps memory blocks sorted by their offsets
        using TFreeBlocksByOffsetMap = std::map<OffsetType, FreeBlockInfo>;

        // Type of the map that keeps memory blocks sorted by their sizes
        using TFreeBlocksBySizeMap = std::multimap<OffsetType, TFreeBlocksByOffsetMap::iterator>;

        struct FreeBlockInfo
        {
            // Block size (no reserved space for the size of the allocation)
            OffsetType Size;

            // Iterator referencing this block in the multimap sorted by the block size
            TFreeBlocksBySizeMap::iterator OrderBySizeIt;

            FreeBlockInfo(OffsetType _Size) : Size(_Size) {}
        };

    public:
        VariableSizeAllocationsManager(OffsetType MaxSize) : m_MaxSize(MaxSize), m_FreeSize(MaxSize)
        {
            // Insert single maximum-size block
            AddNewBlock(0, m_MaxSize);
            ResetCurrAlignment();
        }

        ~VariableSizeAllocationsManager() {}

        // clang-format off
        VariableSizeAllocationsManager(VariableSizeAllocationsManager&& rhs) noexcept :
            m_FreeBlocksByOffset{ std::move(rhs.m_FreeBlocksByOffset) },
            m_FreeBlocksBySize{ std::move(rhs.m_FreeBlocksBySize) },
            m_MaxSize{ rhs.m_MaxSize },
            m_FreeSize{ rhs.m_FreeSize },
            m_CurrAlignment{ rhs.m_CurrAlignment }
        {
            // clang-format on
            rhs.m_MaxSize       = 0;
            rhs.m_FreeSize      = 0;
            rhs.m_CurrAlignment = 0;
        }

        // clang-format off
        VariableSizeAllocationsManager& operator = (VariableSizeAllocationsManager&& rhs) = default;
        VariableSizeAllocationsManager(const VariableSizeAllocationsManager&) = delete;
        VariableSizeAllocationsManager& operator = (const VariableSizeAllocationsManager&) = delete;
        // clang-format on

        // Offset returned by Allocate() may not be aligned, but the size of the allocation
        // is sufficient to properly align it
        struct Allocation
        {
            // clang-format off
            Allocation(OffsetType offset, OffsetType size) :
                UnalignedOffset{ offset },
                Size{ size }
            {}
            // clang-format on

            Allocation() {}

            static constexpr OffsetType InvalidOffset = ~OffsetType {0};
            static Allocation           InvalidAllocation() { return Allocation {InvalidOffset, 0}; }

            inline bool IsValid() const { return UnalignedOffset != InvalidAllocation().UnalignedOffset; }

            inline bool operator==(const Allocation& rhs) const
            {
                return UnalignedOffset == rhs.UnalignedOffset && Size == rhs.Size;
            }

            OffsetType UnalignedOffset = InvalidOffset;
            OffsetType Size            = 0;
        };

        Allocation Allocate(OffsetType Size, OffsetType Alignment);
        void       Free(Allocation&& allocation);
        void       Free(OffsetType Offset, OffsetType Size);

        // clang-format off
        inline bool IsFull() const { return m_FreeSize == 0; };
        inline bool IsEmpty()const { return m_FreeSize == m_MaxSize; };
        inline OffsetType GetMaxSize() const { return m_MaxSize; }
        inline OffsetType GetFreeSize()const { return m_FreeSize; }
        inline OffsetType GetUsedSize()const { return m_MaxSize - m_FreeSize; }
        // clang-format on

        size_t GetNumFreeBlocks() const;

        void Extend(size_t ExtraSize);

    private:
        void AddNewBlock(OffsetType Offset, OffsetType Size);
        void ResetCurrAlignment();

        TFreeBlocksByOffsetMap m_FreeBlocksByOffset;
        TFreeBlocksBySizeMap   m_FreeBlocksBySize;

        OffsetType m_MaxSize       = 0;
        OffsetType m_FreeSize      = 0;
        OffsetType m_CurrAlignment = 0;
        // When adding new members, do not forget to update move ctor
    };

    class IDescriptorAllocator : public D3D12LinkedDeviceChild
    {
    public:
        IDescriptorAllocator(D3D12LinkedDevice* Parent) : D3D12LinkedDeviceChild(Parent) {}

        // Allocate Count descriptors
        virtual DescriptorHeapAllocation Allocate(uint32_t Count)                    = 0;
        virtual void                     Free(DescriptorHeapAllocation&& Allocation) = 0;
        virtual UINT32                   GetDescriptorSize() const                   = 0;
    };
    
    // The class represents descriptor heap allocation (continuous descriptor range in a descriptor heap)
    //
    //                  m_FirstCpuHandle
    //                   |
    //  | ~  ~  ~  ~  ~  X  X  X  X  X  X  X  ~  ~  ~  ~  ~  ~ |  D3D12 Descriptor Heap
    //                   |
    //                  m_FirstGpuHandle
    //
    class DescriptorHeapAllocation
    {
    public:
        // Creates null allocation
        DescriptorHeapAllocation() :
            // clang-format off
            m_NumHandles{ 1 }, // One null descriptor handle
            m_pDescriptorHeap{ nullptr },
            m_DescriptorSize{ 0 },
            m_pAllocator { nullptr }
        {
            m_FirstCpuHandle.ptr = 0;
            m_FirstGpuHandle.ptr = 0;
            m_DescriptorHandleOffsetIndex = InvalidDescriptorHandleOffsetIndex;
        }

        // Initializes non-null allocation
        DescriptorHeapAllocation(
            IDescriptorAllocator* Allocator,
            ID3D12DescriptorHeap* pHeap,
            D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle,
            D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle,
            UINT32                      NHandles,
            UINT16                      AllocationManagerId) noexcept :
            // clang-format off
            m_FirstCpuHandle{ CpuHandle },
            m_FirstGpuHandle{ GpuHandle },
            m_pAllocator{ Allocator },
            m_NumHandles{ NHandles },
            m_pDescriptorHeap{ pHeap },
            m_AllocationManagerId{ AllocationManagerId }
        {
            ASSERT(m_pAllocator != nullptr && m_pDescriptorHeap != nullptr);
            auto DescriptorSize = m_pAllocator->GetDescriptorSize();
            ASSERT(DescriptorSize < std::numeric_limits<UINT16>::max(), "DescriptorSize exceeds allowed limit");
            m_DescriptorSize = static_cast<UINT16>(DescriptorSize);
            D3D12_CPU_DESCRIPTOR_HANDLE m_HeapStartHandle = m_pDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
            m_DescriptorHandleOffsetIndex = (m_FirstCpuHandle.ptr - m_HeapStartHandle.ptr) / m_DescriptorSize;
        }

        // Move constructor (copy is not allowed)
        DescriptorHeapAllocation(DescriptorHeapAllocation&& Allocation) noexcept :
            // clang-format off
            m_FirstCpuHandle{ std::move(Allocation.m_FirstCpuHandle) },
            m_FirstGpuHandle{ std::move(Allocation.m_FirstGpuHandle) },
            m_NumHandles{ std::move(Allocation.m_NumHandles) },
            m_pAllocator{ std::move(Allocation.m_pAllocator) },
            m_AllocationManagerId{ std::move(Allocation.m_AllocationManagerId) },
            m_pDescriptorHeap{ std::move(Allocation.m_pDescriptorHeap) },
            m_DescriptorSize{ std::move(Allocation.m_DescriptorSize) },
            m_DescriptorHandleOffsetIndex{ std::move(Allocation.m_DescriptorHandleOffsetIndex) }
        // clang-format on
        {
            Allocation.Reset();
        }

        // Move assignment (assignment is not allowed)
        DescriptorHeapAllocation& operator=(DescriptorHeapAllocation&& Allocation) noexcept
        {
            m_FirstCpuHandle              = std::move(Allocation.m_FirstCpuHandle);
            m_FirstGpuHandle              = std::move(Allocation.m_FirstGpuHandle);
            m_NumHandles                  = std::move(Allocation.m_NumHandles);
            m_pAllocator                  = std::move(Allocation.m_pAllocator);
            m_AllocationManagerId         = std::move(Allocation.m_AllocationManagerId);
            m_pDescriptorHeap             = std::move(Allocation.m_pDescriptorHeap);
            m_DescriptorSize              = std::move(Allocation.m_DescriptorSize);
            m_DescriptorHandleOffsetIndex = std::move(Allocation.m_DescriptorHandleOffsetIndex);

            Allocation.Reset();

            return *this;
        }

        void Reset()
        {
            m_FirstCpuHandle.ptr          = 0;
            m_FirstGpuHandle.ptr          = 0;
            m_pAllocator                  = nullptr;
            m_pDescriptorHeap             = nullptr;
            m_NumHandles                  = 0;
            m_AllocationManagerId         = InvalidAllocationMgrId;
            m_DescriptorSize              = 0;
            m_DescriptorHandleOffsetIndex = InvalidDescriptorHandleOffsetIndex;
        }

        // clang-format off
        DescriptorHeapAllocation(const DescriptorHeapAllocation&) = delete;
        DescriptorHeapAllocation& operator=(const DescriptorHeapAllocation&) = delete;
        // clang-format on

        void ReleaseDescriptorHeapAllocation();

        // Destructor automatically releases this allocation through the allocator
        ~DescriptorHeapAllocation()
        {
            this->ReleaseDescriptorHeapAllocation();
        }

        // Returns CPU descriptor handle at the specified offset
        D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(UINT32 Offset = 0) const;

        // Returns GPU descriptor handle at the specified offset
        D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(UINT32 Offset = 0) const;

        // Returns DescriptorHandle at the specified offset
        DescriptorHandle GetDescriptorHandle(UINT32 Offset = 0) const;

        template<typename HandleType>
        HandleType GetHandle(UINT32 Offset = 0) const;

        template<>
        D3D12_CPU_DESCRIPTOR_HANDLE GetHandle<D3D12_CPU_DESCRIPTOR_HANDLE>(UINT32 Offset) const
        {
            return GetCpuHandle(Offset);
        }

        template<>
        D3D12_GPU_DESCRIPTOR_HANDLE GetHandle<D3D12_GPU_DESCRIPTOR_HANDLE>(UINT32 Offset) const
        {
            return GetGpuHandle(Offset);
        }

        template<>
        DescriptorHandle GetHandle<DescriptorHandle>(UINT32 Offset) const
        {
            return GetDescriptorHandle(Offset);
        }

        // Returns pointer to D3D12 descriptor heap that contains this allocation
        inline ID3D12DescriptorHeap* GetDescriptorHeap() const { return m_pDescriptorHeap; }

        // Retreive the first handle offset in descriptor heap
        UINT GetDescriptorHeapOffsetIndex(UINT32 Offset = 0) const;
        
        // clang-format off
        inline size_t GetNumHandles()          const { return m_NumHandles; }
        inline bool   IsNull()                 const { return m_FirstCpuHandle.ptr == 0; }
        inline bool   IsShaderVisible()        const { return m_FirstGpuHandle.ptr != 0; }
        inline size_t GetAllocationManagerId() const { return m_AllocationManagerId; }
        inline UINT   GetDescriptorSize()      const { return m_DescriptorSize; }
        // clang-format on

    private:
        // First CPU descriptor handle in this allocation
        D3D12_CPU_DESCRIPTOR_HANDLE m_FirstCpuHandle = {0};

        // First GPU descriptor handle in this allocation
        D3D12_GPU_DESCRIPTOR_HANDLE m_FirstGpuHandle = {0};

        // Keep strong reference to the parent heap to make sure it is alive while allocation is alive - TOO EXPENSIVE
        // RefCntAutoPtr<IDescriptorAllocator> m_pAllocator;

        // Pointer to the descriptor heap allocator that created this allocation
        IDescriptorAllocator* m_pAllocator = nullptr;

        // Pointer to the D3D12 descriptor heap that contains descriptors in this allocation
        ID3D12DescriptorHeap* m_pDescriptorHeap = nullptr;

        // Number of descriptors in the allocation
        UINT32 m_NumHandles = 0;

        // The handle offset of this allocation in descriptorheap
        UINT32 m_DescriptorHandleOffsetIndex = InvalidDescriptorHandleOffsetIndex;

        static constexpr UINT32 InvalidDescriptorHandleOffsetIndex = 0xFFFFFFFF;

        static constexpr UINT16 InvalidAllocationMgrId = 0xFFFF;

        // Allocation manager ID. One allocator may support several
        // allocation managers. This field is required to identify
        // the manager within the allocator that was used to create
        // this allocation
        UINT16 m_AllocationManagerId = InvalidAllocationMgrId;

        // Descriptor size
        UINT16 m_DescriptorSize = 0;
    };

    // The class performs suballocations within one D3D12 descriptor heap.
    // It uses VariableSizeAllocationsManager to manage free space in the heap
    //
    // |  X  X  X  X  O  O  O  X  X  O  O  X  O  O  O  O  |  D3D12 descriptor heap
    //
    //  X - used descriptor
    //  O - available descriptor
    //
    class DescriptorHeapAllocationManager
    {
    public:
        // Creates a new D3D12 descriptor heap
        DescriptorHeapAllocationManager(IDescriptorAllocator*             ParentAllocator,
                                        size_t                            ThisManagerId,
                                        const D3D12_DESCRIPTOR_HEAP_DESC& HeapDesc);

        // Uses subrange of descriptors in the existing D3D12 descriptor heap
        // that starts at offset FirstDescriptor and uses NumDescriptors descriptors
        DescriptorHeapAllocationManager(IDescriptorAllocator*                        ParentAllocator,
                                        size_t                                       ThisManagerId,
                                        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> pd3d12DescriptorHeap,
                                        UINT32                                       FirstDescriptor,
                                        UINT32                                       NumDescriptors);

        // = default causes compiler error when instantiating std::vector::emplace_back() in Visual Studio 2015
        // (Version 14.0.23107.0 D14REL)
        DescriptorHeapAllocationManager(DescriptorHeapAllocationManager&& rhs) noexcept :
            // clang-format off
            m_ParentAllocator{ rhs.m_ParentAllocator },
            m_ThisManagerId{ rhs.m_ThisManagerId },
            m_HeapDesc{ rhs.m_HeapDesc },
            m_DescriptorSize{ rhs.m_DescriptorSize },
            m_NumDescriptorsInAllocation{ rhs.m_NumDescriptorsInAllocation },
            m_FirstCPUHandle{ rhs.m_FirstCPUHandle },
            m_FirstGPUHandle{ rhs.m_FirstGPUHandle },
            m_MaxAllocatedSize{ rhs.m_MaxAllocatedSize },
            // Mutex is not movable
            //m_FreeBlockManagerMutex     (std::move(rhs.m_FreeBlockManagerMutex))
            m_FreeBlockManager{ std::move(rhs.m_FreeBlockManager) },
            m_pd3d12DescriptorHeap{ std::move(rhs.m_pd3d12DescriptorHeap) }
        // clang-format on
        {
            rhs.m_NumDescriptorsInAllocation = 0; // Must be set to zero so that debug check in dtor passes
            rhs.m_ThisManagerId              = static_cast<size_t>(-1);
            rhs.m_FirstCPUHandle.ptr         = 0;
            rhs.m_FirstGPUHandle.ptr         = 0;
            rhs.m_MaxAllocatedSize           = 0;
        }

        // clang-format off
        // No copies or move-assignments
        DescriptorHeapAllocationManager& operator = (DescriptorHeapAllocationManager&&) = delete;
        DescriptorHeapAllocationManager(const DescriptorHeapAllocationManager&) = delete;
        DescriptorHeapAllocationManager& operator = (const DescriptorHeapAllocationManager&) = delete;
        // clang-format on

        ~DescriptorHeapAllocationManager()
        {
            // ASSERT(m_AllocationsCounter == 0, m_AllocationsCounter, " allocations have not been released. If these
            // allocations are referenced by release queue, the app will crash when
            // DescriptorHeapAllocationManager::FreeAllocation() is called.");
            ASSERT(m_FreeBlockManager.GetFreeSize() == m_NumDescriptorsInAllocation,
                   "Not all descriptors were released");
        }

        // Allocates Count descriptors
        DescriptorHeapAllocation Allocate(uint32_t Count);

        void FreeAllocation(DescriptorHeapAllocation&& Allocation);

        // clang-format off
        inline size_t GetNumAvailableDescriptors()const { return m_FreeBlockManager.GetFreeSize(); }
        inline UINT32 GetMaxDescriptors()         const { return m_NumDescriptorsInAllocation; }
        inline size_t GetMaxAllocatedSize()       const { return m_MaxAllocatedSize; }
        // clang-format on

    private:
        IDescriptorAllocator* m_ParentAllocator;
        
        // External ID assigned to this descriptor allocations manager
        size_t m_ThisManagerId = static_cast<size_t>(-1);

        // Heap description
        const D3D12_DESCRIPTOR_HEAP_DESC m_HeapDesc;

        const UINT m_DescriptorSize = 0;

        // Number of descriptors in the allocation.
        // If this manager was initialized as a subrange in the existing heap,
        // this value may be different from m_HeapDesc.NumDescriptors
        UINT32 m_NumDescriptorsInAllocation = 0;

        // Allocations manager used to handle descriptor allocations within the heap
        std::mutex                     m_FreeBlockManagerMutex;
        VariableSizeAllocationsManager m_FreeBlockManager;

        // Strong reference to D3D12 descriptor heap object
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_pd3d12DescriptorHeap;

        // First CPU descriptor handle in the available descriptor range
        D3D12_CPU_DESCRIPTOR_HANDLE m_FirstCPUHandle = {0};

        // First GPU descriptor handle in the available descriptor range
        D3D12_GPU_DESCRIPTOR_HANDLE m_FirstGPUHandle = {0};

        size_t m_MaxAllocatedSize = 0;

        // Note: when adding new members, do not forget to update move ctor
    };

    // CPU descriptor heap is intended to provide storage for resource view descriptor handles.
    // It contains a pool of DescriptorHeapAllocationManager object instances, where every instance manages
    // its own CPU-only D3D12 descriptor heap:
    //
    //           m_HeapPool[0]                m_HeapPool[1]                 m_HeapPool[2]
    //   |  X  X  X  X  X  X  X  X |, |  X  X  X  O  O  X  X  O  |, |  X  O  O  O  O  O  O  O  |
    //
    //    X - used descriptor                m_AvailableHeaps = {1,2}
    //    O - available descriptor
    //
    // Allocation routine goes through the list of managers that have available descriptors and tries to process
    // the request using every manager. If there are no available managers or no manager was able to handle the request,
    // the function creates a new descriptor heap manager and lets it handle the request
    //
    // Render device contains four CPUDescriptorHeap object instances (one for each D3D12 heap type). The heaps are accessed
    // when a texture or a buffer view is created.
    //
    class CPUDescriptorHeap final : public IDescriptorAllocator
    {
    public:
        // Initializes the heap
        CPUDescriptorHeap(D3D12LinkedDevice*          LinkedDevice,
                          UINT32                      NumDescriptorsInHeap,
                          D3D12_DESCRIPTOR_HEAP_TYPE  Type,
                          D3D12_DESCRIPTOR_HEAP_FLAGS Flags);

        // clang-format off
        CPUDescriptorHeap(const CPUDescriptorHeap&) = delete;
        CPUDescriptorHeap(CPUDescriptorHeap&&) = delete;
        CPUDescriptorHeap& operator = (const CPUDescriptorHeap&) = delete;
        CPUDescriptorHeap& operator = (CPUDescriptorHeap&&) = delete;
        // clang-format on

        ~CPUDescriptorHeap()
        {
            ASSERT(m_CurrentSize == 0, "Not all allocations released");

            ASSERT(m_AvailableHeaps.size() == m_HeapPool.size(), "Not all descriptor heap pools are released");
            UINT32 TotalDescriptors = 0;
            for (auto& Heap : m_HeapPool)
            {
                ASSERT(Heap.GetNumAvailableDescriptors() == Heap.GetMaxDescriptors(),
                       "Not all descriptors in the descriptor pool are released");
                TotalDescriptors += Heap.GetMaxDescriptors();
            }

            // LOG_INFO_MESSAGE(std::setw(38), std::left, GetD3D12DescriptorHeapTypeLiteralName(m_HeapDesc.Type), " CPU
            // heap allocated pool count: ", m_HeapPool.size(),
            //     ". Max descriptors: ", m_MaxSize, '/', TotalDescriptors,
            //     " (", std::fixed, std::setprecision(2), m_MaxSize * 100.0 / std::max(TotalDescriptors, 1u), "%).");
        }

        virtual DescriptorHeapAllocation Allocate(uint32_t Count) override final;
        virtual void                     Free(DescriptorHeapAllocation&& Allocation) override final;
        virtual UINT32                   GetDescriptorSize() const override final { return m_DescriptorSize; }

    private:
        void FreeAllocation(DescriptorHeapAllocation&& Allocation);

        // Pool of descriptor heap managers
        std::mutex                                   m_HeapPoolMutex;
        std::vector<DescriptorHeapAllocationManager> m_HeapPool;
        // Indices of available descriptor heap managers
        std::unordered_set<size_t> m_AvailableHeaps;

        D3D12_DESCRIPTOR_HEAP_DESC m_HeapDesc;
        const UINT                 m_DescriptorSize = 0;

        // Maximum heap size during the application lifetime - for statistic purposes
        UINT32 m_MaxSize     = 0;
        UINT32 m_CurrentSize = 0;
    };

    // GPU descriptor heap provides storage for shader-visible descriptors
    // The heap contains single D3D12 descriptor heap that is split into two parts.
    // The first part stores static and mutable resource descriptor handles.
    // The second part is intended to provide temporary storage for dynamic resources.
    // Space for dynamic resources is allocated in chunks, and then descriptors are suballocated within every
    // chunk. DynamicSuballocationsManager facilitates this process.
    //
    //
    //     static and mutable handles      ||                 dynamic space
    //                                     ||    chunk 0     chunk 1     chunk 2     unused
    //  | X O O X X O X O O O O X X X X O  ||  | X X X O | | X X O O | | O O O O |  O O O O  ||
    //                                               |         |
    //                                     suballocation       suballocation
    //                                    within chunk 0       within chunk 1
    //
    // Render device contains two GPUDescriptorHeap instances (CBV_SRV_UAV and SAMPLER). The heaps
    // are used to allocate GPU-visible descriptors for shader resource binding objects. The heaps
    // are also used by the command contexts (through DynamicSuballocationsManager to allocated dynamic descriptors)
    //
    //  _______________________________________________________________________________________________________________________________
    // | Render Device                                                                                                                 |
    // |                                                                                                                               |
    // | m_CPUDescriptorHeaps[CBV_SRV_UAV] |  X  X  X  X  X  X  X  X  |, |  X  X  X  X  X  X  X  X  |, |  X  O  O  X  O  O  O  O  |    |
    // | m_CPUDescriptorHeaps[SAMPLER]     |  X  X  X  X  O  O  O  X  |, |  X  O  O  X  O  O  O  O  |                                  |
    // | m_CPUDescriptorHeaps[RTV]         |  X  X  X  O  O  O  O  O  |, |  O  O  O  O  O  O  O  O  |                                  |
    // | m_CPUDescriptorHeaps[DSV]         |  X  X  X  O  X  O  X  O  |                                                                |
    // |                                                                               ctx1        ctx2                                |
    // | m_GPUDescriptorHeaps[CBV_SRV_UAV]  | X O O X X O X O O O O X X X X O  ||  | X X X O | | X X O O | | O O O O |  O O O O  ||    |
    // | m_GPUDescriptorHeaps[SAMPLER]      | X X O O X O X X X O O X O O O O  ||  | X X O O | | X O O O | | O O O O |  O O O O  ||    |
    // |                                                                                                                               |
    // |_______________________________________________________________________________________________________________________________|
    //
    //  ________________________________________________               ________________________________________________
    // |Device Context 1                                |             |Device Context 2                                |
    // |                                                |             |                                                |
    // | m_DynamicGPUDescriptorAllocator[CBV_SRV_UAV]   |             | m_DynamicGPUDescriptorAllocator[CBV_SRV_UAV]   |
    // | m_DynamicGPUDescriptorAllocator[SAMPLER]       |             | m_DynamicGPUDescriptorAllocator[SAMPLER]       |
    // |________________________________________________|             |________________________________________________|
    //
    class GPUDescriptorHeap final : public IDescriptorAllocator
    {
    public:
        GPUDescriptorHeap(D3D12LinkedDevice*          LinkedDevice,
                          UINT32                      NumDescriptorsInHeap,
                          UINT32                      NumDynamicDescriptors,
                          D3D12_DESCRIPTOR_HEAP_TYPE  Type,
                          D3D12_DESCRIPTOR_HEAP_FLAGS Flags);

        // clang-format off
        GPUDescriptorHeap(const GPUDescriptorHeap&) = delete;
        GPUDescriptorHeap(GPUDescriptorHeap&&) = delete;
        GPUDescriptorHeap& operator = (const GPUDescriptorHeap&) = delete;
        GPUDescriptorHeap& operator = (GPUDescriptorHeap&&) = delete;
        // clang-format on

        ~GPUDescriptorHeap()
        {
            auto TotalStaticSize  = m_HeapAllocationManager.GetMaxDescriptors();
            auto TotalDynamicSize = m_DynamicAllocationsManager.GetMaxDescriptors();
            auto MaxStaticSize    = m_HeapAllocationManager.GetMaxAllocatedSize();
            auto MaxDynamicSize   = m_DynamicAllocationsManager.GetMaxAllocatedSize();
        }

        virtual DescriptorHeapAllocation Allocate(uint32_t Count) override final;
        virtual void                     Free(DescriptorHeapAllocation&& Allocation) override final;
        virtual UINT32                   GetDescriptorSize() const override final;

        DescriptorHeapAllocation AllocateDynamic(uint32_t Count);

        inline ID3D12DescriptorHeap* GetDescriptorHeap() const { return m_pd3d12DescriptorHeap.Get(); }

        inline const D3D12_DESCRIPTOR_HEAP_DESC& GetHeapDesc() const { return m_HeapDesc; }
        inline UINT32 GetMaxStaticDescriptors() const { return m_HeapAllocationManager.GetMaxDescriptors(); }
        inline UINT32 GetMaxDynamicDescriptors() const { return m_DynamicAllocationsManager.GetMaxDescriptors(); }

    protected:
        const D3D12_DESCRIPTOR_HEAP_DESC             m_HeapDesc;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_pd3d12DescriptorHeap; // Must be defined after m_HeapDesc

        const UINT m_DescriptorSize;

        // Allocation manager for static/mutable part
        DescriptorHeapAllocationManager m_HeapAllocationManager;

        // Allocation manager for dynamic part
        DescriptorHeapAllocationManager m_DynamicAllocationsManager;
    };

    // The class facilitates allocation of dynamic descriptor handles. It requests a chunk of heap
    // from the master GPU descriptor heap and then performs linear suballocation within the chunk
    // At the end of the frame all allocations are disposed.
    //     static and mutable handles     ||                 dynamic space
    //                                    ||    chunk 0                 chunk 2
    //  |                                 ||  | X X X O |             | O O O O |           || GPU Descriptor Heap
    //                                        |                       |
    //                                        m_Suballocations[0]     m_Suballocations[1]
    //
    class DynamicSuballocationsManager final : public IDescriptorAllocator
    {
    public:
        DynamicSuballocationsManager(GPUDescriptorHeap* ParentGPUHeap,
                                     UINT32             DynamicChunkSize,
                                     std::string        ManagerName) :
            IDescriptorAllocator(ParentGPUHeap->GetParentLinkedDevice()), m_ParentGPUHeap {ParentGPUHeap},
            m_DynamicChunkSize {DynamicChunkSize}, m_ManagerName {std::move(ManagerName)}
        {}

        // clang-format off
        DynamicSuballocationsManager(const DynamicSuballocationsManager&) = delete;
        DynamicSuballocationsManager(DynamicSuballocationsManager&&) = delete;
        DynamicSuballocationsManager& operator = (const DynamicSuballocationsManager&) = delete;
        DynamicSuballocationsManager& operator = (DynamicSuballocationsManager&&) = delete;
        // clang-format on

        ~DynamicSuballocationsManager()
        {
            ASSERT(m_Suballocations.empty() && m_CurrDescriptorCount == 0 && m_CurrSuballocationsTotalSize == 0,
                   "All dynamic suballocations must be released!");
        }

        /*
        // 在一帧的结尾释放掉这里申请的所有资源
        void ReleaseAllocations(UINT32 FenceValue);
        */
        void RetireAllcations();

        virtual DescriptorHeapAllocation Allocate(UINT32 Count) override final;
        virtual void                     Free(DescriptorHeapAllocation&& Allocation) override final;
        virtual UINT32                   GetDescriptorSize() const override final;

        DescriptorHandle AllocateDescHandle();

        inline size_t GetSuballocationCount() const { return m_Suballocations.size(); }

    private:
        // Parent GPU descriptor heap that is used to allocate chunks
        GPUDescriptorHeap* m_ParentGPUHeap;
        const std::string  m_ManagerName;

        // List of chunks allocated from the master GPU descriptor heap. All chunks are disposed at the end
        // of the frame
        std::vector<DescriptorHeapAllocation> m_Suballocations;

        UINT32 m_CurrentSuballocationOffset = 0;
        UINT32 m_DynamicChunkSize           = 0;

        UINT32 m_CurrDescriptorCount         = 0;
        UINT32 m_PeakDescriptorCount         = 0;
        UINT32 m_CurrSuballocationsTotalSize = 0;
        UINT32 m_PeakSuballocationsTotalSize = 0;
    };


}