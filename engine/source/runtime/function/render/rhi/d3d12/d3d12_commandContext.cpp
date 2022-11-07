#include "d3d12_commandContext.h"
#include "d3d12_device.h"
#include "d3d12_linkedDevice.h"
#include "d3d12_commandQueue.h"
#include "d3d12_pipelineState.h"
#include "d3d12_rootSignature.h"
#include "d3d12_descriptor.h"
#include "d3d12_resource.h"

namespace RHI
{
    // D3D12CommandAllocatorPool
    D3D12CommandAllocatorPool::D3D12CommandAllocatorPool(D3D12LinkedDevice*      Parent,
                                                         D3D12_COMMAND_LIST_TYPE CommandListType) noexcept :
        D3D12LinkedDeviceChild(Parent),
        CommandListType(CommandListType)
    {}

	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> D3D12CommandAllocatorPool::RequestCommandAllocator()
    {
        auto CreateCommandAllocator = [this] {
            Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CommandAllocator;
            VERIFY_D3D12_API(GetParentLinkedDevice()->GetDevice()->CreateCommandAllocator(
                CommandListType, IID_PPV_ARGS(CommandAllocator.ReleaseAndGetAddressOf())));
            return CommandAllocator;
        };
        auto CommandAllocator = CommandAllocatorPool.RetrieveFromPool(CreateCommandAllocator);
        CommandAllocator->Reset();
        return CommandAllocator;
    }

	void
    D3D12CommandAllocatorPool::DiscardCommandAllocator(Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CommandAllocator,
                                                       D3D12SyncHandle                                SyncHandle)
    {
        CommandAllocatorPool.ReturnToPool(std::move(CommandAllocator), SyncHandle);
    }

	// D3D12CommandContext
    D3D12CommandContext::D3D12CommandContext(D3D12LinkedDevice*       Parent,
                                             RHID3D12CommandQueueType Type,
                                             D3D12_COMMAND_LIST_TYPE  CommandListType) :
        D3D12LinkedDeviceChild(Parent),
        Type(Type), CommandListHandle(Parent, CommandListType), CommandAllocator(nullptr),
        CommandAllocatorPool(Parent, CommandListType), CpuConstantAllocator(Parent)
    {}

	D3D12CommandQueue* D3D12CommandContext::GetCommandQueue() const noexcept
    {
        return GetParentLinkedDevice()->GetCommandQueue(Type);
    }

	ID3D12GraphicsCommandList* D3D12CommandContext::GetGraphicsCommandList() const noexcept
    {
        return CommandListHandle.GetGraphicsCommandList();
    }

	ID3D12GraphicsCommandList4* D3D12CommandContext::GetGraphicsCommandList4() const noexcept
    {
        return CommandListHandle.GetGraphicsCommandList4();
    }

	ID3D12GraphicsCommandList6* D3D12CommandContext::GetGraphicsCommandList6() const noexcept
    {
        return CommandListHandle.GetGraphicsCommandList6();
    }

    void D3D12CommandContext::Open()
    {
        if (!CommandAllocator)
        {
            CommandAllocator = CommandAllocatorPool.RequestCommandAllocator();
        }
        CommandListHandle.Open(CommandAllocator.Get());

        if (Type == RHID3D12CommandQueueType::Direct || Type == RHID3D12CommandQueueType::AsyncCompute)
        {
            ID3D12DescriptorHeap* DescriptorHeaps[2] = {
                GetParentLinkedDevice()->GetResourceDescriptorHeap(),
                GetParentLinkedDevice()->GetSamplerDescriptorHeap(),
            };

            CommandListHandle->SetDescriptorHeaps(2, DescriptorHeaps);
        }

        // Reset cache
        Cache = {};
    }

    void D3D12CommandContext::Close() { CommandListHandle.Close(); }

