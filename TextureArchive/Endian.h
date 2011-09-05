#ifndef ENDIAN_H_
#define ENDIAN_H_

#ifndef __BIG_ENDIAN__

static inline void swap(unsigned char& data) { }

static inline void swap(unsigned short& data) {
   unsigned short swapped;
   unsigned char* d = reinterpret_cast<unsigned char*>(&data);
   unsigned char* s = reinterpret_cast<unsigned char*>(&swapped);
   s[0] = d[1], s[1] = d[0];
   data = swapped;
}

static inline void swap(unsigned int& data) {
   unsigned int swapped;
   unsigned char* d = reinterpret_cast<unsigned char*>(&data);
   unsigned char* s = reinterpret_cast<unsigned char*>(&swapped);
   s[0] = d[3], s[1] = d[2], s[2] = d[1], s[3] = d[0];
   data = swapped;
}

static inline void swap(unsigned long long& data) {
   unsigned int* d = reinterpret_cast<unsigned int*>(&data);
   swap(d[0]);
   swap(d[1]);
}

#else

template <typename T>
static inline void swap(T& data) { }

#endif

#endif /* ENDIAN_H_ */
