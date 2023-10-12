#pragma once
#include "d3d12_core.h"

namespace RHI
{
    constexpr D3D12_PRIMITIVE_TOPOLOGY_TYPE RHITranslateD3D12(RHI_PRIMITIVE_TOPOLOGY Topology)
    {
        // clang-format off
		switch (Topology)
		{
		case RHI_PRIMITIVE_TOPOLOGY::Undefined: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
		case RHI_PRIMITIVE_TOPOLOGY::Point:		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
		case RHI_PRIMITIVE_TOPOLOGY::Line:		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		case RHI_PRIMITIVE_TOPOLOGY::Triangle:	return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		case RHI_PRIMITIVE_TOPOLOGY::Patch:		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
		}
        // clang-format on
        return {};
    }

    constexpr D3D12_COMPARISON_FUNC RHITranslateD3D12(RHI_COMPARISON_FUNC Func)
    {
        // clang-format off
		switch (Func)
		{
		case RHI_COMPARISON_FUNC::Never:		return D3D12_COMPARISON_FUNC_NEVER;
		case RHI_COMPARISON_FUNC::Less:			return D3D12_COMPARISON_FUNC_LESS;
		case RHI_COMPARISON_FUNC::Equal:		return D3D12_COMPARISON_FUNC_EQUAL;
		case RHI_COMPARISON_FUNC::LessEqual:	return D3D12_COMPARISON_FUNC_LESS_EQUAL;
		case RHI_COMPARISON_FUNC::Greater:		return D3D12_COMPARISON_FUNC_GREATER;
		case RHI_COMPARISON_FUNC::NotEqual:		return D3D12_COMPARISON_FUNC_NOT_EQUAL;
		case RHI_COMPARISON_FUNC::GreaterEqual:	return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
		case RHI_COMPARISON_FUNC::Always:		return D3D12_COMPARISON_FUNC_ALWAYS;
		}
        // clang-format on
        return {};
    }

    constexpr D3D12_BLEND_OP RHITranslateD3D12(RHI_BLEND_OP Op)
    {
        // clang-format off
		switch (Op)
		{
		case RHI_BLEND_OP::Add:				return D3D12_BLEND_OP_ADD;
		case RHI_BLEND_OP::Subtract:		return D3D12_BLEND_OP_SUBTRACT;
		case RHI_BLEND_OP::ReverseSubtract:	return D3D12_BLEND_OP_REV_SUBTRACT;
		case RHI_BLEND_OP::Min:				return D3D12_BLEND_OP_MIN;
		case RHI_BLEND_OP::Max:				return D3D12_BLEND_OP_MAX;
		}
        // clang-format on
        return {};
    }

    constexpr D3D12_BLEND RHITranslateD3D12(RHI_FACTOR Factor)
    {
        // clang-format off
		switch (Factor)
		{
		case RHI_FACTOR::Zero:					return D3D12_BLEND_ZERO;
		case RHI_FACTOR::One:					return D3D12_BLEND_ONE;
		case RHI_FACTOR::SrcColor:				return D3D12_BLEND_SRC_COLOR;
		case RHI_FACTOR::OneMinusSrcColor:		return D3D12_BLEND_INV_SRC_COLOR;
		case RHI_FACTOR::DstColor:				return D3D12_BLEND_DEST_COLOR;
		case RHI_FACTOR::OneMinusDstColor:		return D3D12_BLEND_INV_DEST_COLOR;
		case RHI_FACTOR::SrcAlpha:				return D3D12_BLEND_SRC_ALPHA;
		case RHI_FACTOR::OneMinusSrcAlpha:		return D3D12_BLEND_INV_SRC_ALPHA;
		case RHI_FACTOR::DstAlpha:				return D3D12_BLEND_DEST_ALPHA;
		case RHI_FACTOR::OneMinusDstAlpha:		return D3D12_BLEND_INV_DEST_ALPHA;
		case RHI_FACTOR::BlendFactor:			return D3D12_BLEND_BLEND_FACTOR;
		case RHI_FACTOR::OneMinusBlendFactor:	return D3D12_BLEND_INV_BLEND_FACTOR;
		case RHI_FACTOR::SrcAlphaSaturate:		return D3D12_BLEND_SRC_ALPHA_SAT;
		case RHI_FACTOR::Src1Color:				return D3D12_BLEND_SRC1_COLOR;
		case RHI_FACTOR::OneMinusSrc1Color:		return D3D12_BLEND_INV_SRC1_COLOR;
		case RHI_FACTOR::Src1Alpha:				return D3D12_BLEND_SRC1_ALPHA;
		case RHI_FACTOR::OneMinusSrc1Alpha:		return D3D12_BLEND_INV_SRC1_ALPHA;
		}
        // clang-format on
        return {};
    }

