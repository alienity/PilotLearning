#include "RHICore.hpp"

namespace RHI
{
    class RHISyncHandle;
    class RHIResource;
    class RHIBuffer;
    class RHITexture;
    class RHIPipelineState;

    enum CommandType
    {
        AsyncCompute,
        Graphics,
        Copy
    };

    class RHICommandBuffer
    {
    public:
        void Begin(CommandType commandType);
        void End();

        void CopyBuffer(RHIBuffer* dest, RHIBuffer* src);
        void CopyBufferRegion(RHIBuffer* dest, UINT destOffset, RHIBuffer* src, UINT srcOffset, UINT numBytes);
        void CopySubresource(RHITexture* dest, UINT destSubIndex, RHITexture* src, UINT srcSubIndex);
        void CopyTextureRegion(RHITexture* dest, UINT x, UINT y, UINT z, RHITexture* src, RHIRect rect);
        void ResetCounter(RHIBuffer* counterResource, UINT counterOffset = 0, UINT value = 0);

        // RHIBuffer* ReserveUploadMemory(UINT sizeInBytes, UINT alignment = RHI_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT)

        void InitializeTexture(RHITexture* dest, std::vector<RHI_SUBRESOURCE_DATA> subresources);
        void InitializeTexture(RHITexture* dest, UINT firstSubresource, std::vector<RHI_SUBRESOURCE_DATA> subresources);
        void InitializeBuffer(RHIBuffer* dest, const void* data, UINT numBytes, UINT destOffset = 0);
        void InitializeTextureArraySlice(RHITexture* dest, UINT sliceIndex, RHITexture* src);

        void WriteBuffer(RHIBuffer* dest, UINT destOffset, const void* data, UINT numBytes);
        void FillBuffer(RHIBuffer* dest, UINT destOffset, DWParam value, UINT numBytes);

        void TransitionBarrier(RHIResource* resource, RHI_RESOURCE_STATES state, UINT subresource = RHI_RESOURCE_BARRIER_ALL_SUBRESOURCES, bool flushImmediate = false);
        void AliasingBarrier(RHIResource* beforeResource, RHIResource* afterResource, bool flushImmediate = false);
        void InsertUAVBarrier(RHIResource* resource, bool flushImmediate = false);
        void FlushResourceBarriers();

        bool AssertResourceState(RHIResource* resource, RHI_RESOURCE_STATES state, UINT subresource);

        void SetPipelineState(RHIPipelineState* pipelineState);

        void SetPredication(RHIResource* buffer, UINT BufferOffset, RHI_PREDICATION_OP Op);

        void DispatchMesh(UINT ThreadGroupCountX, UINT ThreadGroupCountY, UINT ThreadGroupCountZ);

    protected:


    };

    class RHIGraphicsCommandBuffer : public RHICommandBuffer
    {
    public:
        void ClearUAV(RHI::RHIBuffer* Target);
        void ClearUAV(RHI::RHITexture* Target);
        void ClearColor(RHI::RHITexture* Target, RHI_RECT* Rect = nullptr);
        void ClearColor(RHI::RHITexture* Target, float Colour[4], RHI_RECT* Rect = nullptr);
        void ClearDepth(RHI::RHITexture* Target);
        void ClearStencil(RHI::RHITexture* Target);
        void ClearDepthAndStencil(RHI::RHITexture* Target);

        void ClearRenderTarget(RHIRenderTargetView* RenderTargetView, RHIDepthStencilView* DepthStencilView);
        void ClearRenderTarget(std::vector<RHIRenderTargetView*> RenderTargetViews, RHIDepthStencilView* DepthStencilView);
        
        void BeginQuery(IRHIQueryHeap* QueryHeap, RHI_QUERY_TYPE Type, UINT HeapIndex);
        void EndQuery(IRHIQueryHeap* QueryHeap, RHI_QUERY_TYPE Type, UINT HeapIndex);
        void ResolveQueryData(IRHIQueryHeap* QueryHeap, RHI_QUERY_TYPE Type, UINT StartIndex, UINT NumQueries, IRHIResource* DestinationBuffer, UINT64 DestinationBufferOffset);

