#ifndef _TEXT_USER_SHARED_H
#define _TEXT_USER_SHARED_H

/*
 * See the comment of task_init_map in task.h:
 *
 *   0x7f0000000000 ~ <shared_len>: r-x: Kernel functions exposed to users
 *
 * The kernel functions with the SECTION_TUS attribute will be mapped into
 * this user address space.
 */

#define SECTION_TUS __attribute__ ((section (".text.user.shared")))

#define TUS2VA(x) ((((uint64)x) - TEXT_USER_SHARED_BASE) + 0x7f0000000000)

/* From linker.ld */
extern char _stext_user_shared;
extern char _etext_user_shared;

#define TEXT_USER_SHARED_BASE (uint64)(&_stext_user_shared)
#define TEXT_USER_SHARED_END  (uint64)(&_etext_user_shared)
#define TEXT_USER_SHARED_LEN  (TEXT_USER_SHARED_END - TEXT_USER_SHARED_BASE)

#endif /* _TEXT_USER_SHARED_H */