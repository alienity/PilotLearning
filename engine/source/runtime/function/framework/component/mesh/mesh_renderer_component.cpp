#include "runtime/function/framework/component/mesh/mesh_renderer_component.h"

#include "runtime/resource/asset_manager/asset_manager.h"
#include "runtime/function/framework/world/world_manager.h"
#include "runtime/function/framework/material/material_manager.h"
#include "runtime/resource/res_type/components/material.h"

#include "runtime/function/framework/component/transform/transform_component.h"
#include "runtime/function/framework/object/object.h"
#include "runtime/function/global/global_context.h"

#include "runtime/function/render/render_swap_context.h"
#include "runtime/function/render/render_system.h"
#include "runtime/function/render/glm_wrapper.h"
#include "runtime/function/render/render_common.h"

namespace MoYu
{
    MeshComponentRes _capsule_mesh    = {false, "asset/objects/basic/capsule.obj", ""};
    MeshComponentRes _cone_mesh       = {false, "asset/objects/basic/cone.obj", ""};
    MeshComponentRes _convexmesh_mesh = {false, "asset/objects/basic/convexmesh.obj", ""};
    MeshComponentRes _cube_mesh       = {false, "asset/objects/basic/cube.obj", ""};
    MeshComponentRes _cylinder_mesh   = {false, "asset/objects/basic/cylinder.obj", ""};
    MeshComponentRes _sphere_mesh     = {false, "asset/objects/basic/sphere.obj", ""};

    MaterialComponentRes _pbr_mat = {"asset/objects/environment/_material/temp.material.json", false, {}};

    MeshRendererComponentRes _capsule_mesh_mat    = {_capsule_mesh, _pbr_mat};
    MeshRendererComponentRes _cone_mesh_mat       = {_cone_mesh, _pbr_mat};
    MeshRendererComponentRes _convexmesh_mesh_mat = {_convexmesh_mesh, _pbr_mat};
    MeshRendererComponentRes _cube_mesh_mat       = {_cube_mesh, _pbr_mat};
    MeshRendererComponentRes _cylinder_mesh_mat   = {_cylinder_mesh, _pbr_mat};
    MeshRendererComponentRes _sphere_mesh_mat     = {_sphere_mesh, _pbr_mat};

    std::string DefaultMeshTypeToName(DefaultMeshType type)
    {
        if (type == MoYu::Capsule)
            return "Capsule";
        else if (type == MoYu::Cone)
            return "Cone";
        else if (type == MoYu::Convexmesh)
            return "Convexmesh";
        else if (type == MoYu::Cube)
            return "Cube";
        else if (type == MoYu::Cylinder)
            return "Cylinder";
        else if (type == MoYu::Sphere)
            return "Sphere";
        else
            return "Capsule";
    }

    MeshRendererComponentRes DefaultMeshTypeToComponentRes(DefaultMeshType type)
    {
        if (type == MoYu::Capsule)
            return _capsule_mesh_mat;
        else if (type == MoYu::Cone)
            return _cone_mesh_mat;
        else if (type == MoYu::Convexmesh)
            return _convexmesh_mesh_mat;
        else if (type == MoYu::Cube)
            return _cube_mesh_mat;
        else if (type == MoYu::Cylinder)
            return _cylinder_mesh_mat;
        else if (type == MoYu::Sphere)
            return _sphere_mesh_mat;
        else
            return _capsule_mesh_mat;
    }

    void MeshRendererComponent::postLoadResource(std::weak_ptr<GObject> object, const std::string json_data)
    {
        m_object = object;

        MeshRendererComponentRes mesh_renderer_res = AssetManager::loadJson<MeshRendererComponentRes>(json_data);
        updateMeshRendererRes(mesh_renderer_res);

        markInit();
    }

    void MeshRendererComponent::save(ComponentDefinitionRes& out_component_res)
    {
        MeshComponentRes mesh_res {};
        (&mesh_res)->m_is_mesh_data   = m_scene_mesh.m_is_mesh_data;
        (&mesh_res)->m_mesh_data_path = m_scene_mesh.m_mesh_data_path;
        (&mesh_res)->m_sub_mesh_file  = m_scene_mesh.m_sub_mesh_file;

        MaterialComponentRes material_res {};
        (&material_res)->m_material_file    = m_mesh_renderer_res.m_material_res.m_material_file;
        (&material_res)->m_is_material_init = true;

        MaterialRes mat_res_data = ToMaterialRes(m_material.m_mat_data, m_material.m_shader_name);
        (&material_res)->m_material_serialized_json_data = AssetManager::saveJson(mat_res_data);

        MeshRendererComponentRes mesh_renderer_res {};
        (&mesh_renderer_res)->m_mesh_res     = mesh_res;
        (&mesh_renderer_res)->m_material_res = material_res;

        out_component_res.m_type_name           = "MeshRendererComponent";
        out_component_res.m_component_name      = this->m_component_name;
        out_component_res.m_component_json_data = AssetManager::saveJson(mesh_renderer_res);
    }

    void MeshRendererComponent::reset()
    {
        m_mesh_renderer_res = {};

        m_scene_mesh = {};
        m_material = {};
    }

