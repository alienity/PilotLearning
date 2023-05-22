#pragma once
#include "runtime/platform/system/system_core.h"

namespace MoYu
{
    class FileSystem
    {
    public:
        static std::vector<std::filesystem::path> getFiles(const std::filesystem::path& directory);
    };

    class File
    {
    public:
        static bool exists(const std::filesystem::path& Path);
    };

    class Directory
    {
    public:
        static bool exists(const std::filesystem::path& Path);
    };

}