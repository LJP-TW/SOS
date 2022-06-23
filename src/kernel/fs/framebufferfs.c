#include <fs/framebufferfs.h>
#include <mm/mm.h>
#include <rpi3.h>
#include <preempt.h>
#include <utils.h>

static uint32 __attribute__((aligned(0x10))) mbox[36];

struct fb_info {
    uint32 width;
    uint32 height;
    uint32 pitch;
    uint32 isrgb;
};

struct fbfs_internal {
    const char *name;
    struct vnode oldnode;
    /* raw frame buffer address */
    uint8 *lfb;
    uint32 lfbsize;
    int isopened;
    int isinit;
};

static int fbfs_mount(struct filesystem *fs, struct mount *mount);
static int fbfs_sync(struct filesystem *fs);

static struct filesystem fbfs = {
    .name = "framebufferfs",
    .mount = fbfs_mount,
    .sync = fbfs_sync
};

static int fbfs_lookup(struct vnode *dir_node, struct vnode **target,
                        const char *component_name);
static int fbfs_create(struct vnode *dir_node, struct vnode **target,
                        const char *component_name);
static int fbfs_mkdir(struct vnode *dir_node, struct vnode **target,
                       const char *component_name);
static int fbfs_isdir(struct vnode *dir_node);
static int fbfs_getname(struct vnode *dir_node, const char **name);
static int fbfs_getsize(struct vnode *dir_node);

static struct vnode_operations fbfs_v_ops = {
    .lookup = fbfs_lookup,
    .create = fbfs_create,
    .mkdir = fbfs_mkdir,
    .isdir = fbfs_isdir,
    .getname = fbfs_getname,
    .getsize = fbfs_getsize
};

static int fbfs_write(struct file *file, const void *buf, size_t len);
static int fbfs_read(struct file *file, void *buf, size_t len);
static int fbfs_open(struct vnode *file_node, struct file *target);
static int fbfs_close(struct file *file);
static long fbfs_lseek64(struct file *file, long offset, int whence);
static int fbfs_ioctl(struct file *file, uint64 request, va_list args);

static struct file_operations fbfs_f_ops = {
    .write = fbfs_write,
    .read = fbfs_read,
    .open = fbfs_open,
    .close = fbfs_close,
    .lseek64 = fbfs_lseek64,
    .ioctl = fbfs_ioctl
};

/* filesystem methods */

static int fbfs_mount(struct filesystem *fs, struct mount *mount)
{
    struct vnode *oldnode;
    struct fbfs_internal *internal;
    const char *name;

    internal = kmalloc(sizeof(struct fbfs_internal));

    oldnode = mount->root;

    oldnode->v_ops->getname(oldnode, &name);

    internal->name = name;
    internal->oldnode.mount = oldnode->mount;
    internal->oldnode.v_ops = oldnode->v_ops;
    internal->oldnode.f_ops = oldnode->f_ops;
    internal->oldnode.parent = oldnode->parent;
    internal->oldnode.internal = oldnode->internal;
    internal->lfb = NULL;
    internal->isopened = 0;
    internal->isinit = 0;

    oldnode->mount = mount;
    oldnode->v_ops = &fbfs_v_ops;
    oldnode->f_ops = &fbfs_f_ops;
    oldnode->internal = internal;

    return 0;
}

static int fbfs_sync(struct filesystem *fs)
{
    return 0;
}

/* vnode_operations methods */

static int fbfs_lookup(struct vnode *dir_node, struct vnode **target,
                        const char *component_name)
{
    return -1;
}

static int fbfs_create(struct vnode *dir_node, struct vnode **target,
                        const char *component_name)
{
    return -1;
}

static int fbfs_mkdir(struct vnode *dir_node, struct vnode **target,
                       const char *component_name)
{
    return -1;
}

static int fbfs_isdir(struct vnode *dir_node)
{
    return 0;
}

static int fbfs_getname(struct vnode *dir_node, const char **name)
{
    struct fbfs_internal *internal;

    internal = dir_node->internal;

    *name = internal->name;

    return 0;
}

static int fbfs_getsize(struct vnode *dir_node)
{
    return -1;
}

/* file_operations methods */

static int fbfs_write(struct file *file, const void *buf, size_t len)
{
    struct fbfs_internal *internal;

    internal = file->vnode->internal;

    if (!internal->isinit) {
        return -1;
    }

    if (file->f_pos + len > internal->lfbsize) {
        return -1;
    }

    memncpy((void *)(internal->lfb + file->f_pos), buf, len);

    file->f_pos += len;

    return len;
}

