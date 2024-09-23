/**
 * License: GNU GPL
 */

#define FUSE_USE_VERSION 30

#include <fuse/fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

char directories[256][256];
int last_dir_idx = -1;

char files[256][256];
int last_file_idx = -1;

char files_content[256][256];
int last_file_content_idx = -1;

void print_dir_list() {
    printf("Dir list:\n");

    for (size_t i = 0; i < 10; i++) {
        printf("%zu = %s\n", i, directories[i]);
    }
}

void print_file_list() {
    printf("File list:\n");

    for (size_t i = 0; i < 10; i++) {
        printf("%zu = %s\n", i, files[i]);
    }
}

void add_dir(const char* dir_name) {
    for (int i = 0; i < last_dir_idx; i++) {
        if (directories[i][0] == '\0') {
            strcpy(directories[i], dir_name);

            print_dir_list();
            return;
        }
    }

    last_dir_idx++;
    strcpy(directories[last_dir_idx], dir_name);

    print_dir_list();
}

int rm_dir(const char* dir_name) {
    for (int i = 0; i <= last_dir_idx; i++) {
        if (strcmp(dir_name, directories[i]) == 0) {
            directories[i][0] = '\0';

            if (last_dir_idx == i) {
                last_dir_idx--;
            }

            return 1;
        }
    }
}

int rm_file(const char* file_name) {
    for (int i = 0; i <= last_file_idx; i++) {
        if (strcmp(file_name, files[i]) == 0) {
            files[i][0] = '\0';

            if (last_file_idx == i) {
                last_file_idx--;
            }

            return 1;
        }
    }
}

static int agfs_rmdir(const char* path) {
    // after, check if directory is empty. If not, return -ENOTEMPTY
    if (!rm_dir(path + 1)) {
        return -ENOENT; // Directory not found
    }

    return 0;
}

int is_dir(const char* path) {
    path++; // Eliminating "/" in the path

    for (int curr_idx = 0; curr_idx <= last_dir_idx; curr_idx++ )
        if (strcmp(path, directories[curr_idx]) == 0) {
            return 1;
        }

    return 0;
}

void add_file(const char* file_name) {
    for (int i = 0; i < last_file_idx; i++) {
        if (files[i][0] == '\0') {
            strcpy(files[i], file_name);
            strcpy(files_content[i], "");

            print_file_list();

            return;
        }
    }

    last_file_idx++;
    strcpy(files[last_file_idx], file_name);

    print_file_list();
}

int is_file(const char* path) {
    path++; // Eliminating "/" in the path

    for (int i = 0; i <= last_file_idx; i++)
        if (strcmp(path, files[i]) == 0)
            return 1;

    return 0;
}

int get_file_index(const char* path) {
    path++; // Eliminating "/" in the path

    for (int i = 0; i <= last_file_idx; i++)
        if (strcmp(path, files[i]) == 0)
            return i;

    return -1;
}

void write_to_file(const char* path, const char* new_content) {
    int file_idx = get_file_index(path);

    if (file_idx == -1) // No such file
        return;

    strcpy(files_content[file_idx], new_content);
}

static int agfs_getattr(const char* path, struct stat* st) {
    st->st_uid = getuid(); // The owner of the file/directory is the user who mounted the filesystem
    st->st_gid = getgid(); // The group of the file/directory is the same as the group of the user who mounted the filesystem
    st->st_atime = time(NULL); // The last "a"ccess of the file/directory is right now
    st->st_mtime = time(NULL); // The last "m"odification of the file/directory is right now

    if (strcmp(path, "/") == 0 || is_dir(path) == 1) {
        st->st_mode = S_IFDIR | 0755;
        st->st_nlink = 2;
    } else if (is_file(path) == 1) {
        st->st_mode = S_IFREG | 0644;
        st->st_nlink = 1;
        st->st_size = 1024;
    } else {
        return -ENOENT;
    }

    return 0;
}

static int agfs_readdir(
    const char* path, 
    void* buffer, 
    fuse_fill_dir_t filler, 
    off_t offset, 
    struct fuse_file_info* fi
) {
    filler(buffer, ".", NULL, 0); // Current Directory
    filler(buffer, "..", NULL, 0); // Parent Directory

    // If the user is trying to show the files/directories of the root directory show the following
    if (strcmp(path, "/") == 0) {
        for (int curr_idx = 0; curr_idx <= last_dir_idx; curr_idx++) {
            if (directories[curr_idx][0] != '\0') {
                filler(buffer, directories[curr_idx], NULL, 0);
            }
        }

        for (int curr_idx = 0; curr_idx <= last_file_idx; curr_idx++) {
            if (files[curr_idx][0] != '\0') {
                filler(buffer, files[curr_idx], NULL, 0);
            }
        }
    }

    return 0;
}

static int agfs_read(
    const char* path,
    char* buffer,
    size_t size,
    off_t offset,
    struct fuse_file_info* fi
) {
    int file_idx = get_file_index(path);

    if (file_idx == -1) return -1;

    char* content = files_content[file_idx];

    memcpy(buffer, content + offset, size);

    return strlen(content) - offset;
}

static int agfs_mkdir(const char* path, mode_t mode) {
    path++;
    add_dir(path);

    return 0;
}

static int agfs_mknod(const char* path, mode_t mode, dev_t rdev) {
    path++;
    add_file(path);

    return 0;
}

static int agfs_unlink(const char* path) {
    if (!rm_file(path + 1)) {
        return -ENOENT; // File not found
    }

    return 0;
}

static int agfs_write(
    const char* path,
    const char* buffer,
    size_t size,
    off_t offset, struct fuse_file_info* info
) {
    write_to_file(path, buffer);

    return (int) size;
}

static int agfs_utimens(
    const char* path,
    const struct timespec tv[2]
) {
    // printf("utimens called for %s\n", path);  // Debug print
    if (1) {
        // File exists, update the times
        return 0; // Success, but in this example, we don't store the actual times.
    }

    return -ENOENT; // File not found
}

static struct fuse_operations operations = {
    .getattr = agfs_getattr,
    .readdir = agfs_readdir,
    .read = agfs_read,
    .mkdir = agfs_mkdir,
    .rmdir = agfs_rmdir,
    .mknod = agfs_mknod,
    .unlink = agfs_unlink,
    .write = agfs_write,
    .utimens = agfs_utimens,
};

int main(int argc, char* argv[]) {
    return fuse_main(argc, argv, &operations, NULL);
}
