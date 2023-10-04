#include "mesh_tools.h"
#include "runtime/function/render/mikktspace.h"
#include "runtime/resource/basic_geometry/icosphere_mesh.h"

namespace MoYu
{
    namespace Geometry
    {
        Vertex VertexInterpolate(Vertex& v0, Vertex& v1, float t)
        {
            Vertex v;
            v.position  = glm::mix(v0.position, v1.position, t);
            v.normal    = glm::mix(v0.normal, v1.normal, t);
            v.texcoord = glm::mix(v0.texcoord, v1.texcoord, t);
            return v;
        }

        Vertex VertexVectorBuild(Vertex& init, Vertex& temp)
        {
            Vertex v;
            v.position  = temp.position - init.position;
            v.normal    = temp.normal - init.normal;
            v.texcoord = temp.texcoord - init.texcoord;
            return v;
        }

        Vertex VertexTransform(Vertex& v0, glm::mat4x4& matrix)
        {
            Vertex v;
            v.position  = matrix * glm::vec4(v0.position, 1);
            v.normal    = matrix * glm::vec4(v0.normal, 0);
            v.texcoord = v0.texcoord;
            return v;
        }

        AxisAlignedBox ToAxisAlignedBox(const BasicMesh& basicMesh)
        {
            float _fmax = std::numeric_limits<float>::max();
            float _fmin = std::numeric_limits<float>::min();

            glm::vec3 _min = glm::vec3(_fmax, _fmax, _fmax);
            glm::vec3 _max = glm::vec3(_fmin, _fmin, _fmin);

            for (size_t i = 0; i < basicMesh.vertices.size(); i++)
            {
                glm::vec3 _cutpos = basicMesh.vertices[i].position;

                _min = glm::min(_cutpos, _min);
                _max = glm::max(_cutpos, _max);
            }

            glm::vec3 _center = (_min + _max) * 0.5f;
            glm::vec3 _half_extent = (_max - _min) * 0.5f;

            Vector3 center = GLMUtil::toVec3(_center);
            Vector3 half_extent = GLMUtil::toVec3(_half_extent);

            AxisAlignedBox _outBox = AxisAlignedBox(center, half_extent);

            return _outBox;
        }

        StaticMeshData ToStaticMesh(const BasicMesh& basicMesh)
        {
            MoYu::StaticMeshData _staticMeshData = {};

            std::uint32_t vertex_buffer_size = basicMesh.vertices.size() * sizeof(Vertex);
            std::uint32_t index_buffer_size  = basicMesh.indices.size() * sizeof(std::uint32_t);

            _staticMeshData.m_InputElementDefinition = Vertex::InputElementDefinition;

            _staticMeshData.m_vertex_buffer = std::make_shared<MoYu::MoYuScratchBuffer>();
            _staticMeshData.m_vertex_buffer->Initialize(vertex_buffer_size);
            memcpy(_staticMeshData.m_vertex_buffer->GetBufferPointer(), basicMesh.vertices.data(), vertex_buffer_size);

            _staticMeshData.m_index_buffer = std::make_shared<MoYu::MoYuScratchBuffer>();
            _staticMeshData.m_index_buffer->Initialize(index_buffer_size);
            memcpy(_staticMeshData.m_index_buffer->GetBufferPointer(), basicMesh.indices.data(), index_buffer_size);

            return _staticMeshData;
        }


    }
}