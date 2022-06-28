#ifndef PTRACE_H
#define PTRACE_H
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/reg.h>
#include <sys/uio.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <syscall.h>
#include <unistd.h>

typedef struct strbs {
  long len;
  char *str;
  struct strbs *next;
} strb;

inline int start_with(const char *prefix, const char *str) {
  size_t len_pre = strlen(prefix);
  size_t len_str = strlen(str);
  char *str_tmp = (char *)malloc(len_str);
  int i = 0;
  for (int j = 0; j < len_str; j++) {
    if ((int)str[j] >= 0 && (int)str[j] <= 127) {
      str_tmp[i] = str[j];
      i++;
    }
  }
  return len_str < len_pre ? 0 : memcmp(prefix, str_tmp, len_pre) == 0;
}

inline strb *init_builder() { return NULL; }

inline strb *init_build_strb(char *init) {
  strb *str = (strb *)malloc(sizeof(strb));
  str->len = strlen(init);
  str->str = init;
  str->next = NULL;
  return str;
}

inline strb *build_str(strb *str, char *add) {
  if (str == NULL) {
    return init_build_strb(add);
  }
  strb *s2 = init_build_strb(add);
  s2->len = str->len + s2->len;
  s2->next = str;
  return s2;
}

inline char *get_str(strb *src) {
  char *dest = (char *)malloc(src->len + 1);
  dest[src->len] = '\0';
  char *write = (char *)((long)dest + src->len);
  while (src->next != NULL) {
    long off = (src->len - src->next->len);
    write = (char *)(long)write - off;
    memcpy(write, src->str, src->len - src->next->len);
    src = src->next;
  }
  memcpy(dest, src->str, src->len);

  return dest;
}

inline void print_str(strb *a) {

  while (a != NULL) {
    printf("{len: %ld, str: %s}->", a->len, a->str);
    a = a->next;
  }
  printf("NULL\n");
}

inline char *read_parm_string_from_address(pid_t child, void *addr) {
  strb *str = init_builder();
  int endOfStr = 1;
  int offset = 0;
  while (endOfStr == 1) {
    char *nStr = (char *)malloc(sizeof(long));
    long r =
        ptrace(PTRACE_PEEKTEXT, child,
               (void *)((unsigned long long)addr + offset * sizeof(long)), 0);
    memcpy((void *)nStr, (void *)&r, sizeof(long));
    str = build_str(str, nStr);
    int i = 0;
    while (i < sizeof(long)) {
      if (nStr[i] == '\0') {
        endOfStr = 0;
        break;
      }
      i++;
    }
    offset++;
  }
  return get_str(str);
}

int trace_process(unsigned long long ch, char *protected_path) {
  ptrace(PTRACE_ATTACH, (pid_t)ch, 0, 0);
  int inSysCall = 0;
  int status;
  while (1) {
    wait(&status);
    if (WIFEXITED(status)) {
      break;
    }
    int orig_rax = ptrace(PTRACE_PEEKUSER, (pid_t)ch, 8 * ORIG_RAX, NULL);
    if (orig_rax == SYS_open || orig_rax == SYS_openat) {
      struct user_regs_struct regs;
      ptrace(PTRACE_GETREGS, (pid_t)ch, NULL, &regs);
      char *openstr;
      if (orig_rax == SYS_open) {
        openstr = read_parm_string_from_address(ch, (void *)regs.rdi);
      } else {
        openstr = read_parm_string_from_address(ch, (void *)regs.rsi);
      }
      if (start_with(protected_path, openstr)) {
        if (inSysCall == 0) {
          inSysCall = 1;
        } else {
          struct user_regs_struct return_regs;
          ptrace(PTRACE_GETREGS, (pid_t)ch, NULL, &return_regs);
          return_regs.rax = -EACCES;
          ptrace(PTRACE_SETREGS, (pid_t)ch, NULL, &return_regs);
          inSysCall = 0;
        }
      }
    }
    ptrace(PTRACE_SYSCALL, (pid_t)ch, NULL, NULL);
  }
  return 0;
}

#endif