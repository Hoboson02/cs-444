#include <unistd.h>
#include "free.h"

int find_low_clear_bit(unsigned char x) {
  for (int i = 0; i < 8; i++)
    if (!(x & (1 << i)))
      return i;
  
  return -1;
}

set_free(unsigned char *block, int num, int set) {

}

find_free(unsigned char *block) {

}