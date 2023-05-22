#ifndef INODE_H

#define INODE_H
#define BLOCK_SIZE 4096
#define INODE_SIZE 64
#define INODE_FIRST_BLOCK 3
#define INODES_PER_BLOCK (BLOCK_SIZE / INODE_SIZE)
#define FREE_INODE_BLOCK_NUM 1

int ialloc(void);

#endif