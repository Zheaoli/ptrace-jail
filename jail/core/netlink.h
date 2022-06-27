#ifndef NETLINK_H
#define NETLINK_H
#include <errno.h>
#include <fcntl.h>
#include <linux/cn_proc.h>
#include <linux/connector.h>
#include <linux/netlink.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include<pthread.h>

typedef enum process_event_type { FORK = 0, EXEC = 1 } process_event_type;
#define NERLINK_SOCKER_RECV_BUFFER_SIZE 65535

typedef struct process_event {
  unsigned long pid;
  process_event_type type;
} process_event;

static process_event *process_event_new() {
  process_event *event = (process_event *)malloc(sizeof(process_event));
  event->pid = 0;
  event->type = EXEC;
  return event;
}

static int free_event(process_event *event) {
  free(event);
  return 0;
}

static int netlink_connect() {
  int rc;
  int netlink_sock;
  struct sockaddr_nl sa_nl;
  netlink_sock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_CONNECTOR);
  if (netlink_sock == -1) {
    return -1;
  }
  int buffersize = NERLINK_SOCKER_RECV_BUFFER_SIZE;
  setsockopt(netlink_sock, SOL_SOCKET, SO_RCVBUF, &buffersize, sizeof(int));
  sa_nl.nl_family = AF_NETLINK;
  sa_nl.nl_groups = CN_IDX_PROC;
  sa_nl.nl_pid = getpid();
  rc = bind(netlink_sock, (struct sockaddr *)&sa_nl, sizeof(sa_nl));

  if (rc == -1) {
    close(netlink_sock);
    return -1;
  }
  return netlink_sock;
}

static int set_process_event_listen(int netlink_socket, bool enable) {
  int rc;
  struct __attribute__((aligned(NLMSG_ALIGNTO))) {
    struct nlmsghdr nl_hdr;
    struct __attribute__((__packed__)) {
      struct cn_msg cn_msg;
      enum proc_cn_mcast_op cn_mcast;
    };
  } nlcn_msg;
  memset(&nlcn_msg, 0, sizeof(nlcn_msg));
  nlcn_msg.nl_hdr.nlmsg_len = sizeof(nlcn_msg);
  nlcn_msg.nl_hdr.nlmsg_pid = getpid();
  nlcn_msg.nl_hdr.nlmsg_type = NLMSG_DONE;

  nlcn_msg.cn_msg.id.idx = CN_IDX_PROC;
  nlcn_msg.cn_msg.id.val = CN_VAL_PROC;
  nlcn_msg.cn_msg.len = sizeof(enum proc_cn_mcast_op);

  nlcn_msg.cn_mcast = enable ? PROC_CN_MCAST_LISTEN : PROC_CN_MCAST_IGNORE;

  rc = send(netlink_socket, &nlcn_msg, sizeof(nlcn_msg), 0);
  if (rc == -1) {
    return -1;
  }
  return 0;
}

static int make_non_blocking_socket(int sfd) {
  int flags, s;
  flags = fcntl(sfd, F_GETFL, 0);
  if (flags == -1) {
    perror("fcntl error");
    return -1;
  }
  flags |= O_NONBLOCK;
  s = fcntl(sfd, F_SETFL, flags);
  if (s == -1) {
    perror("fcntl set error");
    return -1;
  }
  return 0;
}

struct arg_struct {
  int netlink_socket;
  process_event *event;
};

static int demo(void *arguments) {
  struct arg_struct *args = (struct arg_struct *)arguments;
  int netlink_socket = args->netlink_socket;
  process_event *event = args->event;
  struct __attribute__((aligned(NLMSG_ALIGNTO))) {
    struct nlmsghdr nl_hdr;
    struct __attribute__((__packed__)) {
      struct cn_msg cn_msg;
      struct proc_event proc_ev;
    };
  } nlcn_msg;
  while (true) {
    int rc = recv(netlink_socket, &nlcn_msg, sizeof(nlcn_msg), 0);
    if (rc == 0 || rc == -1) {
      return rc;
    }
    switch (nlcn_msg.proc_ev.what) {
    case PROC_EVENT_NONE:
      break;
    case PROC_EVENT_EXEC:
      event->pid = nlcn_msg.proc_ev.event_data.exec.process_pid;
      event->type = EXEC;
      break;
    default:
      printf("unhandled proc event,%d,%d,%d\n",
             nlcn_msg.proc_ev.event_data.exec.process_pid,
             nlcn_msg.proc_ev.what, errno);
      break;
    }
  }
  return 0;
}

static int handle_process_event(int netlink_socket, process_event *event) {
  int rc;
  struct arg_struct args;
  args.netlink_socket = netlink_socket;
  args.event = event;
  pthread_t thread;
  rc = pthread_create(&thread, NULL, &demo, &args);
  if (rc != 0) {
    return -1;
  }
  while (true) {
    sleep(1);
  }
  return 0;
}

#endif