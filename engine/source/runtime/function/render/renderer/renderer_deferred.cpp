#include "renderer_deferred.h"

#include "runtime/function/render/renderer/renderer_registry.h"

namespace Pilot
{
    DeferredRenderer::DeferredRenderer(RHI::D3D12Device* Device, ShaderCompiler* Compiler) : Renderer(Device, Compiler)
    {
        /*
        Shaders::Compile(Compiler);
        RootSignatures::Compile(Device, Registry);
        PipelineStates::Compile(Device, Registry);

    	RHI::CommandSignatureDesc Builder(4, sizeof(CommandSignatureParams));
        Builder.AddConstant(0, 0, 1);
        Builder.AddVertexBufferView(0);
        Builder.AddIndexBufferView();
        Builder.AddDrawIndexed();

    	CommandSignature = RHI::D3D12CommandSignature(
            Device, Builder, Registry.GetRootSignature(RootSignatures::GBuffer)->GetApiHandle());

        IndirectCommandBuffer    = RHI::D3D12Buffer(Device->GetLinkedDevice(),
                                                 CommandBufferCounterOffset + sizeof(UINT64),
                                                 sizeof(CommandSignatureParams),
                                                 D3D12_HEAP_TYPE_DEFAULT,
                                                 D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        IndirectCommandBufferUav = RHI::D3D12UnorderedAccessView(
            Device->GetLinkedDevice(), &IndirectCommandBuffer, RenderScene::MeshLimit, CommandBufferCounterOffset);
        */
    }

    DeferredRenderer::~DeferredRenderer() 
    {

    }

    void DeferredRenderer::OnRender(RHI::D3D12CommandContext& Context)
    {

    }

}

