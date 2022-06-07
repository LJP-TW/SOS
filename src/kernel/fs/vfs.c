#include <fs/vfs.h>
#include <mm/mm.h>
#include <utils.h>
#include <string.h>
#include <current.h>
#include <task.h>
#include <panic.h>

struct mount *rootmount;

static struct list_head filesystems;

/*
 * Return directory vnode, and set @pathname to the last component name.
 * If the @pathname is end with '/', set @pathname to NULL
 *
 * For example:
 *
 * If @pathname is "/abc/def/ghi".
 * Return the vnode of "/abc/def", and set @pathname to "ghi".
 *
 * If @pathname is "/abc/def/".
 * Return the vnode of "/abc/def", and set @pathname to NULL.
 *
 */
static struct vnode *get_dir_vnode(struct vnode *dir_node,
                                   const char **pathname)
{
    struct vnode *result;
    const char *start;
    const char *end;
    char buf[0x100];

    start = end = *pathname;

    if (*start == '/') {
        result = rootmount->root;
    } else {
        result = dir_node;
    }

    while (1) {
        if (!strncmp("./", start, 2)) {
            start += 2;
            end = start;
            continue;
        } else if (!strncmp("../", start, 3)) {
            if (result->parent) {
                result = result->parent;
            }

            start += 3;
            end = start;
            continue;
        }

        // Find next component
        while (*end != '\0' && *end != '/') {
            end++;
        }

        if (*end == '/') {
            int ret;

            if (start == end) {
                // Case like "//"
                end++;
                start = end;
                continue;
            }

            // TODO: Check if the length is less than 0x100
            memncpy(buf, start, end - start);
            buf[end - start] = 0;

            ret = result->v_ops->lookup(result, &result, buf);

            if (ret < 0) {
                return NULL;
            }

            end++;
            start = end;
        } else {
            break;
        }
    }

    *pathname = *start ? start : NULL;

    return result;
}

static struct filesystem *find_filesystem(const char *filesystem)
{
    struct filesystem *fs;

    list_for_each_entry(fs, &filesystems, fs_list) {
        if (!strcmp(fs->name, filesystem)) {
            return fs;
        }
    }

    return NULL;
}

void vfs_init(void)
{
    INIT_LIST_HEAD(&filesystems);
}

void vfs_init_rootmount(struct filesystem *fs)
{
    struct vnode *n;
    int ret;

    ret = fs->alloc_vnode(fs, &n);
    if (ret < 0) {
        panic("vfs_init_rootmount failed");
    }

    rootmount = kmalloc(sizeof(struct mount));

    rootmount->root = n;
    rootmount->fs = fs;

    n->mount = rootmount;
    n->parent = NULL;
}

int register_filesystem(struct filesystem *fs)
{
    list_add_tail(&fs->fs_list, &filesystems);

    return 0;
}

int vfs_open(const char *pathname, int flags, struct file *target)
{
    const char *curname;
    struct vnode *dir_node;
    struct vnode *file_node;
    int ret;

    curname = pathname;
    dir_node = get_dir_vnode(current->work_dir, &curname);

    if (!dir_node) {
        return -1;
    }

    if (!curname) {
        return -1;
    }

    ret = dir_node->v_ops->lookup(dir_node, &file_node, curname);

    if (flags & O_CREAT) {
        // TODO: Support overwrite a existed 
        
        if (ret == 0) {
            return -1;
        }
        
        ret = dir_node->v_ops->create(dir_node, &file_node, curname);
    }

    if (ret < 0) {
        return ret;
    }

    if (!file_node) {
        return -1;
    }

    ret = file_node->f_ops->open(file_node, target);

    if (ret < 0) {
        return ret;
    }

    target->flags = 0;

    return 0;  
}

int vfs_close(struct file *file)
{
    return file->f_ops->close(file);
}

int vfs_write(struct file *file, const void *buf, size_t len)
{
    return file->f_ops->write(file, buf, len);
}

int vfs_read(struct file *file, void *buf, size_t len)
{
    return file->f_ops->read(file, buf, len);
}

long vfs_lseek64(struct file *file, long offset, int whence)
{
    return file->f_ops->lseek64(file, offset, whence);
}

int vfs_ioctl(struct file *file, uint64 request, va_list args)
{
    return file->f_ops->ioctl(file, request, args);
}

int vfs_mkdir(const char *pathname)
{
    const char *curname;
    struct vnode *dir_node;
    struct vnode *newdir_node;
    int ret;

    curname = pathname;

    if (current) {
        dir_node = get_dir_vnode(current->work_dir, &curname);
    } else {
        dir_node = get_dir_vnode(rootmount->root, &curname);
    }

    if (!dir_node) {
        return -1;
    }

    if (!curname) {
        return -1;
    }

    ret = dir_node->v_ops->mkdir(dir_node, &newdir_node, curname);
    
    return ret;
}

