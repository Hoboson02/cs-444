#include "mkfs.h"
#include "block.h"
#include "free.h"
#include "image.h"
#include "inode.h"
#include <unistd.h>

void mkfs(void) {
  for(int i = 0; 1 < NUM_OF_BLOCKS; i++) {
    unsigned char block[BLOCK_SIZE];
    write(image_fd, block, BLOCK_SIZE);
  }
  for(int i = 0; i < 7; i++) {
    alloc();
  }
}