        void SetRootSignature(RHIRootSignature* RootSignature);

        void SetRenderTargets(std::vector<RHIRenderTargetView*> RenderTargetViews);
        void SetRenderTargets(std::vector<RHIRenderTargetView*> RenderTargetViews, RHIDepthStencilView* DepthStencilView);
        void SetRenderTarget(RHIRenderTargetView* RenderTargetView);
        void SetRenderTarget(RHIRenderTargetView* RenderTargetView, RHIDepthStencilView* DepthStencilView);
        void SetDepthStencilTarget(RHIDepthStencilView* DepthStencilView) { SetRenderTarget(nullptr, DepthStencilView); }

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
        void SetPrimitiveTopology(RHI_PRIMITIVE_TOPOLOGY Topology);

        void SetConstantArray(UINT RootIndex, UINT NumConstants, const void* pConstants);
        void SetConstant(UINT RootIndex, UINT Offset, DWParam Val);
        void SetConstants(UINT RootIndex, DWParam X);
        void SetConstants(UINT RootIndex, DWParam X, DWParam Y);
        void SetConstants(UINT RootIndex, DWParam X, DWParam Y, DWParam Z);
        void SetConstants(UINT RootIndex, DWParam X, DWParam Y, DWParam Z, DWParam W);
        void SetConstantBuffer(UINT RootIndex, RHI_GPU_VIRTUAL_ADDRESS CBV);
        void SetDynamicConstantBufferView(UINT RootParameterIndex, UINT64 BufferSize, const void* BufferData);
        template<typename T>
        void SetDynamicConstantBufferView(UINT RootParameterIndex, const T& Data) { SetConstantBuffer(RootParameterIndex, sizeof(T), &Data); }
        void SetBufferSRV(UINT RootIndex, RHIBuffer* BufferSRV, UINT64 Offset = 0);
        void SetBufferUAV(UINT RootIndex, RHIBuffer* BufferUAV, UINT64 Offset = 0);
        void SetDescriptorTable(UINT RootIndex, RHI_GPU_DESCRIPTOR_HANDLE FirstHandle);

        void SetDynamicDescriptor(UINT RootIndex, UINT Offset, RHI_CPU_DESCRIPTOR_HANDLE Handle);
        void SetDynamicDescriptors(UINT RootIndex, UINT Offset, UINT Count, const RHI_CPU_DESCRIPTOR_HANDLE Handles[]);
        void SetDynamicSampler(UINT RootIndex, UINT Offset, RHI_CPU_DESCRIPTOR_HANDLE Handle);
        void SetDynamicSamplers(UINT RootIndex, UINT Offset, UINT Count, const RHI_CPU_DESCRIPTOR_HANDLE Handles[]);

        void SetIndexBuffer(const RHI_INDEX_BUFFER_VIEW& IBView);
        void SetVertexBuffer(UINT Slot, const RHI_VERTEX_BUFFER_VIEW& VBView);
        void SetVertexBuffers(UINT StartSlot, UINT Count, const RHI_VERTEX_BUFFER_VIEW VBViews[]);
        void SetDynamicVB(UINT Slot, UINT64 NumVertices, UINT64 VertexStride, const void* VBData);
        void SetDynamicIB(UINT64 IndexCount, const UINT16* IBData);
        void SetDynamicSRV(UINT RootIndex, UINT64 BufferSize, const void* BufferData);

