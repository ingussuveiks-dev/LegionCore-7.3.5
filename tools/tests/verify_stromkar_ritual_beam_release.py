"""Inspect the registered three-actor ritual beam destination/GUID correction."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
p=runpy.run_path(str(Path(__file__).with_name('verify_stromkar_awakening_release.py')))
n,ins,values=(p[k] for k in ('n','ins','values'))
name='spell_stromkar_ritual_beam@@'
load,receiver,check,launch,register=('?'+x+'@'+name for x in ('Load','Receiver','Check','Launch','Register'))
assert {101875,1539}<=values(load)
assert {201032,1037,103144,370922}<=values(receiver)
for method,prefix in ((check,'?Receiver@'+name),(launch,'?Receiver@'+name),(launch,'?GetHitDest@SpellScript@@'),(launch,'?SetExplTargetDest@SpellScript@@'),(launch,'?SetUnitTarget@SpellCastTargets@@')):
    p['calls_prefix'](method,prefix)
assert {n['symbol'](check),n['symbol'](launch)}<=p['references'](register)
assert b'spell_stromkar_ritual_beam\0' in n['binary']
print('PASS selected ritual script native scope/receiver and registered Check/Launch callbacks; real destination plus explicit GUID before native SpellGo. Client beam appearance remains pending.')
