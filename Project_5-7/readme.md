# Project 7 Instructions

## Introduction
Remember:

Understand this spec completely before writing a line of code. DM questions early and often.

Don't hardcode any magic numbers!

So far we've just been building the lower-level building blocks of the file system. Now, at last, it's time to construct something tangible.

We're going to add the root directory to the file system! And we're going to create functions to access those directory entries.

The big picture:

Create the root directory--this will be the job of mkfs().
Write a function to "open" the directory so you can read the entries.
Write a function that will read directory entries.
Write a function that will close the directory.
Use those functions to write a function called ls() that will show the entries in the root directory.
In a later project, we'll implement an algorithm to find the inode of an item at the end of a path, like the inode for frobozz on the path /foo/bar/baz/frobozz.

## Backstory
A riveting tale of directory construction...

### Directories
A directory is a special type of file. It's entire purpose is to map a file name to an inode number for that file.

It holds fixed-length records in its data blocks. There are no "holes" in this data--the entries are packed back-to-back.

Each fixed length record of 32 bytes that looks like this:

Directory Entry Layout
Offset	Length	Field	Type
0	2	inode number	Big-endian 16-bit integer
2	16	File name, up to 15 characters	Null-terminated string
18	14	Reserved	
Since each data block is 4096 bytes, there are 4096/32 or 128 directory entries per block.

### Directory Metadata
The directory inode has a couple interesting features.

The flags field needs to be set to represent that this is a directory as opposed to a regular user file.

We set it to the value 2 to indicate this:

inodes flag values
flags	Description
0	Indicates unknown type
1	File is a regular file
2	File is a directory
The size field is the number of bytes taken up by all used directory entries in this directory.

For example, if the directory contained 377 items, the total size of the directory would be 377×32 (32 bytes per entry) which is 12064.

We can use division to compute the number of entries based on the size.

### Directory Entries
Normally we'd add directory entries when a user creates a new file.

But in this case, there are two "hard-coded" entries that every directory gets: . and ...

The . means "this directory", and the .. means the parent directory.

Remember that a directory entry just maps a name (like ..) to the file/directory it refers to.

In the case of the root directory, . just maps to the root inode itself.

But what does .. map to in the root? What's the parent of /? In that case, we'll just also map .. to the root inode itself. This way you just keep looping at the root and can't go "past" it.l

Try it in Unix: cd /../../../../../../.. and you'll still be in the / directory.

Later we'll look at ways to add other entries besides these two hard-coded ones.

## Create the Root Directory
mkfs() will be expanded by this project. In addition to its previous abilities, it will also need to make the root directory.

In order to create a new file of any kind, including a directory, we need to:

Get an inode for that file (ialloc()).
Get a data block to hold the hardcoded entries (alloc()).
Initialize the inode to have the correct metadata.
Write the directory entries to an in-memory block.
Write the block out to disk.
Broken down:

Call ialloc() to get a new inode.

We'll need the inode_num later to add it to the directory!

Call alloc() to get a new data block.

This will hold the directory entries.

Be sure to note the returned block number because we're going to need to write to it in just a minute!

Initialize the inode returned from ialloc(), above.

flags needs to be set to 2.

size needs to be set to the byte size of the directory. Since we have two entries (. and ..) and each is 32 bytes, the size must be 64 bytes.

block_ptr[0] needs to point to the data block we just got from alloc(), above.

Make an unsigned char block[BLOCK_SIZE] array that you can populate with the new directory data.

We're going to pack the . and .. directory entries in here.

Add the directory entries. You're going to need to math it out.

You know the first two-byte value is the inode number, so you can pack that in with write_16(). Remember that for the root inode, both the . and .. inode numbers are the same as the root inode itself (i.e. the inode_num you got back from ialloc()).

The next up-to-16 bytes are the file name. You can copy that in with strcpy(). (You might have to cast some arguments to char*.)

You have to do this process for both . and .. entries.

Compute the offsets into your in-memory block and copy the data there.

Write the directory data block back out to disk with bwrite().

Call iput() to write the new directory inode out to disk and free up the in-core inode.

At this point, we should have a root directory. But other than doing a raw iget() and bread() to get the block, we don't have a nice way of interfacing with it.

Also, the root directory inode number should be 0, since it's the first one we've allocated.

## Directory Open/Read/Close Ops
Unix-like systems include this functionality that's analogous to opening, reading, and closing a regular file.

Except in this case we open a directory, read entries from the directory, and close the directory.

When we open the directory, we'll get a struct pointer back representing the open directory. We'll pass that pointer to the function to read a directory entry. We can call this function repeatedly to read subsequent directory items. Then when we're done, we'll call the close function on the directory.

These functions are read-only. There's no way to write to a directory using them. (The directory is modified by the OS when you create or delete a file.)

