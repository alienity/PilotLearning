#include "triangle_mesh.h"
#include "runtime/function/render/mikktspace.h"

namespace MoYu
{
	namespace Geometry
	{
        TriangleMesh::TriangleMesh()
		{
            Vertex _t;
            _t.position = glm::vec3(-1, -1, -1);
            _t.normal   = glm::vec3(0, 0, 1);
            _t.texcoord = glm::uvec2(0, 0);
            vertices.push_back(_t);
            _t.position = glm::vec3(1, -1, -1);
            _t.normal   = glm::vec3(0, 0, 1);
            _t.texcoord = glm::uvec2(1, 0);
            vertices.push_back(_t);
            _t.position = glm::vec3(1, 1, -1);
            _t.normal   = glm::vec3(0, 0, 1);
            _t.texcoord = glm::uvec2(1, 1);
            vertices.push_back(_t);
            
            indices.push_back(0);
            indices.push_back(1);
            indices.push_back(2);
        }

        BasicMesh TriangleMesh::ToBasicMesh()
        {
            BasicMesh _mesh = TriangleMesh();

            {
                SMikkTSpaceInterface iface {};
                iface.m_getNumFaces          = [](const SMikkTSpaceContext* pContext) -> int { return 1; };
                iface.m_getNumVerticesOfFace = [](const SMikkTSpaceContext* pContext, const int iFace) -> int {
                    return 3;
                };
                iface.m_getNormal =
                    [](const SMikkTSpaceContext* pContext, float fvNormOut[], const int iFace, const int iVert) {
                        BasicMesh* working_mesh = static_cast<BasicMesh*>(pContext->m_pUserData);
                        auto index = iFace * 3 + iVert;
                        auto vertex = working_mesh->vertices[index];
                        fvNormOut[0] = vertex.normal.x;
                        fvNormOut[1] = vertex.normal.y;
                        fvNormOut[2] = vertex.normal.z;
                    };
                iface.m_getPosition =
                    [](const SMikkTSpaceContext* pContext, float fvPosOut[], const int iFace, const int iVert) {
                        BasicMesh* working_mesh = static_cast<BasicMesh*>(pContext->m_pUserData);
                        auto index = iFace * 3 + iVert;
                        auto vertex = working_mesh->vertices[index];
                        fvPosOut[0] = vertex.position.x;
                        fvPosOut[1] = vertex.position.y;
                        fvPosOut[2] = vertex.position.z;
                    };
                iface.m_getTexCoord =
                    [](const SMikkTSpaceContext* pContext, float fvTexcOut[], const int iFace, const int iVert) {
                        BasicMesh* working_mesh = static_cast<BasicMesh*>(pContext->m_pUserData);
                        auto index = iFace * 3 + iVert;
                        auto vertex = working_mesh->vertices[index];
                        fvTexcOut[0] = vertex.texcoord.x;
                        fvTexcOut[1] = vertex.texcoord.y;
                    };
                iface.m_setTSpaceBasic = [](const SMikkTSpaceContext* pContext,
                                            const float               fvTangent[],
                                            const float               fSign,
                                            const int                 iFace,
                                            const int                 iVert) {
                    BasicMesh* working_mesh = static_cast<BasicMesh*>(pContext->m_pUserData);
                    auto  index  = iFace * 3 + iVert;
                    auto* vertex = &working_mesh->vertices[index];
                    vertex->tangent.x = fvTangent[0];
                    vertex->tangent.y = fvTangent[1];
                    vertex->tangent.z = fvTangent[2];
                    vertex->tangent.w = fSign;
                };

                SMikkTSpaceContext context {};
                context.m_pInterface = &iface;
                context.m_pUserData  = &_mesh;

                genTangSpaceDefault(&context);
            }

            return _mesh;
        }

        StaticMeshData TriangleMesh::ToStaticMesh()
        {
            BasicMesh _basicMesh = ToBasicMesh();

            MoYu::StaticMeshData _staticMeshData = {};

            std::uint32_t vertex_buffer_size = _basicMesh.vertices.size() * sizeof(Vertex);
            std::uint32_t index_buffer_size  = _basicMesh.indices.size() * sizeof(std::uint32_t);

            _staticMeshData.m_InputElementDefinition = Vertex::InputElementDefinition;

            _staticMeshData.m_vertex_buffer = std::make_shared<MoYu::MoYuScratchBuffer>();
            _staticMeshData.m_vertex_buffer->Initialize(vertex_buffer_size);
            memcpy(_staticMeshData.m_vertex_buffer->GetBufferPointer(), _basicMesh.vertices.data(), vertex_buffer_size);

            _staticMeshData.m_index_buffer = std::make_shared<MoYu::MoYuScratchBuffer>();
            _staticMeshData.m_index_buffer->Initialize(index_buffer_size);
            memcpy(_staticMeshData.m_index_buffer->GetBufferPointer(), _basicMesh.indices.data(), index_buffer_size);

            return _staticMeshData;
        }

    } // namespace Geometry
} // namespace MoYu
