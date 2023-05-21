.PHONY: test

simfs.a: image.o block.o free.o inode.o mkfs.o
	ar rcs $@ $^

image.o: image.c image.h
	gcc -Wall -Wextra -c $<

block.o: block.c block.h
	gcc -Wall -Wextra -c $<

free.o: free.c free.h
	gcc -Wall -Wextra -c $<

inode.o: inode.c inode.h
	gcc -Wall -Wextra -c $<

mkfs.o: mkfs.c mkfs.h
	gcc -Wall -Wextra -c $<

simfs_test: simfs_test.c simfs.a
	gcc -Wall -Wextra -o $@ $^

test: simfs_test
	./simfs_test