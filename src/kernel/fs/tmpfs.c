#include <fs/tmpfs.h>
#include <mm/mm.h>
#include <string.h>
#include <utils.h>

#define TMPFS_FILE_MAXSIZE PAGE_SIZE

struct tmpfs_file_t {
    char *data;
    int size;
    int capacity;
};

#define TMPFS_DIR_MAXSIZE 0x10

struct tmpfs_dir_t {
    int size;
    struct vnode *entries[TMPFS_DIR_MAXSIZE];
};

#define TMPFS_TYPE_UNDEFINE     0x0
#define TMPFS_TYPE_FILE         0x1
#define TMPFS_TYPE_DIR          0x2

#define TMPFS_NAME_MAXLEN       0x10

struct tmpfs_internal {
    char name[TMPFS_NAME_MAXLEN];
    int type;
    union {
        struct tmpfs_file_t *file;
        struct tmpfs_dir_t *dir;
    };
    struct vnode *oldnode;
};

static int tmpfs_mount(struct filesystem *fs, struct mount *mount);
static int tmpfs_alloc_vnode(struct filesystem *fs, struct vnode **target);

static struct filesystem tmpfs = {
    .name = "tmpfs",
    .mount = tmpfs_mount,
    .alloc_vnode = tmpfs_alloc_vnode
};

static int tmpfs_lookup(struct vnode *dir_node, struct vnode **target,
                        const char *component_name);
static int tmpfs_create(struct vnode *dir_node, struct vnode **target,
                        const char *component_name);
static int tmpfs_mkdir(struct vnode *dir_node, struct vnode **target,
                       const char *component_name);
static int tmpfs_isdir(struct vnode *dir_node);
static int tmpfs_getname(struct vnode *dir_node, const char **name);
static int tmpfs_getsize(struct vnode *dir_node);

static struct vnode_operations tmpfs_v_ops = {
    .lookup = tmpfs_lookup,
    .create = tmpfs_create,
    .mkdir = tmpfs_mkdir,
    .isdir = tmpfs_isdir,
    .getname = tmpfs_getname,
    .getsize = tmpfs_getsize
};

static int tmpfs_write(struct file *file, const void *buf, size_t len);
static int tmpfs_read(struct file *file, void *buf, size_t len);
static int tmpfs_open(struct vnode *file_node, struct file *target);
static int tmpfs_close(struct file *file);
static long tmpfs_lseek64(struct file *file, long offset, int whence);
static int tmpfs_ioctl(struct file *file, uint64 request, va_list args);

static struct file_operations tmpfs_f_ops = {
    .write = tmpfs_write,
    .read = tmpfs_read,
    .open = tmpfs_open,
    .close = tmpfs_close,
    .lseek64 = tmpfs_lseek64,
    .ioctl = tmpfs_ioctl
};

/* filesystem methods */

static int tmpfs_mount(struct filesystem *fs, struct mount *mount)
{
    struct vnode *node, *oldnode;
    struct tmpfs_internal *internal;
    struct tmpfs_dir_t *dir;
    const char *name;

    oldnode = mount->root;

    oldnode->v_ops->getname(oldnode, &name);

    if (strlen(name) >= TMPFS_NAME_MAXLEN) {
        return -1;
    }

    node = kmalloc(sizeof(struct vnode));
    internal = kmalloc(sizeof(struct tmpfs_internal));
    dir = kmalloc(sizeof(struct tmpfs_dir_t));

    dir->size = 0;

    node->mount = oldnode->mount;
    node->v_ops = oldnode->v_ops;
    node->f_ops = oldnode->f_ops;
    node->internal = oldnode->internal;

    strcpy(internal->name, name);
    internal->type = TMPFS_TYPE_DIR;
    internal->dir = dir;
    internal->oldnode = node;

    oldnode->mount = mount;
    oldnode->v_ops = &tmpfs_v_ops;
    oldnode->f_ops = &tmpfs_f_ops;
    oldnode->internal = internal;

    return 0;
}

static int tmpfs_alloc_vnode(struct filesystem *fs, struct vnode **target)
{
    struct vnode *node;
    struct tmpfs_internal *internal;
    struct tmpfs_dir_t *dir;

    node = kmalloc(sizeof(struct vnode));
    internal = kmalloc(sizeof(struct tmpfs_internal));
    dir = kmalloc(sizeof(struct tmpfs_dir_t));

    dir->size = 0;

    internal->name[0] = '\0';
    internal->type = TMPFS_TYPE_DIR;
    internal->dir = dir;
    internal->oldnode = NULL;

    node->mount = NULL;
    node->v_ops = &tmpfs_v_ops;
    node->f_ops = &tmpfs_f_ops;
    node->internal = internal;

    *target = node;

    return 0;
}

/* vnode_operations methods */

static int tmpfs_lookup(struct vnode *dir_node, struct vnode **target,
                        const char *component_name)
{
    struct tmpfs_internal *internal;
    struct tmpfs_dir_t *dir;
    int i;

    internal = dir_node->internal;

    if (internal->type != TMPFS_TYPE_DIR) {
        return -1;
    }

    dir = internal->dir;

    for (i = 0; i < dir->size ; ++i) {
        struct vnode *node;
        const char *name;
        int ret;

        node = dir->entries[i];

        ret = node->v_ops->getname(node, &name);

        if (ret < 0) {
            continue;
        }

        if (!strcmp(name, component_name)) {
            break;
        }
    }

    if (i >= dir->size) {
        return -1;
    }

    *target = dir->entries[i];

    return 0;
}

