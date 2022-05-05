#ifndef _EXEC_H
#define _EXEC_H

// TODO: Add argv & envp
void sched_new_user_prog(char *filename);

void exit_user_prog(void);

void exec_user_prog(void *entry, char *user_sp, char *kernel_sp);

#endif /* _EXEC_H */