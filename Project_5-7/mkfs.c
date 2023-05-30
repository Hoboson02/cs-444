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


// ## Create the Root Directory
// Broken down:

  int inode_num = ialloc(); // 1. Call ialloc() to get a new inode.

// We'll need the inode_num later to add it to the directory!

  int block_num = alloc(); // 2. Call alloc() to get a new data block.

// This will hold the directory entries.

// Be sure to note the returned block number because we're going to need to write to it in just a minute!

  struct inode *root = iget(inode_num); // 3. Initialize the inode returned from ialloc(), above.
  
  root->flags = DIRECTORY_FLAG; //   - flags needs to be set to 2.

  root->size = DIRECTORY_SIZE; //   - size needs to be set to the byte size of the directory. Since we have two entries (. and ..) and each is 32 bytes, the size must be 64 bytes.

  root->block_ptr[0] = block_num;//   - block_ptr[0] needs to point to the data block we just got from alloc(), above.

  unsigned char block[BLOCK_SIZE];// 4. Make an unsigned char block[BLOCK_SIZE] array that you can populate with the new directory data.

// We're going to pack the . and .. directory entries in here.

  write_u16(block, inode_num); // 5. Add the directory entries. You're going to need to math it out.

// You know the first two-byte value is the inode number, so you can pack that in with write_16(). Remember that for the root inode, both the . and .. inode numbers are the same as the root inode itself (i.e. the inode_num you got back from ialloc()).

  strcpy((char*)block + FILE_OFFSET, "."); // The next up-to-16 bytes are the file name. You can copy that in with strcpy(). (You might have to cast some arguments to char*.)

// You have to do this process for both . and .. entries.
  write_u16(block + FILE_OFFSET + DIRECTORY_ENTRY_SIZE, inode_num);
  strcpy((char*)block + FILE_OFFSET, ".");
// Compute the offsets into your in-memory block and copy the data there.

  bwrite(block_num, block); // 6. Write the directory data block back out to disk with bwrite().

  iput(root); // 7. Call iput() to write the new directory inode out to disk and free up the in-core inode.

// At this point, we should have a root directory. But other than doing a raw iget() and bread() to get the block, we don't have a nice way of interfacing with it.

// Also, the root directory inode number should be 0, since it's the first one we've allocated.
}

// ## Directory Open/Read/Close Ops
// ### Opening a Directory
// So the process is:
struct directory *directory_open(int inode_num) {
  struct inode* open_directory = iget(inode_num); // 1. Use iget() to get the inode for this file.
  
  if (open_directory == NULL) {// 2. If it fails, directory_open() should return NULL.
    return NULL;
  }
  struct directory *directory_struct = malloc(sizeof(struct directory)); // 3. malloc() space for a new struct directory.

  directory_struct->inode = open_directory; // 4. In the struct, set the inode pointer to point to the inode returned by iget().

  directory_struct->offset = 0; // 5. Initialize offset to 0.

  return directory_struct; // Return the pointer to the struct.
}
// ## Reading a Directory

// So the steps will be:
int directory_get(struct directory *dir, struct directory_entry *ent) {
  if (dir->offset >= dir->inode->size) {// 1. Check the offset against the size of the directory. If the offset is greater-than or equal-to the directory size (in its inode), we must be off the end of the directory. If so, return -1 to indicate that.
    return -1;
  }
  int data_block_index = dir->offset / BLOCK_SIZE; // 2. Compute the block in the directory we need to read. The directory file itself might span multiple data blocks if there are enough entries in it. Remember that a block only holds 128 entries. (When we just create it, it will only be one block, but we might as well do this math now so it will work later.)

  // 3. We need to read the appropriate data block in so we can extract the directory entry from it.
  // But what we have, the data_block_index, is giving us the index into the block_ptr array in the directory's inode.

  // Sheesh! More indirection; there's a shocker.

  // So in order to get the block number to read, we have to look that up. Luckily, we have the inode in the struct directory * that was passed into this function:

  int data_block_num = dir->inode->block_ptr[data_block_index];
  unsigned char block[BLOCK_SIZE];
  // // and finally...

  bread(data_block_num, block);
  // And there we've read the block containing the directory entry we want into memory.

  int offset_in_block = dir->offset % BLOCK_SIZE; // 4. Compute the offset of the directory entry in the block we just read.

  // We know the absolute offset of where we are in the file (passed in the struct directory.

  // We know the size of the block, 4096 bytes.

  // So we can compute the offset within the block:

  // offset_in_block = offset % 4096;
  ent->inode_num = read_u16(block + offset_in_block); // 5. Extract the directory entry from the raw data in the block into the struct directory_entry * we passed in.

  // Now we have the data in our block array, and we've computed where it starts with offset_in_block. We just have to deserialize it.

  // Use read_16() to extract the inode number and store it in ent->inode_num.

  strcpy(ent->name, (char*)block + offset_in_block + offset_in_block + FILE_OFFSET); // Use strcpy() to extract the file name and store it in ent->name. You might have to do some casting to char *.

  dir->offset += DIRECTORY_ENTRY_SIZE;
  return 0;
  // Just that!
}

// ### Closing a Directory
// The function for closing is:

// void directory_close(struct directory *d)
// It only has to do two things:

// 1. iput() the directory's in-core inode to free it up.

// 2. free() the struct directory.