int vfs_mount(const char *mountpath, const char *filesystem)
{
    const char *curname;
    struct vnode *dir_node;
    struct filesystem *fs;
    struct mount *mo;
    int ret;

    curname = mountpath;

    if (current) {
        dir_node = get_dir_vnode(current->work_dir, &curname);
    } else {
        dir_node = get_dir_vnode(rootmount->root, &curname);
    }
    
    if (!dir_node) {
        return -1;
    }

    if (curname) {
        ret = dir_node->v_ops->lookup(dir_node, &dir_node, curname);
        
        if (ret < 0) {
            return ret;
        }
    }

    if (!dir_node->v_ops->isdir(dir_node)) {
        return -1;
    }

    fs = find_filesystem(filesystem);

    if (!fs) {
        return -1;
    }

    mo = kmalloc(sizeof(struct mount));

    mo->root = dir_node;
    mo->fs = fs;

    ret = fs->mount(fs, mo);

    if (ret < 0) {
        kfree(mo);
        return ret;
    }

    return 0;
}

int vfs_lookup(const char *pathname, struct vnode **target)
{
    const char *curname;
    struct vnode *dir_node;
    struct vnode *file_node;
    int ret;

    curname = pathname;
    dir_node = get_dir_vnode(current->work_dir, &curname);

    if (!dir_node) {
        return -1;
    }

    if (!curname) {
        *target = dir_node;

        return 0;
    }

    ret = dir_node->v_ops->lookup(dir_node, &file_node, curname);

    if (ret < 0) {
        return ret;
    }

    *target = file_node;

    return 0;
}

static int do_open(const char *pathname, int flags)
{
    int i, ret;

    for (i = 0; i <= current->maxfd; ++i) {
        if (current->fds[i].vnode == NULL) {
            break;
        }
    }

    if (i > current->maxfd) {
        if (current->maxfd >= TASK_MAX_FD) {
            return -1;
        }

        current->maxfd += 1;
        i = current->maxfd;
    }

    ret = vfs_open(pathname, flags, &current->fds[i]);

    if (ret < 0) {
        current->fds[i].vnode = NULL;

        return ret;
    }

    return i;
}

static int do_close(int fd)
{
    int ret;

    if (fd < 0 || current->maxfd < fd) {
        return -1;
    }

    if (current->fds[fd].vnode == NULL) {
        return -1;
    }

    ret = vfs_close(&current->fds[fd]);

    if (ret < 0) {
        return ret;
    }

    return 0;
}

static int do_write(int fd, const void *buf, uint64 count)
{
    int ret;

    if (fd < 0 || current->maxfd < fd) {
        return -1;
    }

    if (current->fds[fd].vnode == NULL) {
        return -1;
    }

    ret = vfs_write(&current->fds[fd], buf, count);

    return ret;
}

static int do_read(int fd, void *buf, uint64 count)
{
    int ret;

    if (fd < 0 || current->maxfd < fd) {
        return -1;
    }

    if (current->fds[fd].vnode == NULL) {
        return -1;
    }

    ret = vfs_read(&current->fds[fd], buf, count);

    return ret;
}

static int do_mkdir(const char *pathname, uint32 mode)
{
    int ret;

    ret = vfs_mkdir(pathname);

    return ret;
}

static int do_mount(const char *target, const char *filesystem)
{
    int ret;

    ret = vfs_mount(target, filesystem);

    return ret;
}

static int do_chdir(const char *path)
{
    struct vnode *result;
    int ret;

    ret = vfs_lookup(path, &result);

    if (ret < 0) {
        return ret;
    }

    if (!result->v_ops->isdir(result)) {
        return -1;
    }

    current->work_dir = result;

    return 0;
}

static long do_lseek64(int fd, int64 offset, int whence)
{
    long ret;

    if (fd < 0 || current->maxfd < fd) {
        return -1;
    }

    if (current->fds[fd].vnode == NULL) {
        return -1;
    }

    ret = vfs_lseek64(&current->fds[fd], offset, whence);

    return ret;
}

static int do_ioctl(int fd, uint64 request, va_list args)
{
    int ret;

    if (fd < 0 || current->maxfd < fd) {
        return -1;
    }

    if (current->fds[fd].vnode == NULL) {
        return -1;
    }

    ret = vfs_ioctl(&current->fds[fd], request, args);

    return ret;
}

void syscall_open(trapframe *frame, const char *pathname, int flags)
{
    int fd = do_open(pathname, flags);

    frame->x0 = fd;
}

void syscall_close(trapframe *frame, int fd)
{
    int ret = do_close(fd);

    frame->x0 = ret;
}

void syscall_write(trapframe *frame, int fd, const void *buf, uint64 count)
{
    int ret = do_write(fd, buf, count);

    frame->x0 = ret;
}

void syscall_read(trapframe *frame, int fd, void *buf, uint64 count)
{
    int ret = do_read(fd, buf, count);

    frame->x0 = ret;
}

void syscall_mkdir(trapframe *frame, const char *pathname, uint32 mode)
{
    int ret = do_mkdir(pathname, mode);

    frame->x0 = ret;
}

void syscall_mount(trapframe *frame, const char *src, const char *target,
                   const char *filesystem, uint64 flags, const void *data)
{
    int ret = do_mount(target, filesystem);

    frame->x0 = ret;
}

void syscall_chdir(trapframe *frame, const char *path)
{
    int ret = do_chdir(path);

    frame->x0 = ret;
}

void syscall_lseek64(trapframe *frame, int fd, int64 offset, int whence)
{
    long ret = do_lseek64(fd, offset, whence);

    frame->x0 = ret;
}

void syscall_ioctl(trapframe *frame, int fd, uint64 request, ...)
{
    int ret;
    va_list args;

    va_start(args, request);

    ret = do_ioctl(fd, request, args);

    va_end(args);

    frame->x0 = ret;
}