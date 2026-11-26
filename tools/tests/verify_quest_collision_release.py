"""Verify actor-aware collision queries in the final x64 PE, including selected filter."""
from pathlib import Path
import re,runpy,struct
from capstone import CS_OP_IMM
p=runpy.run_path(str(Path(__file__).with_name('verify_frost_scene_equip_release.py')))
n,ins,refs=p['n'],p['ins'],p['refs']
stamp=re.search(r'Timestamp is ([0-9A-Fa-f]+)',(n['args'].release/'worldserver.map').read_text(errors='replace'))
assert stamp and int(stamp[1],16)==struct.unpack_from('<I',n['binary'],n['pe']+8)[0]
def require(caller,*targets):
 for target in targets:assert n['symbol'](target) in n['calls'](caller),(caller,target)
def anycall(caller,prefix):return any(n['resolve'](v) in n['calls'](caller) for name,v in n['symbols'].items() if name.startswith(prefix))
owner='?CanCollideWith@GameObject@@'
require(owner,'?GetAnyOwner@Unit@@')
assert 16 in {o.imm for i in ins(owner) for o in i.operands if o.type==CS_OP_IMM}
ctor='??0QuestCollisionQuery@@'
invoke='?_Do_call@?$_Func_impl_no_alloc@V<lambda_1>@?1???0QuestCollisionQuery@@'
require(invoke,owner)
table=p['data_symbol']('??_7?$_Func_impl_no_alloc@V<lambda_1>@?1???0QuestCollisionQuery@@')
assert table in refs(ctor)
# MSVC stores _Do_call in slot 2, which can point to an incremental-link thunk.
dispatch=struct.unpack_from('<Q',n['binary'],n['offset'](table)+2*8)[0]
assert n['resolve'](dispatch)==n['symbol'](invoke)
height='?GetHeight@Map@@QEBAMAEB'
los='?isInLineOfSight@Map@@'
for caller,target in [(height,'?getHeight@DynamicMapTree@@'),(los,'?isInLineOfSight@DynamicMapTree@@')]:
 require(caller,ctor,target)
for name in n['symbols']:
 if name.startswith('?getObjectHitPos@Map@@'):require(name,ctor,'?getObjectHitPos@DynamicMapTree@@')
require('?GetHeight@WorldObject@@',height)
require('?IsWithinLOS@WorldObject@@',los)
require('?GetWaterOrGroundLevel@Map@@',height)
require('?BuildPlayerArrivalObjects@Map@@',height)
for caller in (owner,ctor,invoke):
 for forbidden in ('?KilledMonsterCredit@Player@@','?CompleteQuest@Player@@','?AddItem@Player@@','?TeleportTo@Player@@'):
  assert not anycall(caller,forbidden),(caller,forbidden)
print('PASS final PE actor-aware Map LOS/contact/height query construction and WorldObject/arrival consumers, selected native filter vtable/CanCollideWith, bounded16 native owner walk; no progression/item/transfer grants. Actual two-owner mesh/open/reset/cancellation checks are native-test evidence, not client traversal.')
