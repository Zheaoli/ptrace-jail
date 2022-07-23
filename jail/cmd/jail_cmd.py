import argparse
import multiprocessing
import sys
from concurrent.futures import ThreadPoolExecutor
from queue import Queue

from loguru import logger

from jail.monitor.monitor import Monitor
from jail.trace.trace import TraceWorker

args = argparse.ArgumentParser()
args.add_argument("--version", action="version", version="0.0.1")
args.add_argument("--config", "-c", help="config file")
args.add_argument("--verbose", "-v", action="store_true")

args_object = args.parse_args()

if not args_object.config:
    logger.info("No config file specified")
    exit(1)

# Main Process to Open Protect
def main():
    ctx = multiprocessing.get_context("spawn")
    logger.remove()
    if args_object.verbose:
        logger.add(sys.stdout, level="DEBUG")
    else:
        logger.add(sys.stdout, level="INFO")
    #creat max_workers=2 ThreadPool
    thread_worker = ThreadPoolExecutor(max_workers=2)
    #creat Queue For Monitor to put New Process Creat and for TraceWorker to Get
    queue = Queue(10000)
    #TraceWorker add to Thread
    worker = TraceWorker(queue, args_object.config)
    worker_future = thread_worker.submit(worker.run)
    # Monitor add to Thread
    monitor = Monitor(queue)
    thread_worker.submit(monitor.loop)
    logger.info("all task submitted")
    # wait all thread done
    while not worker_future.done():
        logger.exception(worker_future.exception())
    thread_worker.shutdown(wait=True)
