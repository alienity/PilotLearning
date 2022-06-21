#pragma once
#include "runtime/platform/system/system_core.h"
#include "memoryMappedView.h"

namespace Pilot
{
    class FileStream;

    class MemoryMappedFile
    {
    public:
        static constexpr std::uint64_t DefaultFileSize = 64;

        explicit MemoryMappedFile(FileStream& Stream, std::uint64_t FileSize = DefaultFileSize);

        [[nodiscard]] std::uint64_t GetCurrentFileSize() const noexcept;

        [[nodiscard]] MemoryMappedView CreateView() const noexcept;
        [[nodiscard]] MemoryMappedView CreateView(std::uint32_t Offset, std::uint64_t SizeInBytes) const noexcept;

        // Invoking GrowMapping can cause any associated MemoryMappedView to be undefined.
        // Make sure to call CreateView again on any associated views
        void GrowMapping(std::uint64_t Size);

    private:
        void InternalCreate(std::uint64_t FileSize);

    private:
        FileStream&           Stream;
        wil::unique_handle    FileMapping;
        std::filesystem::path Path;
        std::uint64_t         CurrentFileSize = 0;
    };

}