	D3D12SyncHandle D3D12CommandContext::Execute(bool WaitForCompletion)
    {
        D3D12SyncHandle SyncHandle = GetCommandQueue()->ExecuteCommandLists({&CommandListHandle}, WaitForCompletion);

        // Release the command allocator so it can be reused.
        CommandAllocatorPool.DiscardCommandAllocator(std::exchange(CommandAllocator, nullptr), SyncHandle);

        CpuConstantAllocator.Version(SyncHandle);
        return SyncHandle;
    }

    D3D12Allocation D3D12CommandContext::ReserveUploadMemory(size_t SizeInBytes)
    {
        return CpuConstantAllocator.Allocate(SizeInBytes);
    }

	void D3D12CommandContext::TransitionBarrier(D3D12Resource*        Resource,
                                                D3D12_RESOURCE_STATES State,
                                                UINT Subresource /*= D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES*/)
    {
        CommandListHandle.TransitionBarrier(Resource, State, Subresource);
    }

	void D3D12CommandContext::AliasingBarrier(D3D12Resource* BeforeResource, D3D12Resource* AfterResource)
    {
        CommandListHandle.AliasingBarrier(BeforeResource, AfterResource);
    }

	void D3D12CommandContext::UAVBarrier(D3D12Resource* Resource) { CommandListHandle.UAVBarrier(Resource); }

	void D3D12CommandContext::FlushResourceBarriers() { CommandListHandle.FlushResourceBarriers(); }

	bool
    D3D12CommandContext::AssertResourceState(D3D12Resource* Resource, D3D12_RESOURCE_STATES State, UINT Subresource)
    {
        return CommandListHandle.AssertResourceState(Resource, State, Subresource);
    }

	void D3D12CommandContext::SetPipelineState(D3D12PipelineState* PipelineState)
    {
        if (Cache.PipelineState != PipelineState)
        {
            Cache.PipelineState = PipelineState;

            CommandListHandle->SetPipelineState(Cache.PipelineState->GetApiHandle());
        }
    }

	void D3D12CommandContext::SetPipelineState(D3D12RaytracingPipelineState* RaytracingPipelineState)
    {
        if (Cache.Raytracing.RaytracingPipelineState != RaytracingPipelineState)
        {
            Cache.Raytracing.RaytracingPipelineState = RaytracingPipelineState;

            CommandListHandle.GetGraphicsCommandList4()->SetPipelineState1(RaytracingPipelineState->GetApiHandle());
        }
    }

	void D3D12CommandContext::SetGraphicsRootSignature(D3D12RootSignature* RootSignature)
    {
        if (Cache.RootSignature != RootSignature)
        {
            Cache.RootSignature = RootSignature;

            CommandListHandle->SetGraphicsRootSignature(RootSignature->GetApiHandle());

            SetDynamicResourceDescriptorTables<RHI_PIPELINE_STATE_TYPE::Graphics>(RootSignature);
        }
    }

	void D3D12CommandContext::SetComputeRootSignature(D3D12RootSignature* RootSignature)
    {
        if (Cache.RootSignature != RootSignature)
        {
            Cache.RootSignature = RootSignature;

            CommandListHandle->SetComputeRootSignature(RootSignature->GetApiHandle());

            SetDynamicResourceDescriptorTables<RHI_PIPELINE_STATE_TYPE::Compute>(RootSignature);
        }
    }

    void D3D12CommandContext::ClearRenderTarget(D3D12RenderTargetView* RenderTargetView,
                                                D3D12DepthStencilView* DepthStencilView)
    {
        std::vector<D3D12RenderTargetView*> rtvs = {};
        if (RenderTargetView != nullptr)
            rtvs.push_back(RenderTargetView);
        this->ClearRenderTarget(rtvs, DepthStencilView);
    }

