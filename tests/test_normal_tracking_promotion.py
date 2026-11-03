"""Guard the explicit normal-product R2 tracking variant, not a QA build."""
import importlib.util
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

R=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location("journal_pe_flavors",R/"scripts/check-built-dll.py")
mod=importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)

class NormalTrackingPromotion(unittest.TestCase):
    def test_no_silent_writer_and_shared_accepted_source(self):
        x=(R/'xmake.lua').read_text()
        build=(R/'scripts/build-linux.sh').read_text()
        wrapper=(R/'scripts/build-normal-tracking.sh').read_text()
        candidate=(R/'scripts/build-questwide-candidate.sh').read_text()
        self.assertIn('option("quest_questwide_normal_tracking")',x)
        self.assertIn('set_default(false)',x)
        self.assertIn('has_config("quest_questwide_normal_tracking")',x)
        self.assertIn('has_config("quest_questwide_production_candidate")',x)
        self.assertIn('Two quest-wide output flavors cannot share',x)
        self.assertIn('research/production_candidate/QuestWideProductionCandidate.cpp',x)
        self.assertIn('research/production_candidate/OrdinaryQuestEvidence.cpp',x)
        self.assertIn('K2040_QUESTWIDE_PRODUCTION_CANDIDATE=1',x)
        self.assertIn('--quest_questwide_production_candidate=n',build)
        self.assertIn('--quest_questwide_normal_tracking="$tracking_flag"',build)
        self.assertIn('K2040_QUESTWIDE_NORMAL_TRACKING:-NO',build)
        self.assertIn('python3 scripts/check-built-dll.py --dll "$dll" --require-readonly-shipping',build)
        self.assertIn('python3 scripts/check-built-dll.py --dll "$dll" --require-questwide-tracking',build)
        self.assertIn('export K2040_QUESTWIDE_NORMAL_TRACKING=YES',wrapper)
        self.assertIn('exec bash "$root/scripts/build-linux.sh"',wrapper)
        self.assertIn('--quest_questwide_production_candidate=y',candidate)
        self.assertNotIn('quest_questwide_normal_tracking=y',candidate)
        self.assertIn('kVerifiedBackend = WriteBackend::kNone',(R/'src/QuestTrackingPolicy.h').read_text())
        self.assertIn('SetActive RETURNED',(R/'research/production_candidate/QuestWideProductionCandidate.cpp').read_text())

    def test_binaries_must_contain_exact_accepted_writer_and_no_qa_writer(self):
        def run(*chunks,readonly=False,tracked=False):
            with tempfile.TemporaryDirectory() as dirname:
                f=Path(dirname)/'test.dll';f.write_bytes(b'|'.join(chunks))
                with patch.object(mod.package,'inspect_pe',return_value=True):
                    return mod.check_dll(f,require_readonly_shipping=readonly,
                                         require_questwide_tracking=tracked)
        good=mod.VERIFIED_QUESTWIDE_MARKERS
        self.assertEqual(len(good),3)
        run(*good,tracked=True)
        for missing in good:
            with self.subTest(missing=missing):
                with self.assertRaisesRegex(ValueError,'accepted R2 quest-wide native writer missing'):
                    run(*(g for g in good if g!=missing),tracked=True)
        with self.assertRaisesRegex(ValueError,'QA or experimental writer'):
            run(*good,b'QuestRepeatableQA: SetActive ENTER',tracked=True)
        with self.assertRaisesRegex(ValueError,'experimental native tracking flavor'):
            run(*good,readonly=True)
        with self.assertRaisesRegex(ValueError,'contradictory'):
            run(*good,readonly=True,tracked=True)
        run(b'NO_WRITER',readonly=True)

if __name__=='__main__': unittest.main()
