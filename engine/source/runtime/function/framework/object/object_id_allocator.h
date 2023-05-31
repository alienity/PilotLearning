#pragma once

#include <atomic>
#include <limits>

namespace MoYu
{
#define KRootId(ObjName) K_Root_##ObjName##_Id
#define KInvalidId(ObjName) K_Invalid_##ObjName##_Id

#define DefineObjectIDAlloc(ObjName) \
    using G##ObjName##ID = std::uint32_t;\
    constexpr G##ObjName##ID KRootId(ObjName) = 0;\
    constexpr G##ObjName##ID KInvalidId(ObjName) = std::numeric_limits<std::int32_t>::max();\
    class ObjName##IDAllocator\
    {\
    public:\
        static G##ObjName##ID alloc();\
    private:\
        static std::atomic<G##ObjName##ID> m_next_id;\
    };\

    // DefineObjectIDAlloc(Internal)
    DefineObjectIDAlloc(Instance)
    DefineObjectIDAlloc(Object)
    DefineObjectIDAlloc(Component)

} // namespace MoYu
