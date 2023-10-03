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
    }
}