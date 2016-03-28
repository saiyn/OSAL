#ifndef _OSAL_UTILITY_H_
#define _OSAL_UTILITY_H_




#define WORD_SIZE_ALIGN(size) (((size) + 4 - 1) & ~(4-1))


uint16 Utf2Unicode(uint8 *dst, uint8 *src, uint16 len);


void DumpBuf(uint8 *buf, uint16 len);



uint8 GetWeekdayType(int y, uint8 m, uint8 d);

uint32_t calc_crc32(uint32_t crc, const void *buf, size_t size);


#endif