    void D3D12CommandContext::ClearRenderTarget(std::vector<D3D12RenderTargetView*> RenderTargetViews,
                                                D3D12DepthStencilView*              DepthStencilView)
    {
        // Transition
        for (auto& RenderTargetView : RenderTargetViews)
        {
            auto& ViewSubresourceSubset = RenderTargetView->GetViewSubresourceSubset();
            for (auto Iter = ViewSubresourceSubset.begin(); Iter != ViewSubresourceSubset.end(); ++Iter)
            {
                for (UINT SubresourceIndex = Iter.StartSubresource(); SubresourceIndex < Iter.EndSubresource();
                     ++SubresourceIndex)
                {
                    TransitionBarrier(
                        RenderTargetView->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, SubresourceIndex);
                }
            }
        }
        if (DepthStencilView)
        {
            auto& ViewSubresourceSubset = DepthStencilView->GetViewSubresourceSubset();
            for (auto Iter = ViewSubresourceSubset.begin(); Iter != ViewSubresourceSubset.end(); ++Iter)
            {
                for (UINT SubresourceIndex = Iter.StartSubresource(); SubresourceIndex < Iter.EndSubresource();
                     ++SubresourceIndex)
                {
                    TransitionBarrier(
                        DepthStencilView->GetResource(), D3D12_RESOURCE_STATE_DEPTH_WRITE, SubresourceIndex);
                }
            }
        }

        // Clear
        for (auto& RenderTargetView : RenderTargetViews)
        {
            D3D12_CLEAR_VALUE ClearValue = RenderTargetView->GetResource()->GetClearValue();
            CommandListHandle->ClearRenderTargetView(RenderTargetView->GetCpuHandle(), ClearValue.Color, 0, nullptr);
        }
        if (DepthStencilView)
        {
            D3D12_CLEAR_VALUE ClearValue = DepthStencilView->GetResource()->GetClearValue();
            FLOAT             Depth      = ClearValue.DepthStencil.Depth;
            UINT8             Stencil    = ClearValue.DepthStencil.Stencil;

            CommandListHandle->ClearDepthStencilView(DepthStencilView->GetCpuHandle(),
                                                     D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
                                                     Depth,
                                                     Stencil,
                                                     0,
                                                     nullptr);
        }
    }

    void D3D12CommandContext::SetRenderTarget(D3D12RenderTargetView* RenderTargetView,
                                              D3D12DepthStencilView* DepthStencilView)
    {
        std::vector<D3D12RenderTargetView*> rtvs = {};
        if (RenderTargetView != nullptr)
            rtvs.push_back(RenderTargetView);
        this->SetRenderTarget(rtvs, DepthStencilView);
    }

