#pragma once
#include <string>
#include <xmmintrin.h>

// This requires SSE4.2 which is present on Intel Nehalem (Nov. 2008)
// and AMD Bulldozer (Oct. 2011) processors.  I could put a runtime
// check for this, but I'm just going to assume people playing with
// DirectX 12 on Windows 10 have fairly recent machines.
#ifdef _M_X64
#define ENABLE_SSE_CRC32 1
#else
#define ENABLE_SSE_CRC32 0
#endif

#if ENABLE_SSE_CRC32
#pragma intrinsic(_mm_crc32_u32)
#pragma intrinsic(_mm_crc32_u64)
#endif

namespace Utility
{
    std::wstring UTF8ToWideString(const std::string& str);
    std::string  WideStringToUTF8(const std::wstring& wstr);
    std::string  ToLower(const std::string& str);
    std::wstring ToLower(const std::wstring& str);
    std::string  GetBasePath(const std::string& str);
    std::wstring GetBasePath(const std::wstring& str);
    std::string  RemoveBasePath(const std::string& str);
    std::wstring RemoveBasePath(const std::wstring& str);
    std::string  GetFileExtension(const std::string& str);
    std::wstring GetFileExtension(const std::wstring& str);
    std::string  RemoveExtension(const std::string& str);
    std::wstring RemoveExtension(const std::wstring& str);

    std::uint64_t Hash64(const void* Object, size_t SizeInBytes);

} // namespace Utility

void SIMDMemCopy(void* __restrict Dest, const void* __restrict Source, size_t NumQuadwords);
void SIMDMemFill(void* __restrict Dest, __m128 FillVector, size_t NumQuadwords);