"""Verify selected Erunak owns both intro captures and ticks once per lifecycle."""
from pathlib import Path
import runpy,struct

retained=runpy.run_path(str(Path(__file__).with_name('verify_sharasdal_gauntlet_arrival_release.py')))
n=retained['n'];ins=n['instructions'];sites=retained['sites'];refs=retained['refs']
values=retained['values'];fields=retained['fields']
suffix='@npc_erunak_stonespeakerAI@npc_erunak_stonespeaker@@'
def method(name):return '?'+name+suffix
factory='?GetAI@npc_erunak_stonespeaker@@'
def rows(at):
 try:raw=struct.unpack_from('<82Q',n['binary'],n['offset'](at))
 except (ValueError,struct.error):return []
 result=[]
 for v in raw:
  try:result.append(n['resolve'](v))
  except ValueError:result.append(v)
 return result
tables=[at for at in refs(factory) if all(n['symbol'](method(m)) in rows(at) for m in ['MoveInLineOfSight','UpdateLifecycle','UpdateAI','Reset'])]
table,=tables;vtable=rows(table)
assert vtable[0x110//8]==n['symbol'](method('UpdateLifecycle'))
assert vtable[0x278//8]==n['symbol'](method('MoveInLineOfSight'))
assert any(n['symbol'](factory) in rows(at) for at in refs('?AddSC_Sharasdal@@'))
los=ins(method('MoveInLineOfSight'));life=ins(method('UpdateLifecycle'));update=ins(method('UpdateAI'))
queue=sites(los,'?AddDelayedEvent@FunctionProcessor@@');assert len(queue)==2
for at,delay in zip(queue,[5000,3000]):
 window=los[at-4:at]
 assert ('lea','rcx, [rdi + 0x60]') in [(x.mnemonic,x.op_str) for x in window]
 assert any(x.mnemonic=='mov' and x.op_str==f'edx, {delay:#x}' for x in window)
assert not sites(los,'?AddDelayedEvent@Unit@@')
assert not sites(update,'?Update@FunctionProcessor@@')
assert sites(life,'?Recover@SharasdalCompanion@@')==[8]
assert sites(life,'?Update@FunctionProcessor@@')==[14]
assert life[14].mnemonic=='jmp'
assert [(x.mnemonic,x.op_str) for x in life[3:11]]==[
 ('mov','edi, edx'),('lea','r8, [rcx + 0x59]'),('lea','rdx, [rcx + 0x5b]'),
 ('mov','rbx, rcx'),('mov','rcx, qword ptr [rcx + 0x20]'),
 ('call',life[8].op_str),('lea','rcx, [rbx + 0x60]'),('mov','edx, edi')]
destructor=ins('??_Gnpc_erunak_stonespeakerAI@npc_erunak_stonespeaker@@')
destroy,=sites(destructor,'??1FunctionProcessor@@')
assert destructor[destroy-1].op_str=='rcx, 0x60' and destructor[destroy-1].mnemonic=='add'
assert n['symbol']('??_Gnpc_erunak_stonespeakerAI@npc_erunak_stonespeaker@@')==vtable[0]
callbacks=[name for name in n['symbols'] if name.startswith('?_Do_call@') and '??MoveInLineOfSight@npc_erunak_stonespeakerAI@' in name]
assert len(callbacks)==2
for name in callbacks:
 assert any(n['symbol'](name) in rows(at)[:8] for at in refs(method('MoveInLineOfSight')))
 bodies=ins(name)
 if '<lambda_1>' in name:
  assert 1868 in values(bodies) and sites(bodies,'?CreateConversation@Unit@@')
 else:
  assert 102792 in values(bodies) and sites(bodies,'?FindNearestCreature@WorldObject@@') and sites(bodies,'?Talk@CreatureAI@@')
  guard,=[i for i,x in enumerate(bodies) if x.mnemonic=='test' and x.op_str=='rcx, rcx']
  assert bodies[guard-1].op_str=='rcx, qword ptr [rdx + 0x9c0]'
  assert bodies[guard+1].mnemonic=='je' and bodies[guard+1].operands[0].imm==bodies[-3].address
  assert any(x.op_str=='r8, qword ptr [rdx + 0x10]' for x in bodies[guard:])
 for forbidden in ('?AddItem@Player@@','?CompleteQuest@Player@@','?UpdateAchievementCriteria@Player@@','?KilledMonsterCredit@Player@@','?AddDelayedEvent@Unit@@'):
  assert not sites(bodies,forbidden)
print('PASS selected Erunak factory/registration/SafeLOS278/lifecycle110/destructor: both3s/5s captures use owned FunctionProcessor60, exact wound recovery then once-per-lifecycle diff, no UpdateAI double clock, native AI destruction clears its queue, selected real callback vtables retain1868 and guarded current-AI102792 attack. Full gauntlet/upper-wall/acquisition/Lady/Kraliss/wound PE retained; no admission/client/full1066 certification.')
