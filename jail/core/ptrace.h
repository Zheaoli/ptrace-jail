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
#define uint8 unsigned int
/* string struts*/
typedef struct strbs {
  long len;
  char *str;
  struct strbs *next;
} strb;

/* this function to check if prefix in str*/
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

static strb *init_builder() { return NULL; }

/* this function to get text from address */
static strb *init_build_strb(char *init) {
  strb *str = (strb *)malloc(sizeof(strb));
  str->len = strlen(init);
  str->str = init;
  str->next = NULL;
  return str;
}


/* this function to get text segmentation from address Linked list */
static strb *build_str(strb *str, char *add) {
  if (str == NULL) {
    return init_build_strb(add);
  }
  strb *s2 = init_build_strb(add);
  s2->len = str->len + s2->len;
  s2->next = str;
  return s2;
}

/* this function to get string from text segmentation */
static char *get_str(strb *src) {
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

/* print function */
static void print_str(strb *a) {

  while (a != NULL) {
    printf("{len: %ld, str: %s}->", a->len, a->str);
    a = a->next;
  }
  printf("NULL\n");
}

/* this function to get string from adddress */
static char *read_parm_string_from_address(pid_t child, void *addr) {
  strb *str = init_builder();
  int endOfStr = 1;
  int offset = 0;
  while (endOfStr == 1) {
    char *nStr = (char *)malloc(sizeof(long));
        // use PTRACE_PEEKTEXT to get address information
    long r =
        ptrace(PTRACE_PEEKTEXT, child,
               (void *)((long *)addr + offset), 0);
    memcpy((void *)nStr, (void *)&r, sizeof(long));
    printf("kernel %s\n", nStr);
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

/* this function to trace process to limit process */
int trace_process(unsigned long long ch, char *protected_path) {
    // add attach to specified pid
  ptrace(PTRACE_ATTACH, (pid_t)ch, 0, 0);
  int inSysCall = 0;
  int status;
  while (1) {
    wait(&status);
    if (WIFEXITED(status)) {
      break;
    }
        // use ptrace to get pid SYS call type numb
    int orig_rax = ptrace(PTRACE_PEEKUSER, (pid_t)ch, 8 * ORIG_RAX, NULL);
        // only deal with SYS_open or SYS_openat
    if (orig_rax == SYS_open || orig_rax == SYS_openat) {
            // use ptrace to get regs information
      struct user_regs_struct regs;
      ptrace(PTRACE_GETREGS, (pid_t)ch, NULL, &regs);
      char *openstr;
      if (orig_rax == SYS_open) {
                // get syscall argument ，this condition is filepath
        openstr = read_parm_string_from_address(ch, (void *)regs.rdi);
      } else {
                // get syscall argument ，this condition is filepath
        openstr = read_parm_string_from_address(ch, (void *)regs.rsi);
      }
            // check if filepath in watch
      printf("the syscall number is %d, the path is %s, validate rule %d\n", orig_rax, openstr, start_with(protected_path, openstr));
      if (start_with(protected_path, openstr)) {
        if (inSysCall == 0) {
            // system call enter, change inSysCall = 1 to identify
          inSysCall = 1;
        } else {
                    // system call return
          struct user_regs_struct return_regs;
                    // use ptrace to get return_regs point
          ptrace(PTRACE_GETREGS, (pid_t)ch, NULL, &return_regs);
                    // use ptrace to change return_regs as EACCES (Permission denied)
          return_regs.rax = -EACCES;
          ptrace(PTRACE_SETREGS, (pid_t)ch, NULL, &return_regs);
          inSysCall = 0;
        }
      }
    }
        // use ptrace to hook block process system call in and out, give time to check
    ptrace(PTRACE_SYSCALL, (pid_t)ch, NULL, NULL);
  }
  return 0;
}

#endif