    void D3D12CommandContext::SetRenderTarget(std::vector<D3D12RenderTargetView*> RenderTargetViews,
                                              D3D12DepthStencilView*              DepthStencilView)
    {
        UINT                        NumRenderTargetDescriptors = static_cast<UINT>(RenderTargetViews.size());
        D3D12_CPU_DESCRIPTOR_HANDLE pRenderTargetDescriptors[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
        D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilDescriptor                                           = {};
        for (UINT i = 0; i < NumRenderTargetDescriptors; ++i)
        {
            pRenderTargetDescriptors[i] = RenderTargetViews[i]->GetCpuHandle();
        }
        if (DepthStencilView)
        {
            DepthStencilDescriptor = DepthStencilView->GetCpuHandle();
        }
        CommandListHandle->OMSetRenderTargets(NumRenderTargetDescriptors,
                                              pRenderTargetDescriptors,
                                              FALSE,
                                              DepthStencilView ? &DepthStencilDescriptor : nullptr);
    }

	void D3D12CommandContext::SetViewport(const RHIViewport& Viewport)
    {
        SetViewport(Viewport.TopLeftX,
                    Viewport.TopLeftY,
                    Viewport.Width,
                    Viewport.Height,
                    Viewport.MinDepth,
                    Viewport.MaxDepth);
    }

    void D3D12CommandContext::SetViewport(FLOAT TopLeftX,
                                          FLOAT TopLeftY,
                                          FLOAT Width,
                                          FLOAT Height,
                                          FLOAT MinDepth,
                                          FLOAT MaxDepth)
    {
        Cache.Graphics.NumViewports = 1;
        Cache.Graphics.Viewports[0] = CD3DX12_VIEWPORT(TopLeftX, TopLeftY, Width, Height, MinDepth, MaxDepth);
        CommandListHandle->RSSetViewports(Cache.Graphics.NumViewports, Cache.Graphics.Viewports);
    }

    void D3D12CommandContext::SetViewports(std::vector<RHIViewport> Viewports)
    {
        Cache.Graphics.NumViewports = static_cast<UINT>(Viewports.size());
        UINT ViewportIndex          = 0;
        for (auto& Viewport : Viewports)
        {
            Cache.Graphics.Viewports[ViewportIndex++] = CD3DX12_VIEWPORT(Viewport.TopLeftX,
                                                                         Viewport.TopLeftY,
                                                                         Viewport.Width,
                                                                         Viewport.Height,
                                                                         Viewport.MinDepth,
                                                                         Viewport.MaxDepth);
        }
        CommandListHandle->RSSetViewports(Cache.Graphics.NumViewports, Cache.Graphics.Viewports);
    }

	void D3D12CommandContext::SetScissorRect(const RHIRect& ScissorRect)
    {
        SetScissorRect(ScissorRect.Left, ScissorRect.Top, ScissorRect.Right, ScissorRect.Bottom);
    }

    void D3D12CommandContext::SetScissorRect(UINT Left, UINT Top, UINT Right, UINT Bottom)
    {
        Cache.Graphics.NumScissorRects = 1;
        Cache.Graphics.ScissorRects[0] = CD3DX12_RECT(Left, Top, Right, Bottom);
        CommandListHandle->RSSetScissorRects(Cache.Graphics.NumScissorRects, Cache.Graphics.ScissorRects);
    }

	void D3D12CommandContext::SetScissorRects(std::vector<RHIRect> ScissorRects)
    {
        Cache.Graphics.NumScissorRects = static_cast<UINT>(ScissorRects.size());
        UINT ScissorRectIndex          = 0;
        for (auto& ScissorRect : ScissorRects)
        {
            Cache.Graphics.ScissorRects[ScissorRectIndex++] =
                CD3DX12_RECT(ScissorRect.Left, ScissorRect.Top, ScissorRect.Right, ScissorRect.Bottom);
        }
        CommandListHandle->RSSetScissorRects(Cache.Graphics.NumScissorRects, Cache.Graphics.ScissorRects);
    }

    void D3D12CommandContext::SetViewportAndScissorRect(const RHIViewport& vp, const RHIRect& rect)
    {
        SetViewport(vp);
        SetScissorRect(rect);
    }

    void D3D12CommandContext::SetViewportAndScissorRect(UINT x, UINT y, UINT w, UINT h)
    {
        SetViewport((float)x, (float)y, (float)w, (float)h);
        SetScissorRect(x, y, x + w, y + h);
    }

    void D3D12CommandContext::SetStencilRef(UINT StencilRef)
    {
        CommandListHandle->OMSetStencilRef(StencilRef);
    }

    void D3D12CommandContext::SetBlendFactor(Pilot::Color BlendFactor)
    {
        CommandListHandle->OMSetBlendFactor((float*)&BlendFactor);
    }

    void D3D12CommandContext::SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY Topology)
    {
        CommandListHandle->IASetPrimitiveTopology(Topology);
    }

    void D3D12CommandContext::SetGraphicsConstantArray(UINT RootIndex, UINT NumConstants, const void* pConstants)
    {
        CommandListHandle->SetGraphicsRoot32BitConstants(RootIndex, NumConstants, pConstants, 0);
    }

    void D3D12CommandContext::SetGraphicsConstant(UINT RootEntry, UINT Offset, DWParam Val)
    {
        CommandListHandle->SetGraphicsRoot32BitConstant(RootEntry, Val.Uint, Offset);
    }

    void D3D12CommandContext::SetGraphicsConstants(UINT RootIndex, DWParam X)
    {
        CommandListHandle->SetGraphicsRoot32BitConstant(RootIndex, X.Uint, 0);
    }

