#pragma once
#include "runtime/function/render/render_common.h"
#include "runtime/core/math/moyu_math2.h"

MoYu::StaticMeshData LoadModel(std::string filename, MoYu::AxisAlignedBox& bounding_box);
