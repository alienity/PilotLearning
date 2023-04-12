#pragma once

#include "render_common.h"

namespace Pilot
{
    // clang-format off

    //--------------------------------------------------------------------------------------
    // Vertex struct holding position information.
    D3D12_INPUT_ELEMENT_DESC MeshInputLayout::D3D12MeshVertexPosition::InputElements[] =
    {
        { "POSITION",    0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    static_assert(sizeof(MeshInputLayout::D3D12MeshVertexPosition) == 12, "Vertex struct/layout mismatch");

    RHI::D3D12InputLayout MeshInputLayout::D3D12MeshVertexPosition::InputLayout =
        RHI::D3D12InputLayout(MeshInputLayout::D3D12MeshVertexPosition::InputElements, InputElementCount);
    
    //--------------------------------------------------------------------------------------
    // Vertex struct holding position and texture mapping information.
    D3D12_INPUT_ELEMENT_DESC MeshInputLayout::D3D12MeshVertexPositionTexture::InputElements[] =
    {
        { "POSITION",    0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",    0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    static_assert(sizeof(MeshInputLayout::D3D12MeshVertexPositionTexture) == 20, "Vertex struct/layout mismatch");

    RHI::D3D12InputLayout MeshInputLayout::D3D12MeshVertexPositionTexture::InputLayout = 
        RHI::D3D12InputLayout( MeshInputLayout::D3D12MeshVertexPositionTexture::InputElements, InputElementCount);

    //--------------------------------------------------------------------------------------
    // Vertex struct holding position, normal, tangent and texture mapping information.
    D3D12_INPUT_ELEMENT_DESC MeshInputLayout::D3D12MeshVertexPositionNormalTangentTexture::InputElements[] =
    {
        { "POSITION",    0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL",      0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT",     0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",    0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    static_assert(sizeof(MeshInputLayout::D3D12MeshVertexPositionNormalTangentTexture) == 48, "Vertex struct/layout mismatch");

    RHI::D3D12InputLayout MeshInputLayout::D3D12MeshVertexPositionNormalTangentTexture::InputLayout = 
        RHI::D3D12InputLayout( MeshInputLayout::D3D12MeshVertexPositionNormalTangentTexture::InputElements, InputElementCount);

    //--------------------------------------------------------------------------------------
    // Vertex struct holding position, normal, tangent and texture mapping information.
    D3D12_INPUT_ELEMENT_DESC MeshInputLayout::D3D12MeshVertexPositionNormalTangentTextureJointBinding::InputElements[] =
    {
        { "POSITION",    0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL",      0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT",     0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",    0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "BLENDINDICES",0, DXGI_FORMAT_R32G32B32A32_UINT,  0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "BLENDWEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    static_assert(sizeof(MeshInputLayout::D3D12MeshVertexPositionNormalTangentTextureJointBinding) == 80, "Vertex struct/layout mismatch");

    RHI::D3D12InputLayout MeshInputLayout::D3D12MeshVertexPositionNormalTangentTextureJointBinding::InputLayout = 
        RHI::D3D12InputLayout( MeshInputLayout::D3D12MeshVertexPositionNormalTangentTextureJointBinding::InputElements, InputElementCount);

    // clang-format on

} // namespace Pilot
