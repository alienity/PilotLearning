#include "runtime/function/framework/component/volume/volume_renderer_component.h"

#include "runtime/resource/asset_manager/asset_manager.h"
#include "runtime/function/framework/world/world_manager.h"
#include "runtime/function/framework/material/material_manager.h"
#include "runtime/resource/res_type/components/material.h"

#include "runtime/function/framework/component/transform/transform_component.h"
#include "runtime/function/framework/object/object.h"
#include "runtime/function/global/global_context.h"

#include "runtime/core/math/moyu_math2.h"

#include "runtime/function/render/render_swap_context.h"
#include "runtime/function/render/render_system.h"
#include "runtime/function/render/render_common.h"

namespace MoYu
{
    void LocalVolumetricFogComponent::postLoadResource(std::weak_ptr<GObject> object, const std::string json_data)
    {
        m_object = object;

        LocalVolumeFogComponentRes local_fog_res = AssetManager::loadJson<LocalVolumeFogComponentRes>(json_data);
        updateLocalFogRendererRes(local_fog_res);

        markInit();
    }

    void LocalVolumetricFogComponent::save(ComponentDefinitionRes& out_component_res)
    {
        LocalVolumeFogComponentRes local_fog_res {};
        (&local_fog_res)->SingleScatteringAlbedo = m_scene_local_fog_res.SingleScatteringAlbedo;
        (&local_fog_res)->FogDistance = m_scene_local_fog_res.FogDistance;
        (&local_fog_res)->Size = m_scene_local_fog_res.Size;
        (&local_fog_res)->PerAxisControl = m_scene_local_fog_res.PerAxisControl;
        (&local_fog_res)->BlendDistanceNear = m_scene_local_fog_res.BlendDistanceNear;
        (&local_fog_res)->BlendDistanceFar = m_scene_local_fog_res.BlendDistanceFar;
        (&local_fog_res)->FalloffMode = m_scene_local_fog_res.FalloffMode;
        (&local_fog_res)->InvertBlend = m_scene_local_fog_res.InvertBlend;
        (&local_fog_res)->DistanceFadeStart = m_scene_local_fog_res.DistanceFadeStart;
        (&local_fog_res)->DistanceFadeEnd = m_scene_local_fog_res.DistanceFadeEnd;
        (&local_fog_res)->NoiseImage = m_scene_local_fog_res.NoiseImage;
        (&local_fog_res)->ScrollSpeed = m_scene_local_fog_res.ScrollSpeed;
        (&local_fog_res)->Tilling = m_scene_local_fog_res.Tilling;

        out_component_res.m_type_name           = "LocalVolumetricFogComponent";
        out_component_res.m_component_name      = this->m_component_name;
        out_component_res.m_component_json_data = AssetManager::saveJson(local_fog_res);
    }

    void LocalVolumetricFogComponent::reset()
    {
        m_scene_local_fog_res = {};
    }

