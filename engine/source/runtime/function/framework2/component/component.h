#pragma once
#include "runtime/function/framework2/object_id_allocator.h"

#include <memory>

namespace MoYu
{
    class GObject;

    class Component
    {
    public:
        inline Component() { m_id = ComponentIDAllocator::alloc(); };
        virtual ~Component() {}

        virtual void tick(float delta_time) {};
        virtual bool isDirty() const { return m_is_dirty; }
        virtual void setDirtyFlag(bool is_dirty) { m_is_dirty = is_dirty; }

    protected:
        std::weak_ptr<GObject> m_parent_object;
        bool m_is_dirty {false};
        GComponentID m_id {K_Invalid_Component_Id};
    };

} // namespace MoYu
