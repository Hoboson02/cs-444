#include "inode.h"
#include "block.h"
#include "free.h"
#include "pack.h"
#include "dirbasename.h"
#include "mkfs.h"
#include <stdlib.h>

int get_block_num(int inode_num) {
  return inode_num / INODES_PER_BLOCK + INODE_FIRST_BLOCK;
}

int get_block_offset(int inode_num) {
  return inode_num % INODES_PER_BLOCK;
}

int get_block_offset_bytes(int block_offset) {
  return block_offset * INODE_SIZE;
}
static struct inode incore[MAX_SYS_OPEN_FILES] = {0};

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
  unsigned char *free_block = calloc(sizeof(unsigned char), BLOCK_SIZE);

  bread(block_num, free_block); // Then you'll read the data from disk into a block

  in->size = read_u32(free_block + block_offset_bytes);
  in->owner_id = read_u16(free_block + block_offset_bytes + 4);
  in->permissions = read_u8(free_block + block_offset_bytes + 6);
  in->flags = read_u8(free_block + block_offset_bytes + 7);
  in->link_count = read_u8(free_block + block_offset_bytes + 8);
  for (int i = 0; i < INODE_PTR_COUNT; i++) {
    in->block_ptr[i] = read_u16(free_block + block_offset_bytes + 9 + (i*2));
  }
}

void write_inode(struct inode *in) {
  int block_num = get_block_num(in->inode_num); // You'll have to map that inode number to a block
  int block_offset_bytes = get_block_offset_bytes(in->inode_num); // and offset, as per above.
  unsigned char *free_block = calloc(sizeof(unsigned char), BLOCK_SIZE);

  bread(block_num, free_block); // Then you'll read the data from disk into a block

  write_u32(free_block + block_offset_bytes, in->size);
  write_u16(free_block + block_offset_bytes + 4, in->owner_id);
  write_u8(free_block + block_offset_bytes + 6, in->permissions);
  write_u8(free_block + block_offset_bytes + 7, in->flags);
  write_u8(free_block + block_offset_bytes + 8, in->link_count);
  for (int i = 0; i < INODE_PTR_COUNT; i++) {
    write_u16(free_block + block_offset_bytes + 9 + (i*2), in->block_ptr[i]);
  }
  bwrite(block_num, free_block); // And lastly you'll write the updated block back out to disk.
}  

// ----------Higher-Level Functions: iget()-------------------------------------------------------------------------------------------

  // The iget() function's purpose is to return a pointer to an in-core inode for a given inode number.

  // But wait—didn't we just write that with read_inode()? Not quite. That function doesn't actually know anything about in-core inodes; it just writes to whatever pointer you pass in.

  // iget() will glue this stuff together.
struct inode *iget(int inode_num) { // Return a pointer to an in-core inode for the given inode number, or NULL on failure.
  struct inode *incore_node = find_incore(inode_num); // Search for the inode number in-core (find_incore())
  if (incore_node != NULL) { // If found:
    incore_node->ref_count++;  // Increment the ref_count
    return incore_node;  // Return the pointer
  }

  struct inode *incore_free_node = find_incore_free();  // Find a free in-core inode (find_incore_free())
  if (incore_free_node == NULL) {// If none found:
    return NULL; // Return NULL
  }
  
  read_inode(incore_free_node, inode_num);  // Read the data from disk into it (read_inode())
  incore_free_node->ref_count = 1;  // Set the inode's ref_count to 1
  incore_free_node->inode_num = inode_num;  // Set the inode's inode_num to the inode number that was passed in
  return incore_free_node;  // Return the pointer to the inode
}

// ----------Higher-Level Functions: iput()-------------------------------------------------------------------------------------------

  // This is the opposite of iget(). It effectively frees the inode if no one is using it.

void iput(struct inode *in) { // decrement the reference count on the inode. If it falls to 0, write the inode to disk.
  if (in->ref_count == 0) {  // If ref_count on in is already 0:
    return; // Return
  }
  in->ref_count--;  // Decrement ref_count
  if (in->ref_count == 0) { // If ref_count is 0:
    write_inode(in);  // Save the inode to disk (write_inode())
  }
}
// ----------Higher-Level Functions: iput()-------------------------------------------------------------------------------------------
// So ialloc() will just be the same as iget(), with the added functionality that ialloc() will allocate a new inode(), whereas iget() only returns existing inodes.

