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
            v.position = glm::mix(v0.position, v1.position, t);
            v.normal   = glm::mix(v0.normal, v1.normal, t);
            v.tangent  = glm::mix(v0.tangent, v1.tangent, t);
            v.uv0      = glm::mix(v0.uv0, v1.uv0, t);
            return v;
        }

        Vertex VertexVectorBuild(Vertex& init, Vertex& temp)
        {
            Vertex v;
            v.position  = temp.position - init.position;
            v.normal    = temp.normal - init.normal;
            v.tangent   = temp.tangent - init.tangent;
            v.uv0       = temp.uv0 - init.uv0;
            return v;
        }

        Vertex VertexTransform(Vertex& v0, glm::mat4x4& matrix)
        {
            Vertex v;
            v.position  = matrix * glm::vec4(v0.position, 1);
            return v;
        }

        AABB ToAxisAlignedBox(const BasicMesh& basicMesh)
        {
            float _fmax = std::numeric_limits<float>::max();
            float _fmin = std::numeric_limits<float>::min();

            glm::float3 _min = glm::float3(_fmax, _fmax, _fmax);
            glm::float3 _max = glm::float3(_fmin, _fmin, _fmin);

            for (size_t i = 0; i < basicMesh.vertices.size(); i++)
            {
                glm::float3 _cutpos = basicMesh.vertices[i].position;

                _min = glm::min(_cutpos, _min);
                _max = glm::max(_cutpos, _max);
            }

            glm::float3 _center = (_min + _max) * 0.5f;
            glm::float3 _half_extent = (_max - _min) * 0.5f;

            //glm::float3 center      = _center;
            //glm::float3 half_extent = _half_extent;

            glm::float3 _minpos = _center - _half_extent * 0.5f;
            glm::float3 _maxpos = _center + _half_extent * 0.5f;

            AABB _outBox = AABB(_minpos, _maxpos);

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

    namespace Geometry
    {
        TriangleMesh::TriangleMesh(float width)
        {
            Vertex _t;
            _t.position = glm::float3(-1, -1, 0) * width;
            _t.normal   = glm::float3(0, 0, 1);
            _t.tangent  = glm::float4(1, 0, 0, 1);
            _t.uv0      = glm::float2(0, 0);
            vertices.push_back(_t);
            _t.position = glm::float3(1, -1, 0) * width;
            _t.normal   = glm::float3(0, 0, 1);
            _t.tangent  = glm::float4(1, 0, 0, 1);
            _t.uv0      = glm::float2(1, 0);
            vertices.push_back(_t);
            _t.position = glm::float3(1, 1, 0) * width;
            _t.normal   = glm::float3(0, 0, 1);
            _t.tangent  = glm::float4(1, 0, 0, 1);
            _t.uv0      = glm::float2(1, 1);
            vertices.push_back(_t);

            indices.push_back(0);
            indices.push_back(1);
            indices.push_back(2);
        }

        BasicMesh TriangleMesh::ToBasicMesh(float width)
        {
            BasicMesh _mesh = TriangleMesh(width);

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
                        fvTexcOut[0] = vertex.uv0.x;
                        fvTexcOut[1] = vertex.uv0.y;
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

        SquareMesh::SquareMesh(float width)
        {
            Vertex _t;
            _t.position = glm::float3(-1, -1, 0) * width;
            _t.normal   = glm::float3(0, 0, 1);
            _t.tangent  = glm::float4(0, 0, 0, 1);
            _t.uv0      = glm::float2(0, 0);
            vertices.push_back(_t);
            _t.position = glm::float3(1, -1, 0) * width;
            _t.normal   = glm::float3(0, 0, 1);
            _t.tangent  = glm::float4(0, 0, 0, 1);
            _t.uv0      = glm::float2(1, 0);
            vertices.push_back(_t);
            _t.position = glm::float3(1, 1, 0) * width;
            _t.normal   = glm::float3(0, 0, 1);
            _t.tangent  = glm::float4(0, 0, 0, 1);
            _t.uv0      = glm::float2(1, 1);
            vertices.push_back(_t);
            _t.position = glm::float3(1, 1, 0) * width;
            _t.normal   = glm::float3(0, 0, 1);
            _t.tangent  = glm::float4(0, 0, 0, 1);
            _t.uv0      = glm::float2(1, 1);
            vertices.push_back(_t);
            _t.position = glm::float3(-1, 1, 0) * width;
            _t.normal   = glm::float3(0, 0, 1);
            _t.tangent  = glm::float4(0, 0, 0, 1);
            _t.uv0      = glm::float2(0, 1);
            vertices.push_back(_t);
            _t.position = glm::float3(-1, -1, 0) * width;
            _t.normal   = glm::float3(0, 0, 1);
            _t.tangent  = glm::float4(0, 0, 0, 1);
            _t.uv0      = glm::float2(0, 0);
            vertices.push_back(_t);

            indices.push_back(0);
            indices.push_back(1);
            indices.push_back(2);
            indices.push_back(3);
            indices.push_back(4);
            indices.push_back(5);
        }

        BasicMesh SquareMesh::ToBasicMesh(float width)
        {
            SquareMesh _mesh = SquareMesh(width);

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
                        fvTexcOut[0] = vertex.uv0.x;
                        fvTexcOut[1] = vertex.uv0.y;
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

        TerrainPlane::TerrainPlane(int meshGridCount, float meshSize)
        {
            float meshGridSize = meshSize / meshGridCount;

            for (int i = 0; i <= meshGridCount; i++)
            {
                for (int j = 0; j <= meshGridCount; j++)
                {
                    Vertex _t;
                    _t.position = glm::float3(j * meshGridSize, 0, i * meshGridSize);
                    _t.normal = glm::float3(0, 1, 0);
                    _t.tangent = glm::float4(1, 0, 0, 1);
                    _t.uv0 = glm::float2(float(j) / meshGridCount, float(i) / meshGridCount);
                    vertices.push_back(_t);

                    if (i != meshGridCount && j != meshGridCount)
                    {
                        int t00 = i * meshGridCount + j;
                        int t10 = (i + 1) * meshGridCount + j;
                        int t01 = i * meshGridCount + (j + 1);
                        int t11 = (i + 1) * meshGridCount + (j + 1);

                        indices.push_back(t00);
                        indices.push_back(t10);
                        indices.push_back(t11);

                        indices.push_back(t00);
                        indices.push_back(t11);
                        indices.push_back(t01);
                    }
                }
            }
        }

        TerrainPlane TerrainPlane::ToBasicMesh(int gridCount, float meshSize)
        {
            TerrainPlane _mesh = TerrainPlane(gridCount, meshSize);

            {
                SMikkTSpaceInterface iface{};
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
                    fvTexcOut[0] = vertex.uv0.x;
                    fvTexcOut[1] = vertex.uv0.y;
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

                SMikkTSpaceContext context{};
                context.m_pInterface = &iface;
                context.m_pUserData = &_mesh;

                genTangSpaceDefault(&context);
            }

            return _mesh;
        }

    }

    /*
    namespace TerrainGeometry
    {
        D3D12TerrainPatch CreatePatch(glm::float3 position, glm::float2 coord, glm::float4 color)
        {
            D3D12TerrainPatch _t;
            _t.position  = position;
            _t.texcoord0 = coord;
            _t.color     = color;
            _t.normal    = glm::float3(0, 1, 0);
            _t.tangent   = glm::vec4(1, 0, 0, 1);
            return _t;
        }

        TerrainPatchMesh::TerrainPatchMesh(uint32_t totallevel, float scale, uint32_t treeIndex, glm::u64vec2 bitOffset, glm::float2 localOffset)
        {
            this->totalLevel      = totallevel;
            this->localScale      = scale;
            this->localMaxHeight  = 0;
            this->linearTreeIndex = treeIndex;
            this->localTreePosBit = bitOffset;
            this->localOffset     = localOffset;

            vertices.push_back(CreatePatch(glm::float3(0.0, 0, 0.0), glm::vec2(0, 0), glm::vec4(0, 0, 0, 0)));
            vertices.push_back(CreatePatch(glm::float3(0.25, 0, 0.0), glm::vec2(0.25, 0), glm::vec4(1, 0, 0, 0)));
            vertices.push_back(CreatePatch(glm::float3(0.5, 0, 0.0), glm::vec2(0.5, 0), glm::vec4(0, 0, 0, 0)));
            vertices.push_back(CreatePatch(glm::float3(0.75, 0, 0.0), glm::vec2(0.75, 0), glm::vec4(1, 0, 0, 0)));
            vertices.push_back(CreatePatch(glm::float3(1.0, 0, 0.0), glm::vec2(1.0, 0), glm::vec4(0, 0, 0, 0)));

            vertices.push_back(CreatePatch(glm::float3(0.0, 0, 0.25),  glm::vec2(0, 0.25), glm::vec4(0, 0, 1, 0)));
            vertices.push_back(CreatePatch(glm::float3(0.25, 0, 0.25), glm::vec2(0.25, 0.25), glm::vec4(0, 0, 0, 0)));
            vertices.push_back(CreatePatch(glm::float3(0.5, 0, 0.25), glm::vec2(0.5, 0.25), glm::vec4(0, 0, 0, 0)));
            vertices.push_back(CreatePatch(glm::float3(0.75, 0, 0.25), glm::vec2(0.75, 0.25), glm::vec4(0, 0, 0, 0)));
            vertices.push_back(CreatePatch(glm::float3(1.0, 0, 0.25), glm::vec2(1.0, 0.25), glm::vec4(0, 0, 0, 1)));

            vertices.push_back(CreatePatch(glm::float3(0.0, 0, 0.5), glm::vec2(0, 0.5), glm::vec4(0, 0, 0, 0)));
            vertices.push_back(CreatePatch(glm::float3(0.25, 0, 0.5), glm::vec2(0.25, 0.5), glm::vec4(0, 0, 0, 0)));
            vertices.push_back(CreatePatch(glm::float3(0.5, 0, 0.5), glm::vec2(0.5, 0.5), glm::vec4(0, 0, 0, 0)));
            vertices.push_back(CreatePatch(glm::float3(0.75, 0, 0.5), glm::vec2(0.75, 0.5), glm::vec4(0, 0, 0, 0)));
            vertices.push_back(CreatePatch(glm::float3(1.0, 0, 0.5), glm::vec2(1.0, 0.5), glm::vec4(0, 0, 0, 0)));

            vertices.push_back(CreatePatch(glm::float3(0.0, 0, 0.75),  glm::vec2(0, 0.75), glm::vec4(0, 0, 1, 0)));
            vertices.push_back(CreatePatch(glm::float3(0.25, 0, 0.75),  glm::vec2(0.25, 0.75), glm::vec4(0, 0, 0, 0)));
            vertices.push_back(CreatePatch(glm::float3(0.5, 0, 0.75),  glm::vec2(0.5, 0.75), glm::vec4(0, 0, 0, 0)));
            vertices.push_back(CreatePatch(glm::float3(0.75, 0, 0.75),  glm::vec2(0.75, 0.75), glm::vec4(0, 0, 0, 0)));
            vertices.push_back(CreatePatch(glm::float3(1.0, 0, 0.75),  glm::vec2(1.0, 0.75), glm::vec4(0, 0, 0, 1)));

            vertices.push_back(CreatePatch(glm::float3(0.0, 0, 1.0),   glm::vec2(0, 1.0), glm::vec4(0, 0, 0, 0)));
            vertices.push_back(CreatePatch(glm::float3(0.25, 0, 1.0),   glm::vec2(0.25, 1.0), glm::vec4(0, 1, 0, 0)));
            vertices.push_back(CreatePatch(glm::float3(0.5, 0, 1.0),   glm::vec2(0.5, 1.0), glm::vec4(0, 0, 0, 0)));
            vertices.push_back(CreatePatch(glm::float3(0.75, 0, 1.0),   glm::vec2(0.75, 1.0), glm::vec4(0, 1, 0, 0)));
            vertices.push_back(CreatePatch(glm::float3(1.0, 0, 1.0),   glm::vec2(1.0, 1.0), glm::vec4(0, 0, 0, 0)));

            #define AddTriIndices(a, b, c) indices.push_back(a); indices.push_back(b); indices.push_back(c);

            #define AddQuadIndices(d) AddTriIndices(0 + d, 5 + d, 6 + d) AddTriIndices(0 + d, 6 + d, 1 + d)

            #define AddQuadIndices4(d) AddQuadIndices(d) AddQuadIndices(d + 1) AddQuadIndices(d + 2) AddQuadIndices(d + 3)

            #define AddQuadIndices16() AddQuadIndices4(0) AddQuadIndices4(5) AddQuadIndices4(10) AddQuadIndices4(15) AddQuadIndices4(20)

            AddQuadIndices16()
        }

        AABB ToAxisAlignedBox(const TerrainPatchMesh& basicMesh)
        {
            return AABB();
        }

        StaticMeshData ToStaticMesh(const TerrainPatchMesh& basicMesh)
        {
            return StaticMeshData();
        }
    }
    */

}