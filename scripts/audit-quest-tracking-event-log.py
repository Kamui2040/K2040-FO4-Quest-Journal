#!/usr/bin/env python3
"""Offline audit of the isolated OG TESQuestEvent observer log.

Only aggregate results are printed: never expose private runtime FormIDs,
save names, local file paths or raw quest-state event lines.
--require-both validates a 0->1 and 1->0 tracking sequence in one quest.
No game, save, installed mod or log modifications are performed.
"""
import argparse
from collections import defaultdict
import json
from pathlib import Path
import re

EVENT = re.compile(
    r"QuestEventObserver: event=(\d+) form=([0-9A-Fa-f]{8}) "
    r"tracked=(true|false) enabled=(true|false) "
    r"completed=(true|false) failed=(true|false) epoch=(\d+)"
)
THREAD = re.compile(r"\[\s*(\d+)\s*\]\s+\[[A-Z]\]")
VALIDATED = "QuestEventObserver: verified two engine call destinations"
SUBSCRIBED = "QuestEventObserver: subscribed (vanilla events only)"
INITIALIZED = "K2040_Quest_Event_Observer v"
POSTSAVE = "QuestEventObserver: save boundary=5"


def audit(text: str) -> dict:
    events = []
    by_quest = defaultdict(list)
    for line in text.splitlines():
        match = EVENT.search(line)
        if not match:
            continue
        typ, form, tracked, enabled, completed, failed, epoch = match.groups()
        evt = dict(
            kind=int(typ),
            form=form.upper(),
            tracked=tracked == "true",
            enabled=enabled == "true",
            completed=completed == "true",
            failed=failed == "true",
            epoch=int(epoch),
        )
        thread = THREAD.search(line)
        if thread:
            evt["thread"] = thread.group(1)
        events.append(evt)
        if evt["kind"] == 0:
            by_quest[(evt["form"], evt["epoch"])].append(evt)

    successful_quests = 0
    for samples in by_quest.values():
        states = [e["tracked"] for e in samples
                  if e["enabled"] and not e["completed"] and not e["failed"]]
        transitions = list(zip(states, states[1:]))
        if (False, True) in transitions and (True, False) in transitions:
            successful_quests += 1

    threads = {str(e["thread"]) for e in events if "thread" in e}
    kinds = defaultdict(int)
    for e in events:
        kinds[str(e["kind"])] += 1
    return {
        "plugin_initialized": INITIALIZED in text,
        "runtime_og_signature_verified": VALIDATED in text,
        "engine_event_subscription_confirmed": SUBSCRIBED in text,
        "event_count": len(events),
        "tracking_event_count": kinds["0"],
        "stage_event_count": kinds["2"],
        "event_kinds": dict(sorted(kinds.items())),
        "distinct_observed_threads": len(threads),
        "completed_save_boundary_observed": POSTSAVE in text,
        "completed_on_off_quest_sequences": successful_quests,
        "vanilla_tracking_event_qa_pass": (
            VALIDATED in text and SUBSCRIBED in text and successful_quests > 0
        ),
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log", type=Path)
    parser.add_argument("--require-both", action="store_true",
                        help="nonzero exit status without verified on/off events")
    args = parser.parse_args()
    report = audit(args.log.read_text(encoding="utf-8", errors="replace"))
    print(json.dumps(report, indent=2, sort_keys=True))
    if args.require_both and not report["vanilla_tracking_event_qa_pass"]:
        parser.exit(2, "FAIL: incomplete OG quest tracking event evidence\n")


if __name__ == "__main__":
    main()
