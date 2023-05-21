#include "runtime/function/framework2/object_id_allocator.h"

#include "core/base/macro.h"

namespace MoYu
{
    std::atomic<GObjectID> ObjectIDAllocator::m_next_id {1};

    GObjectID ObjectIDAllocator::alloc()
    {
        std::atomic<GObjectID> new_object_ret = m_next_id.load();
        m_next_id++;
        if (m_next_id >= K_Invalid_Object_Id)
        {
            LOG_FATAL("gobject id overflow");
        }

        return new_object_ret;
    }


    std::atomic<GComponentID> ComponentIDAllocator::m_next_id {1};

    GComponentID ComponentIDAllocator::alloc()
    {
        std::atomic<GComponentID> new_component_ret = m_next_id.load();
        m_next_id++;
        if (m_next_id >= K_Invalid_Component_Id)
        {
            LOG_FATAL("gcomponent id overflow");
        }

        return new_component_ret;
    }

} // namespace MoYu
