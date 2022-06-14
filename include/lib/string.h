#ifndef _STRING_H
#define _STRING_H

int strcmp(const char *str1, const char *str2);
int strncmp(const char *str1, const char *str2, int n);
int strcasecmp(const char *s1, const char *s2);
int strlen(const char *str);
int strcpy(char *dst, const char *src);
char *strcat(char *dest, const char *src);
char *strncat(char *dest, const char *src, int n);

int atoi(const char *str);

#endif  /* _STRING_H */