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

#define MAX_NAME_SIZE 32
#define MAX_CHILDREN_NUM 20
#define MAX_CONTENT_SIZE 126

typedef struct node {
    char name[MAX_NAME_SIZE];
    int is_dir;
    struct node* parent;
    // if file
    char** content;
    // if dir
    struct node** children;
    int children_num;
} Node;

Node* create_node(const char* name, int is_dir) {
    Node* new_node = malloc(sizeof(Node));

    strcpy(new_node->name, name);
    new_node->is_dir = is_dir;
    new_node->parent = NULL;
    new_node->content = (is_dir) ? NULL : malloc(sizeof(char) * MAX_CONTENT_SIZE);
    new_node->children = (is_dir) ? calloc(MAX_CHILDREN_NUM, sizeof(Node)) : NULL; // might give some trouble
    new_node->children_num = (is_dir) ? 0 : -1;

    return new_node;
}

Node* root;

void init_root() {
    root = create_node("/", 1);
}

// find node to getattr
Node* find_node(const char* path) {
    if (strcmp("/", path) == 0) return root;

    Node* rt = root;



}

char files[256][256];
int last_file_idx = -1;

char files_content[256][256];
int last_file_content_idx = -1;

// void print_dir_list(Node dir) {
//     printf("Dir list:\n");
//
//     for (size_t i = 0; i < 10; i++) {
//         printf("%zu = %s\n", i, directories[i]);
//     }
// }

void print_file_list() {
    printf("File list:\n");

    for (size_t i = 0; i < 10; i++) {
        printf("%zu = %s\n", i, files[i]);
    }
}

// dir functions

// void add_dir(const char* dir_name) {
//     for (int i = 0; i < last_dir_idx; i++) {
//         if (directories[i][0] == '\0') {
//             strcpy(directories[i], dir_name);
//
//             print_dir_list();
//             return;
//         }
//     }
//
//     last_dir_idx++;
//     strcpy(directories[last_dir_idx], dir_name);
//
//     print_dir_list();
// }

// int rm_dir(const char* dir_name) {
//     for (int i = 0; i <= last_dir_idx; i++) {
//         if (strcmp(dir_name, directories[i]) == 0) {
//             directories[i][0] = '\0';
//
//             if (last_dir_idx == i) {
//                 last_dir_idx--;
//             }
//
//             return 1;
//         }
//     }
// }

// file functions
int is_file(const char* path) {
    path++; // Eliminating "/" in the path

    for (int i = 0; i <= last_file_idx; i++)
        if (strcmp(path, files[i]) == 0)
            return 1;

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

// fuse operations
static int agfs_getattr(const char* path, struct stat* st) {
    st->st_uid = getuid(); // The owner of the file/directory is the user who mounted the filesystem
    st->st_gid = getgid(); // The group of the file/directory is the same as the group of the user who mounted the filesystem
    st->st_atime = time(NULL); // The last "a"ccess of the file/directory is right now
    st->st_mtime = time(NULL); // The last "m"odification of the file/directory is right now

    printf("agfs_getattr: Path: %s\n", path);

    Node* node = find_node(path);

    if (!node) {
        return -ENOENT; // No file or directory
    }

    if (node->is_dir) {
        st->st_mode = S_IFDIR | 0755;
        st->st_nlink = 2;
    } else if (!node->is_dir) {
        st->st_mode = S_IFREG | 0644;
        st->st_nlink = 1;
        st->st_size = 1024;
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
    printf("agfs_readdir: Path: %s\n", path);

    filler(buffer, ".", NULL, 0); // Current Directory
    filler(buffer, "..", NULL, 0); // Parent Directory

    // If the user is trying to show the files/directories of the root directory show the following
    // find the directories of that specific directory and list them
    // this function, readdir, is called upon the directory we're in
    Node* directory = find_node(path);

    // might have to check if I can have a dir not found in this case
    if (directory && directory->is_dir) {
        // filler every entry of that dir

        for (int i = 0; i < MAX_CHILDREN_NUM; i++) {
            if (directory->children[i]) {
                filler(buffer, directory->children[i]->name, NULL, 0);
            }

        }

        // for (int curr_idx = 0; curr_idx <= last_dir_idx; curr_idx++) {
        //     if (directories[curr_idx][0] != '\0' && strcmp(path + 1, directories[curr_idx])) {
        //         filler(buffer, directories[curr_idx], NULL, 0);
        //     }
        // }

        // for (int curr_idx = 0; curr_idx <= last_file_idx; curr_idx++) {
        //     if (files[curr_idx][0] != '\0') { // && strcmp(path + 1, files[curr_idx])
        //         filler(buffer, files[curr_idx], NULL, 0);
        //     }
        // }
    }

    return 0;
}

// static int agfs_rmdir(const char* path) {
//     // after, check if directory is empty. If not, return -ENOTEMPTY
//     if (!rm_dir(path + 1)) {
//         return -ENOENT; // Directory not found
//     }
//
//     return 0;
// }

static int agfs_read(
    const char* path,
    char* buffer,
    size_t size,
    off_t offset,
    struct fuse_file_info* fi
) {
    printf("agfs_read: Path: %s\n", path);
    int file_idx = get_file_index(path);

    if (file_idx == -1) return -1;

    char* content = files_content[file_idx];

    memcpy(buffer, content + offset, size);

    return strlen(content) - offset;
}

// static int agfs_mkdir(const char* path, mode_t mode) {
//     printf("agfs_mkdir: Path: %s\n", path);
//     path++;
//     add_dir(path);
//
//     return 0;
// }

static int agfs_mknod(const char* path, mode_t mode, dev_t rdev) {
    printf("agfs_mknod: Path: %s\n", path);
    path++;
    add_file(path);

    return 0;
}

static int agfs_unlink(const char* path) {
    printf("agfs_unlink: Path: %s\n", path);
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
    printf("agfs_write: Path: %s\n", path);
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
    // .mkdir = agfs_mkdir,
    // .rmdir = agfs_rmdir,
    .mknod = agfs_mknod,
    .unlink = agfs_unlink,
    .write = agfs_write,
    .utimens = agfs_utimens,
};

int main(int argc, char* argv[]) {
    init_root();
    
    return fuse_main(argc, argv, &operations, NULL);
}
