#include "cube_mesh.h"
#include "runtime/function/render/mikktspace.h"

namespace MoYu
{
	namespace Geometry
	{
        CubeMesh::CubeMesh(float width)
		{
            Vertex _t;
            _t.position = glm::vec3(-1, -1, 1) * width;
            _t.normal   = glm::vec3(0, 0, 1);
            _t.texcoord = glm::uvec2(0, 0);
            vertices.push_back(_t);
            _t.position = glm::vec3(1, -1, 1) * width;
            _t.normal   = glm::vec3(0, 0, 1);
            _t.texcoord = glm::uvec2(1, 0);
            vertices.push_back(_t);
            _t.position = glm::vec3(1, 1, 1) * width;
            _t.normal   = glm::vec3(0, 0, 1);
            _t.texcoord = glm::uvec2(1, 1);
            vertices.push_back(_t);
            _t.position = glm::vec3(1, 1, 1) * width;
            _t.normal   = glm::vec3(0, 0, 1);
            _t.texcoord = glm::uvec2(1, 1);
            vertices.push_back(_t);
            _t.position = glm::vec3(-1, 1, 1) * width;
            _t.normal   = glm::vec3(0, 0, 1);
            _t.texcoord = glm::uvec2(0, 1);
            vertices.push_back(_t);
            _t.position = glm::vec3(-1, -1, 1) * width;
            _t.normal   = glm::vec3(0, 0, 1);
            _t.texcoord = glm::uvec2(0, 0);
            vertices.push_back(_t);

            _t.position = glm::vec3(1, -1, 1) * width;
            _t.normal   = glm::vec3(1, 0, 0);
            _t.texcoord = glm::uvec2(0, 0);
            vertices.push_back(_t);
            _t.position = glm::vec3(1, -1, -1) * width;
            _t.normal   = glm::vec3(1, 0, 0);
            _t.texcoord = glm::uvec2(1, 0);
            vertices.push_back(_t);
            _t.position = glm::vec3(1, 1, -1) * width;
            _t.normal   = glm::vec3(1, 0, 0);
            _t.texcoord = glm::uvec2(1, 1);
            vertices.push_back(_t);
            _t.position = glm::vec3(1, 1, -1) * width;
            _t.normal   = glm::vec3(1, 0, 0);
            _t.texcoord = glm::uvec2(1, 1);
            vertices.push_back(_t);
            _t.position = glm::vec3(1, 1, 1) * width;
            _t.normal   = glm::vec3(1, 0, 0);
            _t.texcoord = glm::uvec2(0, 1);
            vertices.push_back(_t);
            _t.position = glm::vec3(1, -1, 1) * width;
            _t.normal   = glm::vec3(1, 0, 0);
            _t.texcoord = glm::uvec2(0, 0);
            vertices.push_back(_t);

            _t.position = glm::vec3(1, -1, -1) * width;
            _t.normal   = glm::vec3(0, 0, -1);
            _t.texcoord = glm::uvec2(0, 0);
            vertices.push_back(_t);
            _t.position = glm::vec3(-1, -1, -1) * width;
            _t.normal   = glm::vec3(0, 0, -1);
            _t.texcoord = glm::uvec2(1, 0);
            vertices.push_back(_t);
            _t.position = glm::vec3(-1, 1, -1) * width;
            _t.normal   = glm::vec3(0, 0, -1);
            _t.texcoord = glm::uvec2(1, 1);
            vertices.push_back(_t);
            _t.position = glm::vec3(-1, 1, -1) * width;
            _t.normal   = glm::vec3(0, 0, -1);
            _t.texcoord = glm::uvec2(1, 1);
            vertices.push_back(_t);
            _t.position = glm::vec3(1, 1, -1) * width;
            _t.normal   = glm::vec3(0, 0, -1);
            _t.texcoord = glm::uvec2(0, 1);
            vertices.push_back(_t);
            _t.position = glm::vec3(1, -1, -1) * width;
            _t.normal   = glm::vec3(0, 0, -1);
            _t.texcoord = glm::uvec2(0, 0);
            vertices.push_back(_t);

            _t.position = glm::vec3(-1, -1, -1) * width;
            _t.normal   = glm::vec3(-1, 0, 0);
            _t.texcoord = glm::uvec2(0, 0);
            vertices.push_back(_t);
            _t.position = glm::vec3(-1, -1, 1) * width;
            _t.normal   = glm::vec3(-1, 0, 0);
            _t.texcoord = glm::uvec2(1, 0);
            vertices.push_back(_t);
            _t.position = glm::vec3(-1, 1, 1) * width;
            _t.normal   = glm::vec3(-1, 0, 0);
            _t.texcoord = glm::uvec2(1, 1);
            vertices.push_back(_t);
            _t.position = glm::vec3(-1, 1, 1) * width;
            _t.normal   = glm::vec3(-1, 0, 0);
            _t.texcoord = glm::uvec2(1, 1);
            vertices.push_back(_t);
            _t.position = glm::vec3(-1, 1, -1) * width;
            _t.normal   = glm::vec3(-1, 0, 0);
            _t.texcoord = glm::uvec2(0, 1);
            vertices.push_back(_t);
            _t.position = glm::vec3(-1, -1, -1) * width;
            _t.normal   = glm::vec3(-1, 0, 0);
            _t.texcoord = glm::uvec2(0, 0);
            vertices.push_back(_t);

            _t.position = glm::vec3(-1, 1, 1) * width;
            _t.normal   = glm::vec3(0, 1, 0);
            _t.texcoord = glm::uvec2(0, 0);
            vertices.push_back(_t);
            _t.position = glm::vec3(1, 1, 1) * width;
            _t.normal   = glm::vec3(0, 1, 0);
            _t.texcoord = glm::uvec2(1, 0);
            vertices.push_back(_t);
            _t.position = glm::vec3(1, 1, -1) * width;
            _t.normal   = glm::vec3(0, 1, 0);
            _t.texcoord = glm::uvec2(1, 1);
            vertices.push_back(_t);
            _t.position = glm::vec3(1, 1, -1) * width;
            _t.normal   = glm::vec3(0, 1, 0);
            _t.texcoord = glm::uvec2(1, 1);
            vertices.push_back(_t);
            _t.position = glm::vec3(-1, 1, -1) * width;
            _t.normal   = glm::vec3(0, 1, 0);
            _t.texcoord = glm::uvec2(0, 1);
            vertices.push_back(_t);
            _t.position = glm::vec3(-1, 1, 1) * width;
            _t.normal   = glm::vec3(0, 1, 0);
            _t.texcoord = glm::uvec2(0, 0);
            vertices.push_back(_t);

            _t.position = glm::vec3(-1, -1, -1) * width;
            _t.normal   = glm::vec3(0, -1, 0);
            _t.texcoord = glm::uvec2(0, 0);
            vertices.push_back(_t);
            _t.position = glm::vec3(1, -1, -1) * width;
            _t.normal   = glm::vec3(0, -1, 0);
            _t.texcoord = glm::uvec2(1, 0);
            vertices.push_back(_t);
            _t.position = glm::vec3(1, -1, 1) * width;
            _t.normal   = glm::vec3(0, -1, 0);
            _t.texcoord = glm::uvec2(1, 1);
            vertices.push_back(_t);
            _t.position = glm::vec3(1, -1, 1) * width;
            _t.normal   = glm::vec3(0, -1, 0);
            _t.texcoord = glm::uvec2(1, 1);
            vertices.push_back(_t);
            _t.position = glm::vec3(-1, -1, 1) * width;
            _t.normal   = glm::vec3(0, -1, 0);
            _t.texcoord = glm::uvec2(0, 1);
            vertices.push_back(_t);
            _t.position = glm::vec3(-1, -1, -1) * width;
            _t.normal   = glm::vec3(0, -1, 0);
            _t.texcoord = glm::uvec2(0, 0);
            vertices.push_back(_t);

            for (size_t i = 0; i < 6; i++)
            {
                indices.push_back(0 + 6 * i);
                indices.push_back(1 + 6 * i);
                indices.push_back(2 + 6 * i);
                indices.push_back(3 + 6 * i);
                indices.push_back(4 + 6 * i);
                indices.push_back(5 + 6 * i);
            }
        }

