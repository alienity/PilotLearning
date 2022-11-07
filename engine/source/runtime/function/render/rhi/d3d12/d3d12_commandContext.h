#pragma once
#include "d3d12_core.h"
#include "d3d12_commandList.h"
#include "d3d12_resourceAllocator.h"
#include "d3d12_profiler.h"

#include "core/color/color.h"

namespace RHI
{
    class D3D12CommandQueue;
    class D3D12PipelineState;
    class D3D12RaytracingPipelineState;
    class D3D12RootSignature;
    class D3D12RenderTargetView;
    class D3D12DepthStencilView;

    struct DWParam
    {
        DWParam(FLOAT f) : Float(f) {}
        DWParam(UINT u) : Uint(u) {}
        DWParam(INT i) : Int(i) {}

        void operator=(FLOAT f) { Float = f; }
        void operator=(UINT u) { Uint = u; }
        void operator=(INT i) { Int = i; }

        union
        {
            FLOAT Float;
            UINT  Uint;
            INT   Int;
        };
    };

    class D3D12CommandAllocatorPool : public D3D12LinkedDeviceChild
    {
    public:
        D3D12CommandAllocatorPool() noexcept = default;
        explicit D3D12CommandAllocatorPool(D3D12LinkedDevice* Parent, D3D12_COMMAND_LIST_TYPE CommandListType) noexcept;

        [[nodiscard]] Microsoft::WRL::ComPtr<ID3D12CommandAllocator> RequestCommandAllocator();

        void DiscardCommandAllocator(Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CommandAllocator,
                                     D3D12SyncHandle                                SyncHandle);

    private:
        D3D12_COMMAND_LIST_TYPE                                    CommandListType;
        CFencePool<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>> CommandAllocatorPool;
    };

    // clang-format off
	class D3D12CommandContext : public D3D12LinkedDeviceChild
    {
    public:
        D3D12CommandContext() noexcept = default;
        explicit D3D12CommandContext(D3D12LinkedDevice* Parent, RHID3D12CommandQueueType Type, D3D12_COMMAND_LIST_TYPE  CommandListType);

        D3D12CommandContext(D3D12CommandContext&&) noexcept = default;
        D3D12CommandContext& operator=(D3D12CommandContext&&) noexcept = default;

        D3D12CommandContext(const D3D12CommandContext&) = delete;
        D3D12CommandContext& operator=(const D3D12CommandContext&) = delete;

        [[nodiscard]] D3D12CommandQueue*          GetCommandQueue() const noexcept;
        [[nodiscard]] ID3D12GraphicsCommandList*  GetGraphicsCommandList() const noexcept;
        [[nodiscard]] ID3D12GraphicsCommandList4* GetGraphicsCommandList4() const noexcept;
        [[nodiscard]] ID3D12GraphicsCommandList6* GetGraphicsCommandList6() const noexcept;
        D3D12CommandListHandle&                   operator->() { return CommandListHandle; }

        void Open();
        void Close();

        // Returns D3D12SyncHandle, may be ignored if WaitForCompletion is true
        D3D12SyncHandle Execute(bool WaitForCompletion);

        void CopyBuffer(D3D12Resource& Dest, D3D12Resource& Src);
        void CopyBufferRegion(D3D12Resource& Dest, size_t DestOffset, D3D12Resource& Src, size_t SrcOffset, size_t NumBytes );
        void CopySubresource(D3D12Resource& Dest, UINT DestSubIndex, D3D12Resource& Src, UINT SrcSubIndex);
        void CopyCounter(D3D12Resource& Dest, size_t DestOffset, D3D12Resource& Src);
        void CopyTextureRegion(D3D12Resource& Dest, UINT x, UINT y, UINT z, D3D12Resource& Source, RECT& rect);
        void ResetCounter(D3D12Resource& Buf, uint32_t Value = 0);

        //// Creates a readback buffer of sufficient size, copies the texture into it,
        //// and returns row pitch in bytes.
        //uint32_t ReadbackTexture(ReadbackBuffer& DstBuffer, PixelBuffer& SrcBuffer);

        D3D12Allocation ReserveUploadMemory(size_t SizeInBytes);

