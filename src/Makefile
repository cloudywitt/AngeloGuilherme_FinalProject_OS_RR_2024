COMPILER = gcc
FILESYSTEM_FILES = main.c

build: $(FILESYSTEM_FILES)
	$(COMPILER) $(FILESYSTEM_FILES) -o agfs `pkg-config fuse --cflags --libs`
	./agfs mnt_point

debug: $(FILESYSTEM_FILES)
	$(COMPILER) $(FILESYSTEM_FILES) -o agfs `pkg-config fuse --cflags --libs`
	./agfs -f mnt_point
clean:
	rm agfs
	umount mnt_point
