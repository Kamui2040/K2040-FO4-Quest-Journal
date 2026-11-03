import importlib.util
from pathlib import Path
import unittest
base=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location("gfx_decoder",base/"scripts/analyze-pipboy-gfx-readonly-log.py")
mod=importlib.util.module_from_spec(spec);spec.loader.exec_module(mod)
START="[info] PipboyGfxReadOnlyQA: READY; Ctrl+Shift+F11 inside vanilla Pip-Boy Quests/Misc"
SUBMIT="[info] PipboyGfxReadOnlyQA: UI task submitted"
END="[info] PipboyGfxReadOnlyQA: END one-shot read-only"
def sample(status="native-misc-list-child-selected",root="true",misc="true",
           from_list="true",form="true",inst="true",kind="true",qt="false",
           triple="false",writer="false"):
    return "\n".join((
      START,SUBMIT,
      f"[info] PipboyGfxReadOnlyQA: result={status} rootFormIDTyped={root} "
      f"syntheticMiscParent={misc} nativeListSelection={from_list} "
      f"childFormIDTyped={form} childInstanceTyped={inst} childTypeTyped={kind} "
      f"childQuestTypeTyped={qt} tripleVerified={triple} writeAuthorized={writer} "
      "(NO game mutation)",
      END))
class Decode(unittest.TestCase):
    def test_misc_and_ordinary(self):
        v=mod.inspect(sample())
        self.assertTrue(v["native_list_selection"])
        self.assertFalse(v["native_write_authorized"])
        ordinary=mod.inspect(sample("ordinary-root-selected","true","false",
                                    "false","false","false","false"))
        self.assertFalse(ordinary["native_list_selection"])
    def test_failure_cases(self):
        for input in (
            sample().replace(SUBMIT,""),
            sample()+"\n"+sample(),
            sample(writer="true"),
            sample(triple="true"),
            sample(from_list="false"),
            sample(status="fake-status"),
            sample(status="ordinary-root-selected",misc="false"),
            sample(status="misc-root-child-not-selected",
                   from_list="false",form="false",inst="false",kind="false",
                   root="false"),
        ):
            with self.subTest(case=input[-60:]):
                with self.assertRaises(ValueError):mod.inspect(input)
if __name__=="__main__":unittest.main()
