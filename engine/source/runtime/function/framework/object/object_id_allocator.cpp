#include "runtime/function/framework/object/object_id_allocator.h"

#include "core/base/macro.h"

namespace MoYu
{
#define DefineObjectIDFuncs(ObjName)\
    std::atomic<G##ObjName##ID> ObjName##IDAllocator::m_next_id {1};\
    G##ObjName##ID ObjName##IDAllocator::alloc()\
    {\
        std::atomic<G##ObjName##ID> new_object_ret = m_next_id.load();\
        m_next_id++;\
        if (m_next_id >= KInvalidId(ObjName))\
        {\
            LOG_FATAL("gobject id overflow");\
        }\
        return new_object_ret;\
    }\

    DefineObjectIDFuncs(Object)
    DefineObjectIDFuncs(Component)

} // namespace MoYu
