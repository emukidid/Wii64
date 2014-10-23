#ifndef ARCHIVEREADER_H_
#define ARCHIVEREADER_H_

#include <cstdio>
#include <memory>

#include <zlib.h>

#include "Archive.h"


template<typename T>
class SortedArray {
public:
	SortedArray(unsigned int size) : size(size), array(new T[size]) { }
	~SortedArray() { delete[] array; }

	T& operator[] (const int index) { return array[index]; }

	bool find(T& entry) { // TODO: Verify correctness
		for(int min = 0, max = size-1, index = size/2; min <= max; index = min + (max-min)/2) {
			if(entry < array[index])
				max = index - 1;
			else if(entry == array[index]) {
				entry = array[index];
				return true;
			} else
				min = index + 1;
		}
/*		for(int index = 0; index < size; index++) 
		{
			if(array[index] == entry)
			{
				entry = array[index];
				return true;
			}
		}*/
		return false;
	}

	typedef T* iterator;
	iterator begin() { return array; }
	iterator end() { return array + size; }

protected:
	unsigned int size;
	T* array;
};

typedef SortedArray<ArchiveEntry> ArchiveTable;

class ArchiveReader {
public:
	ArchiveReader(const char* filename);
	~ArchiveReader();

	bool find(ArchiveEntry& entry) { return table->find(entry); }
	ArchiveEntryInfo readInfo(const ArchiveEntry&);
	bool readTexture(void* textureData, const ArchiveEntry&);
	bool readTexture(void* textureData, const ArchiveEntry&,
	                 const ArchiveEntryInfo&, unsigned int stride);
	char *getDescription();
	char *getAuthor();
	char *getPacker();
	char *getDatepacked();
	unsigned char *getIcon();

private:
	FILE* file;
	ArchiveTable* table;
	
	char description[68];
	char author[20];
	char packer[20];
	char datepacked[16]; // Date packed yyyy/mm/dd
	unsigned char icon[96*72*2]; // RGB5A3 icon<br>
	static const int COMPRESSION = 9, CHUNK_SIZE = 2 * 1024;
	z_stream stream;

	ArchiveTable* readMetadata();
	bool readCompressed(unsigned char* textureData,
	                    unsigned int width = CHUNK_SIZE,
	                    unsigned int stride = CHUNK_SIZE);

	friend int main(int, char*[]);
};

#endif /* ARCHIVEREADER_H_ */
