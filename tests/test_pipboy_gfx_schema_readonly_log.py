import importlib.util
from pathlib import Path
import unittest
root=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('schema_qa',root/'scripts/analyze-pipboy-gfx-schema-readonly-log.py')
mod=importlib.util.module_from_spec(spec);spec.loader.exec_module(mod)
base=('PipboyGfxReadOnlyQA: READY;\nPipboyGfxReadOnlyQA: UI task submitted\n'
      'PipboyGfxReadOnlyQA: result=native-misc-list-child-selected rootFormIDTyped=true '
      'syntheticMiscParent=true nativeListSelection=true childFormIDTyped=true '
      'childInstanceTyped=true childTypeTyped=false childQuestTypeTyped=false '
      'tripleVerified=false writeAuthorized=false (NO game mutation)\n')
line=('PipboySchemaReadOnlyQA: available=true overflow=false total=6 integer=3 '
      'number=1 boolean=1 string=1 object=0 array=0 other=0 nameMask=3 numericNameMask=3 ordinalMask=7 numericOrdinalMask=3 '
      'tripleVerified=false writeAuthorized=false\n')
end='PipboyGfxReadOnlyQA: END one-shot read-only\n'
class SchemaLog(unittest.TestCase):
    def test_valid(self):
        obj=mod.inspect(base+line+end)
        self.assertEqual(obj['members'],6)
        self.assertEqual(obj['whitelisted_name_mask'],3)
        self.assertEqual(obj['numeric_name_mask'],3)
        self.assertEqual(obj['known_fields'],['formID','instance'])
        self.assertEqual(obj['numeric_known_fields'],['formID','instance'])
        self.assertEqual(obj['unlisted_field_count'],4)
        self.assertEqual(obj['unlisted_numeric_field_count'],2)
        self.assertFalse(obj['native_write_authorized'])
    def test_accepted_shape_without_private_values(self):
        # Synthetic schema reproducing only anonymous statistics of
        # the accepted runtime capture, not any user's quest data.
        sample=(base+'PipboySchemaReadOnlyQA: available=true overflow=false '
                'total=8 integer=3 number=0 boolean=4 string=1 object=0 '
                'array=0 other=0 nameMask=16435 numericNameMask=3 '
                'ordinalMask=0 numericOrdinalMask=0 '
                'tripleVerified=false writeAuthorized=false\n'+end)
        obj=mod.inspect(sample)
        self.assertEqual(obj['known_fields'],
            ['formID','instance','active','enabled','text'])
        self.assertEqual(obj['numeric_known_fields'],['formID','instance'])
        self.assertEqual(obj['unlisted_field_count'],3)
        self.assertEqual(obj['unlisted_numeric_field_count'],1)
        self.assertEqual(obj['ordinal_name_mask'],0)
        self.assertFalse(obj['native_command_triple_verified'])
    def test_fixed_whitelist_agrees_with_cpp(self):
        import re
        hdr=(root/'research/native_pipboy_selection/ReadOnlySelectedSchema.h').read_text()
        literal=hdr.split('kAllowedNames[] = {',1)[1].split('};',1)[0]
        fields=re.findall(r'"([A-Za-z0-9]+)"',literal)
        self.assertEqual(fields,list(mod.ALLOWED_NAMES))
    def test_fail_closed(self):
        for text in (base+end,base+line+line+end,
            base+line.replace('total=6','total=60')+end,
            base+line.replace('nameMask=3','nameMask=65536')+end,
            base+line.replace('overflow=false','overflow=true')+end,
            base+line.replace('writeAuthorized=false','writeAuthorized=true')+end,
            base+line.replace('integer=3','integer=4')+end,
            base+line.replace('total=6','total=1').replace('integer=3','integer=0').replace('number=1','number=0').replace('boolean=1','boolean=0').replace('string=1','string=1')+end):
            with self.subTest(case=text[-130:]):
                with self.assertRaises(ValueError):mod.inspect(text)
if __name__=='__main__':unittest.main()
