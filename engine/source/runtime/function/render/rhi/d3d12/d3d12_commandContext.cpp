#include "d3d12_commandContext.h"
#include "d3d12_device.h"
#include "d3d12_linkedDevice.h"
#include "d3d12_commandQueue.h"
#include "d3d12_pipelineState.h"
#include "d3d12_rootSignature.h"
#include "d3d12_descriptor.h"
#include "d3d12_resource.h"

#include "runtime/core/base/utility.h"
#include "runtime/core/base/macro.h"

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
        Type(Type), CommandListType(CommandListType), CommandListHandle(Parent, CommandListType),
        CommandAllocator(nullptr), CommandAllocatorPool(Parent, CommandListType), CpuConstantAllocator(Parent)
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

    D3D12GraphicsContext& D3D12CommandContext::GetGraphicsContext()
    {
        ASSERT(CommandListType != D3D12_COMMAND_LIST_TYPE_COMPUTE, "Cannot convert async compute context to graphics");
        return reinterpret_cast<D3D12GraphicsContext&>(*this);
    }

    D3D12ComputeContext& D3D12CommandContext::GetComputeContext()
    {
        return reinterpret_cast<D3D12ComputeContext&>(*this);
    }

    void D3D12CommandContext::Open()
    {
        if (!CommandAllocator)
        {
            CommandAllocator = CommandAllocatorPool.RequestCommandAllocator();
        }
        bool isCommandListNew = CommandListHandle.Open(CommandAllocator.Get());
        if (!isCommandListNew)
            return;

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

    void D3D12CommandContext::Close()
    {
        CommandListHandle.Close();
    }

	D3D12SyncHandle D3D12CommandContext::Execute(bool WaitForCompletion)
    {
        D3D12SyncHandle SyncHandle = GetCommandQueue()->ExecuteCommandLists({&CommandListHandle}, WaitForCompletion);

        // Release the command allocator so it can be reused.
        CommandAllocatorPool.DiscardCommandAllocator(std::exchange(CommandAllocator, nullptr), SyncHandle);

        CpuConstantAllocator.Version(SyncHandle);
        return SyncHandle;
    }

    void D3D12CommandContext::CopyBuffer(D3D12Resource* Dest, D3D12Resource* Src)
    {
        TransitionBarrier(Dest, D3D12_RESOURCE_STATE_COPY_DEST);
        TransitionBarrier(Src, D3D12_RESOURCE_STATE_COPY_SOURCE);
        FlushResourceBarriers();
        CommandListHandle->CopyResource(Dest->GetResource(), Src->GetResource());
    }

    void D3D12CommandContext::CopyBufferRegion(D3D12Resource* Dest, UINT64 DestOffset, D3D12Resource* Src, UINT64 SrcOffset, UINT64 NumBytes)
    {
        TransitionBarrier(Dest, D3D12_RESOURCE_STATE_COPY_DEST);
        TransitionBarrier(Src, D3D12_RESOURCE_STATE_COPY_SOURCE);
        FlushResourceBarriers();
        CommandListHandle->CopyBufferRegion(Dest->GetResource(), DestOffset, Src->GetResource(), SrcOffset, NumBytes);
    }

    void D3D12CommandContext::CopySubresource(D3D12Resource* Dest, UINT DestSubIndex, D3D12Resource* Src, UINT SrcSubIndex)
    {
        FlushResourceBarriers();
        D3D12_TEXTURE_COPY_LOCATION DestLocation = {
            Dest->GetResource(), D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX, DestSubIndex};
        D3D12_TEXTURE_COPY_LOCATION SrcLocation = {
            Src->GetResource(), D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX, SrcSubIndex};
        CommandListHandle->CopyTextureRegion(&DestLocation, 0, 0, 0, &SrcLocation, nullptr);
    }

    void D3D12CommandContext::CopyTextureRegion(D3D12Resource* Dest, UINT x, UINT y, UINT z, D3D12Resource* Source, RECT& Rect)
    {
        TransitionBarrier(Dest, D3D12_RESOURCE_STATE_COPY_DEST);
        TransitionBarrier(Source, D3D12_RESOURCE_STATE_COPY_SOURCE);
        FlushResourceBarriers();

        D3D12_TEXTURE_COPY_LOCATION destLoc = CD3DX12_TEXTURE_COPY_LOCATION(Dest->GetResource(), 0);
        D3D12_TEXTURE_COPY_LOCATION srcLoc  = CD3DX12_TEXTURE_COPY_LOCATION(Source->GetResource(), 0);

        D3D12_BOX box = {};
        box.back      = 1;
        box.left      = Rect.left;
        box.right     = Rect.right;
        box.top       = Rect.top;
        box.bottom    = Rect.bottom;

        CommandListHandle->CopyTextureRegion(&destLoc, x, y, z, &srcLoc, &box);
    }

    void D3D12CommandContext::ResetCounter(D3D12Resource* CounterResource, UINT64 CounterOffset, UINT Value /*= 0*/)
    {
        D3D12Allocation Allocation = CpuConstantAllocator.Allocate(sizeof(UINT));
        std::memcpy(Allocation.CpuVirtualAddress, &Value, sizeof(UINT));

        D3D12_RESOURCE_STATES originalState = CounterResource->GetResourceState().GetSubresourceState(0);

        TransitionBarrier(
            CounterResource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, true);
        CommandListHandle->CopyBufferRegion(
            CounterResource->GetResource(), CounterOffset, Allocation.Resource, Allocation.Offset, sizeof(UINT));
        TransitionBarrier(CounterResource, originalState, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, true);
    }

    D3D12Allocation D3D12CommandContext::ReserveUploadMemory(UINT64 SizeInBytes, UINT Alignment)
    {
        return CpuConstantAllocator.Allocate(SizeInBytes, Alignment);
    }

    void D3D12CommandContext::WriteBuffer(D3D12Resource* Dest, UINT64 DestOffset, const void* BufferData, UINT64 NumBytes)
    {

    }

    void D3D12CommandContext::FillBuffer(D3D12Resource* Dest, UINT64 DestOffset, DWParam Value, UINT64 NumBytes)
    {

    }

    void D3D12CommandContext::TransitionBarrier(D3D12Resource* Resource, D3D12_RESOURCE_STATES State, UINT Subresource, bool FlushImmediate)
    {
        D3D12_RESOURCE_STATES trackedResourceState = CommandListHandle.GetResourceStateTracked(Resource);

        if (trackedResourceState != State)
        {
            CommandListHandle.TransitionBarrier(Resource, State, Subresource);
        }
        
        if (trackedResourceState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS &&
            State == D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
        {
            CommandListHandle.UAVBarrier(Resource);
        }

        if (FlushImmediate)
            CommandListHandle.FlushResourceBarriers();
    }

    void D3D12CommandContext::AliasingBarrier(D3D12Resource* BeforeResource, D3D12Resource* AfterResource, bool FlushImmediate)
    {
        CommandListHandle.AliasingBarrier(BeforeResource, AfterResource);

        if (FlushImmediate)
            CommandListHandle.FlushResourceBarriers();
    }

    void D3D12CommandContext::InsertUAVBarrier(D3D12Resource* Resource, bool FlushImmediate)
    {
        CommandListHandle.UAVBarrier(Resource);

        if (FlushImmediate)
            CommandListHandle.FlushResourceBarriers();
    }

	void D3D12CommandContext::FlushResourceBarriers()
    {
        CommandListHandle.FlushResourceBarriers();
    }

	bool D3D12CommandContext::AssertResourceState(D3D12Resource* Resource, D3D12_RESOURCE_STATES State, UINT Subresource)
    {
        return CommandListHandle.AssertResourceState(Resource, State, Subresource);
    }

    void D3D12CommandContext::SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE Type, ID3D12DescriptorHeap* HeapPtr)
    {
        if (m_CurrentDescriptorHeaps[Type] != HeapPtr)
        {
            m_CurrentDescriptorHeaps[Type] = HeapPtr;
            BindDescriptorHeaps();
        }
    }

    void D3D12CommandContext::SetDescriptorHeaps(UINT HeapCount, D3D12_DESCRIPTOR_HEAP_TYPE Type[], ID3D12DescriptorHeap* HeapPtrs[])
    {
        bool AnyChanged = false;

        for (UINT i = 0; i < HeapCount; ++i)
        {
            if (m_CurrentDescriptorHeaps[Type[i]] != HeapPtrs[i])
            {
                m_CurrentDescriptorHeaps[Type[i]] = HeapPtrs[i];
                AnyChanged                        = true;
            }
        }

        if (AnyChanged)
            BindDescriptorHeaps();
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

    void D3D12CommandContext::SetPredication(D3D12Resource* Buffer, UINT64 BufferOffset, D3D12_PREDICATION_OP Op)
    {
        CommandListHandle->SetPredication(Buffer->GetResource(), BufferOffset, Op);
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

    // ================================Graphic=====================================

    void D3D12GraphicsContext::ClearRenderTarget(std::vector<D3D12RenderTargetView*> RenderTargetViews, D3D12DepthStencilView* DepthStencilView)
    {
        // Transition
        for (auto& RenderTargetView : RenderTargetViews)
        {
            auto& ViewSubresourceSubset = RenderTargetView->GetViewSubresourceSubset();
            for (auto Iter = ViewSubresourceSubset.begin(); Iter != ViewSubresourceSubset.end(); ++Iter)
            {
                for (UINT SubresourceIndex = Iter.StartSubresource(); SubresourceIndex < Iter.EndSubresource(); ++SubresourceIndex)
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
                for (UINT SubresourceIndex = Iter.StartSubresource(); SubresourceIndex < Iter.EndSubresource(); ++SubresourceIndex)
                {
                    TransitionBarrier(
                        DepthStencilView->GetResource(), D3D12_RESOURCE_STATE_DEPTH_WRITE, SubresourceIndex);
                }
            }
        }

        // Flush
        FlushResourceBarriers();

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

    void D3D12GraphicsContext::ClearRenderTarget(D3D12RenderTargetView* RenderTargetView, D3D12DepthStencilView* DepthStencilView)
    {
        std::vector<D3D12RenderTargetView*> rtvs = {};
        if (RenderTargetView != nullptr)
            rtvs.push_back(RenderTargetView);
        this->ClearRenderTarget(rtvs, DepthStencilView);
    }

    void D3D12GraphicsContext::ClearUAV(D3D12UnorderedAccessView* UnorderedAccessView)
    {
        // TODO:
    }

    void D3D12GraphicsContext::BeginQuery(ID3D12QueryHeap* QueryHeap, D3D12_QUERY_TYPE Type, UINT HeapIndex)
    {
        // TODO:
    }

    void D3D12GraphicsContext::EndQuery(ID3D12QueryHeap* QueryHeap, D3D12_QUERY_TYPE Type, UINT HeapIndex)
    {
        // TODO:
    }

    void D3D12GraphicsContext::ResolveQueryData(ID3D12QueryHeap* QueryHeap, D3D12_QUERY_TYPE Type, UINT StartIndex, UINT NumQueries, ID3D12Resource* DestinationBuffer, UINT64 DestinationBufferOffset)
    {
        // TODO:
    }

	void D3D12GraphicsContext::SetRootSignature(D3D12RootSignature* RootSignature)
    {
        if (Cache.RootSignature != RootSignature)
        {
            Cache.RootSignature = RootSignature;

            CommandListHandle->SetGraphicsRootSignature(RootSignature->GetApiHandle());

            SetDynamicResourceDescriptorTables<RHI_PIPELINE_STATE_TYPE::Graphics>(RootSignature);
        }
    }

    void D3D12GraphicsContext::SetRenderTargets(std::vector<D3D12RenderTargetView*> RenderTargetViews)
    {
        SetRenderTargets(RenderTargetViews, nullptr);
    }

    void D3D12GraphicsContext::SetRenderTargets(std::vector<D3D12RenderTargetView*> RenderTargetViews, D3D12DepthStencilView* DepthStencilView)
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

    void D3D12GraphicsContext::SetRenderTarget(D3D12RenderTargetView* RenderTargetView)
    {
        SetRenderTarget(RenderTargetView, nullptr);
    }

    void D3D12GraphicsContext::SetRenderTarget(D3D12RenderTargetView* RenderTargetView, D3D12DepthStencilView* DepthStencilView)
    {
        std::vector<D3D12RenderTargetView*> rtvs = {};
        if (RenderTargetView != nullptr)
            rtvs.push_back(RenderTargetView);
        this->SetRenderTargets(rtvs, DepthStencilView);
    }

    void D3D12GraphicsContext::SetViewport(const RHIViewport& Viewport)
    {
        SetViewport(Viewport.TopLeftX,
                    Viewport.TopLeftY,
                    Viewport.Width,
                    Viewport.Height,
                    Viewport.MinDepth,
                    Viewport.MaxDepth);
    }

    void D3D12GraphicsContext::SetViewport(FLOAT TopLeftX,
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

    void D3D12GraphicsContext::SetViewports(std::vector<RHIViewport> Viewports)
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

    void D3D12GraphicsContext::SetScissorRect(const RHIRect& ScissorRect)
    {
        SetScissorRect(ScissorRect.Left, ScissorRect.Top, ScissorRect.Right, ScissorRect.Bottom);
    }

    void D3D12GraphicsContext::SetScissorRect(UINT Left, UINT Top, UINT Right, UINT Bottom)
    {
        Cache.Graphics.NumScissorRects = 1;
        Cache.Graphics.ScissorRects[0] = CD3DX12_RECT(Left, Top, Right, Bottom);
        CommandListHandle->RSSetScissorRects(Cache.Graphics.NumScissorRects, Cache.Graphics.ScissorRects);
    }

    void D3D12GraphicsContext::SetScissorRects(std::vector<RHIRect> ScissorRects)
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

    void D3D12GraphicsContext::SetViewportAndScissorRect(const RHIViewport& vp, const RHIRect& rect)
    {
        SetViewport(vp);
        SetScissorRect(rect);
    }

    void D3D12GraphicsContext::SetViewportAndScissorRect(UINT x, UINT y, UINT w, UINT h)
    {
        SetViewport((float)x, (float)y, (float)w, (float)h);
        SetScissorRect(x, y, x + w, y + h);
    }

    void D3D12GraphicsContext::SetStencilRef(UINT StencilRef) { CommandListHandle->OMSetStencilRef(StencilRef); }

    void D3D12GraphicsContext::SetBlendFactor(Pilot::Color BlendFactor)
    {
        CommandListHandle->OMSetBlendFactor((float*)&BlendFactor);
    }

    void D3D12GraphicsContext::SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY Topology)
    {
        CommandListHandle->IASetPrimitiveTopology(Topology);
    }

    void D3D12GraphicsContext::SetConstantArray(UINT RootIndex, UINT NumConstants, const void* pConstants)
    {
        CommandListHandle->SetGraphicsRoot32BitConstants(RootIndex, NumConstants, pConstants, 0);
    }

    void D3D12GraphicsContext::SetConstant(UINT RootEntry, UINT Offset, DWParam Val)
    {
        CommandListHandle->SetGraphicsRoot32BitConstant(RootEntry, Val.Uint, Offset);
    }

    void D3D12GraphicsContext::SetConstants(UINT RootIndex, DWParam X)
    {
        CommandListHandle->SetGraphicsRoot32BitConstant(RootIndex, X.Uint, 0);
    }

    void D3D12GraphicsContext::SetConstants(UINT RootIndex, DWParam X, DWParam Y)
    {
        CommandListHandle->SetGraphicsRoot32BitConstant(RootIndex, X.Uint, 0);
        CommandListHandle->SetGraphicsRoot32BitConstant(RootIndex, Y.Uint, 1);
    }

    void D3D12GraphicsContext::SetConstants(UINT RootIndex, DWParam X, DWParam Y, DWParam Z)
    {
        CommandListHandle->SetGraphicsRoot32BitConstant(RootIndex, X.Uint, 0);
        CommandListHandle->SetGraphicsRoot32BitConstant(RootIndex, Y.Uint, 1);
        CommandListHandle->SetGraphicsRoot32BitConstant(RootIndex, Z.Uint, 2);
    }

    void D3D12GraphicsContext::SetConstants(UINT RootIndex, DWParam X, DWParam Y, DWParam Z, DWParam W)
    {
        CommandListHandle->SetGraphicsRoot32BitConstant(RootIndex, X.Uint, 0);
        CommandListHandle->SetGraphicsRoot32BitConstant(RootIndex, Y.Uint, 1);
        CommandListHandle->SetGraphicsRoot32BitConstant(RootIndex, Z.Uint, 2);
        CommandListHandle->SetGraphicsRoot32BitConstant(RootIndex, W.Uint, 3);
    }

    void D3D12GraphicsContext::SetConstantBuffer(UINT RootIndex, D3D12_GPU_VIRTUAL_ADDRESS CBV)
    {
        CommandListHandle->SetGraphicsRootConstantBufferView(RootIndex, CBV);
    }

    void D3D12GraphicsContext::SetDynamicConstantBufferView(UINT RootIndex, UINT64 BufferSize, const void* BufferData)
    {
        ASSERT(BufferData != nullptr && Pilot::IsAligned(BufferData, 16));
        D3D12Allocation cb = CpuConstantAllocator.Allocate(BufferSize);
        // SIMDMemCopy(cb.DataPtr, BufferData, Math::AlignUp(BufferSize, 16) >> 4);
        memcpy(cb.CpuVirtualAddress, BufferData, BufferSize);
        CommandListHandle->SetGraphicsRootConstantBufferView(RootIndex, cb.GpuVirtualAddress);
    }

    void D3D12GraphicsContext::SetBufferSRV(UINT RootIndex, const std::shared_ptr<D3D12Buffer> BufferSRV, UINT64 Offset)
    {
        ASSERT((BufferSRV->GetResourceState().GetSubresourceState(0) &
                (D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)) != 0);
        CommandListHandle->SetGraphicsRootShaderResourceView(RootIndex, BufferSRV->GetGpuVirtualAddress(0) + Offset);
    }

    void D3D12GraphicsContext::SetBufferUAV(UINT RootIndex, const std::shared_ptr<D3D12Buffer> BufferUAV, UINT64 Offset)
    {
        ASSERT((BufferUAV->GetResourceState().GetSubresourceState(0) & D3D12_RESOURCE_STATE_UNORDERED_ACCESS) != 0);
        CommandListHandle->SetGraphicsRootUnorderedAccessView(RootIndex, BufferUAV->GetGpuVirtualAddress(0) + Offset);
    }

    void D3D12GraphicsContext::SetDescriptorTable(UINT RootIndex, D3D12_GPU_DESCRIPTOR_HANDLE FirstHandle)
    {
        CommandListHandle->SetGraphicsRootDescriptorTable(RootIndex, FirstHandle);
    }

    void D3D12GraphicsContext::SetIndexBuffer(const D3D12_INDEX_BUFFER_VIEW& IBView)
    {
        CommandListHandle->IASetIndexBuffer(&IBView);
    }

    void D3D12GraphicsContext::SetVertexBuffer(UINT Slot, const D3D12_VERTEX_BUFFER_VIEW& VBView)
    {
        SetVertexBuffers(Slot, 1, &VBView);
    }

    void D3D12GraphicsContext::SetVertexBuffers(UINT StartSlot, UINT Count, const D3D12_VERTEX_BUFFER_VIEW VBViews[])
    {
        CommandListHandle->IASetVertexBuffers(StartSlot, Count, VBViews);
    }

    void D3D12GraphicsContext::SetDynamicVB(UINT Slot, UINT64 NumVertices, UINT64 VertexStride, const void* VertexData)
    {
        ASSERT(VertexData != nullptr && Pilot::IsAligned(VertexData, 16));

        size_t BufferSize = Pilot::AlignUp(NumVertices * VertexStride, 16);
        D3D12Allocation vb = CpuConstantAllocator.Allocate(BufferSize);

        SIMDMemCopy(vb.CpuVirtualAddress, VertexData, BufferSize >> 4);

        D3D12_VERTEX_BUFFER_VIEW VBView;
        VBView.BufferLocation = vb.GpuVirtualAddress;
        VBView.SizeInBytes    = (UINT)BufferSize;
        VBView.StrideInBytes  = (UINT)VertexStride;

        CommandListHandle->IASetVertexBuffers(Slot, 1, &VBView);
    }

    void D3D12GraphicsContext::SetDynamicIB(UINT64 IndexCount, const UINT16* IndexData)
    {
        ASSERT(IndexData != nullptr && Pilot::IsAligned(IndexData, 16));

        size_t BufferSize = Pilot::AlignUp(IndexCount * sizeof(uint16_t), 16);
        D3D12Allocation ib = CpuConstantAllocator.Allocate(BufferSize);

        SIMDMemCopy(ib.CpuVirtualAddress, IndexData, BufferSize >> 4);

        D3D12_INDEX_BUFFER_VIEW IBView;
        IBView.BufferLocation = ib.GpuVirtualAddress;
        IBView.SizeInBytes    = (UINT)(IndexCount * sizeof(uint16_t));
        IBView.Format         = DXGI_FORMAT_R16_UINT;

        CommandListHandle->IASetIndexBuffer(&IBView);
    }

    void D3D12GraphicsContext::SetDynamicSRV(UINT RootIndex, UINT64 BufferSize, const void* BufferData)
    {
        ASSERT(BufferData != nullptr && Pilot::IsAligned(BufferData, 16));
        D3D12Allocation cb = CpuConstantAllocator.Allocate(BufferSize);
        SIMDMemCopy(cb.CpuVirtualAddress, BufferData, Pilot::AlignUp(BufferSize, 16) >> 4);
        CommandListHandle->SetGraphicsRootShaderResourceView(RootIndex, cb.GpuVirtualAddress);
    }

    void D3D12GraphicsContext::Draw(UINT VertexCount, UINT VertexStartOffset)
    {
        DrawInstanced(VertexCount, 1, VertexStartOffset, 0);
    }

    void D3D12GraphicsContext::DrawIndexed(UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation)
    {
        DrawIndexedInstanced(IndexCount, 1, StartIndexLocation, BaseVertexLocation, 0);
    }

    void D3D12GraphicsContext::DrawInstanced(UINT VertexCountPerInstance,
                                             UINT InstanceCount,
                                             UINT StartVertexLocation,
                                             UINT StartInstanceLocation)
    {
        FlushResourceBarriers();
        m_DynamicViewDescriptorHeap.CommitGraphicsRootDescriptorTables(CommandListHandle);
        m_DynamicSamplerDescriptorHeap.CommitGraphicsRootDescriptorTables(CommandListHandle);
        CommandListHandle->DrawInstanced(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
    }

    void D3D12GraphicsContext::DrawIndexedInstanced(UINT IndexCountPerInstance,
                                                    UINT InstanceCount,
                                                    UINT StartIndexLocation,
                                                    INT  BaseVertexLocation,
                                                    UINT StartInstanceLocation)
    {
        FlushResourceBarriers();
        m_DynamicViewDescriptorHeap.CommitGraphicsRootDescriptorTables(CommandListHandle);
        m_DynamicSamplerDescriptorHeap.CommitGraphicsRootDescriptorTables(CommandListHandle);
        CommandListHandle->DrawIndexedInstanced(
        IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
    }

    void D3D12GraphicsContext::DrawIndirect(D3D12Resource& ArgumentBuffer, UINT64 ArgumentBufferOffset)
    {
        ExecuteIndirect(Graphics::DrawIndirectCommandSignature, ArgumentBuffer, ArgumentBufferOffset);
    }

    void D3D12GraphicsContext::ExecuteIndirect(D3D12CommandSignature& CommandSig,
                                               D3D12Resource&         ArgumentBuffer,
                                               UINT64                 ArgumentStartOffset,
                                               UINT32                 MaxCommands,
                                               D3D12Resource*         CommandCounterBuffer,
                                               UINT64                 CounterOffset)
    {
        FlushResourceBarriers();
        m_DynamicViewDescriptorHeap.CommitComputeRootDescriptorTables(CommandListHandle);
        m_DynamicSamplerDescriptorHeap.CommitComputeRootDescriptorTables(CommandListHandle);
        CommandListHandle->ExecuteIndirect(CommandSig,
                                           MaxCommands,
                                           ArgumentBuffer.GetResource(),
                                           ArgumentStartOffset,
                                           CommandCounterBuffer == nullptr ? nullptr : CommandCounterBuffer->GetResource(),
                                           CounterOffset);
    }

    // ================================Compute=====================================

    void D3D12ComputeContext::ClearUAV(std::shared_ptr<D3D12Buffer> Target)
    {
        FlushResourceBarriers();

        // After binding a UAV, we can get a GPU handle that is required to clear it as a UAV (because it essentially
        // runs a shader to set all of the values).
        D3D12_GPU_DESCRIPTOR_HANDLE GpuVisibleHandle = m_DynamicViewDescriptorHeap.UploadDirect(Target.GetUAV());
        const UINT                  ClearColor[4]    = {};
        CommandListHandle->ClearUnorderedAccessViewUint(
            GpuVisibleHandle, Target.GetUAV(), Target->GetResource(), ClearColor, 0, nullptr);
    }

    void D3D12ComputeContext::ClearUAV(std::shared_ptr<D3D12Texture> Target)
    {
        FlushResourceBarriers();

        // After binding a UAV, we can get a GPU handle that is required to clear it as a UAV (because it essentially
        // runs a shader to set all of the values).
        D3D12_GPU_DESCRIPTOR_HANDLE GpuVisibleHandle = m_DynamicViewDescriptorHeap.UploadDirect(Target.GetUAV());
        CD3DX12_RECT                ClearRect(0, 0, (LONG)Target.GetWidth(), (LONG)Target.GetHeight());

        // TODO: My Nvidia card is not clearing UAVs with either Float or Uint variants.
        const float* ClearColor = Target.GetClearColor().GetPtr();
        CommandListHandle->ClearUnorderedAccessViewFloat(
            GpuVisibleHandle, Target.GetUAV(), Target->GetResource(), ClearColor, 1, &ClearRect);
    }

	void D3D12ComputeContext::SetRootSignature(D3D12RootSignature* RootSignature)
    {
        if (Cache.RootSignature != RootSignature)
        {
            Cache.RootSignature = RootSignature;

            CommandListHandle->SetComputeRootSignature(RootSignature->GetApiHandle());

            SetDynamicResourceDescriptorTables<RHI_PIPELINE_STATE_TYPE::Compute>(RootSignature);
        }
    }

    void D3D12ComputeContext::SetConstantArray(UINT RootIndex, UINT NumConstants, const void* pConstants)
    {
        CommandListHandle->SetComputeRoot32BitConstants(RootIndex, NumConstants, pConstants, 0);
    }

    void D3D12ComputeContext::SetConstant(UINT RootEntry, UINT Offset, DWParam Val)
    {
        CommandListHandle->SetComputeRoot32BitConstant(RootEntry, Val.Uint, Offset);
    }

    void D3D12ComputeContext::SetConstants(UINT RootIndex, DWParam X)
    {
        CommandListHandle->SetComputeRoot32BitConstant(RootIndex, X.Uint, 0);
    }

    void D3D12ComputeContext::SetConstants(UINT RootIndex, DWParam X, DWParam Y)
    {
        CommandListHandle->SetComputeRoot32BitConstant(RootIndex, X.Uint, 0);
        CommandListHandle->SetComputeRoot32BitConstant(RootIndex, Y.Uint, 1);
    }

    void D3D12ComputeContext::SetConstants(UINT RootIndex, DWParam X, DWParam Y, DWParam Z)
    {
        CommandListHandle->SetComputeRoot32BitConstant(RootIndex, X.Uint, 0);
        CommandListHandle->SetComputeRoot32BitConstant(RootIndex, Y.Uint, 1);
        CommandListHandle->SetComputeRoot32BitConstant(RootIndex, Z.Uint, 2);
    }

    void D3D12ComputeContext::SetConstants(UINT RootIndex, DWParam X, DWParam Y, DWParam Z, DWParam W)
    {
        CommandListHandle->SetComputeRoot32BitConstant(RootIndex, X.Uint, 0);
        CommandListHandle->SetComputeRoot32BitConstant(RootIndex, Y.Uint, 1);
        CommandListHandle->SetComputeRoot32BitConstant(RootIndex, Z.Uint, 2);
        CommandListHandle->SetComputeRoot32BitConstant(RootIndex, W.Uint, 3);
    }

    void D3D12ComputeContext::SetConstantBuffer(UINT RootIndex, D3D12_GPU_VIRTUAL_ADDRESS CBV)
    {
        CommandListHandle->SetComputeRootConstantBufferView(RootIndex, CBV);
    }

    void D3D12ComputeContext::SetDynamicConstantBufferView(UINT RootIndex, UINT64 BufferSize, const void* BufferData)
    {
        ASSERT(BufferData != nullptr && Pilot::IsAligned(BufferData, 16));
        D3D12Allocation cb = CpuConstantAllocator.Allocate(BufferSize);
        // SIMDMemCopy(cb.DataPtr, BufferData, Math::AlignUp(BufferSize, 16) >> 4);
        memcpy(cb.CpuVirtualAddress, BufferData, BufferSize);
        CommandListHandle->SetComputeRootConstantBufferView(RootIndex, cb.GpuVirtualAddress);
    }

    void D3D12ComputeContext::SetBufferSRV(UINT RootIndex, const std::shared_ptr<D3D12Buffer> BufferSRV, UINT64 Offset)
    {
        ASSERT((BufferSRV->GetResourceState().GetSubresourceState(0) &
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE) != 0);
        CommandListHandle->SetComputeRootShaderResourceView(RootIndex, BufferSRV->GetGpuVirtualAddress(0) + Offset);
    }

    void D3D12ComputeContext::SetBufferUAV(UINT RootIndex, const std::shared_ptr<D3D12Buffer> BufferUAV, UINT64 Offset)
    {
        ASSERT((BufferUAV->GetResourceState().GetSubresourceState(0) & D3D12_RESOURCE_STATE_UNORDERED_ACCESS) != 0);
        CommandListHandle->SetComputeRootUnorderedAccessView(RootIndex, BufferUAV->GetGpuVirtualAddress(0) + Offset);
    }

    void D3D12ComputeContext::SetDescriptorTable(UINT RootIndex, D3D12_GPU_DESCRIPTOR_HANDLE FirstHandle)
    {
        CommandListHandle->SetComputeRootDescriptorTable(RootIndex, FirstHandle);
    }

    void D3D12ComputeContext::Dispatch(UINT ThreadGroupCountX, UINT ThreadGroupCountY, UINT ThreadGroupCountZ)
    {
        CommandListHandle.FlushResourceBarriers();
        CommandListHandle->Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
    }

    void D3D12ComputeContext::Dispatch1D(UINT64 ThreadCountX, UINT64 GroupSizeX)
    {
        CommandListHandle.FlushResourceBarriers();
        UINT ThreadGroupCountX = RoundUpAndDivide(ThreadCountX, GroupSizeX);
        Dispatch(ThreadGroupCountX, 1, 1);
    }

    void D3D12ComputeContext::Dispatch2D(UINT64 ThreadCountX, UINT64 ThreadCountY, UINT64 GroupSizeX, UINT64 GroupSizeY)
    {
        CommandListHandle.FlushResourceBarriers();
        UINT ThreadGroupCountX = RoundUpAndDivide(ThreadCountX, GroupSizeX);
        UINT ThreadGroupCountY = RoundUpAndDivide(ThreadCountY, GroupSizeY);
        Dispatch(ThreadGroupCountX, ThreadGroupCountY, 1);
    }

    void D3D12ComputeContext::Dispatch3D(UINT64 ThreadCountX, UINT64 ThreadCountY, UINT64 ThreadCountZ, UINT64 GroupSizeX, UINT64 GroupSizeY, UINT64 GroupSizeZ)
    {
        CommandListHandle.FlushResourceBarriers();
        UINT ThreadGroupCountX = RoundUpAndDivide(ThreadCountX, GroupSizeX);
        UINT ThreadGroupCountY = RoundUpAndDivide(ThreadCountY, GroupSizeY);
        UINT ThreadGroupCountZ = RoundUpAndDivide(ThreadCountZ, GroupSizeZ);
        Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
    }

    void D3D12ComputeContext::DispatchIndirect(D3D12Resource& ArgumentBuffer, UINT64 ArgumentBufferOffset)
    {

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
        ASSERT(CommandContext.GetCommandQueue()->GetType() != D3D12_COMMAND_LIST_TYPE_COPY);
    }

    void D3D12CommandContext::InitializeTexture(D3D12LinkedDevice* Parent, D3D12Resource* Dest, std::vector<D3D12_SUBRESOURCE_DATA> Subresources)
    {
        D3D12CommandContext::InitializeTexture(Parent, Dest, 0, Subresources);
    }

    void D3D12CommandContext::InitializeTexture(D3D12LinkedDevice* Parent, D3D12Resource* Dest, UINT FirstSubresource, std::vector<D3D12_SUBRESOURCE_DATA> Subresources)
    {
        /*
        Parent->BeginResourceUpload();
        Parent->Upload(Subresources, FirstSubresource, Dest->GetResource());
        Parent->EndResourceUpload(true);
        */

        UINT NumSubresources = Subresources.size();
        UINT64 uploadBufferSize = GetRequiredIntermediateSize(Dest->GetResource(), FirstSubresource, NumSubresources);

        D3D12CommandContext& InitContext = Parent->BeginResourceUpload();

        // copy data to the intermediate upload heap and then schedule a copy from the upload heap to the default texture
        RHI::D3D12Allocation mem = InitContext.ReserveUploadMemory(uploadBufferSize);
        UpdateSubresources(InitContext.GetGraphicsCommandList(), Dest->GetResource(), mem.Resource, 
            mem.Offset, FirstSubresource, NumSubresources, Subresources.data());
        InitContext.TransitionBarrier(Dest, D3D12_RESOURCE_STATE_GENERIC_READ, true);

        // Execute the command list and wait for it to finish so we can release the upload buffer
        Parent->EndResourceUpload(true);
    }

    void D3D12CommandContext::InitializeBuffer(D3D12LinkedDevice* Parent, D3D12Resource* Dest, const void* Data, UINT64 NumBytes, UINT64 DestOffset)
    {
        D3D12CommandContext& InitContext = Parent->BeginResourceUpload();

        RHI::D3D12Allocation mem = InitContext.ReserveUploadMemory(NumBytes);
        SIMDMemCopy(mem.CpuVirtualAddress, Data, Pilot::DivideByMultiple(NumBytes, 16));

        D3D12_RESOURCE_STATES originalState = Dest->GetResourceState().GetSubresourceState(0);

        // copy data to the intermediate upload heap and then schedule a copy from the upload heap to the default texture
        InitContext.TransitionBarrier(
            Dest, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, true);
        InitContext->CopyBufferRegion(Dest->GetResource(), DestOffset, mem.Resource, 0, NumBytes);
        InitContext.TransitionBarrier(Dest, originalState, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, true);

        //// Execute the command list and wait for it to finish so we can release the upload buffer
        //Parent->EndResourceUpload(true);
    }

    void D3D12CommandContext::InitializeTextureArraySlice(D3D12LinkedDevice* Parent, D3D12Resource* Dest, UINT SliceIndex, D3D12Resource* Src)
    {
        D3D12CommandContext& InitContext = Parent->BeginResourceUpload();

        InitContext.TransitionBarrier(Dest, D3D12_RESOURCE_STATE_COPY_DEST, true);

        const D3D12_RESOURCE_DESC& DestDesc = Dest->GetResource()->GetDesc();
        const D3D12_RESOURCE_DESC& SrcDesc  = Src->GetResource()->GetDesc();

        ASSERT(SliceIndex < DestDesc.DepthOrArraySize && SrcDesc.DepthOrArraySize == 1 &&
               DestDesc.Width == SrcDesc.Width && DestDesc.Height == SrcDesc.Height &&
               DestDesc.MipLevels <= SrcDesc.MipLevels);

        UINT SubResourceIndex = SliceIndex * DestDesc.MipLevels;

        for (UINT i = 0; i < DestDesc.MipLevels; ++i)
        {
            D3D12_TEXTURE_COPY_LOCATION destCopyLocation = {
                Dest->GetResource(), D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX, SubResourceIndex + i};

            D3D12_TEXTURE_COPY_LOCATION srcCopyLocation = {
                Src->GetResource(), D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX, i};

            InitContext->CopyTextureRegion(&destCopyLocation, 0, 0, 0, &srcCopyLocation, nullptr);
        }

        InitContext.TransitionBarrier(Dest, D3D12_RESOURCE_STATE_GENERIC_READ);
        Parent->EndResourceUpload(true);
    }

















}
