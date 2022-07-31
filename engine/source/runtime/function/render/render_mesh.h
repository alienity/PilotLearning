#pragma once

#include "runtime/core/math/moyu_math.h"
#include "runtime/function/render/rhi/d3d12/d3d12_core.h"

#include <array>
#include <glm/glm.hpp>

namespace Pilot
{
    struct MeshVertex
    {
        // Vertex struct holding position information.
        struct D3D12MeshVertexPosition
        {
            glm::vec3 position;

            static const RHI::D3D12InputLayout InputLayout;

        private:
            static constexpr unsigned int         InputElementCount = 1;
            static const D3D12_INPUT_ELEMENT_DESC InputElements[InputElementCount];
        };

        // Vertex struct holding position and color information.
        struct D3D12MeshVertexPositionTexture
        {
            glm::vec3 position;
            glm::vec2 texcoord;

            static const RHI::D3D12InputLayout InputLayout;

        private:
            static constexpr unsigned int         InputElementCount = 2;
            static const D3D12_INPUT_ELEMENT_DESC InputElements[InputElementCount];
        };

        // Vertex struct holding position and texture mapping information.

        struct D3D12MeshVertexPositionNormalTangentTexture
        {
            glm::vec3 position;
            glm::vec3 normal;
            glm::vec4 tangent;
            glm::vec2 texcoord;

            static const RHI::D3D12InputLayout InputLayout;

        private:
            static constexpr unsigned int         InputElementCount = 4;
            static const D3D12_INPUT_ELEMENT_DESC InputElements[InputElementCount];
        };

        struct D3D12MeshVertexPositionNormalTangentTextureJointBinding
        {
            glm::vec3  position;
            glm::vec3  normal;
            glm::vec4  tangent;
            glm::vec2  texcoord;
            glm::ivec4 indices;
            glm::vec4  weights;

            static const RHI::D3D12InputLayout InputLayout;

        private:
            static constexpr unsigned int         InputElementCount = 6;
            static const D3D12_INPUT_ELEMENT_DESC InputElements[InputElementCount];
        };

    };
} // namespace Pilot
