#include <unistd.h>
#include "block.h"
#include "image.h"
#include "free.h"

unsigned char *bread(int block_num, unsigned char *block) {
  int offset = block_num * BLOCK_SIZE;
  lseek(image_fd, offset, SEEK_SET);
  read(image_fd, block, BLOCK_SIZE);
  return block;
}

void bwrite(int block_num, unsigned char *block) {
  int offset = block_num * BLOCK_SIZE;
  lseek(image_fd, offset, SEEK_SET);
  write(image_fd, block, BLOCK_SIZE);
}

int alloc(void) {
  unsigned char data_block[BLOCK_SIZE];
  bread(FREE_DATA_BLOCK_NUM, data_block);
  int low_free_bit = find_free(data_block);
  if(low_free_bit != -1) {
    set_free(data_block, low_free_bit, 1);
    bwrite(FREE_DATA_BLOCK_NUM ,data_block);
  }
  return low_free_bit;
}