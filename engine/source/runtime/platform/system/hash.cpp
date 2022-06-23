#pragma once
#include "hash.h"
#include "city.h"

std::uint64_t Hash::Hash64(const void* Object, size_t SizeInBytes)
{
    return CityHash64(static_cast<const char*>(Object), SizeInBytes);
}