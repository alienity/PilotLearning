#pragma once
#include "runtime/function/render/render_type.h"


struct ModelLoaderMesh
{
    std::vector<Pilot::MeshVertexDataDefinition> vertexs_;
    std::vector<std::uint32_t>                   indices_;
};

struct MeshLoaderNode
{
    std::vector<ModelLoaderMesh> meshes_;
    std::vector<MeshLoaderNode>  children_;
};

void LoadModel(std::string filename, MeshLoaderNode& mesh_);
