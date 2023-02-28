#include "d3d12_linkedDevice.h"
#include "d3d12_device.h"
#include "d3d12_resource.h"
#include "d3d12_commandQueue.h"
#include "d3d12_descriptorHeap.h"
#include "d3d12_commandContext.h"
#include "d3d12_samplerManager.h"
#include "d3d12_graphicsCommon.h"
#include "d3d12_graphicsMemory.h"
#include "d3d12_resourceUploadBatch.h"

#include "runtime/core/base/utility.h"

//// D3D12.DescriptorAllocatorPageSize
//// Descriptor Allocator Page Size
//static int CVar_DescriptorAllocatorPageSize = 2048;
//
//// D3D12.GlobalResourceViewHeapSize
//// Global Resource View Heap Size
//static int CVar_GlobalResourceViewHeapSize = 65535;
//
//// D3D12.GlobalSamplerHeapSize
//// Global Sampler Heap Size
//static int CVar_GlobalSamplerHeapSize = D3D12_MAX_SHADER_VISIBLE_SAMPLER_HEAP_SIZE;

static int CVar_GlobalCPUViewHeapSize           = 256;
static int CVar_GlobalStaticGPUViewHeapSize     = 16384;
static int CVar_GlobalDynamicGPUViewHeapSize    = 8192;
static int CVar_GlobalStaticGPUSamplerHeapSize  = 1024;
static int CVar_GlobalDynamicGPUSamplerHeapSize = 1024;

