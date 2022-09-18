#include "runtime/function/framework/object/object.h"
#include "runtime/function/framework/level/level.h"

#include "runtime/engine.h"

#include "runtime/core/meta/reflection/reflection.h"

#include "runtime/resource/asset_manager/asset_manager.h"

#include "runtime/function/framework/component/component.h"
#include "runtime/function/framework/component/transform/transform_component.h"
#include "runtime/function/global/global_context.h"

#include <cassert>
#include <unordered_set>

#include "_generated/serializer/all_serializer.h"

namespace Pilot
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
        for (auto& component : m_components)
        {
            PILOT_REFLECTION_DELETE(component);
        }
        m_components.clear();
    }

    void GObject::tick(float delta_time)
    {
        for (auto& component : m_components)
        {
            if (shouldComponentTick(component.getTypeName()))
            {
                component->tick(delta_time);
            }
        }
    }

    bool GObject::hasComponent(const std::string& compenent_type_name) const
    {
        for (const auto& component : m_components)
        {
            if (component.getTypeName() == compenent_type_name)
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
        m_chilren_ids.insert(
            m_chilren_ids.end(), object_instance_res.m_chilren_id.begin(), object_instance_res.m_chilren_id.end());


        // load object instanced components
        m_components = object_instance_res.m_instanced_components;
        for (auto component : m_components)
        {
            if (component)
            {
                component->postLoadResource(weak_from_this());
            }
        }

        // load object definition components
        m_definition_url = object_instance_res.m_definition;

        ObjectDefinitionRes definition_res;

        const bool is_loaded_success = g_runtime_global_context.m_asset_manager->loadAsset(m_definition_url, definition_res);
        if (!is_loaded_success)
            return false;

        for (auto loaded_component : definition_res.m_components)
        {
            if (loaded_component.getPtr() == NULL)
                continue;

            const std::string type_name = loaded_component.getTypeName();
            // don't create component if it has been instanced
            if (hasComponent(type_name))
                continue;

            loaded_component->postLoadResource(weak_from_this());

            m_components.push_back(loaded_component);
        }

        return true;
    }

    void GObject::save(ObjectInstanceRes& out_object_instance_res)
    {
        out_object_instance_res.m_name       = m_name;
        out_object_instance_res.m_definition = m_definition_url;
        
        out_object_instance_res.m_id = m_id;
        out_object_instance_res.m_parent_id = m_parent_id;
        out_object_instance_res.m_sibling_index = m_sibling_index;

        out_object_instance_res.m_chilren_id.insert(
            out_object_instance_res.m_chilren_id.end(), m_chilren_ids.begin(), m_chilren_ids.end());

        out_object_instance_res.m_instanced_components = m_components;
    }

    void GObject::setParent(std::weak_ptr<GObject> pParent, std::optional<std::uint32_t> sibling_index)
    {
        if (!pParent.expired())
        {
            std::shared_ptr<GObject> spParent = pParent.lock();

            GObjectID parentID = spParent->m_id;
            m_parent_id = parentID;

            std::vector<GObjectID>& childrenIDs = spParent->m_chilren_ids;

            size_t children_size = childrenIDs.size();
            std::uint32_t new_sibling_index = sibling_index.value_or(children_size);
            assert(new_sibling_index >= 0 && new_sibling_index <= children_size + 1);

            m_sibling_index = new_sibling_index;
            childrenIDs.insert(childrenIDs.begin() + new_sibling_index, m_id);
        }
    }

    std::weak_ptr<GObject> GObject::getParent() const
    {
        if (m_current_level != nullptr)
        {
            if (m_parent_id != k_invalid_gobject_id)
            {
                std::weak_ptr<GObject> pParent = m_current_level->getGObjectByID(m_parent_id);
                return pParent;
            }
        }
        return std::weak_ptr<GObject>();
    }

    std::vector<std::weak_ptr<GObject>> GObject::getChildren() const
    {
        std::vector<std::weak_ptr<GObject>> m_children;

        if (m_current_level != nullptr)
        {
            for (size_t i = 0; i < m_chilren_ids.size(); i++)
            {
                GObjectID m_childID = m_chilren_ids[i];
                std::weak_ptr<GObject> m_child_obj = m_current_level->getGObjectByID(m_childID);
                m_children.push_back(m_child_obj);
            }
            return m_children;
        }
        return m_children;
    }

} // namespace Pilot