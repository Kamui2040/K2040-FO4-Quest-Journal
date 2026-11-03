"""Synthetic QA log coverage: no user quest data or private logs."""
import importlib.util
from pathlib import Path
import unittest

ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location(
    "codeobj_type",ROOT/"scripts/analyze-pipboy-codeobj-type-log.py")
mod=importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)
prefix=(
    "PipboyGfxReadOnlyQA: READY;\n"
    "PipboyGfxReadOnlyQA: UI task submitted\n"
    "PipboyGfxReadOnlyQA: result=native-misc-list-child-selected "
    "rootFormIDTyped=true syntheticMiscParent=true "
    "nativeListSelection=true childFormIDTyped=true "
    "childInstanceTyped=true childTypeTyped=false "
    "childQuestTypeTyped=false tripleVerified=false "
    "writeAuthorized=false (NO game mutation)\n"
)
schema=(
    "PipboySchemaReadOnlyQA: available=true overflow=false "
    "total=8 integer=3 number=0 boolean=4 string=1 "
    "object=0 array=0 other=0 nameMask=16435 numericNameMask=3 "
    "ordinalMask=0 numericOrdinalMask=0 "
    "tripleVerified=false writeAuthorized=false\n"
)
suffix="PipboyGfxReadOnlyQA: END one-shot read-only\n"

def sample(state,member,abi="false",writer="false"):
    binding=("PipboyCodeObjectReadOnlyQA: state="+state+" memberType="+member+
             " callbackAbiVerified="+abi+" writeAuthorized="+writer+
             " (NO invocation)\n")
    return prefix+binding+schema+suffix

class CodeObjectLog(unittest.TestCase):
    def test_exact_type_and_no_writes(self):
        for kind in mod.TYPES:
            state=("setquestactive-closure" if kind=="closure"
                   else "setquestactive-not-closure")
            parsed=mod.inspect(sample(state,kind))
            self.assertEqual(parsed["member_type"],kind)
            self.assertFalse(parsed["native_callback_abi_verified"])
            self.assertFalse(parsed["native_write_authorized"])
    def test_unobserved_type_only_when_missing(self):
        for state in mod.MISSING:
            obj=mod.inspect(sample(state,"not-observed"))
            self.assertEqual(obj["binding_state"],state)
    def test_inconsistent_and_unsafe_rejected(self):
        for source in (
            sample("setquestactive-closure","object"),
            sample("setquestactive-not-closure","closure"),
            sample("setquestactive-not-closure","not-observed"),
            sample("setquestactive-absent","object"),
            sample("setquestactive-closure","closure",abi="true"),
            sample("setquestactive-closure","closure",writer="true"),
            sample("setquestactive-closure","unknown-type"),
            sample("setquestactive-closure","closure")+
                "PipboyCodeObjectReadOnlyQA: invalid\n",
            prefix+schema+suffix,
            sample("setquestactive-closure","closure").replace(
                "tripleVerified=false","tripleVerified=true"),
        ):
            with self.subTest(case=source[-90:]):
                with self.assertRaises(ValueError):
                    mod.inspect(source)

if __name__=="__main__":
    unittest.main()
