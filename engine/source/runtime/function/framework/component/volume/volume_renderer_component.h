#pragma once

#include "runtime/function/framework/component/component.h"
#include "runtime/function/render/render_common.h"
#include "runtime/resource/res_type/components/volume_renderer.h"

namespace MoYu
{
    class LocalVolumetricFogComponent : public Component
    {
    public:
        LocalVolumetricFogComponent() { m_component_name = "LocalVolumetricFogComponent"; };

        void reset();

        void postLoadResource(std::weak_ptr<GObject> object, const std::string json_data) override;

        void save(ComponentDefinitionRes& out_component_res) override;

        void tick(float delta_time) override;

        void updateLocalFogRendererRes(const LocalVolumeFogComponentRes& res);

        // for editor
        LocalVolumeFogComponentRes& getSceneLocalFog() { return m_scene_local_fog_res; }

    private:
        LocalVolumeFogComponentRes m_scene_local_fog_res;
        
    };
} // namespace MoYu
