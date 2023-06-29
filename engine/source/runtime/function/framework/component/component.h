#pragma once
#include "runtime/function/framework/object/object_id_allocator.h"

#include <memory>
#include <string>

namespace MoYu
{
    class GObject;

    enum ComponentStatus
    {
        Init,
        Dirty,
        Idle,
        Erase,
        None
    };

    class Component
    {
    public:
        inline Component() : m_id(ComponentIDAllocator::alloc()), m_status(ComponentStatus::Init) {}
        virtual ~Component() {}

        // Instantiating the component after definition loaded
        virtual void postLoadResource(std::weak_ptr<GObject> object, void* data) { m_object = object; }

        virtual void tick(float delta_time) {};

        virtual bool isInit() const { return m_status == ComponentStatus::Init; }
        virtual bool isDirty() const { return m_status == ComponentStatus::Init || m_status == ComponentStatus::Dirty; }
        virtual bool isToErase() const { return m_status == ComponentStatus::Erase; }
        virtual bool isIdle() const { return m_status == ComponentStatus::Idle; }
        virtual bool isNone() const { return m_status == ComponentStatus::None; }

        virtual void markInit() { m_status = ComponentStatus::Init; }
        virtual void markDirty() { m_status = ComponentStatus::Dirty; }
        virtual void markToErase() { m_status = ComponentStatus::Erase; }
        virtual void markIdle() { m_status = ComponentStatus::Idle; }
        virtual void markNone() { m_status = ComponentStatus::None; }
        
        void setParentNode(std::weak_ptr<GObject> pobj) { m_object = pobj; }

        GComponentID getComponentId() { return m_id; }

        std::string getTypeName() { return m_component_name; }

        bool m_tick_in_editor_mode {false};

    protected:
        std::string m_component_name;

        std::weak_ptr<GObject> m_object;

        GComponentID m_id {K_Invalid_Component_Id};

        ComponentStatus m_status {ComponentStatus::Init};
    };

} // namespace MoYu
