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

void test_inode() {
  image_open("test_file.img", 0);
  CTEST_ASSERT(ialloc() == 0, "Testing first with 0");
  CTEST_ASSERT(alloc() == 0, "Testing empty block map");
  CTSET_ASSERT(ialloc() == 1, "Testing second with 1");
  CTEST_ASSERT(alloc() == 0, "Testing non-empty block map");
  image_close();
}

void test_mkfs() {
  image_open("test_file.img", 0);
  unsigned char block[BLOCK_SIZE];
  unsigned char outblock[BLOCK_SIZE] = {0};
  image_close();
}

int main(void) {
  CTEST_VERBOSE(1);

  test_image();
  test_blockb();
  test_free();
  test_inode();
  test_mkfs();

  CTEST_RESULTS();
  CTSET_EXIT();
}