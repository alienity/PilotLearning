#pragma once
#ifndef GEOMETRY_CUBE_H
#define GEOMETRY_CUBE_H
#include "runtime/resource/basic_geometry/mesh_tools.h"

namespace MoYu
{
    namespace Geometry
    {
		struct CubeMesh : BasicMesh
		{
            CubeMesh();

            static BasicMesh ToBasicMesh();
		};

    } // namespace Geometry
}
#endif // !SPHEREMESH
