#include "runtime/function/framework/object/object.h"
#include "runtime/function/framework/level/level.h"

#include "runtime/engine.h"

#include "runtime/resource/asset_manager/asset_manager.h"

#include "runtime/function/framework/component/component.h"
#include "runtime/function/framework/component/transform/transform_component.h"
#include "runtime/function/global/global_context.h"

#include <cassert>
#include <unordered_set>

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

    void GObject::tick(float delta_time)
    {
        for (auto& component : m_components)
        {
            if (shouldComponentTick(component->getTypeName()))
            {
                component->tick(delta_time);
            }
        }

        // mark transform component clean
        std::shared_ptr<TransformComponent> trans_ptr = getTransformComponent();
        trans_ptr->setDirtyFlag(false);
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

    std::shared_ptr<TransformComponent> GObject::getTransformComponent()
    {
        if (!m_transform_component_ptr)
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
    
    std::shared_ptr<GObject> GObject::getParent() const
    {
        if (m_current_level != nullptr)
        {
            if (isRootNode())
            {
                return std::shared_ptr<GObject>();
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
                GObjectID              m_childID   = m_chilren_id[i];
                std::shared_ptr<GObject> m_child_obj = m_current_level->getGObjectByID(m_childID);
                m_children.push_back(m_child_obj);
            }
        }
        return m_children;
    }

} // namespace MoYu