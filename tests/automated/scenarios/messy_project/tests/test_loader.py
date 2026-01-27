"""Tests for the parallel loader v2."""
import os
import tempfile
import unittest
from src.loader_v2 import ParallelLoader


class TestParallelLoader(unittest.TestCase):

    def setUp(self):
        self.test_dir = tempfile.mkdtemp()
        # Create test files
        for i in range(10):
            with open(os.path.join(self.test_dir, f"chunk_{i}.txt"), "w") as f:
                f.write(f"data_{i}")

    def test_loads_all_chunks(self):
        loader = ParallelLoader(num_workers=4)
        results = loader.load(self.test_dir)
        self.assertEqual(len(results), 10)

    def test_no_data_loss_under_concurrency(self):
        """Verify no race condition - all data accounted for."""
        loader = ParallelLoader(num_workers=4)
        results = loader.load(self.test_dir)
        # All chunks should be present
        contents = set(results)
        expected = {f"data_{i}" for i in range(10)}
        self.assertEqual(contents, expected)

    def test_custom_worker_count(self):
        loader = ParallelLoader(num_workers=2)
        results = loader.load(self.test_dir)
        self.assertEqual(len(results), 10)


if __name__ == "__main__":
    unittest.main()