        BasicMesh CubeMesh::ToBasicMesh(float width)
        {
            BasicMesh _mesh = CubeMesh(width);

            {
                SMikkTSpaceInterface iface {};
                iface.m_getNumFaces = [](const SMikkTSpaceContext* pContext) -> int {
                    BasicMesh* working_mesh = static_cast<BasicMesh*>(pContext->m_pUserData);
                    float f_size = (float)working_mesh->indices.size() / 3.f;
                    int   i_size = (int)working_mesh->indices.size() / 3;
                    assert((f_size - (float)i_size) == 0.f);
                    return i_size;
                };
                iface.m_getNumVerticesOfFace = [](const SMikkTSpaceContext* pContext, const int iFace) -> int {
                    return 3;
                };
                iface.m_getNormal =
                    [](const SMikkTSpaceContext* pContext, float fvNormOut[], const int iFace, const int iVert) {
                        BasicMesh* working_mesh = static_cast<BasicMesh*>(pContext->m_pUserData);
                        int indices_index = iFace * 3 + iVert;
                        int index = working_mesh->indices[indices_index];
                        auto vertex = working_mesh->vertices[index];
                        fvNormOut[0] = vertex.normal.x;
                        fvNormOut[1] = vertex.normal.y;
                        fvNormOut[2] = vertex.normal.z;
                    };
                iface.m_getPosition =
                    [](const SMikkTSpaceContext* pContext, float fvPosOut[], const int iFace, const int iVert) {
                        BasicMesh* working_mesh = static_cast<BasicMesh*>(pContext->m_pUserData);
                        int indices_index = iFace * 3 + iVert;
                        int index = working_mesh->indices[indices_index];
                        auto vertex = working_mesh->vertices[index];
                        fvPosOut[0] = vertex.position.x;
                        fvPosOut[1] = vertex.position.y;
                        fvPosOut[2] = vertex.position.z;
                    };
                iface.m_getTexCoord =
                    [](const SMikkTSpaceContext* pContext, float fvTexcOut[], const int iFace, const int iVert) {
                        BasicMesh* working_mesh = static_cast<BasicMesh*>(pContext->m_pUserData);
                        int indices_index = iFace * 3 + iVert;
                        int index = working_mesh->indices[indices_index];
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
                    int indices_index = iFace * 3 + iVert;
                    int index = working_mesh->indices[indices_index];
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

    } // namespace Geometry
} // namespace MoYu
