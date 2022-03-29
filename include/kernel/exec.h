#ifndef _EXEC_H
#define _EXEC_H

// Change current EL to EL0 and execute the user program at @mem
// Set user stack to @user_sp
void exec_user_prog(char *mem, char *user_sp);

#endif /* _EXEC_H */