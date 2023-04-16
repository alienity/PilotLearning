#include "RHICore.hpp"

namespace RHI
{
    class RHICommandBuffer;
    class RHIRootSignature;
    class RHIPipelineState;
    class RHICommandSignature;
    class RHISyncHandle;
    class RHIResource;
    class RHIBuffer;
    class RHITexture;
    class RHIRenderTargetView;
    class RHIDepthStencilView;

    enum RHICommandType
    {
        Graphics,
        Compute,
    };

    enum CubemapFace
    {
        Unknown = 0,
        PositiveX,
        NegativeX,
        PositiveY,
        NegativeY,
        PositiveZ,
        NegativeZ
    };

    enum RTClearFlags
    {
        None,
        Color,
        Depth,
        Stencil,
        All,
        DepthStencil,
        ColorDepth,
        ColorStencil
    };

    class RHICommandBuffer
    {
    public:
        void Open(RHICommandType type);
        void Close();

        void BeginSample(std::string name);
        void EndSample(std::string name);

        void Clear();

        void CopyBuffer(RHIBuffer* dest, RHIBuffer* src);
        void CopyBufferRegion(RHIBuffer* dest, UINT destOffset, RHIBuffer* src, UINT srcOffset, UINT numBytes);
        void CopySubresource(RHITexture* dest, UINT destSubIndex, RHITexture* src, UINT srcSubIndex);
        void CopyTextureRegion(RHITexture* dest, UINT x, UINT y, UINT z, RHITexture* src, RHIRect rect);
        void ResetCounter(RHIBuffer* counterResource, UINT counterOffset = 0, UINT value = 0);

