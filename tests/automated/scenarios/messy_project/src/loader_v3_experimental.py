"""Experimental lock-free loader - INCOMPLETE.

Attempting to use a queue instead of a locked list.
Theory: should be faster under high contention.

Status: Started but not finished. Not sure if worth pursuing
given v2 works fine.

TODO:
- Finish the implementation
- Benchmark against v2
- Decide if the complexity is worth it
"""
from concurrent.futures import ThreadPoolExecutor
from queue import Queue


class ParallelLoaderLockFree:
    def __init__(self, num_workers=4, batch_size=32):
        self.num_workers = num_workers
        self.batch_size = batch_size
        self.result_queue = Queue()

    def _load_chunk(self, chunk_path):
        # TODO: implement
        pass

    def load(self, data_dir):
        # TODO: implement
        # Should use queue.put() instead of list.append()
        # Need to drain queue at the end
        raise NotImplementedError("Work in progress")
