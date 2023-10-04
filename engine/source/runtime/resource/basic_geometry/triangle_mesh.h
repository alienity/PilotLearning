#pragma once
#ifndef GEOMETRY_TRIANGLE_H
#define GEOMETRY_TRIANGLE_H
#include "runtime/resource/basic_geometry/mesh_tools.h"

#include <vector>
#include <map>

namespace MoYu
{
    namespace Geometry
    {
		struct TriangleMesh : BasicMesh
		{
            TriangleMesh();

            static BasicMesh ToBasicMesh();
		};

    } // namespace Geometry
}
#endif // !SPHEREMESH
