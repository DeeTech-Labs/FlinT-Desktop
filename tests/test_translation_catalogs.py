"""Validate page catalogs and source coverage without requiring Qt."""
import json
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]


class TranslationCatalogTests(unittest.TestCase):
    def test_page_catalogs_cover_sources_and_match(self):
        catalogs = {}
        for language in ("ru", "en"):
            directory = ROOT / "translations" / language
            metadata = json.loads((directory / "meta.json").read_text(encoding="utf-8"))
            self.assertEqual(metadata["language"], language)
            entries = {}
            for path in directory.rglob("*.json"):
                if path.name == "meta.json":
                    continue
                for context, messages in json.loads(path.read_text(encoding="utf-8"))["contexts"].items():
                    for source, translation in messages.items():
                        key = (context, source)
                        self.assertNotIn(key, entries, f"Duplicate key in {path}")
                        self.assertTrue(translation.strip(), (path, source))
                        self.assertEqual(sorted(re.findall(r"%\d+", source)),
                                         sorted(re.findall(r"%\d+", translation)), (path, source))
                        if language == "en":
                            self.assertEqual(source, translation, f"English must be the base: {path}")
                        entries[key] = translation
            catalogs[language] = entries
        self.assertEqual(catalogs["ru"].keys(), catalogs["en"].keys())
        sources = [*(ROOT / "qml").rglob("*.qml"), ROOT / "src/device/DeviceSession.cpp"]
        for path in sources:
            for source in re.findall(r'(?:qsTr|\btr)\("([^"\\]*)"\)', path.read_text(encoding="utf-8")):
                self.assertIn((path.stem, source), catalogs["en"], f"Missing translation: {path}")


if __name__ == "__main__":
    unittest.main()
