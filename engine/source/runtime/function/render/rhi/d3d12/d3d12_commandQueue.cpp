#include "d3d12_commandQueue.h"
#include "d3d12_linkedDevice.h"
#include "d3d12_commandContext.h"

namespace RHI
{
    // clang-format off
    D3D12CommandQueue::D3D12CommandQueue(D3D12LinkedDevice* Parent, RHID3D12CommandQueueType Type) :
        D3D12LinkedDeviceChild(Parent), m_CommandListType(RHITranslateD3D12(Type)), 
        m_CommandQueue([&] {
            Microsoft::WRL::ComPtr<ID3D12CommandQueue> CommandQueue;
            D3D12_COMMAND_QUEUE_DESC Desc = {m_CommandListType,
                                             D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
                                             D3D12_COMMAND_QUEUE_FLAG_NONE,
                                             Parent->GetNodeMask()};
            VERIFY_D3D12_API(Parent->GetDevice()->CreateCommandQueue(&Desc, IID_PPV_ARGS(&CommandQueue)));
            return CommandQueue;
        }()),
        m_Fence(Parent->GetParentDevice(), 0, D3D12_FENCE_FLAG_NONE),
        m_ResourceBarrierCommandAllocatorPool(std::make_shared<D3D12CommandAllocatorPool>(Parent, m_CommandListType)),
        m_ResourceBarrierCommandAllocator(m_ResourceBarrierCommandAllocatorPool->RequestCommandAllocator()),
        m_ResourceBarrierCommandListHandle(std::make_shared<D3D12CommandListHandle>(Parent, m_CommandListType))
    {
#ifdef _DEBUG
        m_CommandQueue->SetName(GetCommandQueueTypeString(Type));
        m_Fence.Get()->SetName(GetCommandQueueTypeFenceString(Type));
#endif
        UINT64 Frequency = 0;
        if (SUCCEEDED(m_CommandQueue->GetTimestampFrequency(&Frequency)))
        {
            this->m_Frequency = Frequency;
        }
    }
    // clang-format on

    UINT64 D3D12CommandQueue::Signal() { return m_Fence.Signal(this); }

	bool D3D12CommandQueue::IsFenceComplete(UINT64 FenceValue) { return m_Fence.IsFenceComplete(FenceValue); }

    void D3D12CommandQueue::HostWaitForValue(UINT64 FenceValue) { m_Fence.HostWaitForValue(FenceValue); }
    
    void D3D12CommandQueue::Wait(D3D12CommandQueue* CommandQueue) const { WaitForSyncHandle(CommandQueue->m_SyncHandle); }

	void D3D12CommandQueue::WaitForSyncHandle(const D3D12SyncHandle& SyncHandle) const
    {
        if (SyncHandle)
        {
            VERIFY_D3D12_API(m_CommandQueue->Wait(SyncHandle.Fence->Get(), SyncHandle.Value));
        }
    }

    D3D12SyncHandle D3D12CommandQueue::ExecuteCommandLists(std::vector<D3D12CommandListHandle*> CommandListHandles,
                                                           bool                                 WaitForCompletion)
    {
        UINT               NumCommandLists       = 0;
        UINT               NumBarrierCommandList = 0;
        ID3D12CommandList* CommandLists[32]      = {};

        // Resolve resource barriers
        for (auto& CommandListHandle : CommandListHandles)
        {
            if (ResolveResourceBarrierCommandList(CommandListHandle))
            {
                CommandLists[NumCommandLists++] = m_ResourceBarrierCommandListHandle->GetCommandList();
                NumBarrierCommandList++;
            }

            CommandLists[NumCommandLists++] = CommandListHandle->GetCommandList();
        }

        m_CommandQueue->ExecuteCommandLists(NumCommandLists, CommandLists);
        UINT64 FenceValue = Signal();
        m_SyncHandle      = D3D12SyncHandle(&m_Fence, FenceValue);

        // Discard command allocator used exclusively to resolve resource barriers
        if (NumBarrierCommandList > 0)
        {
            m_ResourceBarrierCommandAllocatorPool->DiscardCommandAllocator(
                std::exchange(m_ResourceBarrierCommandAllocator, {}), m_SyncHandle);
        }

        if (WaitForCompletion)
        {
            HostWaitForValue(FenceValue);
            ASSERT(m_SyncHandle.IsComplete());
        }

        return m_SyncHandle;
    }

    bool D3D12CommandQueue::ResolveResourceBarrierCommandList(D3D12CommandListHandle* CommandListHandle)
    {
        std::vector<D3D12_RESOURCE_BARRIER> ResourceBarriers = CommandListHandle->ResolveResourceBarriers();

        bool AnyResolved = !ResourceBarriers.empty();
        if (AnyResolved)
        {
            if (!m_ResourceBarrierCommandAllocator)
            {
                m_ResourceBarrierCommandAllocator = m_ResourceBarrierCommandAllocatorPool->RequestCommandAllocator();
            }

            m_ResourceBarrierCommandListHandle->Open(m_ResourceBarrierCommandAllocator.Get());
            (*m_ResourceBarrierCommandListHandle)->ResourceBarrier(static_cast<UINT>(ResourceBarriers.size()), ResourceBarriers.data());
            m_ResourceBarrierCommandListHandle->Close();
        }

        return AnyResolved;
    }
}

