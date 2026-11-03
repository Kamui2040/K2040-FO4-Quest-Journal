"""Synthetic read-only native dispatch log tests; no game code or user data."""
import importlib.util
from pathlib import Path
import unittest

ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location(
    "dispatch_qa",ROOT/"scripts/analyze-pipboy-dispatch-prefix-log.py")
mod=importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)
source=ROOT/"tests/test_pipboy_codeobj_type_log.py"
base_spec=importlib.util.spec_from_file_location("type_fixture",source)
fixture=importlib.util.module_from_spec(base_spec)
base_spec.loader.exec_module(fixture)

def record(status="captured",writer="false",abi="false",window=True):
    text=fixture.sample("setquestactive-not-closure","object")
    intro=(f"PipboyDispatchNativeReadOnlyQA: status={status} "
           f"callbackAbiVerified={abi} writeAuthorized={writer} "
           "(NO invocation)\n")
    parts=[]
    if window:
        for method,rva in mod.EXPECTED.items():
            for index in range(mod.WINDOW_COUNT):
                pattern=("a5" if method=="call" else "5a")
                parts.append(
                    f"PipboyDispatchNativeReadOnlyQA: method={method} "
                    f"window={index} rva={rva+index*mod.WINDOW_BYTES} "
                    f"hex={pattern*mod.WINDOW_BYTES}\n")
    return text.replace(
        "PipboyGfxReadOnlyQA: END one-shot read-only\n",
        intro+"".join(parts)+"PipboyGfxReadOnlyQA: END one-shot read-only\n")

class DispatchLogTest(unittest.TestCase):
    def test_complete_capture(self):
        result=mod.inspect(record())
        self.assertEqual(result["capture"],"read-only-bounded")
        self.assertFalse(result["callback_abi_verified"])
        for method in ("call","map"):
            self.assertEqual(result["methods"][method]["bytes"],768)
    def test_valid_fail_closed_gates(self):
        for status in mod.ERRORS:
            self.assertEqual(mod.inspect(record(status,window=False))["status"],status)
    def test_rejects_unsafe_malformed_or_duplicate(self):
        src=record()
        for data in (
            record(writer="true"),record(abi="true"),
            record(status="pe-mismatch"),
            src.replace("window=2", "window=1",1),
            src.replace("rva="+str(mod.EXPECTED["call"]), "rva=0",1),
            src.replace("hex="+"a5"*192,"hex=12",1),
            src+"PipboyDispatchNativeReadOnlyQA: method=map window=0 rva=0 hex=aa\n",
            src.replace("memberType=object","memberType=closure"),
            src.replace("status=captured","status=unknown"),
        ):
            with self.subTest(case=data[-90:]):
                with self.assertRaises(ValueError):
                    mod.inspect(data)

if __name__=="__main__":unittest.main()