    GameObjectComponentDesc component2SwapData(MoYu::GObjectID     game_object_id,
                                               MoYu::GComponentID  transform_component_id,
                                               TransformComponent* m_transform_component_ptr,
                                               MoYu::GComponentID  mesh_renderer_component_id,
                                               SceneMesh*          m_scene_mesh_ptr,
                                               SceneMaterial*      m_scene_mat_ptr)
    {
        Matrix4x4 transform_matrix = m_transform_component_ptr->getMatrixWorld();

        std::tuple<Quaternion, Vector3, Vector3> rts = GLMUtil::decomposeMat4x4(transform_matrix);

        Quaternion m_rotation = std::get<0>(rts);
        Vector3    m_position = std::get<1>(rts);
        Vector3    m_scale    = std::get<2>(rts);

        SceneTransform scene_transform     = {};
        scene_transform.m_identifier       = SceneCommonIdentifier {game_object_id, transform_component_id};
        scene_transform.m_position         = m_position;
        scene_transform.m_rotation         = m_rotation;
        scene_transform.m_scale            = m_scale;
        scene_transform.m_transform_matrix = transform_matrix;

        SceneMeshRenderer scene_mesh_renderer = {};
        scene_mesh_renderer.m_identifier      = SceneCommonIdentifier {game_object_id, mesh_renderer_component_id};
        scene_mesh_renderer.m_scene_mesh      = *m_scene_mesh_ptr;
        scene_mesh_renderer.m_material        = *m_scene_mat_ptr;

        GameObjectComponentDesc light_component_desc = {};
        light_component_desc.m_component_type        = ComponentType::C_Transform | ComponentType::C_MeshRenderer;
        light_component_desc.m_transform_desc        = scene_transform;
        light_component_desc.m_mesh_renderer_desc    = scene_mesh_renderer;

        return light_component_desc;
    }

    void MeshRendererComponent::tick(float delta_time)
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
            GameObjectComponentDesc mesh_renderer_desc = component2SwapData(game_object_id,
                                                                            transform_component_id,
                                                                            m_transform_component_ptr,
                                                                            mesh_renderer_component_id,
                                                                            &m_scene_mesh,
                                                                            &m_material);

            logic_swap_data.addDeleteGameObject({game_object_id, {mesh_renderer_desc}});

            this->markNone();
        }
        else if (m_transform_component_ptr->isMatrixDirty() || this->isDirty())
        {
            GameObjectComponentDesc mesh_renderer_desc = component2SwapData(game_object_id,
                                                                            transform_component_id,
                                                                            m_transform_component_ptr,
                                                                            mesh_renderer_component_id,
                                                                            &m_scene_mesh,
                                                                            &m_material);

            logic_swap_data.addDirtyGameObject({game_object_id, {mesh_renderer_desc}});

            this->markIdle();
        }
    }

    void MeshRendererComponent::updateMeshRendererRes(const MeshRendererComponentRes& res)
    {
        m_mesh_renderer_res.m_mesh_res     = res.m_mesh_res;
        m_mesh_renderer_res.m_material_res = res.m_material_res;

        m_scene_mesh = {m_mesh_renderer_res.m_mesh_res.m_is_mesh_data,
                        m_mesh_renderer_res.m_mesh_res.m_sub_mesh_file,
                        m_mesh_renderer_res.m_mesh_res.m_mesh_data_path};

        MaterialManager* m_mat_manager_ptr = g_runtime_global_context.m_material_manager.get();

        MaterialRes m_mat_res = m_mat_manager_ptr->loadMaterialRes(m_mesh_renderer_res.m_material_res.m_material_file);

        if (m_mesh_renderer_res.m_material_res.m_is_material_init)
        {
            std::string m_material_serialized_json = m_mesh_renderer_res.m_material_res.m_material_serialized_json_data;
            MaterialRes mat_res_data = AssetManager::loadJson<MaterialRes>(m_material_serialized_json);
            m_mat_res = mat_res_data;
        }

        ScenePBRMaterial m_mat_data = ToPBRMaterial(m_mat_res);

        m_material.m_shader_name = m_mat_res.shader_name;
        m_material.m_mat_data    = m_mat_data;

        markDirty();
    }

    void MeshRendererComponent::updateMeshRes(std::string mesh_file_path)
    {
        MeshComponentRes m_mesh_res = {false, mesh_file_path, ""};
        m_mesh_renderer_res.m_mesh_res = m_mesh_res;

        m_scene_mesh.m_is_mesh_data   = m_mesh_renderer_res.m_mesh_res.m_is_mesh_data;
        m_scene_mesh.m_sub_mesh_file  = m_mesh_renderer_res.m_mesh_res.m_sub_mesh_file;
        m_scene_mesh.m_mesh_data_path = m_mesh_renderer_res.m_mesh_res.m_mesh_data_path;

        markDirty();
    }

    void MeshRendererComponent::updateMaterial(std::string material_path, std::string serialized_json_str)
    {
        MaterialComponentRes m_material_res = {
            material_path, serialized_json_str.empty() ? true : false, serialized_json_str};
        m_mesh_renderer_res.m_material_res = m_material_res;

        MaterialManager* m_mat_manager_ptr = g_runtime_global_context.m_material_manager.get();
        MaterialRes m_mat_res = m_mat_manager_ptr->loadMaterialRes(m_mesh_renderer_res.m_material_res.m_material_file);

        ScenePBRMaterial m_mat_data = ToPBRMaterial(m_mat_res);

        m_material.m_shader_name = m_mat_res.shader_name;
        m_material.m_mat_data    = m_mat_data;

        markDirty();
    }

} // namespace MoYu
