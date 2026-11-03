import pathlib
import sys
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / "scripts"))
from importlib import import_module

sanitize = import_module("sanitize-papyrus-pas").sanitize


class PapyrusSanitizeTests(unittest.TestCase):
    def setUp(self):
        self.orig = (
            '.info\n'
            '  .source "Z:\\home\\private\\worker.psc"\n'
            '  .modifyTime 123\n'
            '  .compileTime 456\n'
            '  .user "private-user"\n'
            '  .computer "LOCAL-HOST"\n'
            '.endInfo\n'
            '.objectTable\n'
            '  CALLMETHOD SetActive target ::nonevar ::temp23\n'
            '.endObjectTable\n'
        )

    def test_identity_is_sanitized_without_touching_instructions(self):
        value = sanitize(self.orig, "K2040QuestJournalWorker")
        self.assertIn('.source "K2040QuestJournalWorker.psc"', value)
        self.assertIn('.user "K2040"', value)
        self.assertIn('.computer "K2040BUILD"', value)
        self.assertNotIn("private-user", value)
        self.assertNotIn("LOCAL-HOST", value)
        self.assertNotIn("Z:\\home", value)
        self.assertEqual(
            self.orig.split(".endInfo", 1)[1],
            value.split(".endInfo", 1)[1],
        )

    def test_rejects_unexpected_script(self):
        with self.assertRaises(ValueError):
            sanitize(self.orig, "OtherScript")

    def test_rejects_duplicate_header_field(self):
        with self.assertRaises(ValueError):
            sanitize(self.orig.replace('.user "private-user"', '.user "x"\n.user "y"'), "K2040QuestJournalWorker")

    def test_rejects_missing_header_field(self):
        with self.assertRaises(ValueError):
            sanitize(self.orig.replace('  .computer "LOCAL-HOST"\n', ''), "K2040QuestJournalWorker")

    def test_rejects_malformed_info_section(self):
        with self.assertRaises(ValueError):
            sanitize(self.orig.replace('.endInfo', '.endObject'), "K2040QuestJournalWorker")


if __name__ == "__main__":
    unittest.main()