    void D3D12CommandContext::SetGraphicsConstants(UINT RootIndex, DWParam X, DWParam Y)
    {
        CommandListHandle->SetGraphicsRoot32BitConstant(RootIndex, X.Uint, 0);
        CommandListHandle->SetGraphicsRoot32BitConstant(RootIndex, Y.Uint, 1);
    }

    void D3D12CommandContext::SetGraphicsConstants(UINT RootIndex, DWParam X, DWParam Y, DWParam Z)
    {
        CommandListHandle->SetGraphicsRoot32BitConstant(RootIndex, X.Uint, 0);
        CommandListHandle->SetGraphicsRoot32BitConstant(RootIndex, Y.Uint, 1);
        CommandListHandle->SetGraphicsRoot32BitConstant(RootIndex, Z.Uint, 2);
    }

    void D3D12CommandContext::SetGraphicsConstants(UINT RootIndex, DWParam X, DWParam Y, DWParam Z, DWParam W)
    {
        CommandListHandle->SetGraphicsRoot32BitConstant(RootIndex, X.Uint, 0);
        CommandListHandle->SetGraphicsRoot32BitConstant(RootIndex, Y.Uint, 1);
        CommandListHandle->SetGraphicsRoot32BitConstant(RootIndex, Z.Uint, 2);
        CommandListHandle->SetGraphicsRoot32BitConstant(RootIndex, W.Uint, 3);
    }

    void D3D12CommandContext::SetComputeConstantArray(UINT RootIndex, UINT NumConstants, const void* pConstants)
    {
        CommandListHandle->SetComputeRoot32BitConstants(RootIndex, NumConstants, pConstants, 0);
    }

    void D3D12CommandContext::SetComputeConstant(UINT RootEntry, UINT Offset, DWParam Val)
    {
        CommandListHandle->SetComputeRoot32BitConstant(RootEntry, Val.Uint, Offset);
    }

    void D3D12CommandContext::SetComputeConstants(UINT RootIndex, DWParam X)
    {
        CommandListHandle->SetComputeRoot32BitConstant(RootIndex, X.Uint, 0);
    }

    void D3D12CommandContext::SetComputeConstants(UINT RootIndex, DWParam X, DWParam Y)
    {
        CommandListHandle->SetComputeRoot32BitConstant(RootIndex, X.Uint, 0);
        CommandListHandle->SetComputeRoot32BitConstant(RootIndex, Y.Uint, 1);
    }

    void D3D12CommandContext::SetComputeConstants(UINT RootIndex, DWParam X, DWParam Y, DWParam Z)
    {
        CommandListHandle->SetComputeRoot32BitConstant(RootIndex, X.Uint, 0);
        CommandListHandle->SetComputeRoot32BitConstant(RootIndex, Y.Uint, 1);
        CommandListHandle->SetComputeRoot32BitConstant(RootIndex, Z.Uint, 2);
    }

    void D3D12CommandContext::SetComputeConstants(UINT RootIndex, DWParam X, DWParam Y, DWParam Z, DWParam W)
    {
        CommandListHandle->SetComputeRoot32BitConstant(RootIndex, X.Uint, 0);
        CommandListHandle->SetComputeRoot32BitConstant(RootIndex, Y.Uint, 1);
        CommandListHandle->SetComputeRoot32BitConstant(RootIndex, Z.Uint, 2);
        CommandListHandle->SetComputeRoot32BitConstant(RootIndex, W.Uint, 3);
    }

    void D3D12CommandContext::SetGraphicsConstantBuffer(UINT RootIndex, D3D12_GPU_VIRTUAL_ADDRESS CBV)
    {
        CommandListHandle->SetGraphicsRootConstantBufferView(RootIndex, CBV);
    }

    void D3D12CommandContext::SetComputeConstantBuffer(UINT RootIndex, D3D12_GPU_VIRTUAL_ADDRESS CBV)
    {
        CommandListHandle->SetComputeRootConstantBufferView(RootIndex, CBV);
    }