// Both of the functions will return a pointer to an in-core inode.

struct inode *ialloc(void) {
  unsigned char inode_map[BLOCK_SIZE];
  unsigned char *free_block = calloc(sizeof(unsigned char), BLOCK_SIZE);
  bread(FREE_INODE_BLOCK_NUM, inode_map);
  int free_bit = find_free(free_block); // Save the inode number of the newly-allocated inode (returned by find_free());
  if(free_bit == -1) { // If none are free: Return NULL
    return NULL;
  }

  struct inode* incore = iget(free_bit);  // Get an in-core version of the inode (iget())
  if (incore == NULL) {// If not found:
    return NULL;  // Return NULL
  }

  // Initialize the inode:
  // Set the size, owner ID, permissions, and flags to 0.
  incore->size = 0;
  incore->owner_id = 0;
  incore->permissions = 0;
  incore->flags = 0;

  for (int i = 0; i < INODE_PTR_COUNT; i++) { // Set all the block pointers to 0.
    incore->block_ptr[i] = 0; 
  }

  incore->inode_num = free_bit; // Set the inode_num field to the inode number (from find_free())

  write_inode(incore);  // Save the inode to disk (write_inode())

  return incore; // Return the pointer to the in-core inode.
}

struct inode *namei(char *path) { // Find the inode for the parent directory that will hold the new entry (namei()).
  if (*path == '/') {// If the path is /, it returns the root directory's in-core inode.
    return iget(ROOT_INODE_NUM);
  }
  else {
    return NULL;
  }
  // If the path is /foo, it returns foo's in-core inode.
  // If the path is /foo/bar, it returns bar's in-core inode.
  // If the path is invalid (i.e. a component isn't found), it returns NULL.
  
}

int directory_make(char *path) {
  char *dirname = calloc(MAX_PATH_LENGTH, sizeof(char));
  char *basename = calloc(MAX_PATH_LENGTH, sizeof(char));
  get_dirname(path, dirname); // Find the directory path that will contain the new directory.
  get_basename(path, basename); // Find the new directory name from the path.
  struct inode *parenti = namei(dirname);  // Find the inode for the parent directory that will hold the new entry (namei()).
  if (!parenti) {
    return -1;
  }
  struct inode *newi = ialloc();  // Create a new inode for the new directory (ialloc()).
  struct inode *new_data_block = alloc();  // Create a new data block for the new directory entries (alloc()).
  char new_block[BLOCK_SIZE];  // Create a new block-sized array for the new directory data block and 
  write_u16(new_block, newi->inode_num);  // initialize it . and .. files.
  strcpy((char*)new_block + FILE_OFFSET, ".");
  write_u16(new_block + DIRECTORY_ENTRY_SIZE, parenti->inode_num);
  strcpy((char*)new_block + FILE_OFFSET + DIRECTORY_ENTRY_SIZE, "..");

  newi->flags = DIRECTORY_FLAG; // Initialize the new directory in-core inode with a proper size and other fields, similar to how we did with the hard-coded root directory in the previous project.
  newi->size = DIRECTORY_SIZE;
  newi->block_ptr[0] = new_data_block;

  bwrite(new_data_block, new_block);  // Write the new directory data block to disk (bwrite()).
  int numitems = parenti->size/DIRECTORY_ENTRY_SIZE;  // From the parent directory inode, find the block that will contain the new directory entry (using the size and block_ptr fields).
  int parent_block_num = numitems/MAX_PATH_LENGTH;
  unsigned char parentblock[BLOCK_SIZE];
  bread(parenti->block_ptr[parent_block_num], parentblock); // Read that block into memory unless you're creating a new one (bread()), and add the new directory entry to it.  

  bwrite(parent_block_num, parentblock);  // Write that block to disk (bwrite()).

  parenti->size += 32;  // Update the parent directory's size field (which should increase by 32, the size of the new directory entry.
  
  iput(newi); // Release the new directory's in-core inode (iput()).

  iput(parenti); // Release the parent directory's in-core inode (iput()).

  return 0;
}