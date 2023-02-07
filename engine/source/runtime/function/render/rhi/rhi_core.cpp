#include "rhi_core.h"

void RHIParsePipelineStream(const PipelineStateStreamDesc& Desc, IPipelineParserCallbacks* Callbacks)
{
    if (Desc.SizeInBytes == 0 || Desc.pPipelineStateSubobjectStream == nullptr)
    {
        Callbacks->ErrorBadInputParameter(1); // first parameter issue
        return;
    }

    bool SubobjectSeen[static_cast<size_t>(RHI_PIPELINE_STATE_SUBOBJECT_TYPE::NumTypes)] = {};
    for (size_t CurOffset = 0, SizeOfSubobject = 0; CurOffset < Desc.SizeInBytes; CurOffset += SizeOfSubobject)
    {
        std::byte* Stream        = static_cast<std::byte*>(Desc.pPipelineStateSubobjectStream) + CurOffset;
        auto       SubobjectType = *reinterpret_cast<RHI_PIPELINE_STATE_SUBOBJECT_TYPE*>(Stream);
        size_t     Index         = static_cast<size_t>(SubobjectType);

        if (Index < 0 || Index >= static_cast<size_t>(RHI_PIPELINE_STATE_SUBOBJECT_TYPE::NumTypes))
        {
            Callbacks->ErrorUnknownSubobject(Index);
            return;
        }
        if (SubobjectSeen[Index])
        {
            Callbacks->ErrorDuplicateSubobject(SubobjectType);
            return; // disallow subobject duplicates in a stream
        }
        SubobjectSeen[Index] = true;

        switch (SubobjectType)
        {
            case RHI_PIPELINE_STATE_SUBOBJECT_TYPE::RootSignature:
                Callbacks->RootSignatureCb(*reinterpret_cast<PipelineStateStreamRootSignature*>(Stream));
                SizeOfSubobject = sizeof(PipelineStateStreamRootSignature);
                break;
            case RHI_PIPELINE_STATE_SUBOBJECT_TYPE::InputLayout:
                Callbacks->InputLayoutCb(*reinterpret_cast<PipelineStateStreamInputLayout*>(Stream));
                SizeOfSubobject = sizeof(PipelineStateStreamInputLayout);
                break;
            case RHI_PIPELINE_STATE_SUBOBJECT_TYPE::VS:
                Callbacks->VSCb(*reinterpret_cast<PipelineStateStreamVS*>(Stream));
                SizeOfSubobject = sizeof(PipelineStateStreamVS);
                break;
            case RHI_PIPELINE_STATE_SUBOBJECT_TYPE::PS:
                Callbacks->PSCb(*reinterpret_cast<PipelineStateStreamPS*>(Stream));
                SizeOfSubobject = sizeof(PipelineStateStreamPS);
                break;
            case RHI_PIPELINE_STATE_SUBOBJECT_TYPE::DS:
                Callbacks->DSCb(*reinterpret_cast<PipelineStateStreamDS*>(Stream));
                SizeOfSubobject = sizeof(PipelineStateStreamPS);
                break;
            case RHI_PIPELINE_STATE_SUBOBJECT_TYPE::HS:
                Callbacks->HSCb(*reinterpret_cast<PipelineStateStreamHS*>(Stream));
                SizeOfSubobject = sizeof(PipelineStateStreamHS);
                break;
            case RHI_PIPELINE_STATE_SUBOBJECT_TYPE::GS:
                Callbacks->GSCb(*reinterpret_cast<PipelineStateStreamGS*>(Stream));
                SizeOfSubobject = sizeof(PipelineStateStreamGS);
                break;
            case RHI_PIPELINE_STATE_SUBOBJECT_TYPE::CS:
                Callbacks->CSCb(*reinterpret_cast<PipelineStateStreamCS*>(Stream));
                SizeOfSubobject = sizeof(PipelineStateStreamCS);
                break;
            case RHI_PIPELINE_STATE_SUBOBJECT_TYPE::AS:
                Callbacks->ASCb(*reinterpret_cast<PipelineStateStreamMS*>(Stream));
                SizeOfSubobject = sizeof(PipelineStateStreamAS);
                break;
            case RHI_PIPELINE_STATE_SUBOBJECT_TYPE::MS:
                Callbacks->MSCb(*reinterpret_cast<PipelineStateStreamMS*>(Stream));
                SizeOfSubobject = sizeof(PipelineStateStreamMS);
                break;
            case RHI_PIPELINE_STATE_SUBOBJECT_TYPE::BlendState:
                Callbacks->BlendStateCb(*reinterpret_cast<PipelineStateStreamBlendState*>(Stream));
                SizeOfSubobject = sizeof(PipelineStateStreamBlendState);
                break;
            case RHI_PIPELINE_STATE_SUBOBJECT_TYPE::RasterizerState:
                Callbacks->RasterizerStateCb(*reinterpret_cast<PipelineStateStreamRasterizerState*>(Stream));
                SizeOfSubobject = sizeof(PipelineStateStreamRasterizerState);
                break;
            case RHI_PIPELINE_STATE_SUBOBJECT_TYPE::SampleState:
                Callbacks->SampleStateCb(*reinterpret_cast<PipelineStateStreamSampleState*>(Stream));
                SizeOfSubobject = sizeof(PipelineStateStreamSampleState);
                break;
            case RHI_PIPELINE_STATE_SUBOBJECT_TYPE::DepthStencilState:
                Callbacks->DepthStencilStateCb(*reinterpret_cast<PipelineStateStreamDepthStencilState*>(Stream));
                SizeOfSubobject = sizeof(PipelineStateStreamDepthStencilState);
                break;
            case RHI_PIPELINE_STATE_SUBOBJECT_TYPE::RenderTargetState:
                Callbacks->RenderTargetStateCb(*reinterpret_cast<PipelineStateStreamRenderTargetState*>(Stream));
                SizeOfSubobject = sizeof(PipelineStateStreamRenderTargetState);
                break;
            case RHI_PIPELINE_STATE_SUBOBJECT_TYPE::PrimitiveTopology:
                Callbacks->PrimitiveTopologyTypeCb(*reinterpret_cast<PipelineStateStreamPrimitiveTopology*>(Stream));
                SizeOfSubobject = sizeof(PipelineStateStreamPrimitiveTopology);
                break;
            default:
                Callbacks->ErrorUnknownSubobject(Index);
                return;
        }
    }
}
