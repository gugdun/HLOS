#include <string.h>

#ifdef ARCH_x86_64
#include <xencore/arch/x86_64/paging.h>
#endif

#include <xencore/xenfs/vfs.h>
#include <xencore/xenio/tty.h>
#include <xencore/xenmem/bitmap.h>

typedef struct vfs_block {
    size_t size;
    struct vfs_block *next;
} vfs_block_t;

typedef struct vfs_page {
    struct vfs_page *next;
    size_t used;
    uint8_t data[PAGE_SIZE_2MB - sizeof(struct vfs_page *) - sizeof(size_t)];
} vfs_page_t;

static vfs_page_t *vfs_pages = (vfs_page_t *)NULL;
static vfs_block_t *vfs_free_list = (vfs_block_t *)NULL;

static vfs_node_t *vfs_root = (vfs_node_t *)NULL;

static void *vfs_alloc(size_t size)
{
    // Align to 8 bytes
    size = (size + 7) & ~7;
    size_t total = size + sizeof(vfs_block_t);

    // Check free list
    vfs_block_t **prev = &vfs_free_list;
    for (vfs_block_t *block = vfs_free_list; block; block = block->next) {
        if (block->size >= size) {
            *prev = block->next;
            return (void *)(block + 1);
        }
        prev = &block->next;
    }

    // Bump allocation
    if (!vfs_pages || (vfs_pages->used + total > sizeof(vfs_pages->data))) {
        vfs_page_t *new_page = (vfs_page_t *)alloc_page();
        if (!new_page) return NULL;
        new_page->next = vfs_pages;
        new_page->used = 0;
        vfs_pages = new_page;
    }

    vfs_block_t *block = (vfs_block_t *)&vfs_pages->data[vfs_pages->used];
    block->size = size;
    vfs_pages->used += total;

    return (void *)(block + 1);
}

static void vfs_free(void *ptr)
{
    if (!ptr) return;
    vfs_block_t *block = (vfs_block_t *)ptr - 1;
    block->next = vfs_free_list;
    vfs_free_list = block;
}

static void vfs_add_child(vfs_node_t *parent, vfs_node_t *child)
{
    size_t new_count = parent->dir.child_count + 1;
    size_t new_size = new_count * sizeof(vfs_node_t *);
    vfs_node_t **new_array = vfs_alloc(new_size);

    if (parent->dir.child_count > 0) {
        memcpy(new_array, parent->dir.children, parent->dir.child_count * sizeof(vfs_node_t *));
        vfs_free(parent->dir.children);
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

static void vfs_free_node(vfs_node_t *node) {
    if (!node) return;

    vfs_free(node->name);

    if (node->type == VFS_NODE_DIR) {
        for (size_t i = 0; i < node->dir.child_count; i++) {
            vfs_free_node(node->dir.children[i]);
        }
        vfs_free(node->dir.children);
    } else if (node->type == VFS_NODE_FILE) {
        vfs_free(node->file.data);
    } else if (node->type == VFS_NODE_SYMLINK) {
        vfs_free(node->symlink.target);
    }

    vfs_free(node);
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

            vfs_node_t *node = vfs_alloc(sizeof(vfs_node_t));
            node->name = vfs_alloc(strlen(token) + 1);
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
            child = vfs_alloc(sizeof(vfs_node_t));
            child->name = vfs_alloc(strlen(token) + 1);
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
                vfs_node_t **new_array = vfs_alloc(new_size);
                memcpy(new_array, current->dir.children, new_size);
                vfs_free(current->dir.children);
                current->dir.children = new_array;
            } else {
                vfs_free(current->dir.children);
                current->dir.children = NULL;
            }

            vfs_free_node(child);
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
    vfs_root = vfs_alloc(sizeof(vfs_node_t));
    vfs_root->name = vfs_alloc(2);
    strcpy(vfs_root->name, "/");
    vfs_root->type = VFS_NODE_DIR;
    vfs_root->parent = NULL;
    vfs_root->dir.children = NULL;
    vfs_root->dir.child_count = 0;
    tty_printf("[VFS] Initialized!\n");
}
