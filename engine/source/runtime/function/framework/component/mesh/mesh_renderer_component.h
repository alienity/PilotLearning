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

        //const std::vector<GameObjectComponentDesc>& getRawMeshes() const { return m_raw_meshes; }

        void tick(float delta_time) override;

        bool addNewMeshRes(std::string mesh_file_path);

        void updateMaterial(std::string mesh_file_path, std::string material_path);

    private:
        MeshComponentRes m_mesh_component_res;
        MaterialComponentRes m_material_component_res;

        //std::vector<GameObjectComponentDesc> m_raw_meshes;
    };
} // namespace MoYu