static int fbfs_read(struct file *file, void *buf, size_t len)
{
    return -1;
}

static int fbfs_open(struct vnode *file_node, struct file *target)
{
    struct fbfs_internal *internal;

    preempt_disable();
    
    internal = file_node->internal;

    if (internal->isopened) {
        preempt_enable();

        return -1;
    }

    internal->isopened = 1;
    
    preempt_enable();

    target->vnode = file_node;
    target->f_pos = 0;
    target->f_ops = file_node->f_ops;

    return 0;
}

static int fbfs_close(struct file *file)
{
    struct fbfs_internal *internal;

    internal = file->vnode->internal;

    file->vnode = NULL;
    file->f_pos = 0;
    file->f_ops = NULL;

    internal->isopened = 0;

    return 0;
}

static long fbfs_lseek64(struct file *file, long offset, int whence)
{
    struct fbfs_internal *internal;
    int base;

    internal = file->vnode->internal;
    
    switch (whence) {
    case SEEK_SET:
        base = 0;
        break;
    case SEEK_CUR:
        base = file->f_pos;
        break;
    case SEEK_END:
        base = internal->lfbsize;
    default:
        return -1;
    }
    
    if (base + offset > internal->lfbsize) {
        return -1;
    }

    file->f_pos = base + offset;

    return 0;
}

static int fbfs_ioctl(struct file *file, uint64 request, va_list args)
{
    struct fb_info *user_fb_info;
    struct fbfs_internal *internal;
    /* dimensions and channel order */
    uint32 width, height, pitch, isrgb;

    if (request != 0) {
        return -1;
    }

    internal = file->vnode->internal;

    mbox[0] = 35 * 4;
    mbox[1] = MBOX_REQUEST_CODE;

    mbox[2] = 0x48003; // set phy wh
    mbox[3] = 8;
    mbox[4] = 8;
    mbox[5] = 1024; // FrameBufferInfo.width
    mbox[6] = 768;  // FrameBufferInfo.height

    mbox[7] = 0x48004; // set virt wh
    mbox[8] = 8;
    mbox[9] = 8;
    mbox[10] = 1024; // FrameBufferInfo.virtual_width
    mbox[11] = 768;  // FrameBufferInfo.virtual_height

    mbox[12] = 0x48009; // set virt offset
    mbox[13] = 8;
    mbox[14] = 8;
    mbox[15] = 0; // FrameBufferInfo.x_offset
    mbox[16] = 0; // FrameBufferInfo.y.offset

    mbox[17] = 0x48005; // set depth
    mbox[18] = 4;
    mbox[19] = 4;
    mbox[20] = 32; // FrameBufferInfo.depth

    mbox[21] = 0x48006; // set pixel order
    mbox[22] = 4;
    mbox[23] = 4;
    mbox[24] = 1; // RGB, not BGR preferably

    mbox[25] = 0x40001; // get framebuffer, gets alignment on request
    mbox[26] = 8;
    mbox[27] = 8;
    mbox[28] = 4096; // FrameBufferInfo.pointer
    mbox[29] = 0;    // FrameBufferInfo.size

    mbox[30] = 0x40008; // get pitch
    mbox[31] = 4;
    mbox[32] = 4;
    mbox[33] = 0; // FrameBufferInfo.pitch

    mbox[34] = MBOX_END_TAG;

    // this might not return exactly what we asked for, could be
    // the closest supported resolution instead
    
    mailbox_call(MAILBOX_CH_PROP, mbox);

    if (mbox[20] == 32 && mbox[28] != 0) {
        mbox[28] &= 0x3FFFFFFF; // convert GPU address to ARM address
        width = mbox[5];        // get actual physical width
        height = mbox[6];       // get actual physical height
        pitch = mbox[33];       // get number of bytes per line
        isrgb = mbox[24];       // get the actual channel order
        internal->lfb = (void *)PA2VA(mbox[28]);
        internal->lfbsize = mbox[29];
    } else {
        // Unable to set screen resolution to 1024x768x32
        return -1;
    }
    
    user_fb_info = va_arg(args, void *);

    user_fb_info->width = width;
    user_fb_info->height = height;
    user_fb_info->pitch = pitch;
    user_fb_info->isrgb  = isrgb;

    internal->isinit = 1;

    return 0;
}

/* Others */

struct filesystem *framebufferfs_init(void)
{
    return &fbfs;
}