"""Unit tests for the CUDA distribution audit (no GPU/toolkit needed)."""
import importlib.util
import hashlib
import io
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
license_spec = importlib.util.spec_from_file_location(
    'cuda_license', Path(__file__).resolve().parents[1] / 'scripts/preserve-cuda-license.py')
license_module = importlib.util.module_from_spec(license_spec)
license_spec.loader.exec_module(license_module)


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


class CudaLicenseTests(unittest.TestCase):
    def test_preserves_toolkit_license_without_network(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp) / 'cuda'
            root.mkdir()
            (root / 'EULA.txt').write_bytes(b'NVIDIA license fixture')
            with patch.object(license_module.urllib.request, 'urlopen', side_effect=AssertionError('network')):
                result = license_module.preserve(root, Path(tmp) / 'out')
            self.assertEqual(result['source'], str(root / 'EULA.txt'))
            self.assertEqual((Path(tmp) / 'out/EULA.txt').read_bytes(), b'NVIDIA license fixture')

    def test_fallback_requires_pinned_official_bytes(self):
        data = b'NVIDIA License Agreement fixture'
        with tempfile.TemporaryDirectory() as tmp:
            with patch.object(license_module, 'EULA_SHA256', hashlib.sha256(data).hexdigest()), patch.object(license_module.urllib.request, 'urlopen', return_value=io.BytesIO(data)):
                result = license_module.preserve(Path(tmp) / 'missing', Path(tmp) / 'out')
            self.assertEqual(result['source'], license_module.EULA_URL)
            self.assertEqual((Path(tmp) / 'out/CUDA-12.6.3-EULA.html').read_bytes(), data)
            with patch.object(license_module.urllib.request, 'urlopen', return_value=io.BytesIO(b'wrong document')):
                with self.assertRaisesRegex(RuntimeError, 'checksum'):
                    license_module.preserve(Path(tmp) / 'missing', Path(tmp) / 'bad')


if __name__ == '__main__':
    unittest.main()