    GameObjectComponentDesc component2SwapData(MoYu::GObjectID     game_object_id,
                                               MoYu::GComponentID  transform_component_id,
                                               TransformComponent* m_transform_component_ptr,
                                               MoYu::GComponentID  mesh_renderer_component_id,
                                               LocalVolumeFogComponentRes* m_scene_fog_ptr)
    {
        glm::float4x4 transform_matrix = m_transform_component_ptr->getMatrixWorld();

        glm::float3     m_scale;
        glm::quat m_orientation;
        glm::float3     m_translation;
        glm::float3     m_skew;
        glm::float4     m_perspective;
        glm::decompose(transform_matrix, m_scale, m_orientation, m_translation, m_skew, m_perspective);

        SceneTransform scene_transform     = {};
        scene_transform.m_identifier       = SceneCommonIdentifier {game_object_id, transform_component_id};
        scene_transform.m_position         = m_translation;
        scene_transform.m_rotation         = m_orientation;
        scene_transform.m_scale            = m_scale;
        scene_transform.m_transform_matrix = transform_matrix;

        SceneVolumeFogRenderer scene_fog_renderer = {};
        scene_fog_renderer.m_identifier = SceneCommonIdentifier {game_object_id, mesh_renderer_component_id};

        scene_fog_renderer.m_scene_volumetric_fog.m_SingleScatteringAlbedo = m_scene_fog_ptr->SingleScatteringAlbedo;
        scene_fog_renderer.m_scene_volumetric_fog.m_FogDistance = m_scene_fog_ptr->FogDistance;
        scene_fog_renderer.m_scene_volumetric_fog.m_Size = m_scene_fog_ptr->Size;
        scene_fog_renderer.m_scene_volumetric_fog.m_PerAxisControl = m_scene_fog_ptr->PerAxisControl;
        scene_fog_renderer.m_scene_volumetric_fog.m_BlendDistanceNear = m_scene_fog_ptr->BlendDistanceNear;
        scene_fog_renderer.m_scene_volumetric_fog.m_BlendDistanceFar = m_scene_fog_ptr->BlendDistanceFar;
        scene_fog_renderer.m_scene_volumetric_fog.m_FalloffMode = m_scene_fog_ptr->FalloffMode;
        scene_fog_renderer.m_scene_volumetric_fog.m_InvertBlend = m_scene_fog_ptr->InvertBlend;
        scene_fog_renderer.m_scene_volumetric_fog.m_DistanceFadeStart = m_scene_fog_ptr->DistanceFadeStart;
        scene_fog_renderer.m_scene_volumetric_fog.m_DistanceFadeEnd = m_scene_fog_ptr->DistanceFadeEnd;
        scene_fog_renderer.m_scene_volumetric_fog.m_ScrollSpeed = m_scene_fog_ptr->ScrollSpeed;
        scene_fog_renderer.m_scene_volumetric_fog.m_Tilling = m_scene_fog_ptr->Tilling;
        scene_fog_renderer.m_scene_volumetric_fog.m_NoiseImage = m_scene_fog_ptr->NoiseImage;
        
        GameObjectComponentDesc light_component_desc = {};
        light_component_desc.m_component_type        = ComponentType::C_Transform | ComponentType::C_Volume;
        light_component_desc.m_transform_desc        = scene_transform;
        light_component_desc.m_volume_render_desc    = scene_fog_renderer;

        return light_component_desc;
    }

    void LocalVolumetricFogComponent::tick(float delta_time)
    {
        if (m_object.expired() || this->isNone())
            return;

        std::shared_ptr<MoYu::GObject> m_obj_ptr = m_object.lock();

        RenderSwapContext& render_swap_context = g_runtime_global_context.m_render_system->getSwapContext();
        RenderSwapData& logic_swap_data = render_swap_context.getLogicSwapData();

        TransformComponent* m_transform_component_ptr = m_obj_ptr->getTransformComponent().lock().get();

        MoYu::GObjectID game_object_id = m_obj_ptr->getID();
        MoYu::GComponentID transform_component_id = m_transform_component_ptr->getComponentId();
        MoYu::GComponentID mesh_renderer_component_id = this->getComponentId();

        if (this->isToErase())
        {
            GameObjectComponentDesc fog_renderer_desc = component2SwapData(game_object_id,
                                                                            transform_component_id,
                                                                            m_transform_component_ptr,
                                                                            mesh_renderer_component_id,
                                                                            &m_scene_local_fog_res);

            logic_swap_data.addDeleteGameObject({game_object_id, {fog_renderer_desc}});

            this->markNone();
        }
        else if (m_transform_component_ptr->isMatrixDirty() || this->isDirty())
        {
            GameObjectComponentDesc fog_renderer_desc = component2SwapData(game_object_id,
                                                                            transform_component_id,
                                                                            m_transform_component_ptr,
                                                                            mesh_renderer_component_id,
                                                                            &m_scene_local_fog_res);

            logic_swap_data.addDirtyGameObject({game_object_id, {fog_renderer_desc}});

            this->markIdle();
        }
    }

    void LocalVolumetricFogComponent::updateLocalFogRendererRes(const LocalVolumeFogComponentRes& res)
    {
        m_scene_local_fog_res = res;
        markDirty();
    }

} // namespace MoYu
