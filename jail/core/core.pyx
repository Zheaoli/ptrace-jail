cdef bint boolean_variable = True
cdef extern from "ptrace.h":
    int trace_process(unsigned long long ch, char *protected_path);

cdef extern from "netlink.h":
    ctypedef struct process_event:
        int pid
        int type
    process_event *process_event_new()
    int netlink_connect();
    int set_process_event_listen(int netlink_socket, bint enable);
    int make_non_blocking_socket(int sfd);
    int handle_process_event(int netlink_socket, process_event *event);
    int free_event(process_event *event)


def connect_netlink():
    fd = netlink_connect()
    if fd == -1:
        raise ValueError
    status = make_non_blocking_socket(fd)
    if status == -1:
        raise ValueError
    status = set_process_event_listen(fd, True)
    if status == -1:
        raise ValueError
    return fd


class ProcessEvent:
    pid: int
    event_type: int
    def __init__(self, pid, event_type):
        self.pid = pid
        self.event_type = event_type


def process_event_handler(netlink_socket):
    temp_event = process_event_new()
    status = handle_process_event(netlink_socket, temp_event)
    if status == -1:
        raise ValueError
    pid = <Py_ssize_t> temp_event.pid
    event_type = <Py_ssize_t> temp_event.type
    free_event(temp_event)
    return ProcessEvent(pid, event_type)

def protect_process(pid: int, target_path: str):
    status = trace_process(pid, target_path.encode())
    if status == -1:
        return -1
    return 0
