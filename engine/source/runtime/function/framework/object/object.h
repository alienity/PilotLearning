#pragma once

#include "runtime/function/framework/component/component.h"
#include "runtime/function/framework/object/object_id_allocator.h"

#include "runtime/resource/res_type/common/object.h"
#include "runtime/core/base/macro.h"
#include "runtime/core/math/moyu_math.h"

#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <memory>

namespace MoYu
{
    class Level;
    class TransformComponent;

    /// GObject : Game Object base class
    class GObject : public std::enable_shared_from_this<GObject>
    {
    public:
        GObject() {}
        GObject(std::shared_ptr<Level> level) : m_current_level(level) {}
        GObject(GObjectID id, std::shared_ptr<Level> level) : m_id(id), m_current_level(level) {}
        virtual ~GObject();

        virtual void tick(float delta_time);

        virtual void lateTick(float delta_time);

        bool isRootNode() const { return m_id == K_Root_Object_Id; }

        bool load(const ObjectInstanceRes& object_instance_res);
        void save(ObjectInstanceRes& out_object_instance_res);

        void      setID(GObjectID id) { m_id = id; }
        GObjectID getID() const { return m_id; }

        std::shared_ptr<GObject> getParent() const;
        std::vector<std::shared_ptr<GObject>> getChildren() const;

        void setSiblingIndex(int sibling_index) { m_sibling_index = sibling_index; }
        int  getSiblingIndex() const { return m_sibling_index; }

        void setParent(GObjectID parentID, std::optional<std::uint32_t> sibling_index = std::nullopt);
        void removeChild(GObjectID childID);

        bool isReadyToErase() const { return m_is_ready_erase; }

        void               setName(std::string name) { m_name = name; }
        const std::string& getName() const { return m_name; }

        std::weak_ptr<TransformComponent> getTransformComponent();

        bool hasComponent(const std::string& compenent_type_name) const;

        std::vector<std::shared_ptr<Component>> getComponents() { return m_components; }

        template<typename TComponent>
        std::shared_ptr<TComponent> tryAddComponent(std::shared_ptr<TComponent> newComponent)
        {
            for (auto& component : m_components)
            {
                if (component->getTypeName() == newComponent->getTypeName())
                {
                    LOG_INFO("object {} already has component {}", m_name, component->getTypeName());
                    return nullptr;
                }
            }
            m_components.push_back(newComponent);

            if (newComponent->getTypeName() == "TransformComponent" && !m_transform_component_ptr.expired())
            {
                m_transform_component_ptr = (std::shared_ptr<TransformComponent>)newComponent;
            }

            return newComponent;
        }

        template<typename TComponent>
        bool tryRemoveComponent(std::shared_ptr<TComponent> toDelComponent)
        {
            auto it = m_components.begin();
            while (it->get() != m_components.end())
            {
                if (it->get() == toDelComponent)
                {
                    m_components.erase(it);
                    return true;
                }                
            }
            return false;
        }

        template<typename TComponent>
        std::shared_ptr<TComponent> tryGetComponent(const std::string& compenent_type_name)
        {
            for (auto& component : m_components)
            {
                if (component->getTypeName() == compenent_type_name)
                {
                    return std::static_pointer_cast<TComponent>(component);
                    //return (std::shared_ptr<TComponent>)component;
                }
            }

            return nullptr;
        }

#define tryGetComponent(COMPONENT_TYPE) tryGetComponent<COMPONENT_TYPE>(#COMPONENT_TYPE)

    protected:
        friend class Level;

        bool m_is_ready_erase {false};

        GObjectID              m_id {K_Invalid_Object_Id};
        GObjectID              m_parent_id {K_Root_Object_Id};
        std::uint32_t          m_sibling_index {0};
        std::vector<GObjectID> m_chilren_id {};

        std::shared_ptr<Level> m_current_level;

        std::string m_name;
        //std::string m_definition_url;

        // we have to use the ReflectionPtr due to that the components need to be reflected 
        // in editor, and it's polymorphism
        std::vector<std::shared_ptr<Component>> m_components;

        std::weak_ptr<TransformComponent> m_transform_component_ptr;
    };
} // namespace MoYu
