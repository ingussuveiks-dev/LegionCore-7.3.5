"""Selected Erunak factory/vtable and real linked evade/route cleanup.

Native host establishes behavior separately; this establishes final-link identity.
"""
from pathlib import Path
import bisect,runpy,struct
from capstone import Cs,CS_ARCH_X86,CS_MODE_64,CS_OP_IMM,CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
n=runpy.run_path(str(Path(__file__).with_name('verify_scenario_lfg_release.py')))
cs=Cs(CS_ARCH_X86,CS_MODE_64);cs.detail=True
def ins(prefix):
 address=n['symbol'](prefix);i=bisect.bisect_right(n['starts'],address-n['base'])-1
 begin,end,unwind=n['functions'][i];assert begin<=address-n['base']<end
 origin=n['root'](begin,unwind)
 for b,e,u in n['functions'][i+1:]:
  if b!=end or n['root'](b,u)!=origin:break
  end=e
 at=n['offset'](address);return list(cs.disasm(n['binary'][at:at+n['base']+end-address],address))
suffix='@npc_erunak_stonespeakerAI@npc_erunak_stonespeaker@@'
def method(name):return '?'+name+suffix
def sites(body,prefix):
 targets={n['resolve'](addr) for name,addr in n['symbols'].items() if name.startswith(prefix)}
 return [i for i,x in enumerate(body) if x.mnemonic in ('call','jmp') and x.operands[0].type==CS_OP_IMM and n['resolve'](x.operands[0].imm) in targets]
evade=ins(method('EnterEvadeMode'));complete=ins(method('CompleteEvadeMovement'));life=ins(method('UpdateLifecycle'))
for name in ('?DeleteThreatList@Unit@@','?CombatStop@Unit@@','?GetMotionSlotType@MotionMaster@@'):
 assert sites(evade,name),name
assert sites(complete,'?MovementExpired@MotionMaster@@')
for name in ('?MoveTargetedHome@MotionMaster@@','?RemoveAllAuras@Unit@@',method('Reset'),'?UpdateAchievementCriteria@Player@@'):
 assert not sites(evade,name) and not sites(complete,name),name
assert sites(life,method('CompleteEvadeMovement'))
assert sites(life,'?Update@FunctionProcessor@@') and sites(life,'?Recover@SharasdalCompanion@@')
assert sites(evade,method('CompleteEvadeMovement')) or sites(evade,'?MovementExpired@MotionMaster@@')
tables=[]
for x in ins('?GetAI@npc_erunak_stonespeaker@@'):
 for op in x.operands:
  if op.type!=CS_OP_MEM or op.mem.base!=X86_REG_RIP:continue
  address=x.address+x.size+op.mem.disp
  try:rows=struct.unpack_from('<90Q',n['binary'],n['offset'](address))
  except (ValueError,struct.error):continue
  if all(n['symbol'](method(m)) in rows for m in ('EnterEvadeMode','UpdateLifecycle','EnterCombat','MovementInform')):tables.append(address)
assert len(set(tables))==1,'Selected Erunak vtable differs'
print('PASS selected linked Erunak combat cleanup/native route expiry/lifecycle and retained wound/story dispatch;no client claim')
