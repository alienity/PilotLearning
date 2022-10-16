#pragma once

#include "runtime/function/framework/component/component.h"

#include "runtime/resource/res_type/components/mesh.h"
#include "runtime/resource/res_type/components/material.h"

#include "runtime/function/render/render_object.h"

#include <vector>

namespace Pilot
{
    class RenderSwapContext;

    REFLECTION_TYPE(MeshComponent)
    CLASS(MeshComponent : public Component, WhiteListFields)
    {
        REFLECTION_BODY(MeshComponent)
    public:
        MeshComponent() {};

        void reset();

        void postLoadResource(std::weak_ptr<GObject> parent_object) override;

        const std::vector<GameObjectComponentDesc>& getRawMeshes() const { return m_raw_meshes; }

        void tick(float delta_time) override;

        bool addNewMeshRes(std::string mesh_file_path);

        MaterialRes& getMaterialRes(std::string material_path);
        void createMaterial(std::string mesh_file_path, std::string material_path);
        void updateMaterial(std::string mesh_file_path, std::string material_path);

    private:
        META(Enable)
        MeshComponentRes m_mesh_res;

        std::unordered_map<std::string, MaterialRes> m_material_res_map;
        std::unordered_map<std::string, bool> m_material_dirty_map;

        std::vector<GameObjectComponentDesc> m_raw_meshes;
    };
} // namespace Pilot
