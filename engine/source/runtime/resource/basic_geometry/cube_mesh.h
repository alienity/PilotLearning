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
            CubeMesh(float width);

            static BasicMesh ToBasicMesh(float width = 0.5f);
		};

    } // namespace Geometry
}
#endif // !SPHEREMESH
