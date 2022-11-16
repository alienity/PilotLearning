#pragma once
#include <string>

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

} // namespace Utility

void SIMDMemCopy(void* __restrict Dest, const void* __restrict Source, size_t NumQuadwords);
void SIMDMemFill(void* __restrict Dest, __m128 FillVector, size_t NumQuadwords);