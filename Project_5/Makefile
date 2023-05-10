.PHONY: test

simfs.a: image.o block.o 
	ar rcs $@ $^

image.o: image.c image.h
	gcc -Wall -Wextra -c $<

block.o: block.c block.h
	gcc -Wall -Wextra -c $<

free.o: free.c free.h
	gcc -Wall -Wextra -c $<

simfs_test: simfs_test.c simfs.a
	gcc -Wall -Wextra -o $@ $^

test: simfs_test
	./simfs_test