### Opening a Directory
Here is the call to open a directory:

struct directory *directory_open(int inode_num)
We pass in an inode number and it should give us a struct back representing the open directory, or NULL if there was an error.

For now, you can just pass 0 in for the inode_num, since that's the root (and only) directory on the file system so far.

Later we'll modify this function to take a path name instead of an inode number, but that's for a different project.

What is that struct?

It's this:

struct directory {
    struct inode *inode;
    unsigned int offset;
};
It has a pointer to the directory (root's) inode. It also has an offset, which indicates how far into this directory we've read so far. It starts off at 0, since we haven't read any entries from it yet.

So the process is:

Use iget() to get the inode for this file.

If it fails, directory_open() should return NULL.

malloc() space for a new struct directory.

In the struct, set the inode pointer to point to the inode returned by iget().

Initialize offset to 0.

Return the pointer to the struct.

## Reading a Directory
This is where the fun is. The idea is that we're going to call this function over and over and each time it will give us the next entry in this open directory.

int directory_get(struct directory *dir, struct directory_entry *ent)
This function takes a struct directory * obtained from directory_open(), and it takes a pointer to a struct directory_entry that it will fill out with the entry information.

It returns 0 on success, and -1 on failure. (A failure would be trying to read off the end of the directory—more on that in a minute.)

What's in a struct directory_entry? It's the stuff that's in the directory entry, unsurprisingly: the name of the file and its corresponding inode number.

struct directory_entry {
    unsigned int inode_num;
    char name[16];
};
(Remember at first we only have two files: . and ...)

So what's the plan?

It's kind of weird to be able to call this function repeatedly and have it return the next entry each time.

Somehow it will have to keep track of the current position (or "offset") in the directory where we're about to read the next entry.

And then each time we call it, we can update that tracked position so that the next time we call, we get the next entry, and so on.

Where shall we store it? Well, if you remember, the struct directory we got from directory_open() has an offset field in it! And we have to pass such an object into directory_get() each time we call it. That's where we can store that state so each call can "remember" where it was.

So the steps will be:

Check the offset against the size of the directory. If the offset is greater-than or equal-to the directory size (in its inode), we must be off the end of the directory. If so, return -1 to indicate that.

Compute the block in the directory we need to read. The directory file itself might span multiple data blocks if there are enough entries in it. Remember that a block only holds 128 entries. (When we just create it, it will only be one block, but we might as well do this math now so it will work later.)

If each block is 4096 bytes, and we're at offset 4800, we must be past the end of block 0 and we're somewhere in block 1. Good old divide and modulo to the rescue!

data_block_index = offset / 4096;
Beware! this isn't the absolute block number on disk. Don't just feed this number into bread()! See the next step, below!

We need to read the appropriate data block in so we can extract the directory entry from it.

But what we have, the data_block_index, is giving us the index into the block_ptr array in the directory's inode.

Sheesh! More indirection; there's a shocker.

So in order to get the block number to read, we have to look that up. Luckily, we have the inode in the struct directory * that was passed into this function:

data_block_num = dir->inode->block_ptr[data_block_index];

// and finally...

bread(data_block_num, block);
And there we've read the block containing the directory entry we want into memory.

Compute the offset of the directory entry in the block we just read.

We know the absolute offset of where we are in the file (passed in the struct directory.

We know the size of the block, 4096 bytes.

So we can compute the offset within the block:

offset_in_block = offset % 4096;
Extract the directory entry from the raw data in the block into the struct directory_entry * we passed in.

Now we have the data in our block array, and we've computed where it starts with offset_in_block. We just have to deserialize it.

Use read_16() to extract the inode number and store it in ent->inode_num.

Use strcpy() to extract the file name and store it in ent->name. You might have to do some casting to char *.

Just that!

### Closing a Directory
The function for closing is:

void directory_close(struct directory *d)
It only has to do two things:

iput() the directory's in-core inode to free it up.

free() the struct directory.

## A Simple ls Function
We might as well put it all to use. Let's write ls(). (You can make a new ls.c file for this.)

The plan here is pretty straightforward for a change.

We're going to:

Open the directory.
Read the entries in a loop until the reader returns -1.
Print out each one in the loop.
Close the directory once the loop is complete.
Code is something like this (untested):

void ls(void)
{
    struct directory *dir;
    struct directory_entry ent;

    dir = directory_open(0);

    while (directory_get(dir, &ent) != -1)
        printf("%d %s\n", ent.inode_num, ent.name);

    directory_close(dir);
}
If it's working, you should see:

0 .
0 ..
And those are all the files in the root directory!

Note: you should definitely write tests for the directory functions, but you don't need to write tests for ls() specifically. It's just there to show you the results of all your handiwork!