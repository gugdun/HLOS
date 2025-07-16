#ifndef _VFS_H
#define _VFS_H

#include <stddef.h>
#include <stdbool.h>

typedef enum {
    VFS_NODE_FILE,
    VFS_NODE_DIR,
    VFS_NODE_SYMLINK,
} vfs_node_type_t;

typedef struct vfs_node {
    char *name;
    vfs_node_type_t type;
    struct vfs_node *parent;

    union {
        struct {
            struct vfs_node **children;
            size_t child_count;
        } dir;
        struct {
            void *data;
            size_t size;
        } file;
        struct {
            char *target;
        } symlink;
    };

    struct vfs_node *next_sibling;
} vfs_node_t;

void vfs_init(void);
vfs_node_t *vfs_create(const char *path, vfs_node_type_t type);
vfs_node_t *vfs_lookup(const char *path);
bool vfs_remove(const char *path);

#endif
