#include "binary_reader.h"
#include "file_stream.h"

namespace MoYu
{

	BinaryReader::BinaryReader(FileStream& Stream) : Stream(Stream)
    {
        assert(Stream.canRead());
        BaseAddress = Stream.readAll();
        Ptr         = BaseAddress.get();
        Sentinel    = Ptr + Stream.getSizeInBytes();
    }

    void BinaryReader::read(void* DstData, std::uint64_t SizeInBytes) const noexcept
    {
        assert(Ptr + SizeInBytes <= Sentinel);

        std::byte* SrcData = Ptr;
        Ptr += SizeInBytes;
        std::memcpy(DstData, SrcData, SizeInBytes);
    }


}