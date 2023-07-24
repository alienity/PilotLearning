#pragma once

#include "runtime/function/framework/component/component.h"
#include "runtime/function/render/render_common.h"
#include "runtime/resource/res_type/components/material.h"
#include "runtime/resource/res_type/components/mesh_renderer.h"

#include <vector>
#include <map>

namespace MoYu
{
    enum DefaultMeshType
    {
        Capsule,
        Cone,
        Convexmesh,
        Cube,
        Cylinder,
        Sphere
    };

    std::string              DefaultMeshTypeToName(DefaultMeshType type);
    MeshRendererComponentRes DefaultMeshTypeToComponentRes(DefaultMeshType type);

    class MeshRendererComponent : public Component
    {
    public:
        MeshRendererComponent() { m_component_name = "MeshRendererComponent"; };

        void reset();

        void postLoadResource(std::weak_ptr<GObject> object, const std::string json_data) override;

        void save(ComponentDefinitionRes& out_component_res) override;

        void tick(float delta_time) override;

        void updateMeshRendererRes(const MeshRendererComponentRes& res);
        void updateMeshRes(std::string mesh_file_path);
        void updateMaterial(std::string material_path, std::string serialized_json_str = "");

        // for editor
        SceneMesh& getSceneMesh() { return m_scene_mesh; }
        SceneMaterial& getSceneMaterial() { return m_material; }

    private:
        MeshRendererComponentRes m_mesh_renderer_res;

        SceneMesh m_scene_mesh;
        SceneMaterial m_material;
    };
} // namespace MoYu
