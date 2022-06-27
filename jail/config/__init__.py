import multiprocessing

ctx = multiprocessing.get_context("fork")
task_queue = ctx.Queue()
