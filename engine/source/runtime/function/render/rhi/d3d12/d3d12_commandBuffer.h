#pragma once
#include "d3d12_core.h"
#include "d3d12_resource.h"

namespace RHI
{
    class D3D12PipelineState;
    class D3D12CommandSignature;

    enum class CommandOperation
    {
        BufferOpen,
        BufferClose,
        BeginSample,
        EndSample,
        CopyBuffer,
        CopyBufferRegion,
        CopySubresource,
        CopyTextureRegion,
        ResetCounter,
        InitializeTexture,
        InitializeBuffer,
        InitializeTextureArraySlice,
        WriteBuffer,
        FillBuffer,
        GenerateMips,
        TransitionBarrier,
        InsertUAVBarrier,
        FlushResourceBarriers,
        ClearUAV,
        ClearColor,
        ClearDepth,
        ClearStencil,
        ClearDepthAndStencil,
        ClearRenderTarget,
        SetRootSignature,
        SetPipelineState,
        SetRenderTarget,
        SetViewport,
        SetScissorRect,
        SetViewportAndScissorRect,
        SetStencilRef,
        SetBlendFactor,
        SetPrimitiveTopology,
        SetConstantArray,
        SetConstant,
        SetConstants,
        SetConstantBuffer,
        SetDynamicConstantBufferView,
        SetBufferSRV,
        SetBufferUAV,
        SetDescriptorTable,
        SetDynamicDescriptor,
        SetDynamicSampler,
        SetIndexBuffer,
        SetVertexBuffer,
        SetDynamicIB,
        SetDynamicVB,
        SetDynamicSRV,
        Draw,
        DrawIndexed,
        DrawInstanced,
        DrawIndexedInstanced,
        DrawIndirect,
        Dispatch,
        Dispatch1D,
        Dispatch2D,
        Dispatch3D,
        DispatchIndirect,
        ExecuteIndirect,
        CreateAsyncGraphicsFence,
        CreateGraphicsFence,
        WaitOnAsyncGraphicsFence
    };

    struct GraphicsFence
    {
        UINT commandBufferId;
        UINT localFenceId;
    };

    enum RHICommandType
    {
        Graphics,
        Compute,
    };

    class CommandBuffer : public D3D12LinkedDeviceChild
    {
    public:
        void Open(RHICommandType type);
        void Close();

        void BeginSample(std::string name);
        void EndSample(std::string name);

        void Clear();

        void CopyBuffer(D3D12Buffer* dest, D3D12Buffer* src);
        void CopyBufferRegion(D3D12Buffer* dest, UINT destOffset, D3D12Buffer* src, UINT srcOffset, UINT numBytes);
        void CopySubresource(D3D12Texture* dest, UINT destSubIndex, D3D12Texture* src, UINT srcSubIndex);
        void CopyTextureRegion(D3D12Texture* dest, UINT x, UINT y, UINT z, D3D12Texture* src, RHIRect rect);
        void ResetCounter(D3D12Buffer* counterResource, UINT counterOffset = 0, UINT value = 0);

        void InitializeTexture(D3D12Texture* dest, std::vector<D3D12_SUBRESOURCE_DATA> subresources);
        void InitializeTexture(D3D12Texture* dest, UINT firstSubresource, std::vector<D3D12_SUBRESOURCE_DATA> subresources);
        void InitializeBuffer(D3D12Buffer* dest, const void* data, UINT numBytes, UINT destOffset = 0);
        void InitializeTextureArraySlice(D3D12Texture* dest, UINT sliceIndex, D3D12Texture* src);

        void WriteBuffer(D3D12Buffer* dest, UINT destOffset, const void* data, UINT numBytes);
        void FillBuffer(D3D12Buffer* dest, UINT destOffset, DWParam value, UINT numBytes);

        void GenerateMips(D3D12Texture* rt);

