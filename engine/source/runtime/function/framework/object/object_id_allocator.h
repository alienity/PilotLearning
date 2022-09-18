#pragma once

#include <atomic>
#include <limits>

namespace Pilot
{
    using GObjectID = std::uint32_t;

    constexpr GObjectID k_invalid_gobject_id = std::numeric_limits<std::uint32_t>::max();

    class ObjectIDAllocator
    {
    public:
        static GObjectID alloc();

    private:
        static std::atomic<GObjectID> m_next_id;
    };
} // namespace Pilot
