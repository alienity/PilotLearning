#include "d3d12_fence.h"
#include "d3d12_device.h"
#include "d3d12_commandQueue.h"

namespace RHI
{
    D3D12Fence::D3D12Fence(D3D12Device*      Parent,
                           UINT64            InitialValue,
                           D3D12_FENCE_FLAGS Flags /*= D3D12_FENCE_FLAG_NONE*/) :
        D3D12DeviceChild(Parent),
        Fence([&] {
            Microsoft::WRL::ComPtr<ID3D12Fence1> Fence;
            VERIFY_D3D12_API(Parent->GetD3D12Device()->CreateFence(InitialValue, Flags, IID_PPV_ARGS(&Fence)));
            return Fence;
        }()),
        CurrentValue(InitialValue + 1), LastSignaledValue(0), LastCompletedValue(InitialValue)
    {
        m_FenceEventHandle = CreateEvent(nullptr, false, false, nullptr);
        ASSERT(m_FenceEventHandle != NULL);
    }

    D3D12Fence::~D3D12Fence()
    {
        if (m_FenceEventHandle != nullptr)
        {
            CloseHandle(m_FenceEventHandle);
            m_FenceEventHandle = nullptr;
        }
    }

    UINT64 D3D12Fence::Signal(D3D12CommandQueue* CommandQueue)
    {
        InternalSignal(CommandQueue->GetCommandQueue(), CurrentValue);
        UpdateLastCompletedValue();
        CurrentValue++;
        return LastSignaledValue;
    }

    UINT64 D3D12Fence::Signal(IDStorageQueue* DStorageQueue)
    {
        InternalSignal(DStorageQueue, CurrentValue);
        UpdateLastCompletedValue();
        CurrentValue++;
        return LastSignaledValue;
    }

    bool D3D12Fence::IsFenceComplete(UINT64 Value)
    {
        if (Value <= LastCompletedValue)
        {
            return true;
        }

        return Value <= UpdateLastCompletedValue();
    }

    void D3D12Fence::HostWaitForValue(UINT64 Value)
    {
        if (IsFenceComplete(Value))
        {
            return;
        }

        // TODO:  Think about how this might affect a multi-threaded situation.  Suppose thread A
        // wants to wait for fence 100, then thread B comes along and wants to wait for 99.  If
        // the fence can only have one event set on completion, then thread B has to wait for
        // 100 before it knows 99 is ready.  Maybe insert sequential events?
        {
            VERIFY_D3D12_API(Fence->SetEventOnCompletion(Value, m_FenceEventHandle));
            WaitForSingleObject(m_FenceEventHandle, INFINITE);
            UpdateLastCompletedValue();
        }
    }

    void D3D12Fence::InternalSignal(ID3D12CommandQueue* CommandQueue, UINT64 Value)
    {
        VERIFY_D3D12_API(CommandQueue->Signal(Fence.Get(), Value));
        LastSignaledValue = Value;
    }

    void D3D12Fence::InternalSignal(IDStorageQueue* DStorageQueue, UINT64 Value)
    {
        DStorageQueue->EnqueueSignal(Fence.Get(), Value);
        LastSignaledValue = Value;
    }

    UINT64 D3D12Fence::UpdateLastCompletedValue()
    {
        LastCompletedValue = Fence->GetCompletedValue();
        return LastCompletedValue;
    }

}