        void TransitionBarrier(D3D12Resource* Resource, D3D12_RESOURCE_STATES State, UINT Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, bool FlushImmediate = false);
        void AliasingBarrier(D3D12Resource* BeforeResource, D3D12Resource* AfterResource, bool FlushImmediate = false);
        void InsertUAVBarrier(D3D12Resource* Resource, bool FlushImmediate = false);
        void FlushResourceBarriers();

        bool AssertResourceState(D3D12Resource* Resource, D3D12_RESOURCE_STATES State, UINT Subresource);

        void ClearUAV(D3D12Buffer* target);
        void ClearUAV(D3D12Texture* target);

        void ClearColor(D3D12Texture* target, RHIRect* rect = nullptr);
        void ClearColor(D3D12Texture* target, FLOAT colour[4], RHIRect* rect = nullptr);
        void ClearDepth(D3D12Texture* target);
        void ClearStencil(D3D12Texture* target);
        void ClearDepthAndStencil(D3D12Texture* target);

        void ClearRenderTarget(D3D12RenderTargetView* RenderTargetView, D3D12DepthStencilView* DepthStencilView);
        void ClearRenderTarget(std::vector<D3D12RenderTargetView*> RenderTargetViews, D3D12DepthStencilView* DepthStencilView);

        void SetRootSignature(D3D12RootSignature* rootSignature);
        void SetPipelineState(D3D12PipelineState* PipelineState);

        void SetPredication(D3D12Buffer* Buffer, UINT64 BufferOffset, D3D12_PREDICATION_OP Op);

        void SetRenderTarget(D3D12RenderTargetView* renderTargetView);
        void SetRenderTarget(D3D12RenderTargetView* renderTargetView, D3D12DepthStencilView* depthStencilView);
        void SetRenderTarget(std::vector<D3D12RenderTargetView*> renderTargetViews);
        void SetRenderTarget(std::vector<D3D12RenderTargetView*> renderTargetViews, D3D12DepthStencilView* depthStencilView);

        void SetViewport(const RHIViewport& viewport);
        void SetViewport(FLOAT topLeftX, FLOAT topLeftY, FLOAT width, FLOAT height, FLOAT minDepth = 0.0f, FLOAT maxDepth = 1.0f);
        void SetViewport(std::vector<RHIViewport> viewports);
        void SetScissorRect(const RHIRect& scissorRect);
        void SetScissorRect(UINT left, UINT top, UINT right, UINT bottom);
        void SetScissorRect(std::vector<RHIRect> scissorRects);
        void SetViewportAndScissorRect(const RHIViewport& vp, const RHIRect& rect);
        void SetViewportAndScissorRect(UINT x, UINT y, UINT w, UINT h);
        void SetStencilRef(UINT stencilRef);
        void SetBlendFactor(FLOAT blendFactor[3]);
        void SetPrimitiveTopology(RHI_PRIMITIVE_TOPOLOGY topology);

        void SetConstantArray(UINT rootIndex, UINT numConstants, const void* pConstants);
        void SetConstant(UINT rootIndex, UINT offset, DWParam val);
        void SetConstants(UINT rootIndex, DWParam x);
        void SetConstants(UINT rootIndex, DWParam x, DWParam y);
        void SetConstants(UINT rootIndex, DWParam x, DWParam y, DWParam z);
        void SetConstants(UINT rootIndex, DWParam x, DWParam y, DWParam z, DWParam w);
        void SetConstantBuffer(UINT rootIndex, D3D12_GPU_VIRTUAL_ADDRESS cbv);
        void SetDynamicConstantBufferView(UINT rootParameterIndex, UINT64 bufferSize, const void* bufferData);
        template<typename T>
        void SetDynamicConstantBufferView(UINT rootParameterIndex, const T& data)
        { SetDynamicConstantBufferView(rootParameterIndex, sizeof(T), &data); }
        void SetBufferSRV(UINT rootIndex, D3D12Buffer* bufferSRV, UINT64 offset = 0);
        void SetBufferUAV(UINT rootIndex, D3D12Buffer* bufferUAV, UINT64 offset = 0);
        void SetDescriptorTable(UINT rootIndex, D3D12_GPU_DESCRIPTOR_HANDLE FirstHandle);

