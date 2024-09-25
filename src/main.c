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
    char* content;
    struct node** children;
    int children_num;
} Node;

Node* create_node(const char* name, int is_dir) {
    Node* new_node = malloc(sizeof(Node));

    strcpy(new_node->name, name);
    new_node->is_dir = is_dir;
    new_node->parent = NULL;
    new_node->content = (is_dir) ? NULL : calloc(MAX_CONTENT_SIZE, sizeof(char));
    new_node->children = (is_dir) ? calloc(MAX_CHILDREN_NUM, sizeof(Node*)) : NULL;
    new_node->children_num = (is_dir) ? 0 : -1;

    return new_node;
}

Node* root;

// Inicializa a raiz do sistema de arquivos
void init_root() {
    root = create_node("/", 1);
}

Node* find_node(const char* path) {
    if (strcmp("/", path) == 0) return root;

    char path_copy[256];
    strcpy(path_copy, path);

    char* token = strtok(path_copy, "/");

    Node* rt = root;

    while (token != NULL) {
        for (int i = 0; i < MAX_CHILDREN_NUM; i++) {
            if (rt->children[i] && strcmp(rt->children[i]->name, token) == 0) {
                rt = rt->children[i];

                break;
            }
        }

        if (strcmp(rt->name, token) == 0) {
            token = strtok(NULL, "/"); // Pega o próximo componente
        } else {
            return NULL;
        }
    }

    return rt;
}

// Acha o nome do diretório atual, baseado no caminho especificado
char* find_current_directory_name(const char* path) {
    if (strcmp("/", path) == 0) {
        return NULL;
    }

    // Copy the path since we will modify it
    static char parent_dir[256];
    strcpy(parent_dir, path);

    // Find the last occurrence of '/'
    char* last_slash = strrchr(parent_dir, '/');
    
    if (last_slash == NULL || last_slash == parent_dir) {
        return "/";  // The file is on root
    }

    // Remove the last component (file or directory name) by terminating the string at the last '/'
    *last_slash = '\0';

    // Return the parent directory
    return parent_dir;
}

// Basicamente pega o último nome do caminho
const char* find_name(const char* path) {
    // Find the last occurrence of '/'
    const char* last_slash = strrchr(path, '/');
    
    // If no slash is found, the entire path is the file name
    if (!last_slash) {
        return path;
    }

    // If the path ends with '/', return an empty string
    if (*(last_slash + 1) == '\0') {
        return "";
    }

    // Return the part of the string after the last '/'
    return last_slash + 1;
}

// Adiciona um nó (arquivo/diretório) em um diretório
int add_node(Node* target, Node* node) {
    if (!target->is_dir) {
        printf("Target is not a directory\n");

        return 0;
    }

    if (target->children_num == MAX_CHILDREN_NUM) {
        printf("Target is full\n");

        return 0;
    }

    for (int i = 0; i < MAX_CHILDREN_NUM; i++) {
        if (!target->children[i]) {
            target->children[i] = node;
            node->parent = target;
            target->children_num++;

            return 1;
        }
    }

    return 0;
}

// Remove um nó (arquivo/diretório) de um diretório
void remove_node(Node* target) {
    Node* parent = target->parent;

    for (int i = 0; i < MAX_CHILDREN_NUM; i++) {
        if (parent->children[i] && strcmp(parent->children[i]->name, target->name) == 0) {
            free(parent->children[i]);
            parent->children[i] = NULL;
            parent->children_num--;

            return;
        }
    }
}

// Escreve no arquivo
void write_to_file(const char* path, const char* new_content) {
    Node* file = find_node(path);

    if (!file || file->is_dir) {
        return;
    }

    strcpy(file->content, new_content);
}

// Função chamada pelo FUSE para buscar informações de um arquivo/diretório
static int agfs_getattr(const char* path, struct stat* st) {
    st->st_uid = getuid(); // The owner of the file/directory is the user who mounted the filesystem
    st->st_gid = getgid(); // The group of the file/directory is the same as the group of the user who mounted the filesystem
    st->st_atime = time(NULL); // The last "a"ccess of the file/directory is right now
    st->st_mtime = time(NULL); // The last "m"odification of the file/directory is right now

    printf("agfs_getattr: Path: %s\n", path);

    Node* node = find_node(path);

    if (!node) {
        printf("No such file, baby\n");
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

// Função chamada pelo FUSE para ler o conteúdo de um diretório
static int agfs_readdir(
    const char* path, 
    void* buffer, 
    fuse_fill_dir_t filler, 
    off_t offset, 
    struct fuse_file_info* fi
) {
    printf("agfs_readdir: Path: %s\n", path);

    filler(buffer, ".", NULL, 0); // Diretório atual
    filler(buffer, "..", NULL, 0); // Diretório pai

    Node* directory = find_node(path);

    if (directory && directory->is_dir) {
        for (int i = 0; i < MAX_CHILDREN_NUM; i++) {
            if (directory->children[i]) {
                filler(buffer, directory->children[i]->name, NULL, 0);
            }

        }
    }

    return 0;
}

// Função chamada pelo FUSE para remover um diretório
static int agfs_rmdir(const char* path) {
    Node* dir = find_node(path);

    if (!dir || !dir->is_dir) {
        return -ENOENT; // Directory not found
    }

    if (dir->children_num > 0) {
        return -ENOTEMPTY;
    }

    remove_node(dir);

    return 0;
}

// Função chamada pelo FUSE para ler o conteúdo de um arquivo
static int agfs_read(
    const char* path,
    char* buffer,
    size_t size,
    off_t offset,
    struct fuse_file_info* fi
) {
    printf("agfs_read: Path: %s\n", path);

    Node* file = find_node(path);

    if (!file || file->is_dir) {
        return -ENOENT;
    }

    memcpy(buffer, file->content + offset, size);

    return strlen(file->content) - offset;
}

// Função chamada pelo FUSE para escrever em um arquivo
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

static int agfs_mkdir(const char* path, mode_t mode) {
    printf("agfs_mkdir: Path: %s\n", path);

    Node* parent = find_node(find_current_directory_name(path));

    if (!parent) {
        return -ENOENT;
    }

    const char* new_dir_name = find_name(path);

    printf("The name is: %s\n", new_dir_name);

    int ret = add_node(parent, create_node(new_dir_name, 1));

    if (!ret) {
        return -ENOMEM;
    }

    return 0;
}

// Função chamada pelo FUSE para criar um arquivo
static int agfs_mknod(const char* path, mode_t mode, dev_t rdev) {
    printf("agfs_mknod: Path: %s\n", path);

    Node* parent = find_node(find_current_directory_name(path));

    const char* new_file_name = find_name(path);

    add_node(parent, create_node(new_file_name, 0));

    return 0;
}

// Função chamada pelo FUSE para apagar um arquivo
static int agfs_unlink(const char* path) {
    printf("agfs_unlink: Path: %s\n", path);

    Node* target = find_node(path);

    if (!target || target->is_dir) {
        return -ENOENT;
    }

    remove_node(target);

    return 0;
}

// Função chamada pelo FUSE para atribuir os horários dos arquivos
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

// Estrutura com as operações do FUSE, para serem chamadas durante a execução
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
    init_root();
    
    return fuse_main(argc, argv, &operations, NULL);
}
