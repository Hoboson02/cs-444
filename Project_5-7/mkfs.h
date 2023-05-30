#ifndef MKFS_H
#define MKFS_H

#define NUM_OF_BLOCKS 1024
#define DIRECTORY_FLAG 2
#define DIRECTORY_ENTRY_SIZE 32
#define DIRECTORY_SIZE 64
#define FILE_OFFSET 2

void mkfs(void);

struct directory {
  struct inode *inode;
  unsigned int offset;
};

struct directory_entry {
  unsigned int inode_num;
  char name[16];
};

struct directory *directory_open(int inode_num);
int directory_get(struct directory *dir, struct directory_entry *ent);
void directory_close(struct directory *d);

#endif