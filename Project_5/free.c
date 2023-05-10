#include <unistd.h>
#include "free.h"
#include "block.h"

int find_low_clear_bit(unsigned char x) {
  for (int i = 0; i < BITS_PER_BYTE; i++)
    if (!(x & (1 << i)))
      return i;
  
  return -1;
}

void set_free(unsigned char *block, int num, int set) {
  int byte_num = num / BITS_PER_BYTE; 
  int bit_num = num % BITS_PER_BYTE;
  if(set) {
    block[byte_num] |= (1 << bit_num);
  }
  else {
    block[byte_num] &= ~(1 << bit_num);
  }
}

int find_free(unsigned char *block) {
  for(int i = 0; i < BLOCK_SIZE; i++) {
    int zero_bit = find_low_clear_bit(block[i]);
    if (zero_bit != -1) {
      return (i*BITS_PER_BYTE) + zero_bit;
    }
  }
  return -1;
}