        static void InitializeTexture(D3D12Resource& Dest, UINT NumSubresources, D3D12_SUBRESOURCE_DATA SubData[]);
        static void InitializeBuffer(D3D12Resource& Dest, const void* Data, size_t NumBytes, size_t DestOffset = 0);
        static void InitializeBuffer(D3D12Resource& Dest, const UploadBuffer& Src, size_t SrcOffset, size_t NumBytes = -1, size_t DestOffset = 0 );
        static void InitializeTextureArraySlice(GpuResource& Dest, UINT SliceIndex, GpuResource& Src);

        void TransitionBarrier(D3D12Resource* Resource, D3D12_RESOURCE_STATES State, UINT Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
        void AliasingBarrier(D3D12Resource* BeforeResource, D3D12Resource* AfterResource);
        void UAVBarrier(D3D12Resource* Resource);
        void FlushResourceBarriers();

        bool AssertResourceState(D3D12Resource* Resource, D3D12_RESOURCE_STATES State, UINT Subresource);

        void SetPipelineState(D3D12PipelineState* PipelineState);
        void SetPipelineState(D3D12RaytracingPipelineState* RaytracingPipelineState);

        void SetGraphicsRootSignature(D3D12RootSignature* RootSignature);
        void SetComputeRootSignature(D3D12RootSignature* RootSignature);
        template<RHI_PIPELINE_STATE_TYPE PsoType>
        void SetRootSignature(D3D12RootSignature* RootSignature)
        {
            if (PsoType == RHI_PIPELINE_STATE_TYPE::Graphics)
                SetGraphicsRootSignature(RootSignature);
            else
                SetComputeRootSignature(RootSignature);
        }

        void ClearRenderTarget(D3D12RenderTargetView* RenderTargetView, D3D12DepthStencilView* DepthStencilView);
        void ClearRenderTarget(std::vector<D3D12RenderTargetView*> RenderTargetViews, D3D12DepthStencilView* DepthStencilView);

        void SetRenderTarget(D3D12RenderTargetView* RenderTargetView, D3D12DepthStencilView* DepthStencilView);
        void SetRenderTarget(std::vector<D3D12RenderTargetView*> RenderTargetViews, D3D12DepthStencilView* DepthStencilView);

        void SetViewport(const RHIViewport& Viewport);
        void SetViewport(FLOAT TopLeftX, FLOAT TopLeftY, FLOAT Width, FLOAT Height, FLOAT MinDepth = 0.0f, FLOAT MaxDepth = 1.0f);
        void SetViewports(std::vector<RHIViewport> Viewports);

        void SetScissorRect(const RHIRect& ScissorRect);
        void SetScissorRect(UINT Left, UINT Top, UINT Right, UINT Bottom);
        void SetScissorRects(std::vector<RHIRect> ScissorRects);

        void SetViewportAndScissorRect(const RHIViewport& vp, const RHIRect& rect);
        void SetViewportAndScissorRect(UINT x, UINT y, UINT w, UINT h);

        void SetStencilRef(UINT StencilRef);
        void SetBlendFactor(Pilot::Color BlendFactor);
        void SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY Topology);

        void SetGraphicsConstantArray(UINT RootIndex, UINT NumConstants, const void* pConstants);
        void SetGraphicsConstant(UINT RootIndex, UINT Offset, DWParam Val);
        void SetGraphicsConstants(UINT RootIndex, DWParam X);
        void SetGraphicsConstants(UINT RootIndex, DWParam X, DWParam Y);
        void SetGraphicsConstants(UINT RootIndex, DWParam X, DWParam Y, DWParam Z);
        void SetGraphicsConstants(UINT RootIndex, DWParam X, DWParam Y, DWParam Z, DWParam W);

        void SetComputeConstantArray(UINT RootIndex, UINT NumConstants, const void* pConstants);
        void SetComputeConstant(UINT RootIndex, UINT Offset, DWParam Val);
        void SetComputeConstants(UINT RootIndex, DWParam X);
        void SetComputeConstants(UINT RootIndex, DWParam X, DWParam Y);
        void SetComputeConstants(UINT RootIndex, DWParam X, DWParam Y, DWParam Z);
        void SetComputeConstants(UINT RootIndex, DWParam X, DWParam Y, DWParam Z, DWParam W);

