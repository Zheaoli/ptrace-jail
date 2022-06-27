from queue import Queue
from selectors import EVENT_READ, DefaultSelector

from jail.core.core import connect_netlink, process_event_handler
from jail.config import task_queue


class Monitor:
    def __init__(self):
        self.selector = DefaultSelector()
        # self._task_queue = process_event_queue
        fd = connect_netlink()
        self.register(fd, process_event_handler)

    def register(self, file_object, callback):
        self.selector.register(file_object, EVENT_READ, callback)

    def loop(self):
        while True:
            events = self.selector.select()
            for key, _ in events:
                callback = key.data
                temp = callback(key.fileobj)
                if temp and temp.pid > 0:
                    task_queue.put(temp)
