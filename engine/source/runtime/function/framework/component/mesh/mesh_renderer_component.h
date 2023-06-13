#pragma once

#include "runtime/function/framework/component/component.h"
#include "runtime/function/render/render_common.h"
#include "runtime/resource/res_type/components/material.h"
#include "runtime/resource/res_type/components/mesh_renderer.h"

#include <vector>
#include <map>

namespace MoYu
{
    class MeshRendererComponent : public Component
    {
    public:
        MeshRendererComponent() {};

        void reset();

        void postLoadResource(std::weak_ptr<GObject> object, void* data) override;

        void tick(float delta_time) override;

        void updateMeshRes(std::string mesh_file_path);

        void updateMaterial(std::string material_path);

    private:
        MeshRendererComponentRes m_mesh_renderer_res;

        SceneMesh m_scene_mesh;
        SceneMaterial m_material;
    };
} // namespace MoYu
