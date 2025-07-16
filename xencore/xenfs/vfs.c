#include <string.h>

#ifdef ARCH_x86_64
#include <xencore/arch/x86_64/paging.h>
#endif

#include <xencore/xenfs/vfs.h>
#include <xencore/xenio/tty.h>
#include <xencore/xenmem/xenalloc.h>

static vfs_node_t *vfs_root = (vfs_node_t *)NULL;

static void vfs_add_child(vfs_node_t *parent, vfs_node_t *child)
{
    size_t new_count = parent->dir.child_count + 1;
    size_t new_size = new_count * sizeof(vfs_node_t *);
    vfs_node_t **new_array = xen_alloc(new_size);

    if (parent->dir.child_count > 0) {
        memcpy(new_array, parent->dir.children, parent->dir.child_count * sizeof(vfs_node_t *));
        xen_free(parent->dir.children);
    }

    new_array[parent->dir.child_count] = child;
    parent->dir.children = new_array;
    parent->dir.child_count = new_count;
}

static vfs_node_t *vfs_find_child(vfs_node_t *parent, const char *name)
{
    if (!parent || parent->type != VFS_NODE_DIR) return NULL;

    for (size_t i = 0; i < parent->dir.child_count; i++) {
        if (strcmp(parent->dir.children[i]->name, name) == 0) {
            return parent->dir.children[i];
        }
    }

    return NULL;
}

static void xen_free_node(vfs_node_t *node) {
    if (!node) return;

    xen_free(node->name);

    if (node->type == VFS_NODE_DIR) {
        for (size_t i = 0; i < node->dir.child_count; i++) {
            xen_free_node(node->dir.children[i]);
        }
        xen_free(node->dir.children);
    } else if (node->type == VFS_NODE_FILE) {
        xen_free(node->file.data);
    } else if (node->type == VFS_NODE_SYMLINK) {
        xen_free(node->symlink.target);
    }

    xen_free(node);
}

vfs_node_t *vfs_create(const char *path, vfs_node_type_t type) {
    if (!path || path[0] != '/') return NULL;

    char temp[256];
    strncpy(temp, path, sizeof(temp));
    temp[sizeof(temp) - 1] = '\0';

    vfs_node_t *current = vfs_root;
    char *token = strtok(temp + 1, "/");
    char *next = NULL;

    while (token) {
        next = strtok(NULL, "/");

        vfs_node_t *child = vfs_find_child(current, token);
        if (next == NULL) {
            // Final component — create node if not found
            if (child) return child;

            vfs_node_t *node = xen_alloc(sizeof(vfs_node_t));
            node->name = xen_alloc(strlen(token) + 1);
            strcpy(node->name, token);
            node->type = type;
            node->parent = current;

            if (type == VFS_NODE_DIR) {
                node->dir.children = NULL;
                node->dir.child_count = 0;
            }

            vfs_add_child(current, node);
            return node;
        }

        // Intermediate component — must be directory
        if (!child) {
            // Create missing intermediate directory
            child = xen_alloc(sizeof(vfs_node_t));
            child->name = xen_alloc(strlen(token) + 1);
            strcpy(child->name, token);
            child->type = VFS_NODE_DIR;
            child->parent = current;
            child->dir.children = NULL;
            child->dir.child_count = 0;

            vfs_add_child(current, child);
        } else if (child->type != VFS_NODE_DIR) {
            return NULL; // Conflict: intermediate is not a directory
        }

        current = child;
        token = next;
    }

    return NULL;
}

vfs_node_t *vfs_lookup(const char *path) {
    if (!path || path[0] != '/') return NULL;
    if (strcmp(path, "/") == 0) return vfs_root;

    char temp[256];
    strncpy(temp, path, sizeof(temp));
    temp[sizeof(temp) - 1] = '\0';

    vfs_node_t *current = vfs_root;
    char *token = strtok(temp + 1, "/");

    while (token) {
        if (current->type != VFS_NODE_DIR) return NULL;

        vfs_node_t *child = vfs_find_child(current, token);
        if (!child) return NULL;

        current = child;
        token = strtok(NULL, "/");
    }

    return current;
}

bool vfs_remove(const char *path) {
    if (!path || path[0] != '/') return false;

    char temp[256];
    strncpy(temp, path, sizeof(temp));
    temp[sizeof(temp) - 1] = '\0';

    vfs_node_t *current = vfs_root;
    char *token = strtok(temp + 1, "/");
    char *next = NULL;

    while (token) {
        next = strtok(NULL, "/");
        vfs_node_t *child = vfs_find_child(current, token);

        if (!child) return false;

        if (!next) {
            // Found target node
            if (child == vfs_root) return false; // Cannot remove root

            // Remove child from parent's children list
            size_t i, j;
            for (i = 0; i < current->dir.child_count; i++) {
                if (current->dir.children[i] == child) {
                    break;
                }
            }

            if (i == current->dir.child_count) return false;

            // Shift remaining
            for (j = i + 1; j < current->dir.child_count; j++) {
                current->dir.children[j - 1] = current->dir.children[j];
            }

            current->dir.child_count--;
            if (current->dir.child_count > 0) {
                size_t new_size = current->dir.child_count * sizeof(vfs_node_t *);
                vfs_node_t **new_array = xen_alloc(new_size);
                memcpy(new_array, current->dir.children, new_size);
                xen_free(current->dir.children);
                current->dir.children = new_array;
            } else {
                xen_free(current->dir.children);
                current->dir.children = NULL;
            }

            xen_free_node(child);
            return true;
        }

        if (child->type != VFS_NODE_DIR) return false;
        current = child;
        token = next;
    }

    return false;
}

void vfs_init()
{
    vfs_root = xen_alloc(sizeof(vfs_node_t));
    vfs_root->name = xen_alloc(2);
    strcpy(vfs_root->name, "/");
    vfs_root->type = VFS_NODE_DIR;
    vfs_root->parent = NULL;
    vfs_root->dir.children = NULL;
    vfs_root->dir.child_count = 0;
    tty_printf("[VFS] Initialized!\n");
}