        template<RHI_PIPELINE_STATE_TYPE PsoType>
        void SetConstantArray(UINT RootIndex, UINT NumConstants, const void* pConstants)
        {
            if (PsoType == RHI_PIPELINE_STATE_TYPE::Graphics)
                SetGraphicsConstantArray(RootIndex, NumConstants, pConstants);
            else
                SetComputeConstantArray(RootIndex, NumConstants, pConstants);
        }
        template<RHI_PIPELINE_STATE_TYPE PsoType>
        void SetConstant(UINT RootIndex, UINT Offset, DWParam Val)
        {
            if (PsoType == RHI_PIPELINE_STATE_TYPE::Graphics)
                SetGraphicsConstant(RootIndex, Offset, Val);
            else
                SetComputeConstant(RootIndex, Offset, Val);
        }
        template<RHI_PIPELINE_STATE_TYPE PsoType>
        void SetConstants(UINT RootIndex, DWParam X)
        {
            if (PsoType == RHI_PIPELINE_STATE_TYPE::Graphics)
                SetGraphicsConstants(RootIndex, X);
            else
                SetComputeConstants(RootIndex, X);
        }
        template<RHI_PIPELINE_STATE_TYPE PsoType>
        void SetConstants(UINT RootIndex, DWParam X, DWParam Y)
        {
            if (PsoType == RHI_PIPELINE_STATE_TYPE::Graphics)
                SetGraphicsConstants(RootIndex, X, Y);
            else
                SetComputeConstants(RootIndex, X, Y);
        }
        template<RHI_PIPELINE_STATE_TYPE PsoType>
        void SetConstants(UINT RootIndex, DWParam X, DWParam Y, DWParam Z)
        {
            if (PsoType == RHI_PIPELINE_STATE_TYPE::Graphics)
                SetGraphicsConstants(RootIndex, X, Y, Z);
            else
                SetComputeConstants(RootIndex, X, Y, Z);
        }
        template<RHI_PIPELINE_STATE_TYPE PsoType>
        void SetConstants(UINT RootIndex, DWParam X, DWParam Y, DWParam Z, DWParam W)
        {
            if (PsoType == RHI_PIPELINE_STATE_TYPE::Graphics)
                SetGraphicsConstants(RootIndex, X, Y, Z, W);
            else
                SetComputeConstants(RootIndex, X, Y, Z, W);
        }

        void SetGraphicsConstantBuffer(UINT RootIndex, D3D12_GPU_VIRTUAL_ADDRESS CBV);
        void SetComputeConstantBuffer(UINT RootIndex, D3D12_GPU_VIRTUAL_ADDRESS CBV);
        template<RHI_PIPELINE_STATE_TYPE PsoType>
        void SetConstantBuffer(UINT RootIndex, D3D12_GPU_VIRTUAL_ADDRESS CBV)
        {
            if (PsoType == RHI_PIPELINE_STATE_TYPE::Graphics)
                SetGraphicsConstantBuffer(RootIndex, CBV);
            else
                SetComputeConstantBuffer(RootIndex, CBV);
        }

        void SetGraphicsConstantBuffer(UINT RootParameterIndex, UINT64 Size, const void* Data);
        void SetComputeConstantBuffer(UINT RootParameterIndex, UINT64 Size, const void* Data);
        template<typename T>
        void SetGraphicsConstantBuffer(UINT RootParameterIndex, const T& Data)
        {
            SetGraphicsConstantBuffer(RootParameterIndex, sizeof(T), &Data);
        }
        template<typename T>
        void SetComputeConstantBuffer(UINT RootParameterIndex, const T& Data)
        {
            SetComputeConstantBuffer(RootParameterIndex, sizeof(T), &Data);
        }
        template<RHI_PIPELINE_STATE_TYPE PsoType, typename T>
        void SetConstantBuffer(UINT RootParameterIndex, const T& Data)
        {
            if (PsoType == RHI_PIPELINE_STATE_TYPE::Graphics)
                SetGraphicsConstantBuffer<T>(RootParameterIndex, Data);
            else
                SetComputeConstantBuffer<T>(RootParameterIndex, Data);
        }

        void SetGraphicsBufferSRV(UINT RootIndex, const std::shared_ptr<D3D12Buffer> BufferSRV, UINT64 Offset = 0);
        void SetGraphicsBufferUAV(UINT RootIndex, const std::shared_ptr<D3D12Buffer> BufferUAV, UINT64 Offset = 0);
        void SetGraphicsDescriptorTable(UINT RootIndex, D3D12_GPU_DESCRIPTOR_HANDLE FirstHandle);

