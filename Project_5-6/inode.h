#ifndef INODE_H

#define INODE_H
#define BLOCK_SIZE 4096
#define INODE_SIZE 64
#define INODE_FIRST_BLOCK 3
#define INODES_PER_BLOCK (BLOCK_SIZE / INODE_SIZE)
#define FREE_INODE_BLOCK_NUM 1

#define INODE_PTR_COUNT 16

struct inode {
  unsigned int size;
  unsigned short owner_id;
  unsigned char permissions;
  unsigned char flags;
  unsigned char link_count;
  unsigned short block_ptr[INODE_PTR_COUNT];

  unsigned int ref_count;  // in-core only
  unsigned int inode_num;
};

int ialloc(void);

#endif