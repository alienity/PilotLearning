#pragma once
#include "d3d12_core.h"
#include "d3d12_linkedDevice.h"
#include "d3d12_commandList.h"
#include "d3d12_fence.h"

namespace RHI
{
    class D3D12CommandAllocatorPool;
    
    class D3D12CommandQueue : public D3D12LinkedDeviceChild
    {
    public:
        D3D12CommandQueue() noexcept = default;
        explicit D3D12CommandQueue(D3D12LinkedDevice* Parent, RHID3D12CommandQueueType Type);

        D3D12CommandQueue(D3D12CommandQueue&&) noexcept = default;
        D3D12CommandQueue& operator=(D3D12CommandQueue&&) noexcept = default;

        D3D12CommandQueue(const D3D12CommandQueue&) noexcept = delete;
        D3D12CommandQueue& operator=(const D3D12CommandQueue&) noexcept = delete;

        [[nodiscard]] D3D12_COMMAND_LIST_TYPE GetType() const noexcept { return m_CommandListType; }
        [[nodiscard]] ID3D12CommandQueue*     GetCommandQueue() const noexcept { return m_CommandQueue.Get(); }
        [[nodiscard]] bool                    SupportTimestamps() const noexcept { return m_Frequency.has_value(); }
        [[nodiscard]] UINT64                  GetFrequency() const noexcept { return m_Frequency.value_or(0); }

        [[nodiscard]] UINT64 Signal();

        [[nodiscard]] bool IsFenceComplete(UINT64 FenceValue);

        void HostWaitForValue(UINT64 FenceValue);

        void Wait(D3D12CommandQueue* CommandQueue) const;
        void WaitForSyncHandle(const D3D12SyncHandle& SyncHandle) const;

        void WaitIdle() { HostWaitForValue(Signal()); }

        RHI::D3D12SyncHandle ExecuteCommandLists(std::vector<D3D12CommandListHandle*> CommandListHandles, bool WaitForCompletion);

    private:
        [[nodiscard]] bool ResolveResourceBarrierCommandList(D3D12CommandListHandle* CommandListHandle);

    private:
        D3D12_COMMAND_LIST_TYPE                    m_CommandListType;
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_CommandQueue;
        RHI::D3D12Fence                            m_Fence;
        RHI::D3D12SyncHandle                       m_SyncHandle;

        // Command allocators used exclusively for resolving resource barriers
        std::shared_ptr<RHI::D3D12CommandAllocatorPool> m_ResourceBarrierCommandAllocatorPool;
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator>  m_ResourceBarrierCommandAllocator;
        std::shared_ptr<RHI::D3D12CommandListHandle>    m_ResourceBarrierCommandListHandle;

        std::optional<UINT64> m_Frequency;
    };
}