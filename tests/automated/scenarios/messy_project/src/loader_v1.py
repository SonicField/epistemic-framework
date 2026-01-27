"""First attempt at parallel data loading - BROKEN.

This version has a race condition when multiple threads
try to append to the results list simultaneously.
"""
import threading
from concurrent.futures import ThreadPoolExecutor


class ParallelLoader:
    def __init__(self, num_workers=4):
        self.num_workers = num_workers
        self.results = []  # BUG: not thread-safe!

    def _load_chunk(self, chunk_path):
        # Simulated loading
        data = open(chunk_path).read()
        self.results.append(data)  # Race condition here

    def load(self, data_dir):
        chunks = self._find_chunks(data_dir)
        with ThreadPoolExecutor(max_workers=self.num_workers) as executor:
            executor.map(self._load_chunk, chunks)
        return self.results

    def _find_chunks(self, data_dir):
        # Implementation omitted
        pass
