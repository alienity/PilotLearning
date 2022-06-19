#pragma once
#include "runtime/platform/file_service/file_stream.h"

namespace Pilot
{
    FileStream::FileStream(const std::filesystem::path& Path, FileMode Mode, FileAccess Access) :
        Path(Path), Mode(Mode), Access(Access), Handle(initializeHandle(Path, Mode, Access))
    {}

    FileStream::FileStream(const std::filesystem::path& Path, FileMode Mode) :
        FileStream(Path, Mode, FileAccess::ReadWrite)
    {}

    void* FileStream::getHandle() const noexcept { return Handle.get(); }

    std::uint64_t FileStream::getSizeInBytes() const noexcept
    {
        LARGE_INTEGER FileSize = {};
        if (GetFileSizeEx(Handle.get(), &FileSize))
        {
            return FileSize.QuadPart;
        }
        return 0;
    }

    bool FileStream::canRead() const noexcept { return Access == FileAccess::Read || Access == FileAccess::ReadWrite; }

    bool FileStream::canWrite() const noexcept
    {
        return Access == FileAccess::Write || Access == FileAccess::ReadWrite;
    }

    void FileStream::reset() { Handle.reset(); }

    std::unique_ptr<std::byte[]> FileStream::readAll() const
    {
        std::uint64_t FileSize = getSizeInBytes();

        DWORD NumberOfBytesRead   = 0;
        DWORD NumberOfBytesToRead = static_cast<DWORD>(FileSize);
        auto  Buffer              = std::make_unique<std::byte[]>(FileSize);
        if (ReadFile(Handle.get(), Buffer.get(), NumberOfBytesToRead, &NumberOfBytesRead, nullptr))
        {
            assert(NumberOfBytesToRead == NumberOfBytesRead);
        }
        return Buffer;
    }

    std::uint64_t FileStream::read(void* Buffer, std::uint64_t SizeInBytes) const
    {
        if (Buffer)
        {
            DWORD NumberOfBytesRead   = 0;
            DWORD NumberOfBytesToRead = static_cast<DWORD>(SizeInBytes);
            if (ReadFile(Handle.get(), Buffer, NumberOfBytesToRead, &NumberOfBytesRead, nullptr))
            {
                return NumberOfBytesRead;
            }
        }
        return 0;
    }

    std::uint64_t FileStream::write(const void* Buffer, std::uint64_t SizeInBytes) const
    {
        if (Buffer)
        {
            DWORD NumberOfBytesWritten = 0;
            DWORD NumberOfBytesToWrite = static_cast<DWORD>(SizeInBytes);
            if (!WriteFile(Handle.get(), Buffer, NumberOfBytesToWrite, &NumberOfBytesWritten, nullptr))
            {
                char actionStr[] = "WriteFile";
                ErrorExit(actionStr);
            }

            return NumberOfBytesWritten;
        }

        return 0;
    }

    void FileStream::seek(std::int64_t Offset, SeekOrigin RelativeOrigin) const
    {
        DWORD         MoveMethod       = getMoveMethod(RelativeOrigin);
        LARGE_INTEGER liDistanceToMove = {};
        liDistanceToMove.QuadPart      = Offset;
        SetFilePointerEx(Handle.get(), liDistanceToMove, nullptr, MoveMethod);
    }

    DWORD FileStream::getMoveMethod(SeekOrigin Origin)
    {
        DWORD dwMoveMethod = 0;
        // clang-format off
	    switch (Origin)
	    {
	    case SeekOrigin::Begin:		dwMoveMethod = FILE_BEGIN;		break;
	    case SeekOrigin::Current:	dwMoveMethod = FILE_CURRENT;	break;
	    case SeekOrigin::End:		dwMoveMethod = FILE_END;		break;
	    }
        // clang-format on
        return dwMoveMethod;
    }

    void FileStream::verifyArguments()
    {
        switch (Mode)
        {
            case FileMode::CreateNew:
                assert(canWrite());
                break;
            case FileMode::Create:
                assert(canWrite());
                break;
            case FileMode::Open:
                assert(canRead() || canWrite());
                break;
            case FileMode::OpenOrCreate:
                assert(canRead() || canWrite());
                break;
            case FileMode::Truncate:
                assert(canWrite());
                break;
        }
    }

    wil::unique_handle FileStream::initializeHandle(const std::filesystem::path& Path, FileMode Mode, FileAccess Access)
    {
        verifyArguments();

        DWORD CreationDisposition = [Mode] {
            DWORD dwCreationDisposition = 0;
            // clang-format off
		switch (Mode)
		{
		case FileMode::CreateNew:		dwCreationDisposition = CREATE_NEW;			break;
		case FileMode::Create:			dwCreationDisposition = CREATE_ALWAYS;		break;
		case FileMode::Open:			dwCreationDisposition = OPEN_EXISTING;		break;
		case FileMode::OpenOrCreate:	dwCreationDisposition = OPEN_ALWAYS;		break;
		case FileMode::Truncate:		dwCreationDisposition = TRUNCATE_EXISTING;	break;
		}
            // clang-format on
            return dwCreationDisposition;
        }();

        DWORD DesiredAccess = [Access] {
            DWORD dwDesiredAccess = 0;
            // clang-format off
		switch (Access)
		{
		case FileAccess::Read:		dwDesiredAccess = GENERIC_READ;					break;
		case FileAccess::Write:		dwDesiredAccess = GENERIC_WRITE;				break;
		case FileAccess::ReadWrite:	dwDesiredAccess = GENERIC_READ | GENERIC_WRITE; break;
		}
            // clang-format on
            return dwDesiredAccess;
        }();

        auto Handle =
            wil::unique_handle(CreateFileW(Path.c_str(), DesiredAccess, 0, nullptr, CreationDisposition, 0, nullptr));
        if (!Handle)
        {
            DWORD Error = GetLastError();
            if (Mode == FileMode::CreateNew && Error == ERROR_ALREADY_EXISTS)
            {
                throw std::runtime_error("ERROR_ALREADY_EXISTS");
            }
            if (Mode == FileMode::Create && Error == ERROR_ALREADY_EXISTS)
            {
                // File overriden
            }
            if (Mode == FileMode::Open && Error == ERROR_FILE_NOT_FOUND)
            {
                throw std::runtime_error("ERROR_FILE_NOT_FOUND");
            }
            if (Mode == FileMode::OpenOrCreate && Error == ERROR_ALREADY_EXISTS)
            {
                // File exists
            }
        }
        assert(Handle);
        return Handle;
    }

} // namespace Pilot