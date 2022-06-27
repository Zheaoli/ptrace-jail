import argparse
import sys
from multiprocessing import Pool

from loguru import logger

from jail.config import ctx
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


def main():
    logger.remove()
    if args_object.verbose:
        logger.add(sys.stdout, level="DEBUG")
    else:
        logger.add(sys.stdout, level="INFO")
    thread_worker = Pool(processes=2)
    # queue = Queue(10000)
    worker = TraceWorker(args_object.config)
    _ = thread_worker.apply_async(worker.run)
    monitor = Monitor()
    thread_worker.apply_async(monitor.loop)
    logger.info("all task submitted")
    # while not worker_process.done():
    #     logger.exception(worker_process.exception())
    thread_worker.close()
    thread_worker.join()
