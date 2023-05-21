#pragma once
#include "runtime/function/render/render_common.h"
#include "runtime/core/math/moyu_math.h"

Pilot::StaticMeshData LoadModel(std::string filename, Pilot::AxisAlignedBox& bounding_box);