    constexpr D3D12_FILL_MODE RHITranslateD3D12(RHI_FILL_MODE FillMode)
    {
        // clang-format off
		switch (FillMode)
		{
		case RHI_FILL_MODE::Wireframe:  return D3D12_FILL_MODE_WIREFRAME;
		case RHI_FILL_MODE::Solid:		return D3D12_FILL_MODE_SOLID;
		}
        // clang-format on
        return {};
    }

    constexpr D3D12_CULL_MODE RHITranslateD3D12(RHI_CULL_MODE CullMode)
    {
        // clang-format off
		switch (CullMode)
		{
		case RHI_CULL_MODE::None:	return D3D12_CULL_MODE_NONE;
		case RHI_CULL_MODE::Front:  return D3D12_CULL_MODE_FRONT;
		case RHI_CULL_MODE::Back:	return D3D12_CULL_MODE_BACK;
		}
        // clang-format on
        return {};
    }

    constexpr D3D12_STENCIL_OP RHITranslateD3D12(RHI_STENCIL_OP Op)
    {
        // clang-format off
		switch (Op)
		{
		case RHI_STENCIL_OP::Keep:				return D3D12_STENCIL_OP_KEEP;
		case RHI_STENCIL_OP::Zero:				return D3D12_STENCIL_OP_ZERO;
		case RHI_STENCIL_OP::Replace:			return D3D12_STENCIL_OP_REPLACE;
		case RHI_STENCIL_OP::IncreaseSaturate:	return D3D12_STENCIL_OP_INCR_SAT;
		case RHI_STENCIL_OP::DecreaseSaturate:	return D3D12_STENCIL_OP_DECR_SAT;
		case RHI_STENCIL_OP::Invert:			return D3D12_STENCIL_OP_INVERT;
		case RHI_STENCIL_OP::Increase:			return D3D12_STENCIL_OP_INCR;
		case RHI_STENCIL_OP::Decrease:			return D3D12_STENCIL_OP_DECR;
		}
        // clang-format on
        return {};
    }

    inline D3D12_DEPTH_STENCILOP_DESC RHITranslateD3D12(const RHIDepthStencilOpDesc& Desc)
    {
        return {RHITranslateD3D12(Desc.StencilFailOp),
                RHITranslateD3D12(Desc.StencilDepthFailOp),
                RHITranslateD3D12(Desc.StencilPassOp),
                RHITranslateD3D12(Desc.StencilFunc)};
    }

    inline D3D12_BLEND_DESC RHITranslateD3D12(const RHIBlendState& BlendState)
    {
        D3D12_BLEND_DESC Desc       = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        Desc.AlphaToCoverageEnable  = BlendState.AlphaToCoverageEnable;
        Desc.IndependentBlendEnable = BlendState.IndependentBlendEnable;
        size_t Index                = 0;
        for (auto& RenderTarget : Desc.RenderTarget)
        {
            auto& RHIRenderTarget = BlendState.RenderTargets[Index++];

            RenderTarget.BlendEnable    = RHIRenderTarget.BlendEnable ? TRUE : FALSE;
            RenderTarget.LogicOpEnable  = FALSE;
            RenderTarget.SrcBlend       = RHITranslateD3D12(RHIRenderTarget.SrcBlendRgb);
            RenderTarget.DestBlend      = RHITranslateD3D12(RHIRenderTarget.DstBlendRgb);
            RenderTarget.BlendOp        = RHITranslateD3D12(RHIRenderTarget.BlendOpRgb);
            RenderTarget.SrcBlendAlpha  = RHITranslateD3D12(RHIRenderTarget.SrcBlendAlpha);
            RenderTarget.DestBlendAlpha = RHITranslateD3D12(RHIRenderTarget.DstBlendAlpha);
            RenderTarget.BlendOpAlpha   = RHITranslateD3D12(RHIRenderTarget.BlendOpAlpha);
            RenderTarget.LogicOp        = D3D12_LOGIC_OP_CLEAR;
            RenderTarget.RenderTargetWriteMask |= RHIRenderTarget.WriteMask.R ? D3D12_COLOR_WRITE_ENABLE_RED : 0;
            RenderTarget.RenderTargetWriteMask |= RHIRenderTarget.WriteMask.G ? D3D12_COLOR_WRITE_ENABLE_GREEN : 0;
            RenderTarget.RenderTargetWriteMask |= RHIRenderTarget.WriteMask.B ? D3D12_COLOR_WRITE_ENABLE_BLUE : 0;
            RenderTarget.RenderTargetWriteMask |= RHIRenderTarget.WriteMask.A ? D3D12_COLOR_WRITE_ENABLE_ALPHA : 0;
        }
        return Desc;
    }

