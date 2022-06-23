#pragma once

class Hash
{
public:
    static std::uint64_t Hash64(const void* Object, size_t SizeInBytes);
};