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

void test_free() {
  unsigned char block[BLOCK_SIZE];
  memset(block, 0xFF, BLOCK_SIZE);
  set_free(block, 0, 0);
  CTEST_ASSERT(find_free(block) == 0, "testing find_free to 0 and set_free to 0");
  set_free(block, 0, 1);
  CTEST_ASSERT(find_free(block) == 0, "testing find_free to 0 and set_free to 1");
}

int main(void) {
  test_image();
  test_blockb();
  test_free();
}