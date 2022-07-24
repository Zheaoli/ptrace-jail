from queue import Queue
from selectors import EVENT_READ, DefaultSelector

from jail.core.core import connect_netlink, process_event_handler

# important class to monitor new process creat
class Monitor:
    def __init__(self, process_event_queue: Queue):
        # creat selector for I/O multiplexing
        self.selector = DefaultSelector()
        # share with Queue
        self._task_queue = process_event_queue
        # creat netlink socket and register function：process_event_handler to callback
        fd = connect_netlink()
        self.register(fd, process_event_handler)

    def register(self, file_object, callback): # set callback for selector instance
        self.selector.register(file_object, EVENT_READ, callback)

    # creat loop to deal with callback result and put result in Queue， wait TraceWorker to get
    def loop(self):
        while True:
            # From selector to get events
            events = self.selector.select()
            for key, _ in events:
                # get simple event and get this event's callback function
                callback = key.data
                # deal socket fileobj with callback function
                # ###
                # ###class ProcessEvent:
                # ###    pid: int
                # ###    event_type: int
                # ###
                # ###   def __init__(self, pid, event_type):
                # ###        self.pid = pid
                # ###        self.event_type = event_type
                # ###
                temp = callback(key.fileobj)
                if temp and temp.pid > 0:
                    # put ProcessEvent into queue
                    self._task_queue.put(temp)
