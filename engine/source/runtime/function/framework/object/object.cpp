#include "runtime/function/framework/object/object.h"
#include "runtime/function/framework/level/level.h"

#include "runtime/engine.h"
#include "runtime/function/global/global_context.h"
#include "runtime/resource/asset_manager/asset_manager.h"

#include "runtime/function/framework/component/component.h"
#include "runtime/function/framework/component/transform/transform_component.h"
#include "runtime/function/framework/component/light/light_component.h"
#include "runtime/function/framework/component/mesh/mesh_renderer_component.h"
#include "runtime/function/framework/component/camera/camera_component.h"
#include "runtime/function/framework/component/terrain/terrain_component.h"
#include "runtime/function/framework/component/volume/volume_renderer_component.h"

namespace MoYu
{
    bool shouldComponentTick(std::string component_type_name)
    {
        if (g_is_editor_mode)
        {
            return g_editor_tick_component_types.find(component_type_name) != g_editor_tick_component_types.end();
        }
        else
        {
            return true;
        }
    }

    GObject::~GObject()
    {
        m_components.clear();
    }

    void GObject::preTick(float delta_time)
    {
        auto _transform_weak_ptr = this->getTransformComponent();
        ASSERT(!_transform_weak_ptr.expired());
        _transform_weak_ptr.lock()->preTick(delta_time);
    }

    void GObject::tick(float delta_time)
    {
        auto _transform_weak_ptr = this->getTransformComponent();
        ASSERT(!_transform_weak_ptr.expired());
        auto _transform_ptr = _transform_weak_ptr.lock();
        
        for (auto& component : m_components)
        {
            if (_transform_ptr->getComponentId() == component->getComponentId())
                continue;
            if (shouldComponentTick(component->getTypeName()))
            {
                component->tick(delta_time);
            }
        }
    }

    void GObject::lateTick(float delta_time)
    {
        auto _transform_weak_ptr = this->getTransformComponent();
        ASSERT(!_transform_weak_ptr.expired());
        _transform_weak_ptr.lock()->lateTick(delta_time);

        auto it = m_components.begin();
        while (it != m_components.end())
        {
            if ((*it)->isToErase())
            {
                it = m_components.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    void GObject::setParent(GObjectID parentID, std::optional<std::uint32_t> sibling_index)
    {
        if (m_current_level != nullptr)
        {
            m_current_level->changeParent(m_id, parentID, sibling_index);
        }
    }

    void GObject::removeChild(GObjectID childID)
    {
        if (m_current_level != nullptr)
        {
            m_current_level->deleteGObject(childID);
        }
    }

    void GObject::markToErase()
    {
         m_status = GObjectStatus::GO_Erase;

        for (auto& component : m_components)
        {
            if (component->getTypeName() != "TransformComponent")
            {
                component->markToErase();
            }
        }
    }

    std::weak_ptr<TransformComponent> GObject::getTransformComponent()
    {
        if (m_transform_component_ptr.expired())
        {
            m_transform_component_ptr = tryGetComponent(TransformComponent);
        }
        return m_transform_component_ptr;
    }

    bool GObject::hasComponent(const std::string& compenent_type_name) const
    {
        for (const auto& component : m_components)
        {
            if (component->getTypeName() == compenent_type_name)
                return true;
        }

        return false;
    }
    
    bool GObject::load(const ObjectInstanceRes& object_instance_res)
    {
        // clear old components
        m_components.clear();

        m_name          = object_instance_res.m_name;
        m_id            = object_instance_res.m_id;
        m_parent_id     = object_instance_res.m_parent_id;
        m_sibling_index = object_instance_res.m_sibling_index;
        m_chilren_id.insert(m_chilren_id.end(), object_instance_res.m_chilren_id.begin(), object_instance_res.m_chilren_id.end());

        // load object instanced components
        for (int i = 0; i < object_instance_res.m_instanced_components.size(); i++)
        {
            MoYu::ComponentDefinitionRes component_define_res = object_instance_res.m_instanced_components[i];

            std::string type_name = component_define_res.m_type_name;
            const std::string component_json_data = component_define_res.m_component_json_data;

            if (type_name == "TransformComponent")
            {
                std::shared_ptr<TransformComponent> m_component = std::make_shared<TransformComponent>();
                m_component->postLoadResource(weak_from_this(), component_json_data);
                m_components.push_back(m_component);
            }
            else if (type_name == "MeshRendererComponent")
            {
                std::shared_ptr<MeshRendererComponent> m_component = std::make_shared<MeshRendererComponent>();
                m_component->postLoadResource(weak_from_this(), component_json_data);
                m_components.push_back(m_component);
            }
            else if (type_name == "LocalVolumetricFogComponent")
            {
                std::shared_ptr<LocalVolumetricFogComponent> m_component = std::make_shared<LocalVolumetricFogComponent>();
                m_component->postLoadResource(weak_from_this(), component_json_data);
                m_components.push_back(m_component);
            }
            else if (type_name == "TerrainComponent")
            {
                std::shared_ptr<TerrainComponent> m_component = std::make_shared<TerrainComponent>();
                m_component->postLoadResource(weak_from_this(), component_json_data);
                m_components.push_back(m_component);
            }
            else if (type_name == "LightComponent")
            {
                std::shared_ptr<LightComponent> m_component = std::make_shared<LightComponent>();
                m_component->postLoadResource(weak_from_this(), component_json_data);
                m_components.push_back(m_component);
            }
            else if (type_name == "CameraComponent")
            {
                std::shared_ptr<CameraComponent> m_component = std::make_shared<CameraComponent>();
                m_component->postLoadResource(weak_from_this(), component_json_data);
                m_components.push_back(m_component);
            }
        }
        return true;
    }

    void GObject::save(ObjectInstanceRes& out_object_instance_res)
    {
        out_object_instance_res.m_name       = m_name;
        
        out_object_instance_res.m_id            = m_id;
        out_object_instance_res.m_parent_id     = m_parent_id;
        out_object_instance_res.m_sibling_index = m_sibling_index;

        out_object_instance_res.m_chilren_id.insert(out_object_instance_res.m_chilren_id.end(), m_chilren_id.begin(), m_chilren_id.end());

        // TODO: ���ӱ���Level
        //out_object_instance_res.m_instanced_components = m_components;
        for (size_t i = 0; i < m_components.size(); i++)
        {
            ComponentDefinitionRes m_comp_res = {};

            m_components[i]->save(m_comp_res);

            out_object_instance_res.m_instanced_components.push_back(m_comp_res);
        }
    }

    std::shared_ptr<GObject> GObject::getParent() const
    {
        if (m_current_level != nullptr)
        {
            if (isRootNode())
            {
                return nullptr;
            }

            std::shared_ptr<GObject> pParent = m_current_level->getGObjectByID(m_parent_id);
            return pParent;
        }
        return nullptr;
    }

    std::vector<std::shared_ptr<GObject>> GObject::getChildren() const
    {
        std::vector<std::shared_ptr<GObject>> m_children;

        if (m_current_level != nullptr)
        {
            for (size_t i = 0; i < m_chilren_id.size(); i++)
            {
                GObjectID m_childID = m_chilren_id[i];
                std::shared_ptr<GObject> m_child_obj = m_current_level->getGObjectByID(m_childID);
                if (m_child_obj != nullptr)
                {
                    m_children.push_back(m_child_obj);
                }
            }
        }
        return m_children;
    }

} // namespace MoYu