	void D3D12CommandContext::SetGraphicsConstantBuffer(UINT RootParameterIndex, UINT64 Size, const void* Data)
    {
        D3D12Allocation Allocation = CpuConstantAllocator.Allocate(Size);
        std::memcpy(Allocation.CpuVirtualAddress, Data, Size);
        CommandListHandle->SetGraphicsRootConstantBufferView(RootParameterIndex, Allocation.GpuVirtualAddress);
    }

	void D3D12CommandContext::SetComputeConstantBuffer(UINT RootParameterIndex, UINT64 Size, const void* Data)
    {
        D3D12Allocation Allocation = CpuConstantAllocator.Allocate(Size);
        std::memcpy(Allocation.CpuVirtualAddress, Data, Size);
        CommandListHandle->SetComputeRootConstantBufferView(RootParameterIndex, Allocation.GpuVirtualAddress);
    }
    
    void D3D12CommandContext::SetGraphicsBufferSRV(UINT RootIndex, const std::shared_ptr<D3D12Buffer> BufferSRV, UINT64 Offset)
    {
        CResourceState& resourceState = BufferSRV->GetResourceState();
        assert(resourceState.IsUniform() && ((resourceState.GetSubresourceState(0) & D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE) != 0));
        CommandListHandle->SetGraphicsRootShaderResourceView(RootIndex, BufferSRV->GetGpuVirtualAddress() + Offset);
    }

    void D3D12CommandContext::SetComputeBufferSRV(UINT RootIndex, const std::shared_ptr<D3D12Buffer> BufferSRV, UINT64 Offset)
    {
        CResourceState& resourceState = BufferSRV->GetResourceState();
        assert(resourceState.IsUniform() && ((resourceState.GetSubresourceState(0) & D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE) != 0));
        CommandListHandle->SetComputeRootShaderResourceView(RootIndex, BufferSRV->GetGpuVirtualAddress() + Offset);
    }

    void D3D12CommandContext::SetGraphicsBufferUAV(UINT RootIndex, const std::shared_ptr<D3D12Buffer> BufferUAV, UINT64 Offset)
    {
        CResourceState& resourceState = BufferUAV->GetResourceState();
        assert(resourceState.IsUniform() && ((resourceState.GetSubresourceState(0) & D3D12_RESOURCE_STATE_UNORDERED_ACCESS) != 0));
        CommandListHandle->SetGraphicsRootUnorderedAccessView(RootIndex, BufferUAV->GetGpuVirtualAddress() + Offset);
    }

    void D3D12CommandContext::SetComputeBufferUAV(UINT RootIndex, const std::shared_ptr<D3D12Buffer> BufferUAV, UINT64 Offset)
    {
        CResourceState& resourceState = BufferUAV->GetResourceState();
        assert(resourceState.IsUniform() && ((resourceState.GetSubresourceState(0) & D3D12_RESOURCE_STATE_UNORDERED_ACCESS) != 0));
        CommandListHandle->SetComputeRootUnorderedAccessView(RootIndex, BufferUAV->GetGpuVirtualAddress() + Offset);
    }

    void D3D12CommandContext::SetGraphicsDescriptorTable(UINT RootIndex, D3D12_GPU_DESCRIPTOR_HANDLE FirstHandle)
    {
        CommandListHandle->SetGraphicsRootDescriptorTable(RootIndex, FirstHandle);
    }

    void D3D12CommandContext::SetComputeDescriptorTable(UINT RootIndex, D3D12_GPU_DESCRIPTOR_HANDLE FirstHandle)
    {
        CommandListHandle->SetComputeRootDescriptorTable(RootIndex, FirstHandle);
    }

    void D3D12CommandContext::DrawInstanced(UINT VertexCount,
                                            UINT InstanceCount,
                                            UINT StartVertexLocation,
                                            UINT StartInstanceLocation)
    {
        CommandListHandle.FlushResourceBarriers();
        CommandListHandle->DrawInstanced(VertexCount, InstanceCount, StartVertexLocation, StartInstanceLocation);
    }

