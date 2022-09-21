#pragma once

#include "runtime/function/framework/component/component.h"
#include "runtime/function/framework/object/object_id_allocator.h"

#include "runtime/resource/res_type/common/object.h"

#include <memory>
#include <string>
#include <unordered_set>
#include <vector>
#include <optional>

namespace Pilot
{
    class Level;

    /// GObject : Game Object base class
    class GObject : public std::enable_shared_from_this<GObject>
    {
    public:
        GObject() {}
        GObject(std::shared_ptr<Level> level) : m_current_level(level) {}
        GObject(GObjectID id, std::shared_ptr<Level> level) : m_id(id), m_current_level(level) {}
        virtual ~GObject();

        virtual void tick(float delta_time);

        bool isRootNode() const { return m_id == k_root_gobject_id; }

        bool load(const ObjectInstanceRes& object_instance_res);
        void save(ObjectInstanceRes& out_object_instance_res);

        void      setID(GObjectID id) { m_id = id; }
        GObjectID getID() const { return m_id; }

        std::weak_ptr<GObject> getParent() const;
        std::vector<std::weak_ptr<GObject>> getChildren() const;

        void setSiblingIndex(int sibling_index) { m_sibling_index = sibling_index; }
        int  getSiblingIndex() const { return m_sibling_index; }

        void setParent(GObjectID parentID, std::optional<std::uint32_t> sibling_index = std::nullopt);
        void removeChild(GObjectID childID);

        void               setName(std::string name) { m_name = name; }
        const std::string& getName() const { return m_name; }

        bool hasComponent(const std::string& compenent_type_name) const;

        std::vector<Reflection::ReflectionPtr<Component>> getComponents() { return m_components; }

        template<typename TComponent>
        TComponent* tryAddComponent(Reflection::ReflectionPtr<TComponent> newComponent)
        {
            for (auto& component : m_components)
            {
                if (component.getTypeName() == newComponent.getTypeName())
                {
                    LOG_INFO("object {} already has component {}", m_name, component.getTypeName());
                    return nullptr;
                }
            }
            m_components.push_back(newComponent);

            return newComponent.getPtr();
        }

        template<typename TComponent>
        bool tryRemoveComponent(Reflection::ReflectionPtr<TComponent> toDelComponent)
        {
            for (auto& component : m_components)
            {
                if (component == toDelComponent)
                {
                    m_components.erase(toDelComponent);
                    PILOT_REFLECTION_DELETE(toDelComponent);
                    return true;
                }
            }
            return false;
        }

        template<typename TComponent>
        TComponent* tryGetComponent(const std::string& compenent_type_name)
        {
            for (auto& component : m_components)
            {
                if (component.getTypeName() == compenent_type_name)
                {
                    return static_cast<TComponent*>(component.operator->());
                }
            }

            return nullptr;
        }

        template<typename TComponent>
        const TComponent* tryGetComponentConst(const std::string& compenent_type_name) const
        {
            for (const auto& component : m_components)
            {
                if (component.getTypeName() == compenent_type_name)
                {
                    return static_cast<const TComponent*>(component.operator->());
                }
            }
            return nullptr;
        }

#define tryGetComponent(COMPONENT_TYPE) tryGetComponent<COMPONENT_TYPE>(#COMPONENT_TYPE)
#define tryGetComponentConst(COMPONENT_TYPE) tryGetComponentConst<const COMPONENT_TYPE>(#COMPONENT_TYPE)

    protected:
        friend class Level;

        GObjectID              m_id {k_invalid_gobject_id};
        GObjectID              m_parent_id {k_root_gobject_id};
        std::uint32_t          m_sibling_index {0};
        std::vector<GObjectID> m_chilren_id {};

        std::shared_ptr<Level> m_current_level;

        std::string m_name;
        std::string m_definition_url;

        // we have to use the ReflectionPtr due to that the components need to be reflected 
        // in editor, and it's polymorphism
        std::vector<Reflection::ReflectionPtr<Component>> m_components;
    };
} // namespace Pilot
