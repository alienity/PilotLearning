#pragma once
#include "runtime/platform/system/system_core.h"

namespace MoYu
{
    class MemoryMappedView
    {
    public:
        explicit MemoryMappedView(std::byte* View, std::uint64_t SizeInBytes);
        MemoryMappedView(MemoryMappedView&& MemoryMappedView) noexcept;
        MemoryMappedView& operator=(MemoryMappedView&& MemoryMappedView) noexcept;
        ~MemoryMappedView();

        MemoryMappedView(const MemoryMappedView&) = delete;
        MemoryMappedView& operator=(const MemoryMappedView&) = delete;

        [[nodiscard]] bool IsMapped() const noexcept { return View != nullptr; }

        void Flush();

        [[nodiscard]] std::byte* GetView(std::uint64_t ViewOffset) const noexcept;

        template<typename T>
        T Read(std::uint64_t ViewOffset)
        {
            static_assert(std::is_trivial_v<T>, "typename T is not trivial");

            return *reinterpret_cast<T*>(GetView(ViewOffset));
        }

        template<typename T>
        void Write(std::uint64_t ViewOffset, const T& Data)
        {
            static_assert(std::is_trivial_v<T>, "typename T is not trivial");

            std::byte* DstAddress = GetView(ViewOffset);
            memcpy(DstAddress, &Data, sizeof(T));
        }

    private:
        void InternalDestroy();

    private:
        std::byte* View     = nullptr;
        std::byte* Sentinel = nullptr;
    };
} // namespace MoYu