namespace RHI
{
    // clang-format off
    D3D12LinkedDevice::D3D12LinkedDevice(D3D12Device* Parent, D3D12NodeMask NodeMask)
        : D3D12DeviceChild(Parent)
        , NodeMask(NodeMask)
    {
        m_GraphicsQueue = std::make_shared<D3D12CommandQueue>(this, RHID3D12CommandQueueType::Direct);
        m_AsyncComputeQueue = std::make_shared<D3D12CommandQueue>(this, RHID3D12CommandQueueType::AsyncCompute);
        m_CopyQueue1 = std::make_shared<D3D12CommandQueue>(this, RHID3D12CommandQueueType::Copy1);
        m_CopyQueue2 = std::make_shared<D3D12CommandQueue>(this, RHID3D12CommandQueueType::Direct); // RHID3D12CommandQueueType::Copy2
        m_Profiler = std::make_shared<D3D12Profiler>(this, 1);

        m_RtvDescriptorHeaps = std::make_shared<CPUDescriptorHeap>(
                this, CVar_GlobalCPUViewHeapSize, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
        m_DsvDescriptorHeaps = std::make_shared<CPUDescriptorHeap>(
                this, CVar_GlobalCPUViewHeapSize, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

        m_CPUResourceDescriptorHeap = std::make_shared<CPUDescriptorHeap>(
                this, CVar_GlobalCPUViewHeapSize, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
        m_CPUSamplerDescriptorHeap = std::make_shared<CPUDescriptorHeap>(
                this, CVar_GlobalCPUViewHeapSize, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

        m_ResourceDescriptorHeap = std::make_shared<GPUDescriptorHeap>(this,
                                                         CVar_GlobalStaticGPUViewHeapSize,
                                                         CVar_GlobalDynamicGPUViewHeapSize,
                                                         D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
                                                         D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
        m_SamplerDescriptorHeap = std::make_shared<GPUDescriptorHeap>(this,
                                                         CVar_GlobalStaticGPUSamplerHeapSize,
                                                         CVar_GlobalDynamicGPUSamplerHeapSize,
                                                         D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
                                                         D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
        // initialize common states
        RHI::InitializeCommonState(this);

        m_GraphicsMemory = std::shared_ptr<GraphicsMemory>(new GraphicsMemory(this));
        m_ResourceUploadBatch = std::shared_ptr<ResourceUploadBatch>(new ResourceUploadBatch(this));

        constexpr size_t NumThreads = 3;
        m_AvailableCommandContexts.reserve(NumThreads);
        for (unsigned int i = 0; i < NumThreads; ++i)
        {
            m_AvailableCommandContexts.emplace_back(std::make_shared<D3D12CommandContext>(
                this, RHID3D12CommandQueueType::Direct, D3D12_COMMAND_LIST_TYPE_DIRECT));
        }
        m_AvailableAsyncCommandContexts.reserve(NumThreads);
        for (unsigned int i = 0; i < NumThreads; ++i)
        {
            m_AvailableAsyncCommandContexts.emplace_back(std::make_shared<D3D12CommandContext>(
                this, RHID3D12CommandQueueType::AsyncCompute, D3D12_COMMAND_LIST_TYPE_COMPUTE));
        }
        m_CopyContext1 = std::make_shared<D3D12CommandContext>(this, RHID3D12CommandQueueType::Copy1, D3D12_COMMAND_LIST_TYPE_DIRECT);
        m_CopyContext2 = std::make_shared<D3D12CommandContext>(this, RHID3D12CommandQueueType::Copy2, D3D12_COMMAND_LIST_TYPE_DIRECT);
    }
    // clang-format on

	D3D12LinkedDevice::~D3D12LinkedDevice()
    {
        // release common states
        RHI::DestroyCommonState();

        m_GraphicsMemory      = nullptr;
        m_ResourceUploadBatch = nullptr;

        m_GraphicsQueue     = nullptr;
        m_AsyncComputeQueue = nullptr;
        m_CopyQueue1        = nullptr;
        m_CopyQueue2        = nullptr;

        SamplerDesc::RetireAll(this);

        m_Profiler          = nullptr;

        // release all resources and descriptors
        for (size_t i = 0; i < MaxSharedBufferCount; i++)
            this->Release(RHI::D3D12SyncHandle());

        m_RtvDescriptorHeaps     = nullptr;
        m_DsvDescriptorHeaps     = nullptr;
        m_ResourceDescriptorHeap = nullptr;
        m_SamplerDescriptorHeap  = nullptr;

        #ifdef _DEBUG
        ASSERT(m_DebugResources.size() == 0);
        #endif
    }

    ID3D12Device* D3D12LinkedDevice::GetDevice() const { return GetParentDevice()->GetD3D12Device(); }

    ID3D12Device5* D3D12LinkedDevice::GetDevice5() const { return GetParentDevice()->GetD3D12Device5(); }

    D3D12CommandQueue* D3D12LinkedDevice::GetCommandQueue(RHID3D12CommandQueueType Type)
    {
        switch (Type)
        {
            case RHID3D12CommandQueueType::Direct:
                return m_GraphicsQueue.get();
            case RHID3D12CommandQueueType::AsyncCompute:
                return m_AsyncComputeQueue.get();
            case RHID3D12CommandQueueType::Copy1:
                return m_CopyQueue1.get();
            case RHID3D12CommandQueueType::Copy2:
                return m_CopyQueue2.get();
        }
        return nullptr;
    }

    D3D12CommandQueue* D3D12LinkedDevice::GetGraphicsQueue()
    {
        return GetCommandQueue(RHID3D12CommandQueueType::Direct);
    }

    D3D12CommandQueue* D3D12LinkedDevice::GetAsyncComputeQueue()
    {
        return GetCommandQueue(RHID3D12CommandQueueType::AsyncCompute);
    }

    D3D12CommandQueue* D3D12LinkedDevice::GetCopyQueue1() { return GetCommandQueue(RHID3D12CommandQueueType::Copy1); }

    D3D12Profiler* D3D12LinkedDevice::GetProfiler() { return m_Profiler.get(); }

    GPUDescriptorHeap* D3D12LinkedDevice::GetResourceDescriptorHeap() noexcept { return m_ResourceDescriptorHeap.get(); }

    GPUDescriptorHeap* D3D12LinkedDevice::GetSamplerDescriptorHeap() noexcept { return m_SamplerDescriptorHeap.get(); }

    D3D12CommandContext* D3D12LinkedDevice::GetCommandContext(UINT ThreadIndex /*= 0*/)
    {
        assert(ThreadIndex < m_AvailableCommandContexts.size());
        return m_AvailableCommandContexts[ThreadIndex].get();
    }

    D3D12CommandContext* D3D12LinkedDevice::GetAsyncComputeCommandContext(UINT ThreadIndex /*= 0*/)
    {
        assert(ThreadIndex < m_AvailableAsyncCommandContexts.size());
        return m_AvailableAsyncCommandContexts[ThreadIndex].get();
    }

    D3D12CommandContext* D3D12LinkedDevice::GetCopyContext1() { return m_CopyContext1.get(); }
    D3D12CommandContext* D3D12LinkedDevice::GetCopyContext2() { return m_CopyContext2.get(); }

    GraphicsMemory* D3D12LinkedDevice::GetGraphicsMemory() { return m_GraphicsMemory.get(); }

    ResourceUploadBatch* D3D12LinkedDevice::GetResourceUploadBatch() { return m_ResourceUploadBatch.get(); }

    void D3D12LinkedDevice::OnBeginFrame() { m_Profiler->OnBeginFrame(); }

    void D3D12LinkedDevice::OnEndFrame()
    {
        m_Profiler->OnEndFrame();
    }

    D3D12_RESOURCE_ALLOCATION_INFO D3D12LinkedDevice::GetResourceAllocationInfo(const D3D12_RESOURCE_DESC& Desc) const
    {
        std::uint64_t Hash = Utility::Hash64(&Desc, sizeof(Desc));
        {

            std::shared_lock<std::shared_mutex> ReadLock(ResourceAllocationInfoTable.Mutex);
            if (auto Iter = ResourceAllocationInfoTable.Table.find(Hash);
                Iter != ResourceAllocationInfoTable.Table.end())
            {
                return Iter->second;
            }
        }

        std::lock_guard<std::shared_mutex> WriteLock(ResourceAllocationInfoTable.Mutex);

        D3D12_RESOURCE_ALLOCATION_INFO ResourceAllocationInfo = GetDevice()->GetResourceAllocationInfo(0, 1, &Desc);
        ResourceAllocationInfoTable.Table.insert(
            robin_hood::pair<std::uint64_t, D3D12_RESOURCE_ALLOCATION_INFO>(Hash, ResourceAllocationInfo));

        return ResourceAllocationInfo;
    }

    bool D3D12LinkedDevice::ResourceSupport4KBAlignment(D3D12_RESOURCE_DESC& Desc) const
    {
        // 4KB alignment is only available for read only textures
        if (!(Desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET ||
              Desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL ||
              Desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) &&
            Desc.SampleDesc.Count == 1)
        {
            // Since we are using small resources we can take advantage of 4KB
            // resource alignments. As long as the most detailed mip can fit in an
            // allocation less than 64KB, 4KB alignments can be used.
            //
            // When dealing with MSAA textures the rules are similar, but the minimum
            // alignment is 64KB for a texture whose most detailed mip can fit in an
            // allocation less than 4MB.
            Desc.Alignment = D3D12_SMALL_RESOURCE_PLACEMENT_ALIGNMENT;

            D3D12_RESOURCE_ALLOCATION_INFO ResourceAllocationInfo = GetResourceAllocationInfo(Desc);
            if (ResourceAllocationInfo.Alignment != D3D12_SMALL_RESOURCE_PLACEMENT_ALIGNMENT)
            {
                // If the alignment requested is not granted, then let D3D tell us
                // the alignment that needs to be used for these resources.
                Desc.Alignment = 0;
                return false;
            }

            return true;
        }

        return false;
    }

    void D3D12LinkedDevice::WaitIdle()
    {
        m_GraphicsQueue->WaitIdle();
        m_AsyncComputeQueue->WaitIdle();
        m_CopyQueue1->WaitIdle();
        m_CopyQueue2->WaitIdle();
    }

    void D3D12LinkedDevice::Retire(Microsoft::WRL::ComPtr<ID3D12Resource> D3D12Resource)
    {
        m_CurrentFrameSharedDatas.m_RetiredResource.push_back(D3D12Resource);
    }

    void D3D12LinkedDevice::Retire(DescriptorHeapAllocation&& Allocation)
    {
        m_CurrentFrameSharedDatas.m_RetiredAllocations.push_back(std::move(Allocation));
    }

    void D3D12LinkedDevice::Release(D3D12SyncHandle syncHandle)
    {
        // release previous Allocation
        if (m_ActiveSharedDatas[m_CurrentBufferIndex].m_SyncHandle)
        {
            if (!m_ActiveSharedDatas[m_CurrentBufferIndex].m_SyncHandle.IsComplete())
            {
                m_ActiveSharedDatas[m_CurrentBufferIndex].m_SyncHandle.WaitForCompletion();

                for (size_t i = 0; i < m_ActiveSharedDatas[m_CurrentBufferIndex].m_RetiredResource.size(); i++)
                {
                    m_ActiveSharedDatas[m_CurrentBufferIndex].m_RetiredResource[i] = nullptr;
                }
                m_ActiveSharedDatas[m_CurrentBufferIndex].m_RetiredResource.clear();

                for (size_t i = 0; i < m_ActiveSharedDatas[m_CurrentBufferIndex].m_RetiredAllocations.size(); i++)
                {
                    m_ActiveSharedDatas[m_CurrentBufferIndex].m_RetiredAllocations[i].ReleaseDescriptorHeapAllocation();
                }
                m_ActiveSharedDatas[m_CurrentBufferIndex].m_RetiredAllocations.clear();
            }
        }

        // stack current frame resource
        if (syncHandle)
        {
            m_ActiveSharedDatas[m_CurrentBufferIndex].m_SyncHandle = syncHandle;
            for (size_t i = 0; i < m_CurrentFrameSharedDatas.m_RetiredResource.size(); i++)
            {
                m_ActiveSharedDatas[m_CurrentBufferIndex].m_RetiredResource.push_back(m_CurrentFrameSharedDatas.m_RetiredResource[i]);
            }
            m_CurrentFrameSharedDatas.m_RetiredResource.clear();
            for (size_t i = 0; i < m_CurrentFrameSharedDatas.m_RetiredAllocations.size(); i++)
            {
                m_ActiveSharedDatas[m_CurrentBufferIndex].m_RetiredAllocations.push_back(std::move(m_CurrentFrameSharedDatas.m_RetiredAllocations[i]));
            }
            m_CurrentFrameSharedDatas.m_RetiredAllocations.clear();
        }
        else // release resource directly
        {
            for (size_t i = 0; i < m_CurrentFrameSharedDatas.m_RetiredResource.size(); i++)
            {
                m_CurrentFrameSharedDatas.m_RetiredResource[i] = nullptr;
            }
            m_CurrentFrameSharedDatas.m_RetiredResource.clear();
            for (size_t i = 0; i < m_CurrentFrameSharedDatas.m_RetiredAllocations.size(); i++)
            {
                m_CurrentFrameSharedDatas.m_RetiredAllocations[i].ReleaseDescriptorHeapAllocation();
            }
            m_CurrentFrameSharedDatas.m_RetiredAllocations.clear();
        }

        // update current BufferIndex
        m_CurrentBufferIndex = (m_CurrentBufferIndex + 1) % MaxSharedBufferCount;
    }

}