        void SetDynamicDescriptor(UINT rootIndex, UINT offset, D3D12_CPU_DESCRIPTOR_HANDLE handle);
        void SetDynamicDescriptor(UINT rootIndex, UINT offset, UINT count, const D3D12_CPU_DESCRIPTOR_HANDLE handles[]);
        void SetDynamicSampler(UINT rootIndex, UINT offset, D3D12_CPU_DESCRIPTOR_HANDLE handle);
        void SetDynamicSampler(UINT rootIndex, UINT offset, UINT count, const D3D12_CPU_DESCRIPTOR_HANDLE handles[]);

        void SetIndexBuffer(const D3D12_INDEX_BUFFER_VIEW& ibView);
        void SetVertexBuffer(UINT slot, const D3D12_VERTEX_BUFFER_VIEW& vbView);
        void SetVertexBuffer(UINT StartSlot, UINT count, const D3D12_VERTEX_BUFFER_VIEW vbViews[]);
        void SetDynamicIB(UINT64 indexCount, const UINT* ibData);
        void SetDynamicVB(UINT slot, UINT64 numVertices, UINT64 VertexStride, const void* vbData);
        void SetDynamicSRV(UINT rootIndex, UINT64 bufferSize, const void* bufferData);

        void Draw(UINT vertexCount, UINT vertexStartOffset = 0);
        void DrawIndexed(UINT indexCount, UINT startIndexLocation = 0, INT baseVertexLocation = 0);
        void DrawInstanced(UINT vertexCount, UINT instanceCount, UINT startVertexLocation, UINT startInstanceLocation);
        void DrawIndexedInstanced(UINT indexCount, UINT instanceCount, UINT startIndexLocation, INT  baseVertexLocation, UINT startInstanceLocation);
        void DrawIndirect(D3D12Buffer* argumentBuffer, UINT64 argumentBufferOffset = 0);

        void Dispatch(UINT threadGroupCountX, UINT threadGroupCountY, UINT threadGroupCountZ);
        void Dispatch1D(UINT64 threadCountX, UINT64 groupSizeX = 64);
        void Dispatch2D(UINT64 threadCountX, UINT64 threadCountY, UINT64 groupSizeX = 8, UINT64 groupSizeY = 8);
        void Dispatch3D(UINT64 threadCountX, UINT64 threadCountY, UINT64 ThreadCountZ, UINT64 groupSizeX, UINT64 groupSizeY, UINT64 groupSizeZ);
        void DispatchIndirect(D3D12Buffer* argumentBuffer, UINT64 argumentBufferOffset = 0);

        void ExecuteIndirect(D3D12CommandSignature* commandSig, D3D12Buffer* argumentBuffer, UINT64 argumentStartOffset = 0, 
            UINT maxCommands = 1, D3D12Buffer* commandCounterBuffer = nullptr, UINT64 counterOffset = 0);

        GraphicsFence CreateAsyncGraphicsFence();
        // The GPU passes through the GraphicsFence fence after it completes the Blit, Clear, Draw, Dispatch or texture copy command 
        // you sent before this call. This includes commands from command buffers that the GPU executes immediately before you create the fence.
        GraphicsFence CreateGraphicsFence();

        // Instructs the GPU to pause processing of the queue until it passes through the GraphicsFence fence.
        void WaitOnAsyncGraphicsFence(GraphicsFence fence); 

    private:

        struct ParammeterOffset
        {
            UINT BeginOffset;
            UINT ParamSize;
        };

        std::vector<CommandOperation> commandOps;
        std::vector<ParammeterOffset> commandOffsets;
        BYTE* commandParams;
        UINT commandParamsLength;

    };

} // namespace RHI
