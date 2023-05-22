#pragma once
#include "runtime/platform/system/system_core.h"

namespace MoYu
{
    class FileStream;

    class BinaryReader
    {
    public:
        explicit BinaryReader(FileStream& Stream);

        void read(void* DstData, std::uint64_t SizeInBytes) const noexcept;

        template<typename T>
        T read() const noexcept
        {
            static_assert(std::is_trivial_v<T>, "typename T is not trivial");
            assert(Ptr + sizeof(T) <= Sentinel);

            std::byte* Data = Ptr;
            Ptr += sizeof(T);
            return *reinterpret_cast<T*>(Data);
        }

    private:
        FileStream&                  Stream;
        std::unique_ptr<std::byte[]> BaseAddress;
        mutable std::byte*           Ptr;
        std::byte*                   Sentinel;
    };


}