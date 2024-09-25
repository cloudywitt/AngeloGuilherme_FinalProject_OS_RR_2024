COMPILER = gcc
FILESYSTEM_FILES = main.c

build: $(FILESYSTEM_FILES)
	$(COMPILER) $(FILESYSTEM_FILES) -o agfs `pkg-config fuse --cflags --libs`
	mkdir mnt_point -p
	./agfs mnt_point

debug: $(FILESYSTEM_FILES)
	$(COMPILER) $(FILESYSTEM_FILES) -o agfs `pkg-config fuse --cflags --libs`
	mkdir mnt_point -p
	./agfs -f mnt_point
clean:
	rm agfs
	fusermount -uz mnt_point
