#pragma once
#include "runtime/function/framework/object/object_id_allocator.h"

#include <memory>
#include <string>

namespace MoYu
{
    class GObject;

    class Component
    {
    protected:
        std::weak_ptr<GObject> m_object;
        bool m_is_dirty {false};

    public:
        inline Component() { m_id = ComponentIDAllocator::alloc(); };
        virtual ~Component() {}

        // Instantiating the component after definition loaded
        virtual void postLoadResource(std::weak_ptr<GObject> object, void* data) { m_object = object; }

        virtual void tick(float delta_time) {};

        virtual bool isDirty() const { return m_is_dirty; }

        virtual void setDirtyFlag(bool is_dirty) { m_is_dirty = is_dirty; }

        GComponentID getComponentId() { return m_id; }

        std::string getTypeName() { return m_component_name; }

        bool m_tick_in_editor_mode {false};

    protected:
        GComponentID m_id {K_Invalid_Component_Id};

        static std::string m_component_name;
    };

} // namespace MoYu
