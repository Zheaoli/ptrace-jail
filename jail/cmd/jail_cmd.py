import argparse
import multiprocessing
import sys
from concurrent.futures import ThreadPoolExecutor, ProcessPoolExecutor
from queue import Queue

from loguru import logger

from jail.monitor.monitor import Monitor
from jail.trace.trace import TraceWorker
from jail.config import ctx, task_queue

args = argparse.ArgumentParser()
args.add_argument("--version", action="version", version="0.0.1")
args.add_argument("--config", "-c", help="config file")
args.add_argument("--verbose", "-v", action="store_true")

args_object = args.parse_args()

if not args_object.config:
    logger.info("No config file specified")
    exit(1)


def main():
    logger.remove()
    if args_object.verbose:
        logger.add(sys.stdout, level="DEBUG")
    else:
        logger.add(sys.stdout, level="INFO")
    thread_worker = ProcessPoolExecutor(max_workers=2, mp_context=ctx)
    # queue = Queue(10000)
    worker = TraceWorker(args_object.config)
    worker_future = thread_worker.submit(worker.run)
    monitor = Monitor()
    thread_worker.submit(monitor.loop)
    logger.info("all task submitted")
    while not worker_future.done():
        logger.exception(worker_future.exception())
    thread_worker.shutdown()