    void D3D12CommandContext::DrawIndexedInstanced(UINT IndexCount,
                                                   UINT InstanceCount,
                                                   UINT StartIndexLocation,
                                                   INT  BaseVertexLocation,
                                                   UINT StartInstanceLocation)
    {
        CommandListHandle.FlushResourceBarriers();
        CommandListHandle->DrawIndexedInstanced(
            IndexCount, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
    }

    void D3D12CommandContext::Dispatch(UINT ThreadGroupCountX, UINT ThreadGroupCountY, UINT ThreadGroupCountZ)
    {
        CommandListHandle.FlushResourceBarriers();
        CommandListHandle->Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
    }

    void D3D12CommandContext::DispatchRays(const D3D12_DISPATCH_RAYS_DESC* pDesc)
    {
        CommandListHandle.FlushResourceBarriers();
        CommandListHandle.GetGraphicsCommandList4()->DispatchRays(pDesc);
    }

    void D3D12CommandContext::DispatchMesh(UINT ThreadGroupCountX, UINT ThreadGroupCountY, UINT ThreadGroupCountZ)
    {
        CommandListHandle.FlushResourceBarriers();
        CommandListHandle.GetGraphicsCommandList6()->DispatchMesh(
            ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
    }

    void D3D12CommandContext::ResetCounter(D3D12Resource* CounterResource, UINT64 CounterOffset, UINT Value /*= 0*/)
    {
        D3D12Allocation Allocation = CpuConstantAllocator.Allocate(sizeof(UINT));
        std::memcpy(Allocation.CpuVirtualAddress, &Value, sizeof(UINT));

        CommandListHandle->CopyBufferRegion(
            CounterResource->GetResource(), CounterOffset, Allocation.Resource, Allocation.Offset, sizeof(UINT));
    }

    template<RHI_PIPELINE_STATE_TYPE PsoType>
    void D3D12CommandContext::SetDynamicResourceDescriptorTables(D3D12RootSignature* RootSignature)
    {
        ID3D12GraphicsCommandList* CommandList = CommandListHandle.GetGraphicsCommandList();

        // Bindless descriptors
        UINT NumParameters      = RootSignature->GetNumParameters();
        UINT Offset             = NumParameters - RootParameters::DescriptorTable::NumRootParameters;
        auto ResourceDescriptor = GetParentLinkedDevice()->GetResourceDescriptorHeap().GetGpuDescriptorHandle(0);
        auto SamplerDescriptor  = GetParentLinkedDevice()->GetSamplerDescriptorHeap().GetGpuDescriptorHandle(0);

        (CommandList->*D3D12DescriptorTableTraits<PsoType>::Bind())(
            RootParameters::DescriptorTable::ShaderResourceDescriptorTable + Offset, ResourceDescriptor);
        (CommandList->*D3D12DescriptorTableTraits<PsoType>::Bind())(
            RootParameters::DescriptorTable::UnorderedAccessDescriptorTable + Offset, ResourceDescriptor);
        (CommandList->*D3D12DescriptorTableTraits<PsoType>::Bind())(
            RootParameters::DescriptorTable::SamplerDescriptorTable + Offset, SamplerDescriptor);
    }

    D3D12ScopedEventObject::D3D12ScopedEventObject(D3D12CommandContext& CommandContext, std::string_view Name) :
        ProfileBlock(CommandContext.GetParentLinkedDevice()->GetProfiler(),
                     CommandContext.GetCommandQueue(),
                     CommandContext.GetGraphicsCommandList(),
                     Name)
#ifdef _DEBUG
        ,
        PixEvent(CommandContext.GetGraphicsCommandList(), 0, Name.data())
#endif
    {
        // Copy queue profiling currently not supported
        assert(CommandContext.GetCommandQueue()->GetType() != D3D12_COMMAND_LIST_TYPE_COPY);
    }

}