        void SetComputeBufferSRV(UINT RootIndex, const std::shared_ptr<D3D12Buffer> BufferSRV, UINT64 Offset = 0);
        void SetComputeBufferUAV(UINT RootIndex, const std::shared_ptr<D3D12Buffer> BufferUAV, UINT64 Offset = 0);
        void SetComputeDescriptorTable(UINT RootIndex, D3D12_GPU_DESCRIPTOR_HANDLE FirstHandle);



        // These version of the API calls should be used as it needs to flush resource barriers before any work
        void DrawInstanced(UINT VertexCount, UINT InstanceCount, UINT StartVertexLocation, UINT StartInstanceLocation);
        void DrawIndexedInstanced(UINT IndexCount, UINT InstanceCount, UINT StartIndexLocation, INT  BaseVertexLocation, UINT StartInstanceLocation);

        void Dispatch(UINT ThreadGroupCountX, UINT ThreadGroupCountY, UINT ThreadGroupCountZ);

        template<UINT ThreadSizeX>
        void Dispatch1D(UINT ThreadGroupCountX)
        {
            ThreadGroupCountX = RoundUpAndDivide(ThreadGroupCountX, ThreadSizeX);
            Dispatch(ThreadGroupCountX, 1, 1);
        }

        template<UINT ThreadSizeX, UINT ThreadSizeY>
        void Dispatch2D(UINT ThreadGroupCountX, UINT ThreadGroupCountY)
        {
            ThreadGroupCountX = RoundUpAndDivide(ThreadGroupCountX, ThreadSizeX);
            ThreadGroupCountY = RoundUpAndDivide(ThreadGroupCountY, ThreadSizeY);
            Dispatch(ThreadGroupCountX, ThreadGroupCountY, 1);
        }

        template<UINT ThreadSizeX, UINT ThreadSizeY, UINT ThreadSizeZ>
        void Dispatch3D(UINT ThreadGroupCountX, UINT ThreadGroupCountY, UINT ThreadGroupCountZ)
        {
            ThreadGroupCountX = RoundUpAndDivide(ThreadGroupCountX, ThreadSizeX);
            ThreadGroupCountY = RoundUpAndDivide(ThreadGroupCountY, ThreadSizeY);
            ThreadGroupCountZ = RoundUpAndDivide(ThreadGroupCountZ, ThreadSizeZ);
            Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
        }

        void DispatchRays(const D3D12_DISPATCH_RAYS_DESC* pDesc);

        void DispatchMesh(UINT ThreadGroupCountX, UINT ThreadGroupCountY, UINT ThreadGroupCountZ);

        void ResetCounter(D3D12Resource* CounterResource, UINT64 CounterOffset, UINT Value = 0);

    private:
        template<typename T>
        constexpr T RoundUpAndDivide(T Value, size_t Alignment)
        {
            return (T)((Value + Alignment - 1) / Alignment);
        }

        template<RHI_PIPELINE_STATE_TYPE PsoType>
        void SetDynamicResourceDescriptorTables(D3D12RootSignature* RootSignature);

    private:
        RHID3D12CommandQueueType                       Type;
        D3D12CommandListHandle                         CommandListHandle;
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CommandAllocator;
        D3D12CommandAllocatorPool                      CommandAllocatorPool;
        D3D12LinearAllocator                           CpuConstantAllocator;

        // TODO: Finish cache
        // State Cache
        UINT64 DirtyStates = 0xffffffff;
        struct StateCache
        {
            D3D12RootSignature* RootSignature;
            D3D12PipelineState* PipelineState;

            struct
            {
                // Viewport
                UINT           NumViewports                                                        = 0;
                D3D12_VIEWPORT Viewports[D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};

                // Scissor Rect
                UINT       NumScissorRects                                                        = 0;
                D3D12_RECT ScissorRects[D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
            } Graphics;

            struct
            {
            } Compute;

            struct
            {
                D3D12RaytracingPipelineState* RaytracingPipelineState;
            } Raytracing;
        } Cache = {};
    };
    // clang-format on

    class D3D12ScopedEventObject
    {
    public:
        D3D12ScopedEventObject(D3D12CommandContext& CommandContext, std::string_view Name);

    private:
        D3D12ProfileBlock ProfileBlock;
#ifdef _DEBUG
        PIXScopedEventObject<ID3D12GraphicsCommandList> PixEvent;
#endif
    };
}