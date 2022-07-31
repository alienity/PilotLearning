#include "render_mesh.h"

namespace Pilot
{
    // clang-format off

    //--------------------------------------------------------------------------------------
    // Vertex struct holding position information.
    const D3D12_INPUT_ELEMENT_DESC MeshVertex::D3D12MeshVertexPosition::InputElements[] =
    {
        { "POSITION",    0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    static_assert(sizeof(MeshVertex::D3D12MeshVertexPosition) == 12, "Vertex struct/layout mismatch");

    const RHI::D3D12InputLayout MeshVertex::D3D12MeshVertexPosition::InputLayout = 
        RHI::D3D12InputLayout( MeshVertex::D3D12MeshVertexPosition::InputElements, 
            MeshVertex::D3D12MeshVertexPosition::InputElementCount);

    //--------------------------------------------------------------------------------------
    // Vertex struct holding position and texture mapping information.
    const D3D12_INPUT_ELEMENT_DESC MeshVertex::D3D12MeshVertexPositionTexture::InputElements[] =
    {
        { "POSITION",    0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",    0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    static_assert(sizeof(MeshVertex::D3D12MeshVertexPositionTexture) == 20, "Vertex struct/layout mismatch");

    const RHI::D3D12InputLayout MeshVertex::D3D12MeshVertexPositionTexture::InputLayout = 
        RHI::D3D12InputLayout( MeshVertex::D3D12MeshVertexPositionTexture::InputElements, 
            MeshVertex::D3D12MeshVertexPositionTexture::InputElementCount);

    //--------------------------------------------------------------------------------------
    // Vertex struct holding position, normal, tangent and texture mapping information.
    const D3D12_INPUT_ELEMENT_DESC MeshVertex::D3D12MeshVertexPositionNormalTangentTexture::InputElements[] =
    {
        { "POSITION",    0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL",      0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT",     0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",    0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    static_assert(sizeof(MeshVertex::D3D12MeshVertexPositionNormalTangentTexture) == 48, "Vertex struct/layout mismatch");

    const RHI::D3D12InputLayout MeshVertex::D3D12MeshVertexPositionNormalTangentTexture::InputLayout = 
        RHI::D3D12InputLayout( MeshVertex::D3D12MeshVertexPositionNormalTangentTexture::InputElements, 
            MeshVertex::D3D12MeshVertexPositionNormalTangentTexture::InputElementCount);

    //--------------------------------------------------------------------------------------
    // Vertex struct holding position, normal, tangent and texture mapping information.
    const D3D12_INPUT_ELEMENT_DESC MeshVertex::D3D12MeshVertexPositionNormalTangentTextureJointBinding::InputElements[] =
    {
        { "POSITION",    0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL",      0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT",     0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",    0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "BLENDINDICES",0, DXGI_FORMAT_R32G32B32A32_UINT,  0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "BLENDWEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    static_assert(sizeof(MeshVertex::D3D12MeshVertexPositionNormalTangentTextureJointBinding) == 80, "Vertex struct/layout mismatch");

    const RHI::D3D12InputLayout MeshVertex::D3D12MeshVertexPositionNormalTangentTextureJointBinding::InputLayout = 
        RHI::D3D12InputLayout( MeshVertex::D3D12MeshVertexPositionNormalTangentTextureJointBinding::InputElements, 
            MeshVertex::D3D12MeshVertexPositionNormalTangentTextureJointBinding::InputElementCount);

    // clang-format on
}
