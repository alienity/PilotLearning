#pragma once
#include "runtime/platform/system/system_core.h"

namespace Pilot
{
	class FileStream;

	class BinaryWriter
	{
	public:
		explicit BinaryWriter(FileStream& Stream);

		void write(const void* Data, std::uint64_t SizeInBytes) const;

		template<typename T>
		void write(const T& Object) const
		{
            write(&Object, sizeof(T));
		}

		template<>
		void write<std::string>(const std::string& Object) const
		{
			write(Object.data(), Object.size() * sizeof(std::string::value_type));
		}

		template<>
		void write<std::wstring>(const std::wstring& Object) const
		{
			write(Object.data(), Object.size() * sizeof(std::wstring::value_type));
		}

	private:
		FileStream& Stream;
	};

}