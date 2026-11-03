import importlib.util
import pathlib
import unittest

script = pathlib.Path(__file__).resolve().parents[1] / "scripts/audit-quest-tracking-event-log.py"
spec = importlib.util.spec_from_file_location("tracking_audit", script)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

HEAD = """[05:22:19] [ 380 ] [I] K2040_Quest_Event_Observer v0.0.1.0
[05:22:26] [ 608 ] [I] QuestEventObserver: verified two engine call destinations; acquiring source
[05:22:26] [ 608 ] [I] QuestEventObserver: subscribed (vanilla events only)
"""
EV = "[05:23:09] [ {thread} ] [I] QuestEventObserver: event={kind} form={form} tracked={track} enabled=true completed=false failed=false epoch={epoch}\n"


def e(kind, form, track, epoch=2, thread=380):
    return EV.format(kind=kind, form=form, track="true" if track else "false",
                     epoch=epoch, thread=thread)


class TrackingAuditTest(unittest.TestCase):
    def test_two_way_tracking_ignores_stage_events(self):
        payload = (HEAD + e(0, "ABCDEF12", False)
                   + e(2, "ABCDEF12", False, thread=528)
                   + e(0, "ABCDEF12", True)
                   + e(0, "ABCDEF12", False))
        report = module.audit(payload)
        self.assertTrue(report["vanilla_tracking_event_qa_pass"])
        self.assertEqual(report["completed_on_off_quest_sequences"], 1)
        self.assertEqual(report["stage_event_count"], 1)
        self.assertEqual(report["tracking_event_count"], 3)
        self.assertEqual(report["distinct_observed_threads"], 2)
        self.assertNotIn("ABCDEF12", str(report))

    def test_missing_subscription_fails(self):
        payload = e(0, "12345678", False) + e(0, "12345678", True) + e(0, "12345678", False)
        self.assertFalse(module.audit(payload)["vanilla_tracking_event_qa_pass"])

    def test_disjoint_quests_and_epochs_cannot_match(self):
        payload = HEAD + e(0, "12345678", False) + e(0, "12345678", True)
        payload += e(0, "87654321", True) + e(0, "87654321", False)
        self.assertFalse(module.audit(payload)["vanilla_tracking_event_qa_pass"])
        payload = HEAD + e(0, "12345678", False, epoch=1)
        payload += e(0, "12345678", True, epoch=2) + e(0, "12345678", False, epoch=2)
        self.assertFalse(module.audit(payload)["vanilla_tracking_event_qa_pass"])

    def test_reject_ineligible_or_bad_log_records(self):
        payload = HEAD + e(0, "87654321", False)
        payload += EV.format(kind=0, form="87654321", track="true", thread=380, epoch=2).replace(
            "completed=false", "completed=true")
        payload += e(0, "87654321", False)
        payload += "QuestEventObserver: event=0 form=DEADBEEF tracked=bogus enabled=true completed=false failed=false epoch=2\n"
        report = module.audit(payload)
        self.assertFalse(report["vanilla_tracking_event_qa_pass"])
        self.assertEqual(report["tracking_event_count"], 3)


if __name__ == "__main__":
    unittest.main()
