extern "C" {
	#include <string.h>
};
#include "ArchiveReader.h"
#include "Endian.h"
#ifdef SHOW_DEBUG
#include "../gui/DEBUG.h"
#endif

template <typename T>
static void readBinary(FILE* file, T& data) {
	fread(&data, sizeof(data), 1, file);
	swap(data);
}

template <typename T>
static void readBinaryWithSize(FILE* file, T& data, int size) {
	memset(data, 0, sizeof(data));
	fread(&data, size, 1, file);
	swap(data);
}

ArchiveReader::ArchiveReader() {
	stream.zalloc = NULL; stream.zfree = NULL; stream.opaque = NULL;
}

char* ArchiveReader::getDescription() { return &description[0]; }
char* ArchiveReader::getAuthor() { return &author[0]; }
char* ArchiveReader::getPacker() { return &packer[0]; }
char* ArchiveReader::getDatepacked() { return &datepacked[0]; }
unsigned char* ArchiveReader::getIcon() { return &icon[0]; }

void ArchiveReader::setArchiveFile(const char* filename) {
	file = fopen(filename, "rb");
	table = readMetadata();
	stream.zalloc = NULL; stream.zfree = NULL; stream.opaque = NULL;
}

void ArchiveReader::reset() {
	fclose(file);
	delete table;
}

ArchiveTable* ArchiveReader::readMetadata() {
	unsigned int magic, tableOffset, tableSize = 0;
	readBinary(file, magic);//GXA1
	if(0x47584131 != magic) return new ArchiveTable(0);

	readBinaryWithSize(file, description, 64);	
	readBinaryWithSize(file, author, 16);
	readBinaryWithSize(file, packer, 16);
	readBinaryWithSize(file, datepacked, 12);
	readBinaryWithSize(file, icon, 96 * 72 * 2);

	//sprintf(txtbuffer,"PAK info:\r\nDescription: [%s]\r\nAuthor: [%s] Packer: [%s] Date Packed: [%s]\r\n", 
	//	description, author, packer, datepacked);
	//DEBUG_print(txtbuffer,DBG_USBGECKO);
	
	readBinary(file, tableOffset);
	readBinary(file, tableSize);

	ArchiveTable* table = new ArchiveTable(tableSize);

	fseek(file, tableOffset, SEEK_SET);
	for(ArchiveTable::iterator it = table->begin(); it != table->end(); ++it) {
		readBinary(file, it->crc.crc64);
		readBinary(file, it->offset);
#ifdef SHOW_DEBUG
		sprintf(txtbuffer,"readMetadata: crc64 = %08x %08x; offset = %x\r\n", (unsigned int)(it->crc.crc64>>32), (unsigned int)(it->crc.crc64&0xFFFFFFFF), it->offset);
		DEBUG_print(txtbuffer,DBG_USBGECKO);
/*		sprintf(txtbuffer,"readMetadata: crc32 = %8x;\r\n", (it->crc.crc32));
		DEBUG_print(txtbuffer,DBG_USBGECKO);
		sprintf(txtbuffer,"readMetadata: format = %d; size = %d; crc64size = %d\r\n", (it->crc.format), (it->crc.size), sizeof(it->crc.crc64));
		DEBUG_print(txtbuffer,DBG_USBGECKO);
		sprintf(txtbuffer,"readMetadata: offset = %x;\r\n", (it->offset));
		DEBUG_print(txtbuffer,DBG_USBGECKO);*/
#endif
	}

	return table;
}

ArchiveEntryInfo ArchiveReader::readInfo(const ArchiveEntry& entry) {
	fseek(file, entry.offset, SEEK_SET);

	unsigned char width_div_4, height_div_4, format;

	readBinary(file, width_div_4);
	readBinary(file, height_div_4);
	readBinary(file, format);

	return ArchiveEntryInfo(
		(unsigned short)width_div_4 * 4,
		(unsigned short)height_div_4 * 4,
		format );
}

bool ArchiveReader::readTexture(void* data, const ArchiveEntry& entry) {
	const size_t textureHeaderSize = 3; // u8 width_div_4, height_div_4, fmt
	fseek(file, entry.offset + textureHeaderSize, SEEK_SET);

	inflateInit2(&stream, 16+MAX_WBITS);

	bool result = readCompressed(static_cast<unsigned char*>(data));

	inflateEnd(&stream);
	return result;
}

bool ArchiveReader::readTexture(void* data, const ArchiveEntry& entry,
		const ArchiveEntryInfo& info, unsigned int stride) {
	const size_t textureHeaderSize = 3; // u8 width_div_4, height_div_4, fmt
	fseek(file, entry.offset + textureHeaderSize, SEEK_SET);

	inflateInit2(&stream, 16+MAX_WBITS);

	unsigned int width = info.width * 4 * 4; // FIXME: Look this up

	bool result = readCompressed(static_cast<unsigned char*>(data), width, stride);

	inflateEnd(&stream);
	return result;
}

bool ArchiveReader::readCompressed(unsigned char* chunk_out,
		unsigned int width, unsigned int stride) {
	if(width > stride || width == 0) return false;
	// We're starting with an empty buffer, but it'll be read first
	unsigned char chunk_in[CHUNK_SIZE];
	stream.avail_in = 0;

	do {
		// Read a line of input
		stream.avail_out = width;
		stream.next_out = chunk_out;
		// This loop will only run more than once if we fail to read the entire line
		// this may be due to exhausting the input buffer or internal behavior of zlib
		do {
			if(stream.avail_in == 0) {
				// If we've exhausted our input, get some more
				stream.next_in = chunk_in;
				stream.avail_in = fread(chunk_in, 1, CHUNK_SIZE, file);
				if(ferror(file)) return false;
				// Somehow we failed to read more even though we didn't reach
				// the end of the stream; I doubt this is possible but it's allowed
				if(stream.avail_in == 0) break;
			}

			int ret = inflate(&stream, Z_NO_FLUSH);
			switch(ret) {
			case Z_STREAM_END:
				return true;
			case Z_NEED_DICT: case Z_DATA_ERROR: case Z_MEM_ERROR:
				return false;
			default:
				break;
			}
		} while(stream.avail_out != 0);

		// Move forward to the next line
		chunk_out += stride;
	} while(true);

	return true;
}

