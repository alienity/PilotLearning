#include "d3d12_commandList.h"
#include "d3d12_linkedDevice.h"
#include "d3d12_resource.h"

namespace RHI
{
    std::vector<PendingResourceBarrier>& D3D12ResourceStateTracker::GetPendingResourceBarriers()
    {
        return PendingResourceBarriers;
    }

	CResourceState& D3D12ResourceStateTracker::GetResourceState(D3D12Resource* Resource)
    {
        CResourceState& ResourceState = ResourceStates[Resource->GetResource()];
        // If ResourceState was just created, its state is uninitialized
        if (ResourceState.IsUninitialized())
        {
            ResourceState = CResourceState(Resource->GetNumSubresources(), D3D12_RESOURCE_STATE_UNKNOWN);
        }
        return ResourceState;
    }

    void D3D12ResourceStateTracker::Reset()
    {
        ResourceStates.clear();
        PendingResourceBarriers.clear();
    }

    void D3D12ResourceStateTracker::Add(const PendingResourceBarrier& PendingResourceBarrier)
    {
        PendingResourceBarriers.push_back(PendingResourceBarrier);
    }

    D3D12CommandListHandle::D3D12CommandListHandle(D3D12LinkedDevice* Parent, D3D12_COMMAND_LIST_TYPE Type) :
        D3D12LinkedDeviceChild(Parent), Type(Type), GraphicsCommandList([&] {
            Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> GraphicsCommandList;
            VERIFY_D3D12_API(Parent->GetDevice5()->CreateCommandList1(
                Parent->GetNodeMask(), Type, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&GraphicsCommandList)));
            return GraphicsCommandList;
        }())
    {
        GraphicsCommandList->QueryInterface(IID_PPV_ARGS(&GraphicsCommandList4));
        GraphicsCommandList->QueryInterface(IID_PPV_ARGS(&GraphicsCommandList6));
#ifdef MOYU_RHI_D3D12_DEBUG_RESOURCE_STATES
        GraphicsCommandList->QueryInterface(IID_PPV_ARGS(&DebugCommandList));
#endif
        CommandListState = D3D12CommandListState::Closed;
    }

	bool D3D12CommandListHandle::Open(ID3D12CommandAllocator* CommandAllocator)
    {
        // skip reset if current commandlist is recording now
        if (CommandListState == D3D12CommandListState::Recording)
            return false;

        VERIFY_D3D12_API(GraphicsCommandList->Reset(CommandAllocator, nullptr));

        // Reset resource state tracking and resource barriers
        ResourceStateTracker.Reset();
        NumResourceBarriers = 0;

        // start commandList state
        CommandListState = D3D12CommandListState::Recording;

        return true;
    }

    void D3D12CommandListHandle::Close()
    {
        CommandListState = D3D12CommandListState::Pending;

        FlushResourceBarriers();
        VERIFY_D3D12_API(GraphicsCommandList->Close());
    }

    const CResourceState D3D12CommandListHandle::GetAllTrackedResourceState(D3D12Resource* Resource)
    {
        const CResourceState& ResourceState = ResourceStateTracker.GetResourceState(Resource);
        return ResourceState;
    }

    D3D12_RESOURCE_STATES D3D12CommandListHandle::GetResourceStateTracked(D3D12Resource* Resource, UINT Subresource)
    {
        CResourceState& ResourceState = ResourceStateTracker.GetResourceState(Resource);
        return ResourceState.GetSubresourceState(Subresource);
    }

    void D3D12CommandListHandle::TransitionBarrier(D3D12Resource*        Resource,
                                                   D3D12_RESOURCE_STATES State,
                                                   UINT Subresource /*= D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES*/)
    {
        CResourceState& ResourceState = ResourceStateTracker.GetResourceState(Resource);
        // First use on the command list
        if (ResourceState.IsUnknown(Subresource))
        {
            ResourceStateTracker.Add(PendingResourceBarrier {Resource, State, Subresource});
        }
        // Known state within the command list
        else
        {
            // If we are applying transition to all subresources and we are in different tracking mode
            // transition each subresource individually
            if (Subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES && !ResourceState.IsUniform())
            {
                // First transition all of the subresources if they are different than the State
                UINT i = 0;
                for (D3D12_RESOURCE_STATES SubresourceState : ResourceState)
                {
                    if (SubresourceState != State)
                    {
                        AddTransition(Resource, SubresourceState, State, i);
                    }

                    i++;
                }
            }
            // Apply barrier at subresource level
            else
            {
                D3D12_RESOURCE_STATES StateKnown = ResourceState.GetSubresourceState(Subresource);
                if (StateKnown != State)
                {
                    AddTransition(Resource, StateKnown, State, Subresource);
                }
            }
        }

        // Update command list resource state
        ResourceState.SetSubresourceState(Subresource, State);
    }

    void D3D12CommandListHandle::AliasingBarrier(D3D12Resource* BeforeResource, D3D12Resource* AfterResource)
    {
        AddAliasing(BeforeResource, AfterResource);
    }

    void D3D12CommandListHandle::UAVBarrier(D3D12Resource* Resource)
    {
        AddUAV(Resource);
    }

    void D3D12CommandListHandle::FlushResourceBarriers()
    {
        if (NumResourceBarriers > 0)
        {
            GraphicsCommandList->ResourceBarrier(NumResourceBarriers, ResourceBarriers);
            NumResourceBarriers = 0;
        }
    }

    bool
    D3D12CommandListHandle::AssertResourceState(D3D12Resource* Resource, D3D12_RESOURCE_STATES State, UINT Subresource)
    {
#ifdef MOYU_RHI_D3D12_DEBUG_RESOURCE_STATES
        if (DebugCommandList)
        {
            return DebugCommandList->AssertResourceState(Resource->GetResource(), Subresource, State);
        }
#endif
        return true;
    }

    std::vector<D3D12_RESOURCE_BARRIER> D3D12CommandListHandle::ResolveResourceBarriers()
    {
        const auto& PendingResourceBarriers = ResourceStateTracker.GetPendingResourceBarriers();

        std::vector<D3D12_RESOURCE_BARRIER> ResourceBarriers;
        ResourceBarriers.reserve(PendingResourceBarriers.size());

        for (const auto& [Resource, State, Subresource] : PendingResourceBarriers)
        {
            CResourceState& ResourceState = Resource->GetResourceState();

            if ((Subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES) && !ResourceState.IsUniform())
            {
                int subresourceNumber = Resource->GetNumSubresources();
                for (int subIdx = 0; subIdx < subresourceNumber; subIdx++)
                {
                    D3D12_RESOURCE_STATES StateBefore = ResourceState.GetSubresourceState(subIdx);
                    D3D12_RESOURCE_STATES StateAfter  = State != D3D12_RESOURCE_STATE_UNKNOWN ? State : StateBefore;

                    if (StateBefore != StateAfter)
                    {
                        ResourceBarriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
                            Resource->GetResource(), StateBefore, StateAfter, subIdx));
                    }

                    // Get the command list resource state associate with this resource
                    D3D12_RESOURCE_STATES StateCommandList =
                        ResourceStateTracker.GetResourceState(Resource).GetSubresourceState(subIdx);
                    D3D12_RESOURCE_STATES StatePrevious =
                        StateCommandList != D3D12_RESOURCE_STATE_UNKNOWN ? StateCommandList : StateAfter;

                    // If global state is not same as commandlist space state, then change the global state
                    if (StateBefore != StatePrevious)
                    {
                        ResourceState.SetSubresourceState(subIdx, StatePrevious);
                    }
                }
            }
            else
            {
                D3D12_RESOURCE_STATES StateBefore = ResourceState.GetSubresourceState(Subresource);
                D3D12_RESOURCE_STATES StateAfter  = State != D3D12_RESOURCE_STATE_UNKNOWN ? State : StateBefore;

                if (StateBefore != StateAfter)
                {
                    ResourceBarriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
                        Resource->GetResource(), StateBefore, StateAfter, Subresource));
                }

                // Get the command list resource state associate with this resource
                D3D12_RESOURCE_STATES StateCommandList =
                    ResourceStateTracker.GetResourceState(Resource).GetSubresourceState(Subresource);
                D3D12_RESOURCE_STATES StatePrevious =
                    StateCommandList != D3D12_RESOURCE_STATE_UNKNOWN ? StateCommandList : StateAfter;

                // If global state is not same as commandlist space state, then change the global state
                if (StateBefore != StatePrevious)
                {
                    ResourceState.SetSubresourceState(Subresource, StatePrevious);
                }
            }
        }

        return ResourceBarriers;
    }

    void D3D12CommandListHandle::Add(const D3D12_RESOURCE_BARRIER& ResourceBarrier)
    {
        assert(NumResourceBarriers < NumBatches);
        ResourceBarriers[NumResourceBarriers++] = ResourceBarrier;
        if (NumResourceBarriers == NumBatches)
        {
            FlushResourceBarriers();
        }
    }

    void D3D12CommandListHandle::AddTransition(D3D12Resource*        Resource,
                                               D3D12_RESOURCE_STATES StateBefore,
                                               D3D12_RESOURCE_STATES StateAfter,
                                               UINT Subresource /*= D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES*/)
    {
        Add(CD3DX12_RESOURCE_BARRIER::Transition(Resource->GetResource(), StateBefore, StateAfter, Subresource));
    }

    void D3D12CommandListHandle::AddAliasing(D3D12Resource* BeforeResource, D3D12Resource* AfterResource)
    {
        Add(CD3DX12_RESOURCE_BARRIER::Aliasing(BeforeResource->GetResource(), AfterResource->GetResource()));
    }

    void D3D12CommandListHandle::AddUAV(D3D12Resource* Resource)
    {
        Add(CD3DX12_RESOURCE_BARRIER::UAV(Resource ? Resource->GetResource() : nullptr));
    }

}

