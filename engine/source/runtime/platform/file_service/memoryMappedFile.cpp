#include "memoryMappedFile.h"
#include "file_Stream.h"

namespace MoYu
{
    MemoryMappedFile::MemoryMappedFile(FileStream& Stream, std::uint64_t FileSize /*= DefaultFileSize*/) :
        Stream(Stream)
    {
        InternalCreate(FileSize);
    }

    std::uint64_t MemoryMappedFile::GetCurrentFileSize() const noexcept { return CurrentFileSize; }

    MemoryMappedView MemoryMappedFile::CreateView() const noexcept
    {
        constexpr DWORD DesiredAccess = FILE_MAP_ALL_ACCESS;
        LPVOID          View          = MapViewOfFile(FileMapping.get(), DesiredAccess, 0, 0, CurrentFileSize);
        return MemoryMappedView(static_cast<std::byte*>(View), CurrentFileSize);
    }

    MemoryMappedView MemoryMappedFile::CreateView(std::uint32_t Offset, std::uint64_t SizeInBytes) const noexcept
    {
        constexpr DWORD DesiredAccess = FILE_MAP_ALL_ACCESS;
        LPVOID          View          = MapViewOfFile(FileMapping.get(), DesiredAccess, 0, Offset, SizeInBytes);
        return MemoryMappedView(static_cast<std::byte*>(View), SizeInBytes);
    }

    void MemoryMappedFile::GrowMapping(std::uint64_t Size)
    {
        // Check the size.
        if (Size <= CurrentFileSize)
        {
            // Don't shrink.
            return;
        }

        // Update the size and create a new mapping.
        InternalCreate(Size);
    }

    void MemoryMappedFile::InternalCreate(std::uint64_t FileSize)
    {
        CurrentFileSize = Stream.getSizeInBytes();
        if (CurrentFileSize == 0)
        {
            // File mapping files with a size of 0 produces an error.
            CurrentFileSize = DefaultFileSize;
        }
        else if (FileSize > CurrentFileSize)
        {
            // Grow to the specified size.
            CurrentFileSize = FileSize;
        }

        FileMapping.reset(CreateFileMapping(
            Stream.getHandle(), nullptr, PAGE_READWRITE, 0, static_cast<DWORD>(CurrentFileSize), nullptr));
        if (!FileMapping)
        {
            // ErrorExit(__FUNCTIONW__);
        }
    }

}
