"""Verify the actual departure factory, post-boarding rearm and native point flight."""
from pathlib import Path
import runpy,re,struct
from capstone import CS_OP_IMM,CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
p=runpy.run_path(str(Path(__file__).with_name('verify_warswords_final_credit_release.py')))
n,ins=p['native'],p['instructions']
def refs(method):
 for x in ins(method):
  for o in x.operands:
   if o.type==CS_OP_IMM:yield o.imm
   elif o.type==CS_OP_MEM and o.mem.base==X86_REG_RIP:yield x.address+x.size+o.mem.disp
def targets(prefix):return {n['resolve'](v) for k,v in n['symbols'].items() if k.startswith(prefix)}
def needs(method,prefix):assert n['calls'](method)&targets(prefix),(method,prefix)
update='?UpdateAI@DepartureAI@npc_boost_exit@@'
factory='?GetAI@npc_boost_exit@@'
mapping=(n['args'].release/'worldserver.map').read_text(errors='replace')
match=re.search(re.escape('??_7DepartureAI@npc_boost_exit@@6B@')+r'\s+([0-9a-fA-F]{16})',mapping)
assert match
address=int(match[1],16)
assert address in refs(factory)
assert n['symbol'](update) in struct.unpack_from('<100Q',n['binary'],n['offset'](address))
for prefix in ('?SetClientControl@Player@@','?SetCanFly@Unit@@','?SetDisableGravity@Unit@@','?MovePoint@MotionMaster@@','?EnterVehicle@Unit@@','?CastSpell@Unit@@'):
 needs(update,prefix)
body=ins(update)
def sites(prefix):return [i for i,x in enumerate(body) if x.mnemonic=='call' and x.operands[0].type==CS_OP_IMM and n['resolve'](x.operands[0].imm) in targets(prefix)]
takeoff=sites('?MovePoint@MotionMaster@@')
assert len(takeoff)==1
fly=sites('?SetCanFly@Unit@@');gravity=sites('?SetDisableGravity@Unit@@');control=sites('?SetClientControl@Player@@')
assert len(fly)==len(gravity)==2
assert any(c<fly[-1]<gravity[-1]<takeoff[0] for c in control),'Missing actual post-control flight rearm before takeoff'
needs('?UpdateMovementFlags@Creature@@','?SetDisableGravity@Unit@@')
needs('?UpdateMovementFlags@Creature@@','?GetMovementTemplate@Creature@@')
needs('?DoInitialize@?$PointMovementGenerator@VCreature@@@@','??0MoveSplineInit@Movement@@')
print('PASS actual departure factory/vtable uses boarding and native control, two flight flag pairs with the second after control and before sole MovePoint; native template flag upkeep and Point spline constructor linked. Data/host tests own Flight1, Fly tier, waits/failures/cleanup; actual client wings/camera/pet remain pending.')
