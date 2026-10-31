"""Prove actual registered Taran owns both captures and their native clock."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
from sharasdal_erunak_entry_pe import sites, values, fields, floats

retained=runpy.run_path(str(Path(__file__).with_name('verify_sheilun_taran_intro_release.py')))
n=retained['n'];ins=n['instructions'];table=retained['table']
suffix='@npc_tazhan_chuAI@npc_tazhan_chu@@'
def method(name):return '?'+name+suffix
def refs(body):
 return [x.address+x.size+o.mem.disp for x in body if x.mnemonic=='lea'
         for o in x.operands if o.type==CS_OP_MEM and o.mem.base==X86_REG_RIP]
def contains_callback(body,callback):
 for at in refs(body):
  raw=struct.unpack_from('<8Q',n['binary'],n['offset'](at))
  for address in raw:
   try:
    if n['resolve'](address)==n['symbol'](callback):return True
   except ValueError:pass
 return False

factory=ins('?GetAI@npc_tazhan_chu@@')
assert factory[6].op_str=='ecx, 0x190'
constructors=sites(n,factory,'??0FunctionProcessor@@')
assert len(constructors)==2
assert [factory[i-1].op_str for i in constructors]==['rcx, [rsi + 0x60]','rcx, [rsi + 0x110]']
los=ins(method('MoveInLineOfSight'));move=ins(method('MovementInform'))
life=ins(method('UpdateLifecycle'));combat=ins(method('UpdateAI'))
for body,delay,operand in [(los,5000,'rcx, [rbx + 0x110]'),(move,4000,'rcx, [rsi + 0x110]')]:
 queued,=sites(n,body,'?AddDelayedEvent@FunctionProcessor@@')
 assert body[queued-3].op_str==operand
 assert body[queued-1].op_str==f'edx, {delay:#x}'
 assert not sites(n,body,'?AddDelayedEvent@Unit@@')
clock,=sites(n,life,'?Update@FunctionProcessor@@')
assert [(x.mnemonic,x.op_str) for x in life[clock-2:clock]]==[
 ('lea','rcx, [rdi + 0x110]'),('mov','edx, esi')]
assert sites(n,life,'?RemoveAura@Unit@@')[0]<clock<sites(n,life,'?IsEmpty@ObjectGuid@@')[0]
assert not any(x.mnemonic in ('je','jne','ja','jb') and x.operands[0].imm>life[clock].address for x in life[:clock])
old_clock,=sites(n,combat,'?Update@FunctionProcessor@@')
assert (combat[old_clock-1].mnemonic,combat[old_clock-1].op_str)==('add','rcx, 0x60')
assert any(x.mnemonic=='jne' and x.operands[0].imm>combat[old_clock].address for x in combat[:old_clock])
destructor_name='??_Gnpc_tazhan_chuAI@npc_tazhan_chu@@'
assert n['resolve'](struct.unpack_from('<Q',n['binary'],n['offset'](table))[0])==n['symbol'](destructor_name)
destructor=ins(destructor_name);destroy=sites(n,destructor,'??1FunctionProcessor@@')
assert len(destroy)==2
assert [(destructor[i-1].mnemonic,destructor[i-1].op_str) for i in destroy]==[
 ('add','rcx, 0x110'),('lea','rcx, [rdi + 0x60]')]
callbacks=[name for name in n['symbols'] if name.startswith('?_Do_call@') and
           ('??MoveInLineOfSight@npc_tazhan_chuAI@' in name or '??MovementInform@npc_tazhan_chuAI@' in name)]
assert len(callbacks)==3
for name in callbacks:
 body=ins(name)
 if '??MoveInLineOfSight@' in name:
  assert contains_callback(los,name)
  talk,=sites(n,body,'?Talk@CreatureAI@@')
  assert ('mov','dl, 1') in [(x.mnemonic,x.op_str) for x in body[:talk]]
 elif '<lambda_1>' in name:
  assert contains_callback(move,name)
  assert 246321 in values(body) and 35.0 in floats(n,body)
  assert sites(n,body,'?GetGameObjectListWithEntryInGrid@WorldObject@@')
  assert any(x.mnemonic=='call' and x.operands[0].type==CS_OP_MEM and x.operands[0].mem.disp==0xa0 for x in body)
  talk,=sites(n,body,'?Talk@CreatureAI@@');point,=sites(n,body,'?MovePoint@MotionMaster@@')
  assert talk<point and ('mov','dl, 4') in [(x.mnemonic,x.op_str) for x in body[:talk]]
  assert {-1017.8,-3048.89,12.82}<=floats(n,body)
 else:
  assert contains_callback(move,name) and {-1017.72,-2933.42,19.48}<=floats(n,body)
 for forbidden in ('?AddItem@Player@@','?CompleteQuest@Player@@','?UpdateAchievementCriteria@Player@@','?KilledMonsterCredit@Player@@','?AddDelayedEvent@Unit@@'):
  assert not sites(n,body,forbidden)
print('PASS registered Taran native factory190/owned processor110/destructor/table, actual4s/5s callback tables and captures, once-per-lifecycle clock before opening early returns, unchanged rescue/combat processor60 and original stun guard, original speech/wall phase/point2 and legacy point4. Actual native heal/wounds retained; no client, cast-success, full route/pickup/return claim.')
