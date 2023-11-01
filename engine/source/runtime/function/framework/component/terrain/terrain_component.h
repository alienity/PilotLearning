#pragma once

#include "runtime/function/framework/component/component.h"
#include "runtime/function/render/render_common.h"
#include "runtime/resource/res_type/components/material.h"
#include "runtime/resource/res_type/components/terrain.h"

#include <vector>
#include <map>

namespace MoYu
{
    class TerrainComponent : public Component
    {
    public:
        TerrainComponent() { m_component_name = "TerrainComponent"; };

        void reset();

        void postLoadResource(std::weak_ptr<GObject> object, const std::string json_data) override;

        void save(ComponentDefinitionRes& out_component_res) override;

        void tick(float delta_time) override;

        void updateTerrainRes(const TerrainComponentRes& res);

        // for editor
        SceneMaterial& getSceneMaterial() { return m_material; }

    //private:
        TerrainComponentRes m_terrain_res;

        SceneTerrainMesh m_terrain;
        SceneMaterial m_material;
    };
} // namespace MoYu