    inline D3D12_RASTERIZER_DESC RHITranslateD3D12(const RHIRasterizerState& RasterizerState)
    {
        D3D12_RASTERIZER_DESC Desc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        Desc.FillMode              = RHITranslateD3D12(RasterizerState.FillMode);
        Desc.CullMode              = RHITranslateD3D12(RasterizerState.CullMode);
        Desc.FrontCounterClockwise = RasterizerState.FrontCounterClockwise;
        Desc.DepthBias             = RasterizerState.DepthBias;
        Desc.DepthBiasClamp        = RasterizerState.DepthBiasClamp;
        Desc.SlopeScaledDepthBias  = RasterizerState.SlopeScaledDepthBias;
        Desc.DepthClipEnable       = RasterizerState.DepthClipEnable;
        Desc.MultisampleEnable     = RasterizerState.MultisampleEnable;
        Desc.AntialiasedLineEnable = RasterizerState.AntialiasedLineEnable;
        Desc.ForcedSampleCount     = RasterizerState.ForcedSampleCount;
        Desc.ConservativeRaster    = RasterizerState.ConservativeRaster ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON :
                                                                          D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
        return Desc;
    }

    inline D3D12_DEPTH_STENCIL_DESC RHITranslateD3D12(const RHIDepthStencilState& DepthStencilState)
    {
        D3D12_DEPTH_STENCIL_DESC Desc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        Desc.DepthEnable              = DepthStencilState.DepthEnable;
        Desc.DepthWriteMask   = DepthStencilState.DepthWrite ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
        Desc.DepthFunc        = RHITranslateD3D12(DepthStencilState.DepthFunc);
        Desc.StencilEnable    = DepthStencilState.StencilEnable;
        Desc.StencilReadMask  = static_cast<UINT8>(DepthStencilState.StencilReadMask);
        Desc.StencilWriteMask = static_cast<UINT8>(DepthStencilState.StencilWriteMask);
        Desc.FrontFace        = RHITranslateD3D12(DepthStencilState.FrontFace);
        Desc.BackFace         = RHITranslateD3D12(DepthStencilState.BackFace);
        return Desc;
    }

    inline DXGI_SAMPLE_DESC RHITranslateD3D12(const RHISampleState& SampleState)
    {
        return DXGI_SAMPLE_DESC {SampleState.Count, SampleState.Quality};
    }

    inline D3D12_RESOURCE_STATES RHITranslateD3D12(const RHIResourceState& ResourceState)
    {
        D3D12_RESOURCE_STATES _TargetState = D3D12_RESOURCE_STATE_COMMON;
        // clang-format off
        switch (ResourceState)
        {
            case RHI_RESOURCE_STATE_COMMON: _TargetState = D3D12_RESOURCE_STATE_COMMON; break;
            case RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER: _TargetState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER; break;
            case RHI_RESOURCE_STATE_INDEX_BUFFER: _TargetState = D3D12_RESOURCE_STATE_INDEX_BUFFER; break;
            case RHI_RESOURCE_STATE_RENDER_TARGET: _TargetState = D3D12_RESOURCE_STATE_RENDER_TARGET; break;
            case RHI_RESOURCE_STATE_UNORDERED_ACCESS: _TargetState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS; break;
            case RHI_RESOURCE_STATE_DEPTH_WRITE: _TargetState = D3D12_RESOURCE_STATE_DEPTH_WRITE; break;
            case RHI_RESOURCE_STATE_DEPTH_READ: _TargetState = D3D12_RESOURCE_STATE_DEPTH_READ; break;
            case RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE: _TargetState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE; break;
            case RHI_RESOURCE_STATE_PIXEL_SHADER_RESOURCE: _TargetState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE; break;
            case RHI_RESOURCE_STATE_STREAM_OUT: _TargetState = D3D12_RESOURCE_STATE_STREAM_OUT; break;
            case RHI_RESOURCE_STATE_INDIRECT_ARGUMENT: _TargetState = D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT; break;
            case RHI_RESOURCE_STATE_COPY_DEST: _TargetState = D3D12_RESOURCE_STATE_COPY_DEST; break;
            case RHI_RESOURCE_STATE_COPY_SOURCE: _TargetState = D3D12_RESOURCE_STATE_COPY_SOURCE; break;
            case RHI_RESOURCE_STATE_RESOLVE_DEST: _TargetState = D3D12_RESOURCE_STATE_RESOLVE_DEST; break;
            case RHI_RESOURCE_STATE_RESOLVE_SOURCE: _TargetState = D3D12_RESOURCE_STATE_RESOLVE_SOURCE; break;
            case RHI_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE: _TargetState = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE; break;
            case RHI_RESOURCE_STATE_SHADING_RATE_SOURCE: _TargetState = D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE; break;
            case RHI_RESOURCE_STATE_GENERIC_READ: _TargetState = D3D12_RESOURCE_STATE_GENERIC_READ; break;
            case RHI_RESOURCE_STATE_ALL_SHADER_RESOURCE: _TargetState = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE; break;
            //case RHI_RESOURCE_STATE_PRESENT: break;
            //case RHI_RESOURCE_STATE_PREDICATION: break;
            default:
                break;
        }
        // clang-format on
        return _TargetState;
    }
}
