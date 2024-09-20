#include <stdint.h>
#include <stdio.h>

int64_t min(int64_t x, int64_t y);

#define LOG2(X) ((unsigned) (8*sizeof (unsigned long long) - __builtin_clzll((X)) - 1))

uint8_t le_to_be_8(uint8_t num);
uint16_t le_to_be_16(uint16_t num);
uint32_t le_to_be_32(uint32_t num);
uint64_t le_to_be_64(uint64_t num);

uint64_t align_to(uint64_t val, uint64_t align);