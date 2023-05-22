#include "file_system.h"

namespace MoYu
{
    std::vector<std::filesystem::path> FileSystem::getFiles(const std::filesystem::path& directory)
    {
        std::vector<std::filesystem::path> files;
        for (auto const& directory_entry : std::filesystem::recursive_directory_iterator {directory})
        {
            if (directory_entry.is_regular_file())
            {
                files.push_back(directory_entry);
            }
        }
        return files;
    }

    bool File::exists(const std::filesystem::path& Path)
    {
        DWORD FileAttributes = GetFileAttributesW(Path.c_str());
        return (FileAttributes != INVALID_FILE_ATTRIBUTES && !(FileAttributes & FILE_ATTRIBUTE_DIRECTORY));
    }

    bool Directory::exists(const std::filesystem::path& Path)
    {
        DWORD FileAttributes = GetFileAttributesW(Path.c_str());
        return (FileAttributes != INVALID_FILE_ATTRIBUTES && (FileAttributes & FILE_ATTRIBUTE_DIRECTORY));
    }


}