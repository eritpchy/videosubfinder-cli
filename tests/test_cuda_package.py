"""Unit tests for the CUDA distribution audit (no GPU/toolkit needed)."""
import importlib.util
import json
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

spec = importlib.util.spec_from_file_location(
    'cuda_package', Path(__file__).resolve().parents[1] / 'scripts/check-cuda-package.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class CudaPackageAuditTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name) / 'dist'
        (self.root / 'lib').mkdir(parents=True)
        (self.root / 'VideoSubFinderCli').write_bytes(b'cli')
        (self.root / 'lib/libnppig.so.12').write_bytes(b'cuda')
        self.archive = Path(self.temp.name) / 'package.tar.gz'
        self.archive.write_bytes(b'archive')

    @patch.object(module.subprocess, 'run')
    @patch.object(module.subprocess, 'check_output')
    def test_reports_actual_sizes_without_claiming_gpu(self, readelf, ldd):
        readelf.return_value = ' (NEEDED) Shared library: [libc.so.6]\n'
        ldd.return_value = subprocess.CompletedProcess([], 0, 'libc.so.6 => /lib/libc.so.6', '')
        report = module.audit(self.root, self.archive)
        self.assertEqual(report['current'], {'archive_bytes': 7, 'expanded_bytes': 7,
                                             'library_count': 1})
        self.assertFalse(report['gpu_execution_verified'])
        self.assertEqual(report['dt_needed']['VideoSubFinderCli'], ['libc.so.6'])
        json.dumps(report)

    def test_rejects_distro_dependency_cascade(self):
        (self.root / 'lib/libgdal.so.30').write_bytes(b'gdal')
        with self.assertRaisesRegex(RuntimeError, 'Unexpected dynamic'):
            module.audit(self.root, self.archive)

    @patch.object(module.subprocess, 'run')
    @patch.object(module.subprocess, 'check_output', return_value='')
    def test_rejects_missing_runtime(self, readelf, ldd):
        ldd.return_value = subprocess.CompletedProcess([], 0, 'libnppig.so.12 => not found', '')
        with self.assertRaisesRegex(RuntimeError, 'Unresolved dependency'):
            module.audit(self.root, self.archive)


if __name__ == '__main__':
    unittest.main()
