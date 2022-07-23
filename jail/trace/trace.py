from concurrent.futures import ProcessPoolExecutor
from multiprocessing.queues import Queue

import psutil
from loguru import logger
from psutil import NoSuchProcess

from jail.config.config import parse_config
from jail.core.core import protect_process

# important function to add limit/protect specified cmdline Process/pid to sys-open specified flie/path
class TraceWorker(object):
    def __init__(self, queue: Queue, config_file: str):
        self.task_queue = queue
        #   creat max_workers=10 ProcessPool
        self.pool = ProcessPoolExecutor(max_workers=10)
        self.config = parse_config(config_file).get("rules")
        logger.debug(self.config)

    def run(self):
        while True:
            #   To get process_event from queue
            task = self.task_queue.get()
            try:
                #   use psutil to get Process information by pid
                pid_object = psutil.Process(task.pid)
                for process_config in self.config:  # check each config
                    stop_match = False
                    for item in pid_object.cmdline():   # check process cmdline if have specified config
                        if process_config.get("name") in item:
                            logger.debug(
                                f"protucted {process_config['filename']} be readed from {pid_object.pid} with cmdline {pid_object.cmdline()} "
                            )
                            #   add conf and pid to start new process to limit/protect
                            self.pool.submit(
                                protect_process,
                                pid_object.pid,
                                process_config["filename"],
                            )
                            stop_match = True
                            break
                    if stop_match:
                        logger.debug("add protect complete")
                        break
            except NoSuchProcess:    #   if process is done bypaas
                pass
