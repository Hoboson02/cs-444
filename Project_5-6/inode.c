#include "inode.h"
#include "block.h"
#include "free.h"
#include <stdlib.h>

int block_num = inode_num / INODES_PER_BLOCK + INODE_FIRST_BLOCK;

int block_offset = inode_num % INODES_PER_BLOCK;

int block_offset_bytes = block_offset * INODE_SIZE;

int flags = read_u8(block + block_offset_bytes + 7);

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
struct inode *find_incore_free(void) {

}

struct inode *find_incore(unsigned int inode_num) {
  
}
  // When processes open files, the OS reads the inode off disk and stores it in an "in-core" inode. These will be our structs, above.

  // The OS has an area of memory set aside for in-core inodes. We're going to make a big array of them and write a couple function to search it.

  // // In inode.h

  // #define MAX_SYS_OPEN_FILES 64
  // // In inode.c at the global scope

  // static struct inode incore[MAX_SYS_OPEN_FILES] = {0};
  // static in this context makes the variable or function only accessible within this source file. This way it doesn't pollute the global namespace for other source files. It's kind of like declaring it "private" to this source file.

  // Variables and functions that are static do not get added to the header file! Other files can't see those functions anyway, so there's no reason to include them.

  // Now we need to write two functions:

    // struct inode *find_incore_free(void): This finds the first free in-core inode in the incore array. It returns a pointer to that in-core inode or NULL if there are no more free in-core inodes.

    // How do we know if an in-core inode is in use? Each one has a reference count, which is loosely how many processes are using it right now. (There are some additional abstraction layers in there, but we'll ignore them for now.)

    // If the ref_count field in the struct inode is 0, it's not being used.

    // So we just have to search through the incore array and find the first one that's free.

    // It's OK to use an O(n) solution for this. In real life, we'd want something better, but linear search is fine for this project.

    // So find_incore_free() will just search the array until it finds a struct inode with ref_count of 0, and return a pointer to that struct.

    // struct inode *find_incore(unsigned int inode_num): This finds an in-core inode record in the incore array by the inode number. It returns a pointer to that in-core inode or NULL if it can't be found.

    // But for finding by inode number, how do we know if a particular struct inode has that number? It's not the same as the index into the incore array!

    // Luckily, we have a field in the struct inode: inode_num. We just have to match this.

    // So find_incore() will search the array until it finds a struct inode with a non-zero ref_count AND the inode_num field matches the number passed to the function.

    // They both return a struct inode *, which should point to one of the elements in the incore array.

  // Protip: TEST, TEST, TEST! You should write a test to find a free inode, then modify it to set some values (including the inode_num) and then you should be able to search for that by number and get the same data back.

  // You want to be confident that these are working before you rely on them!


// ----------Reading and Writing inodes from Memory and Disk-------------------------------------------------------------------------------------------

  // Next, we need to be able to read and write inodes to and from disk.

  // We're going to write two functions:

    // void read_inode(struct inode *in, int inode_num): This takes a pointer to an empty struct inode that you're going to read the data into. The inode_num is the number of the inode you wish to read from disk.

    // You'll have to map that inode number to a block and offset, as per above.

    // Then you'll read the data from disk into a block, and unpack it with the functions from pack.c. And you'll store the results in the struct inode * that was passed in.

    // void write_inode(struct inode *in): This stores the inode data pointed to by in on disk. The inode_num field in the struct holds the number of the inode to be written.

    // You'll have to map that inode number to a block and offset, as per above.

    // Then you'll read the data from disk into a block, and pack the new inode data with the functions from pack.c. And lastly you'll write the updated block back out to disk.

  // TEST! TEST! TEST!


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