#include "binary_writer.h"
#include "file_stream.h"

namespace Pilot
{

	BinaryWriter::BinaryWriter(FileStream& Stream) : Stream(Stream) { assert(Stream.canWrite()); }

	void BinaryWriter::write(const void* Data, std::uint64_t SizeInBytes) const { Stream.write(Data, SizeInBytes); }

}