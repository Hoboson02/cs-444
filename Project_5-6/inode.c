#include "inode.h"
#include "block.h"
#include "free.h"
#include <stdlib.h>

int get_block_num(inode_num) {
  return inode_num / INODES_PER_BLOCK + INODE_FIRST_BLOCK;
}

int get_block_offset(inode_num) {
  return inode_num % INODES_PER_BLOCK;
}

int get_block_offset_bytes(block_offset) {
  return block_offset * INODE_SIZE;
}

int get_flags(block, block_offset_bytes) {
  return read_u8(block + block_offset_bytes + 7);
}

static struct inode incore[MAX_SYS_OPEN_FILES] = {0};

int ialloc(void) {
  unsigned char inode_map[BLOCK_SIZE];
  bread(FREE_INODE_BLOCK_NUM, inode_map);
  int low_free_bit = find_free(inode_map);
  if(low_free_bit != -1) {
    set_free(inode_map, low_free_bit, 1);
    bwrite(FREE_INODE_BLOCK_NUM ,inode_map);
  }
  return low_free_bit;
}


// ----------Finding an inode and Reading Data-------------------------------------------------------------------------------------------

  // Think of the inode blocks (there are 4 of them) on disk like a contiguous array of inodes.

  // So if I give you an index into that array (i.e. an inode number), you'll have to determine two things: the block number that holds that inode, and the offset within the block where it begins.


  // int block_num = inode_num / INODES_PER_BLOCK + INODE_FIRST_BLOCK;
  // And all we need is the remainder to get the exactimage.png inode number within that block:

  // int block_offset = inode_num % INODES_PER_BLOCK;
  // That gives us the offset (in inodes!) of where we can find inode_num in the block.

  // But we probably want an offset in bytes.

  // Each inode is 64 bytes, so we can get the answer easily enough:

  // int block_offset_bytes = block_offset * INODE_SIZE;
  // And now we have the byte offset within the block where the inode begins.

  // So if I want to read the flags, which I see from the above table is at offset 7 within the inode, I can:

  // // Assuming `block` is the array we read with `bread()`

  // int flags = read_u8(block + block_offset_bytes + 7);


// ----------In-Core inodes-------------------------------------------------------------------------------------------
// Now we need to write two functions:
struct inode *find_incore_free(void) {
  for (int i = 0; i <MAX_SYS_OPEN_FILES; i++) {
    if (incore[i].flags == 0) {   // If the ref_count field in the struct inode is 0, it's not being used.
      return &incore[i]; // This finds the first free in-core inode in the incore array. It returns a pointer to that in-core inode
    }
  }
  return NULL; // or NULL if there are no more free in-core inodes.
}

struct inode *find_incore(unsigned int inode_num) {
  for (int i = 0; i <MAX_SYS_OPEN_FILES; i++) {
    if(incore[i].ref_count != 0 && incore[i].inode_num == inode_num) { // So find_incore() will search the array until it finds a struct inode with a non-zero ref_count AND the inode_num field matches the number passed to the function. Luckily, we have a field in the struct inode: inode_num. We just have to match this.
      return &incore[i]; //It returns a pointer to that in-core inode 
    }
  }
  return NULL; // or NULL if it can't be found.
}

// ----------Reading and Writing inodes from Memory and Disk-------------------------------------------------------------------------------------------
  // We're going to write two functions:
void read_inode(struct inode *in, int inode_num) {
  int block_num = get_block_num(inode_num); // You'll have to map that inode number to a block
  int block_offset_bytes = get_block_offset_bytes(inode_num); // and offset, as per above.
  unsigned char *free_block = calloc(BLOCK_SIZE, sizeof(unsigned char));

  bread(block_num, free_block); // Then you'll read the data from disk into a block

  in->size = read_u32(free_block, block_offset_bytes);
  in->owner_id = read_u16(free_block, block_offset_bytes + 4);
  in->permissions = read_u8(free_block, block_offset_bytes + 6);
  in->flags = read_u8(free_block, block_offset_bytes + 7);
  in->link_count = read_u8(free_block, block_offset_bytes + 8);
  for (int i = 0; i < INODE_PTR_COUNT; i++) {
    in->block_ptr[i] = read_u16(free_block + block_offset_bytes + 9 + (i*2));
  }
}

void write_inode(struct inode *in) {
  // This stores the inode data pointed to by in on disk. The inode_num field in the struct holds the number of the inode to be written.
  // You'll have to map that inode number to a block and offset, as per above.
  // Then you'll read the data from disk into a block, and pack the new inode data with the functions from pack.c. And lastly you'll write the updated block back out to disk.
}  

// ----------Higher-Level Functions: iget()-------------------------------------------------------------------------------------------

  // The iget() function's purpose is to return a pointer to an in-core inode for a given inode number.

  // But wait—didn't we just write that with read_inode()? Not quite. That function doesn't actually know anything about in-core inodes; it just writes to whatever pointer you pass in.

  // iget() will glue this stuff together.

  // Here's the function signature (which you should add to inode.h):
    // struct inode *iget(int inode_num): Return a pointer to an in-core inode for the given inode number, or NULL on failure.
  
  // The algorithm is this:
    // Search for the inode number in-core (find_incore())
    // If found:
    // Increment the ref_count
    // Return the pointer
    // Find a free in-core inode (find_incore_free())
    // If none found:
    // Return NULL
    // Read the data from disk into it (read_inode())
    // Set the inode's ref_count to 1
    // Set the inode's inode_num to the inode number that was passed in
    // Return the pointer to the inode

  // So what it does is gives you the inode one way or another. If the inode was already in-core, it just increments the reference count and returns a pointer.

  // If the inode wasn't already in-core, it allocates space for it, loads it up, sets the ref_count to 1, and returns the pointer.


// ----------Higher-Level Functions: iput()-------------------------------------------------------------------------------------------

  // This is the opposite of iget(). It effectively frees the inode if no one is using it.

    // void iput(struct inode *in): decrement the reference count on the inode. If it falls to 0, write the inode to disk.
  // Algorithm:

    // If ref_count on in is already 0:
      // Return
    // Decrement ref_count
    // If ref_count is 0:
      // Save the inode to disk (write_inode())
  // That's it.

  // TEST! TEST! TEST!