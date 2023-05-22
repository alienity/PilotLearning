#pragma once
#include "runtime/function/render/render_common.h"
#include "runtime/core/math/moyu_math.h"

MoYu::StaticMeshData LoadModel(std::string filename, MoYu::AxisAlignedBox& bounding_box);
