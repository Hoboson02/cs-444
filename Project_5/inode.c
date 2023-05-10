#include "inode.h"
#include "block.h"
#include "free.h"


int ialloc(void) {
  unsigned char inode_map[BLOCK_SIZE];
  bread(FREE_INODE_BLOCK_NUM, inode_map);
  int low_free_bit = find_free(inode_map);
  if(low_free_bit != -1) {
    set_free(inode_map, low_free_bit, 1);
  }
  bwrite(FREE_INODE_BLOCK_NUM ,inode_map);
  return low_free_bit;
}