        RHIBuffer* ReserveUploadMemory(UINT sizeInBytes, UINT alignment = RHI_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

        void InitializeTexture(RHITexture* dest, std::vector<RHI_SUBRESOURCE_DATA> subresources);
        void InitializeTexture(RHITexture* dest, UINT firstSubresource, std::vector<RHI_SUBRESOURCE_DATA> subresources);
        void InitializeBuffer(RHIBuffer* dest, const void* data, UINT numBytes, UINT destOffset = 0);
        void InitializeTextureArraySlice(RHITexture* dest, UINT sliceIndex, RHITexture* src);

        void WriteBuffer(RHIBuffer* dest, UINT destOffset, const void* data, UINT numBytes);
        void FillBuffer(RHIBuffer* dest, UINT destOffset, DWParam value, UINT numBytes);

        void GenerateMips(RHITexture* rt);

        void TransitionBarrier(RHIResource* resource, RHI_RESOURCE_STATES state, UINT subresource = RHI_RESOURCE_BARRIER_ALL_SUBRESOURCES, BOOL flushImmediate = false);
        void AliasingBarrier(RHIResource* beforeResource, RHIResource* afterResource, BOOL flushImmediate = false);
        void InsertUAVBarrier(RHIResource* resource, BOOL flushImmediate = false);
        void FlushResourceBarriers();

        BOOL AssertResourceState(RHIResource* resource, RHI_RESOURCE_STATES state, UINT subresource);

        void ClearUAV(RHIBuffer* target);
        void ClearUAV(RHITexture* target);
        
        void ClearColor(RHITexture* target, RHIRect* rect = nullptr);
        void ClearColor(RHITexture* target, FLOAT colour[4], RHIRect* rect = nullptr);
        void ClearDepth(RHITexture* target);
        void ClearStencil(RHITexture* target);
        void ClearDepthAndStencil(RHITexture* target);

        void ClearRenderTarget(BOOL clearDepth, BOOL clearColor, COLOR backgroundColor, FLOAT depth);
        void ClearRenderTarget(RTClearFlags clearFlags, COLOR backgroundColor, FLOAT depth, UINT stencil);

        void SetRootSignature(RHIRootSignature* rootSignature);
        void SetPipelineState(RHIPipelineState* pipelineState);

        void SetPredication(RHIResource* buffer, UINT bufferOffset, RHI_PREDICATION_OP op);

        void ClearRenderTarget(RHIRenderTargetView* renderTargetView, RHIDepthStencilView* depthStencilView);
        void ClearRenderTarget(std::vector<RHIRenderTargetView*> renderTargetViews, RHIDepthStencilView* depthStencilView);

        void SetRenderTarget(RHIRenderTargetView* renderTargetView);
        void SetRenderTarget(RHIRenderTargetView* renderTargetView, RHIDepthStencilView* depthStencilView);
        void SetRenderTarget(std::vector<RHIRenderTargetView*> renderTargetViews);
        void SetRenderTarget(std::vector<RHIRenderTargetView*> renderTargetViews, RHIDepthStencilView* depthStencilView);

        void SetViewport(const RHIViewport& viewport);
        void SetViewport(FLOAT topLeftX, FLOAT topLeftY, FLOAT width, FLOAT height, FLOAT minDepth = 0.0f, FLOAT maxDepth = 1.0f);
        void SetViewports(std::vector<RHIViewport> viewports);
        void SetScissorRect(const RHIRect& scissorRect);
        void SetScissorRect(UINT left, UINT top, UINT right, UINT bottom);
        void SetScissorRects(std::vector<RHIRect> scissorRects);
        void SetViewportAndScissorRect(const RHIViewport& vp, const RHIRect& rect);
        void SetViewportAndScissorRect(UINT x, UINT y, UINT w, UINT h);
        void SetStencilRef(UINT stencilRef);
        void SetBlendFactor(FLOAT3 blendFactor);
        void SetPrimitiveTopology(RHI_PRIMITIVE_TOPOLOGY topology);

        void SetConstantArray(UINT rootIndex, UINT numConstants, const void* pConstants);
        void SetConstant(UINT rootIndex, UINT offset, DWParam val);
        void SetConstants(UINT rootIndex, DWParam x);
        void SetConstants(UINT rootIndex, DWParam x, DWParam y);
        void SetConstants(UINT rootIndex, DWParam x, DWParam y, DWParam z);
        void SetConstants(UINT rootIndex, DWParam x, DWParam y, DWParam z, DWParam w);
        void SetConstantBuffer(UINT rootIndex, RHI_GPU_VIRTUAL_ADDRESS cbv);
        void SetDynamicConstantBufferView(UINT rootParameterIndex, UINT64 bufferSize, const void* bufferData);
        template<typename T>
        void SetDynamicConstantBufferView(UINT rootParameterIndex, const T& data) { SetDynamicConstantBufferView(rootParameterIndex, sizeof(T), &data); }
        void SetBufferSRV(UINT rootIndex, RHIBuffer* bufferSRV, UINT64 offset = 0);
        void SetBufferUAV(UINT rootIndex, RHIBuffer* bufferUAV, UINT64 offset = 0);
        void SetDescriptorTable(UINT rootIndex, RHI_GPU_DESCRIPTOR_HANDLE FirstHandle);

        void SetDynamicDescriptor(UINT rootIndex, UINT offset, RHI_CPU_DESCRIPTOR_HANDLE handle);
        void SetDynamicDescriptors(UINT rootIndex, UINT offset, UINT count, const RHI_CPU_DESCRIPTOR_HANDLE handles[]);
        void SetDynamicSampler(UINT rootIndex, UINT offset, RHI_CPU_DESCRIPTOR_HANDLE handle);
        void SetDynamicSamplers(UINT rootIndex, UINT offset, UINT count, const RHI_CPU_DESCRIPTOR_HANDLE handles[]);

        void SetIndexBuffer(const RHI_INDEX_BUFFER_VIEW& ibView);
        void SetVertexBuffer(UINT slot, const RHI_VERTEX_BUFFER_VIEW& vbView);
        void SetVertexBuffers(UINT StartSlot, UINT count, const RHI_VERTEX_BUFFER_VIEW vbViews[]);
        void SetDynamicVB(UINT slot, UINT64 numVertices, UINT64 VertexStride, const void* vbData);
        void SetDynamicIB(UINT64 indexCount, const UINT* ibData);
        void SetDynamicSRV(UINT rootIndex, UINT64 bufferSize, const void* bufferData);

        void Draw(UINT vertexCount, UINT vertexStartOffset = 0);
        void DrawIndexed(UINT indexCount, UINT startIndexLocation = 0, INT baseVertexLocation = 0);
        void DrawInstanced(UINT vertexCount, UINT instanceCount, UINT startVertexLocation, UINT startInstanceLocation);
        void DrawIndexedInstanced(UINT indexCount, UINT instanceCount, UINT startIndexLocation, INT baseVertexLocation, UINT startInstanceLocation);
        void DrawIndirect(RHIResource* argumentBuffer, UINT64 argumentBufferOffset = 0);

        void Dispatch(UINT threadGroupCountX, UINT threadGroupCountY, UINT threadGroupCountZ);
        void Dispatch1D(UINT64 threadCountX, UINT64 groupSizeX = 64);
        void Dispatch2D(UINT64 threadCountX, UINT64 threadCountY, UINT64 groupSizeX = 8, UINT64 groupSizeY = 8);
        void Dispatch3D(UINT64 threadCountX, UINT64 threadCountY, UINT64 ThreadCountZ, UINT64 groupSizeX, UINT64 groupSizeY, UINT64 groupSizeZ);
        void DispatchIndirect(RHIResource* argumentBuffer, UINT64 argumentBufferOffset = 0);

        void ExecuteIndirect(RHICommandSignature* commandSig, RHIResource* argumentBuffer, UINT64 argumentStartOffset = 0,
            UINT maxCommands = 1, RHIResource* commandCounterBuffer = nullptr, UINT64 counterOffset = 0);

        void DispatchMesh(UINT ThreadGroupCountX, UINT ThreadGroupCountY, UINT ThreadGroupCountZ);

    protected:


    };

}