static int tmpfs_create(struct vnode *dir_node, struct vnode **target,
                        const char *component_name)
{
    struct tmpfs_internal *internal, *newint;
    struct tmpfs_file_t *file;
    struct tmpfs_dir_t *dir;
    struct vnode *node;
    int ret;

    if (strlen(component_name) >= 0x10) {
        return -1;
    }

    internal = dir_node->internal;

    if (internal->type != TMPFS_TYPE_DIR) {
        return -1;
    }

    dir = internal->dir;

    if (dir->size >= TMPFS_DIR_MAXSIZE) {
        return -1;
    }

    ret = tmpfs_lookup(dir_node, &node, component_name);

    if (!ret) {
        return -1;
    }

    node = kmalloc(sizeof(struct vnode));
    newint = kmalloc(sizeof(struct tmpfs_internal));
    file = kmalloc(sizeof(struct tmpfs_file_t));

    file->data = kmalloc(TMPFS_FILE_MAXSIZE);
    file->size = 0;
    file->capacity = TMPFS_FILE_MAXSIZE;

    strcpy(newint->name, component_name);
    newint->type = TMPFS_TYPE_FILE;
    newint->file = file;
    newint->oldnode = NULL;

    node->mount = dir_node->mount;
    node->v_ops = &tmpfs_v_ops;
    node->f_ops = &tmpfs_f_ops;
    node->parent = dir_node;
    node->internal = newint;

    dir->entries[dir->size] = node;
    dir->size++;

    *target = node;

    return 0;    
}

static int tmpfs_mkdir(struct vnode *dir_node, struct vnode **target,
                       const char *component_name)
{
    struct tmpfs_internal *internal, *newint;
    struct tmpfs_dir_t *dir, *newdir;
    struct vnode *node;
    int ret;

    if (strlen(component_name) >= 0x10) {
        return -1;
    }

    internal = dir_node->internal;

    if (internal->type != TMPFS_TYPE_DIR) {
        return -1;
    }

    dir = internal->dir;

    if (dir->size >= TMPFS_DIR_MAXSIZE) {
        return -1;
    }

    ret = tmpfs_lookup(dir_node, &node, component_name);

    if (!ret) {
        return -1;
    }

    node = kmalloc(sizeof(struct vnode));
    newint = kmalloc(sizeof(struct tmpfs_internal));
    newdir = kmalloc(sizeof(struct tmpfs_dir_t));

    newdir->size = 0;

    strcpy(newint->name, component_name);
    newint->type = TMPFS_TYPE_DIR;
    newint->dir = newdir;
    newint->oldnode = NULL;

    node->mount = dir_node->mount;
    node->v_ops = &tmpfs_v_ops;
    node->f_ops = &tmpfs_f_ops;
    node->parent = dir_node;
    node->internal = newint;

    dir->entries[dir->size] = node;
    dir->size++;

    *target = node;

    return 0;
}

static int tmpfs_isdir(struct vnode *dir_node)
{
    struct tmpfs_internal *internal;

    internal = dir_node->internal;

    if (internal->type != TMPFS_TYPE_DIR) {
        return 0;
    }

    return 1;
}

static int tmpfs_getname(struct vnode *dir_node, const char **name)
{
    struct tmpfs_internal *internal;

    internal = dir_node->internal;

    *name = internal->name;

    return 0;
}

static int tmpfs_getsize(struct vnode *dir_node)
{
    struct tmpfs_internal *internal;

    internal = dir_node->internal;

    if (internal->type != TMPFS_TYPE_FILE) {
        return -1;
    }

    return internal->file->size;
}

/* file_operations methods */

static int tmpfs_write(struct file *file, const void *buf, size_t len)
{
    struct tmpfs_internal *internal;
    struct tmpfs_file_t *f;

    internal = file->vnode->internal;

    if (internal->type != TMPFS_TYPE_FILE) {
        return -1;
    }

    f = internal->file;

    if (len > f->capacity - file->f_pos) {
        len = f->capacity - file->f_pos;
    }

    if (!len) {
        return 0;
    }

    memncpy(&f->data[file->f_pos], buf, len);

    file->f_pos += len;

    if (file->f_pos > f->size) {
        f->size = file->f_pos;
    }

    return len;
}

static int tmpfs_read(struct file *file, void *buf, size_t len)
{
    struct tmpfs_internal *internal;
    struct tmpfs_file_t *f;

    internal = file->vnode->internal;

    if (internal->type != TMPFS_TYPE_FILE) {
        return -1;
    }

    f = internal->file;

    if (len > f->size - file->f_pos) {
        len = f->size - file->f_pos;
    }

    if (!len) {
        return 0;
    }

    memncpy(buf, &f->data[file->f_pos], len);

    file->f_pos += len;

    return len;
}

static int tmpfs_open(struct vnode *file_node, struct file *target)
{
    // TODO: Verify file access permission

    target->vnode = file_node;
    target->f_pos = 0;
    target->f_ops = file_node->f_ops;

    return 0;
}

static int tmpfs_close(struct file *file)
{
    file->vnode = NULL;
    file->f_pos = 0;
    file->f_ops = NULL;

    return 0;
}

static long tmpfs_lseek64(struct file *file, long offset, int whence)
{
    int filesize;
    int base;
    
    filesize = file->vnode->v_ops->getsize(file->vnode);

    if (filesize < 0) {
        return -1;
    }

    switch (whence) {
    case SEEK_SET:
        base = 0;
        break;
    case SEEK_CUR:
        base = file->f_pos;
        break;
    case SEEK_END:
        base = filesize;
        break;
    default:
        return -1;
    }

    if (base + offset > filesize) {
        // TODO: Create a gap (a "hole")
        return -1;
    }

    file->f_pos = base + offset;

    return 0;
}

static int tmpfs_ioctl(struct file *file, uint64 request, va_list args)
{
    return -1;
}

/* Others */

struct filesystem *tmpfs_init(void)
{
    return &tmpfs;
}