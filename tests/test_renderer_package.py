"""Prevent either application's fonts/rasterizer drifting without a new wire identity."""
import hashlib
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
SHARED = ROOT / 'src/watch_render/shared'

class RendererPackageTest(unittest.TestCase):
    def test_renderer_identity_and_firmware_match(self):
        names = ['scene.h', 'renderer.h', 'builders.h', 'boot.h', 'fonts.cpp',
                 *['fonts/' + p.name for p in sorted((SHARED / 'fonts').glob('*.inc'))]]
        digest = hashlib.sha256()
        firmware = ROOT.parent / 'flint_os/components/flint/render'
        for name in names:
            content = (SHARED / name).read_bytes()
            digest.update(name.encode() + b'\0' + content)
            if firmware.exists():
                self.assertEqual(content, (firmware / name).read_bytes(), name)
        self.assertIn(digest.hexdigest(), (SHARED / 'version.h').read_text())
        if firmware.exists():
            self.assertEqual((SHARED / 'version.h').read_bytes(), (firmware / 'version.h').read_bytes())
