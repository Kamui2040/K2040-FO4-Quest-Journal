"""Synthetic fixtures ONLY — do not commit actual game disassembly."""
import importlib.util
import io
import unittest
from contextlib import redirect_stderr, redirect_stdout
from pathlib import Path
from tempfile import TemporaryDirectory

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "selector_decoder", ROOT / "scripts/analyze-selector-readonly-log.py"
)
MOD = importlib.util.module_from_spec(SPEC)
import sys
sys.modules[SPEC.name] = MOD
SPEC.loader.exec_module(MOD)


def sample_records(hex_payload=None):
    code = hex_payload or ("90" * MOD.CODE_BYTES)
    lines = [
        "[test] " + MOD.START_TOKEN,
        "[test] " + MOD.SUBMIT_TOKEN,
    ]
    for i in range(MOD.SLOT_COUNT):
        lines.append(f"[test] SelectorReadOnlyQA: slot={i} "
                     f"RVA=0x{0x12000 + i * 0x100:X} bytes={code} (read only)")
    lines.append("[test] " + MOD.END_TOKEN)
    return "\n".join(lines) + "\n"


class SelectorCaptureDecoderTest(unittest.TestCase):
    def test_full_sample_only(self):
        entries = MOD.parse_lines(sample_records())
        self.assertEqual(len(entries), 4)
        self.assertTrue(all(len(entry.hex_bytes) == 384 for entry in entries))
        self.assertEqual([e.index for e in entries], list(range(4)))

    def test_missing_failure_and_duplicate_samples(self):
        sample = sample_records()
        for missing in [MOD.START_TOKEN, MOD.SUBMIT_TOKEN, MOD.END_TOKEN,
                        "slot=2"]:
            with self.subTest(missing=missing):
                lines = [line for line in sample.splitlines() if missing not in line]
                with self.assertRaises(MOD.CaptureError):
                    MOD.parse_lines("\n".join(lines))
        with self.assertRaises(MOD.CaptureError):
            MOD.parse_lines(sample + sample)
        with self.assertRaises(MOD.CaptureError):
            MOD.parse_lines(sample + sample.splitlines()[2] + "\n")

    def test_bounds_and_bad_encoding(self):
        sample = sample_records()
        first = sample.splitlines()[2]
        for bad in (first.replace("90"*192, "90"*191),
                    first.replace("bytes=", "bytes=z"),
                    first.replace("slot=0", "slot=9"),
                    first.replace("RVA=0x12000", "RVA=0x1")):
            with self.subTest(line=bad[:40]):
                with self.assertRaises(MOD.CaptureError):
                    MOD.parse_lines(sample.replace(first, bad))

    def test_restrict_input_and_no_raw_default_output(self):
        with TemporaryDirectory() as temp:
            file = Path(temp) / "K2040_Quest_Journal_Selector_ReadOnly_20261029_130000_123.log"
            file.write_text(sample_records(), encoding="utf-8")
            self.assertEqual(len(MOD.read_log(file)), 4)
            output = io.StringIO()
            with redirect_stdout(output):
                self.assertEqual(MOD.main([str(file)]), 0)
            self.assertIn('"capture": "complete-read-only"', output.getvalue())
            self.assertNotIn("9090909090", output.getvalue())
            other = Path(temp) / "other-mod.log"
            other.write_text(sample_records(), encoding="utf-8")
            with self.assertRaises(MOD.CaptureError):
                MOD.read_log(other)
            link = Path(temp) / "K2040_Quest_Journal_Selector_ReadOnly_20261029_130000_124.log"
            link.symlink_to(file)
            with self.assertRaises(MOD.CaptureError):
                MOD.read_log(link)
            file.write_text("x" * (MOD.MAX_LOG_BYTES + 1))
            with self.assertRaises(MOD.CaptureError):
                MOD.read_log(file)

    def test_newer_anonymous_row_audit(self):
        import json
        labels = sorted(MOD.SAFE_STATUSES)
        statuses = [{"kind": key, "count": (
            3 if key == "structural-candidate-NOT-command-ABI" else
            1 if key in ("misc-root-type-six-candidate", "missing-instance")
            else 0)} for key in labels]
        data = {
            "available": True, "auditVersion": 2,
            "rootRows": 2, "miscRows": 2, "syntheticRows": 1,
            "ordinaryObjectiveRows": 2, "unknownParentChildren": 0,
            "typedRowTypeFields": 4,
            "typedInstanceFields": 4, "questTypeKeys": 1,
            "typeKeys": 3, "bothKeys": 0, "typedRootTypeFields": 2,
            "typedMiscTypeFields": 1, "truncated": False,
            "statusCounts": statuses,
            "schemaQuestRoot": ["formID:2", "instance:2"],
            "schemaMiscChild": ["type:2"],
            "schemaSyntheticRoot": [],
            "schemaObjectiveDisplay": ["completed:0", "text:6"]
        }
        content = (sample_records() + "[test] " +
                   MOD.ROW_AUDIT_MARKER + json.dumps(data,separators=(",",":")) +
                   MOD.ROW_AUDIT_TRAILER + "\n")
        parsed = MOD.parse_row_audit(content)
        self.assertEqual(parsed["miscRows"], 2)
        self.assertEqual(parsed["ordinaryObjectiveRows"], 2)
        self.assertEqual(parsed["auditVersion"], 2)
        self.assertIsNone(MOD.parse_row_audit(sample_records()))
        legacy=dict(data)
        for key in ("auditVersion", "ordinaryObjectiveRows",
                    "unknownParentChildren", "schemaObjectiveDisplay"):
            del legacy[key]
        legacy_result=MOD.parse_row_audit(
            MOD.ROW_AUDIT_MARKER+json.dumps(legacy)+MOD.ROW_AUDIT_TRAILER)
        self.assertNotIn("auditVersion", legacy_result)
        self.assertEqual(len(MOD.parse_lines(content)), 4)
        for bad in [
            {**data, "rootRows": 3},
            {**data, "truncated": True},
            {**data, "schemaQuestRoot": ["FormID:2\nprivate_id:4"]},
            {**data, "statusCounts": statuses[:-1]},
            {**data, "bothKeys": 9},
            {**data, "ordinaryObjectiveRows": 512},
            {**data, "unknownParentChildren": 1},
            {**data, "auditVersion": 3},
            {**data, "schemaObjectiveDisplay": ["formID:99"]},
        ]:
            with self.subTest(type=next((k for k in bad if bad[k]!=data[k]),'?')):
                raw=MOD.ROW_AUDIT_MARKER+json.dumps(bad)+MOD.ROW_AUDIT_TRAILER
                with self.assertRaises(MOD.CaptureError):
                    MOD.parse_row_audit(raw)
        with self.assertRaises(MOD.CaptureError):
            MOD.parse_row_audit(content+MOD.ROW_AUDIT_MARKER+
                                json.dumps(data)+MOD.ROW_AUDIT_TRAILER)

    def test_optional_decoder_is_local_and_read_only(self):
        if not MOD.shutil.which("objdump"):
            self.skipTest("binutils unavailable")
        sample = MOD.parse_lines(sample_records())[0]
        decoded = MOD.disassemble(sample)
        self.assertIn("nop", decoded.lower())


if __name__ == "__main__":
    unittest.main()
