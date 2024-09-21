#include "../include/helpers.h"
#include <assert.h>
#include <string.h>
#include <errno.h>


FILE* create_file(char* filename)
{	
	FILE* file_ptr = fopen(filename, "wb+");
	return file_ptr;
}

size_t get_input_file_size(FILE* fptr) {
    fseek(fptr, 0L, SEEK_END);
    size_t size = ftell(fptr);
    rewind(fptr);
    return size;
}

int64_t min(int64_t x, int64_t y) {
	return x < y ? x : y;
}

uint8_t le_to_be_8(uint8_t num) {
	return num;
}

uint16_t le_to_be_16(uint16_t num) {
	return (num>>8) | (num<<8);
}

//        byte3					 byte2						byte1					byte0
// | 31 30 29 28 27 26 25 | 24 23 22 21 20 19 18 17 | 16 15 14 13 12 11 10 9 | 8 7 6 5 4 3 2 1 0 |
uint32_t le_to_be_32(uint32_t num) {
	return 	((num>>24)& 0xff)    | 				// move byte 3 to byte 0
			((num<<8)& 0xff0000) | 				// move byte 1 to byte 2
			((num>>8)& 0xff00) 	| 				// move byte 2 to byte 1
			((num<<24)& 0xff000000);			// byte 0 to byte 3
}


uint64_t le_to_be_64(uint64_t num) {
	return (le_to_be_32((uint32_t) num & 0xffffffff00000000) | le_to_be_32((uint32_t) num & 0x00000000ffffffff));
}

uint64_t align_to(uint64_t val, uint64_t align) {
  if (align == 0)
	return val;
  return (val + align - 1) & ~(align - 1);
}