        void Draw(UINT VertexCount, UINT VertexStartOffset = 0);
        void DrawIndexed(UINT IndexCount, UINT StartIndexLocation = 0, INT BaseVertexLocation = 0);
        void DrawInstanced(UINT VertexCount, UINT InstanceCount, UINT StartVertexLocation, UINT StartInstanceLocation);
        void DrawIndexedInstanced(UINT IndexCount, UINT InstanceCount, UINT StartIndexLocation, INT  BaseVertexLocation, UINT StartInstanceLocation);
        void DrawIndirect(RHIResource* ArgumentBuffer, UINT64 ArgumentBufferOffset = 0);
        void ExecuteIndirect(RHICommandSignature* CommandSig, RHIResource* ArgumentBuffer, UINT64 ArgumentStartOffset = 0,
            UINT32 MaxCommands = 1, RHIResource* CommandCounterBuffer = nullptr, UINT64 CounterOffset = 0);
    };

    class RHIComputeCommandBuffer : public RHICommandBuffer
    {
    public:
        void ClearUAV(RHI::RHIBuffer* Target);
        void ClearUAV(RHI::RHITexture* Target);

        void SetRootSignature(RHIRootSignature* RootSignature);

        void SetConstantArray(UINT RootIndex, UINT NumConstants, const void* pConstants);
        void SetConstant(UINT RootIndex, UINT Offset, DWParam Val);
        void SetConstants(UINT RootIndex, DWParam X);
        void SetConstants(UINT RootIndex, DWParam X, DWParam Y);
        void SetConstants(UINT RootIndex, DWParam X, DWParam Y, DWParam Z);
        void SetConstants(UINT RootIndex, DWParam X, DWParam Y, DWParam Z, DWParam W);
        void SetConstantBuffer(UINT RootIndex, RHI_GPU_VIRTUAL_ADDRESS CBV);
        void SetDynamicConstantBufferView(UINT RootParameterIndex, UINT64 BufferSize, const void* BufferData);
        template<typename T>
        void SetDynamicConstantBufferView(UINT RootParameterIndex, const T& Data)
        {
            SetDynamicConstantBufferView(RootParameterIndex, sizeof(T), &Data);
        }
        void SetDynamicSRV(UINT RootIndex, UINT64 BufferSize, const void* BufferData);
        void SetBufferSRV(UINT RootIndex, RHIBuffer* BufferSRV, UINT64 Offset = 0);
        void SetBufferUAV(UINT RootIndex, RHIBuffer* BufferUAV, UINT64 Offset = 0);
        void SetDescriptorTable(UINT RootIndex, RHI_GPU_DESCRIPTOR_HANDLE FirstHandle);

        void SetDynamicDescriptor(UINT RootIndex, UINT Offset, RHI_CPU_DESCRIPTOR_HANDLE Handle);
        void SetDynamicDescriptors(UINT RootIndex, UINT Offset, UINT Count, const RHI_CPU_DESCRIPTOR_HANDLE Handles[]);
        void SetDynamicSampler(UINT RootIndex, UINT Offset, RHI_CPU_DESCRIPTOR_HANDLE Handle);
        void SetDynamicSamplers(UINT RootIndex, UINT Offset, UINT Count, const RHI_CPU_DESCRIPTOR_HANDLE Handles[]);
        
        void Dispatch(UINT ThreadGroupCountX, UINT ThreadGroupCountY, UINT ThreadGroupCountZ);
        void Dispatch1D(UINT64 ThreadCountX, UINT64 GroupSizeX = 64);
        void Dispatch2D(UINT64 ThreadCountX, UINT64 ThreadCountY, UINT64 GroupSizeX = 8, UINT64 GroupSizeY = 8);
        void Dispatch3D(UINT64 ThreadCountX, UINT64 ThreadCountY, UINT64 ThreadCountZ, UINT64 GroupSizeX, UINT64 GroupSizeY, UINT64 GroupSizeZ);
        void DispatchIndirect(RHIResource* ArgumentBuffer, UINT64 ArgumentBufferOffset = 0);
        void ExecuteIndirect(RHICommandSignature* CommandSig, RHIResource* ArgumentBuffer, UINT64 ArgumentStartOffset = 0,
            UINT32 MaxCommands = 1, RHIResource* CommandCounterBuffer = nullptr, UINT64 CounterOffset = 0);
    };


}



