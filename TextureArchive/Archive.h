#ifndef ARCHIVE_H_
#define ARCHIVE_H_

#include <vector>

struct TextureCRC64 {
	union {
		struct {
			unsigned int  crc32;
			unsigned char pal_crc32_byte1;
			unsigned char pal_crc32_byte2;
			unsigned char pal_crc32_byte3;
			unsigned      format : 4;
			unsigned      size   : 4;
		} __attribute__((packed));
		unsigned long long crc64;
	};

	TextureCRC64() { }
	TextureCRC64(unsigned long long crc64) : crc64(crc64) { }
	// TODO: Assignment, endianness handling
	friend bool operator == (const TextureCRC64& lhs, const TextureCRC64& rhs) {
		return lhs.crc64 == rhs.crc64;
	}
	friend bool operator < (const TextureCRC64& lhs, const TextureCRC64& rhs) {
		return lhs.crc64 < rhs.crc64;
	}
};

struct ArchiveEntry {
	TextureCRC64 crc;
	unsigned int offset;

	ArchiveEntry() { }
	ArchiveEntry(TextureCRC64 crc) : crc(crc), offset(0) { }
	ArchiveEntry(TextureCRC64 crc, unsigned int offset) :
		crc(crc), offset(offset) { }

	ArchiveEntry& operator = (const ArchiveEntry& rhs) {
		crc = rhs.crc;
		offset = rhs.offset;

		return *this;
	}

	friend bool operator == (const ArchiveEntry& lhs, const ArchiveEntry& rhs) {
		return lhs.crc == rhs.crc;
	}

	friend bool operator < (const ArchiveEntry& lhs, const ArchiveEntry& rhs) {
		return lhs.crc < rhs.crc;
	}
};

struct ArchiveEntryInfo {
	unsigned short width, height;
	unsigned char format;

	ArchiveEntryInfo(unsigned short w = 0, unsigned short h = 0, unsigned char fmt = 0) :
		width(w), height(h), format(fmt) { }
};

class Archive {
public:
	typedef std::vector<ArchiveEntry*>::iterator TableIterator;

	void addEntry(ArchiveEntry* entry) { table.push_back(entry); }

	size_t tableSize() { return table.size(); }
	TableIterator tableBegin() { return table.begin(); }
	TableIterator tableEnd() { return table.end(); }

private:
	std::vector<ArchiveEntry*> table;
};

#endif /* ARCHIVE_H_ */
