"""Trace the loaded native attachment store, scale correction and client notice."""
from pathlib import Path
import runpy
import sys
import re
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

assert len(sys.argv)==3, 'Expected Release directory and native ROW_LAYOUT export'
layout_path=Path(sys.argv[2])
release_dir=Path(sys.argv[1])
sys.argv=sys.argv[:2] # Shared PE reader accepts only the Release argument.
p=runpy.run_path(str(Path(__file__).with_name('verify_stromkar_execute_jump_release.py')))
n,ins=p['n'],p['instructions']
layout=next(line.split('\t') for line in layout_path.read_text(encoding='utf-8-sig').splitlines() if line.startswith('ROW_LAYOUT\t'))
size,scale,table,fields=map(int,layout[1:])
assert fields==26 and 0<scale<size-4
def values(method):return {o.imm for x in ins(method) for o in x.operands if o.type==CS_OP_IMM}
def refs(method):return {x.address+x.size+o.mem.disp for x in ins(method) for o in x.operands if o.type==CS_OP_MEM and o.mem.base==X86_REG_RIP}
def needs(method,prefix):
    assert {n['resolve'](a) for name,a in n['symbols'].items() if name.startswith(prefix)} & n['calls'](method), (method,prefix)
stores=re.findall(r'\?sSpellVisualKitModelAttachStore@@\S+\s+([0-9A-Fa-f]{16})\s+', (release_dir/'worldserver.map').read_text())
assert len(stores)==1, 'Actual full typed attachment data symbol missing'
store=int(stores[0],16) # Shared symbol helper intentionally indexes functions only.
load='?LoadStores@DB2Manager@@'
apply='?Apply@StromkarBladeVisual@@'
extra='?LoadingExtraHotfixData@DB2Manager@@'
assert store in refs(load) and store in refs(extra), 'Full typed store is not loaded/applied in actual DB2 lifecycle'
needs(extra,apply)
assert {318611,64756,26999,255,19} <= values(apply), 'Scoped native record/kit/item attachment/positioner guards missing'
assert any(x.mnemonic=='mov' and x.operands[0].type==CS_OP_MEM and x.operands[0].size==4
           and x.operands[0].mem.disp==scale and x.operands[1].type==CS_OP_IMM
           and x.operands[1].imm==0x3e800000 for x in ins(apply)), 'Actual native scale field is not set to0.25'
needs(apply,'?AddClientHotfix@DB2HotfixGeneratorBase@@')
needs('?AddClientHotfix@DB2HotfixGeneratorBase@@','?InsertNewHotfix@DB2Manager@@')
for method in ('?HandleHotfixRequest@WorldSession@@','?HandleDBQueryBulk@WorldSession@@'):
    needs(method,'?GetStorage@DB2Manager@@')
    needs(method,'?WriteRecord@DB2StorageBase@@')
needs('?SendHotfixList@WorldSession@@','?GetHotfixData@DB2Manager@@')
assert b'LowDefModelAttachID, StartDelay, SpellVisualKitID FROM spell_visual_kit_model_attach' in n['binary'], 'Prepared native parent-field projection missing'
print(f'PASS selected full26-field store loaded in DB2 lifecycle, scoped318611/64756 custom scale0.25 at native offset{scale}, generated client notice and both actual hotfix record serializers; table hash{table}. Rendering/client cache acceptance remain separate.')
