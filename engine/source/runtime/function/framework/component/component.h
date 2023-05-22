#pragma once
#include "runtime/function/framework/object/object_id_allocator.h"

namespace MoYu
{
    class GObject;
    // Component
    class Component
    {
    protected:
        std::weak_ptr<GObject> m_parent_object;
        bool     m_is_dirty {false};

    public:
        inline Component() { m_id = ComponentIDAllocator::alloc(); };
        virtual ~Component() {}

        // Instantiating the component after definition loaded
        virtual void postLoadResource(std::weak_ptr<GObject> parent_object) { m_parent_object = parent_object;}

        virtual void tick(float delta_time) {};

        virtual bool isDirty() const { return m_is_dirty; }

        virtual void setDirtyFlag(bool is_dirty) { m_is_dirty = is_dirty; }

        std::string getTypeName() { return m_component_name; }

        bool m_tick_in_editor_mode {false};

    protected:
        GComponentID m_id {K_Invalid_Component_Id};

        std::string m_component_name;
    };

} // namespace MoYu
