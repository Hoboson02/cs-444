#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "image.h"
#include "block.h"
#include "free.h"
#include "inode.h"
#include "mkfs.h"
#include "ctest.h"


void test_image() {
  image_open("test_file.img", 0);
  image_close();
}

void test_blockb() {
  unsigned char block[BLOCK_SIZE];
  unsigned char block2[BLOCK_SIZE];
  memset(block, 'h', BLOCK_SIZE);
  bwrite(12, block);
  bread(12, block2);
  if (memcmp(block, block2, BLOCK_SIZE) !=0) {
    printf("Test Failed: The blocks produce differing results");
  }
  else {
    printf("Test Passed: Both blocks produce the same result");
  }
}

int main(void) {
  test_image();
  test_blockb();
}