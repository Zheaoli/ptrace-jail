from concurrent.futures import ProcessPoolExecutor
from multiprocessing.queues import Queue

import psutil
from loguru import logger
from psutil import NoSuchProcess

from jail.config.config import parse_config
from jail.core.core import protect_process


class TraceWorker(object):
    def __init__(self, queue: Queue, config_file: str):
        self.task_queue = queue
        self.pool = ProcessPoolExecutor(max_workers=10)
        self.config = parse_config(config_file).get("rules")
        logger.debug(self.config)

    def run(self):
        while True:
            task = self.task_queue.get()
            try:
                pid_object = psutil.Process(task.pid)
                for process_config in self.config:
                    stop_match = False
                    for item in pid_object.cmdline():
                        if process_config.get("name") == item:
                            logger.debug(
                                f"protucted {process_config['filename']} be readed from {pid_object.pid} with cmdline {pid_object.cmdline()} "
                            )
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
            except NoSuchProcess:
                pass
