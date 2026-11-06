"""Six earlier escort callbacks belong to the actual Hruthnir AI in Release."""
from pathlib import Path
import hashlib
import json
import runpy
import struct
from capstone import CS_OP_MEM, Cs, CS_ARCH_X86, CS_MODE_64
from capstone.x86_const import X86_REG_RDX,X86_REG_RIP
from sharasdal_erunak_entry_pe import sites,values,fields
from monk_intro_clock_pe import verify_clock

p=runpy.run_path(str(Path(__file__).with_name('verify_monk_hait_click_release.py')))
n,refs,table,resolved=p['n'],p['refs'],p['table'],p['resolved']
raw_ins=n['instructions'];suffix='@npc_hrutnirAI@npc_hrutnir@@'
def method(name):return '?'+name+suffix
def ins(prefix):
 body=raw_ins(prefix)
 if prefix not in (method('MovementInform'),method('UpdateAI')):return body
 # These two real switch tables share the function's unwind range. Decode
 # their referenced RVA table as data, verifying every target against code,
 # instead of interpreting table bytes as spurious out-of-PE CALL opcodes.
 count=7 if prefix==method('MovementInform') else 6
 lookup,=[i for i,op in enumerate(body) if op.mnemonic=='mov' and len(op.operands)==2 and op.operands[1].type==CS_OP_MEM and op.operands[1].mem.base==X86_REG_RDX and op.operands[1].mem.scale==4]
 before=body[lookup-1];memory=body[lookup].operands[1].mem
 assert before.mnemonic=='lea' and before.op_str.startswith('rdx, [rip ') and before.operands[1].mem.base==X86_REG_RIP
 assert before.address+before.size+before.operands[1].mem.disp==n['base']
 assert body[lookup-3].mnemonic=='cmp' and body[lookup-3].operands[1].imm==count-1 and body[lookup-2].mnemonic=='ja'
 assert [(op.mnemonic,op.op_str) for op in body[lookup+1:lookup+3]]==[('add','rcx, rdx'),('jmp','rcx')]
 address=n['base']+memory.disp;code=[op for op in body if op.address<address]
 targets=struct.unpack_from('<'+str(count)+'I',n['binary'],n['offset'](address))
 assert all(n['base']+target in {op.address for op in code} for target in targets)
 last_ret=max(op.address+op.size for op in code if op.mnemonic=='ret')
 padding=n['binary'][n['offset'](last_ret):n['offset'](address)]
 alignment=list(Cs(CS_ARCH_X86,CS_MODE_64).disasm(padding,last_ret))
 assert sum(op.size for op in alignment)==len(padding) and all(op.mnemonic in ('nop','int3') for op in alignment)
 return code
actor,raw=table('??_7npc_hrutnirAI@npc_hrutnir@@6B@',82)
script,script_raw=table('??_7npc_hrutnir@@6BScriptObject@@@',16)
factory=ins('?GetAI@npc_hrutnir@@');destroy=ins('??_Gnpc_hrutnirAI@npc_hrutnir@@')
current_start=any(name.startswith(method('ContinueMagnarStart')) for name in n['symbols'])
if current_start:
 assert sites(n,destroy,'??1npc_hrutnirAI@npc_hrutnir@@')
 destroy=ins('??1npc_hrutnirAI@npc_hrutnir@@')
current_pillik=any(name.startswith(method('TryPillikHandoff')) for name in n['symbols'])
if current_pillik:
 assert sites(n,factory,'??0npc_hrutnirAI@npc_hrutnir@@')
 factory+=ins('??0npc_hrutnirAI@npc_hrutnir@@')
assert actor in refs(factory) and script in refs(ins('?AddSC_EarthWarder@@'))
assert n['symbol']('?GetAI@npc_hrutnir@@') in resolved(script_raw)
assert resolved(raw)[0x110//8]==n['symbol'](method('UpdateLifecycle'))
assert resolved(raw)[0]==n['symbol']('??_Gnpc_hrutnirAI@npc_hrutnir@@')
construct,=sites(n,factory,'??0FunctionProcessor@@');destruct,=sites(n,destroy,'??1FunctionProcessor@@')
assert (factory[construct-1].mnemonic,factory[construct-1].op_str)==('lea','rcx, [rsi + 0xc0]')
assert (destroy[destruct-1].mnemonic,destroy[destruct-1].op_str)==('lea','rcx, [rsi + 0xc0]' if current_start else 'rcx, [rdi + 0xc0]')
assert sites(n,destroy,method('CancelMagnarIntro'))[0]<destruct
assert sites(n,destroy,'?Remove@Conversation@@')[0]<destruct
assert (0x178 if current_pillik else (0x168 if current_start else 0x140)) in values(factory) and {0x68,0x78,0x80,0xad}<=fields(factory)
verify_clock(n,method('UpdateLifecycle'),0xc0)
assert not sites(n,ins(method('UpdateAI')),'?Update@FunctionProcessor@@')
assert not sites(n,ins(method('Reset')),'?KillAllFunctions@FunctionProcessor@@')

parents=[ins(method(name)) for name in ('EnterEvadeMode','MovementInform','MoveInLineOfSight')]
movement_queues=[(0xc0,5000),(0xc0,4000),(0xc0,15000),(0xc0,2000),(0x36a0,13000)]
if not current_start:movement_queues.append((0x36a0,6000))
for body,expected in zip(parents,[[(0xc0,5000)],movement_queues,[(0xc0,5000)]]):
 queued=sites(n,body,'?AddDelayedEvent@FunctionProcessor@@');assert len(queued)==len(expected)
 for at,(member,delay) in zip(queued,expected):
  window=[(op.mnemonic,op.op_str) for op in body[max(0,at-12):at]]
  owner=('lea','rcx, [rbx + 0xc0]') if member==0xc0 else ('add','rcx, 0x36a0')
  assert owner in window and ('mov',f'edx, {delay:#x}') in window,(member,delay,window)
callbacks=[name for name in n['symbols'] if name.startswith('?_Do_call@') and 'npc_hrutnirAI@' in name]
assert len(callbacks)==8
for callback in callbacks:
 name='??_7'+callback[len('?_Do_call@'):].removesuffix('EEAAXXZ')+'6B@'
 address,slots=table(name,7)
 parent=ins(method('ScheduleMagnarStart')) if '??ScheduleMagnarStart' in callback else parents[0] if '??EnterEvadeMode' in callback else parents[2] if '??MoveInLineOfSight' in callback else parents[1]
 assert address in refs(parent) and n['symbol'](callback) in resolved(slots),('Actual callback vtable not installed by its parent',callback)
owned=[name for name in callbacks if '??ScheduleMagnarStart' not in name and not any(f'<lambda_{index}>' in name and '??MovementInform' in name for index in (5,6))]
assert len(owned)==6
def callback_body(name):
 body=ins(name)
 if current_pillik and '<lambda_1>' in name and '??MovementInform' in name:
  selected=[key for key in n['symbols'] if key.startswith(('??$invoke@','??R<lambda_1>')) and '<lambda_1>' in key and '??MovementInform'+suffix in key]
  assert len(selected)==1 and sites(n,body,selected[0]), 'Installed weak callback does not dispatch its actual native body'
  body+=ins(selected[0])
 return body
bodies=[callback_body(name) for name in owned]
assert sum(len(sites(n,body,'?Talk@CreatureAI@@')) for body in bodies)==6
fight,=[body for body in bodies if sites(n,body,'?SendChat@CreatureTextMgr@@')]
assert {94714,96571,1000,5}<=values(fight)
assert sites(n,fight,'?FindNearestCreature@WorldObject@@') and sites(n,fight,'?GetCreatureListWithEntryInGrid@WorldObject@@') and sites(n,fight,'?Kill@Unit@@') and sites(n,fight,'?DespawnOrUnsummon@Creature@@')
assert 0x9c0 not in fields(fight),'Actual target speech must not dereference transient AI'
assert any(op.mnemonic=='call' and op.operands[0].type==CS_OP_MEM and op.operands[0].mem.disp==0x10 for op in fight),'Original companion AttackStart retained'
assert sum(bool(sites(n,body,'?MovePath@MotionMaster@@')) for body in bodies)==1
assert any(9859004 in values(body) and sites(n,body,'?MovePath@MotionMaster@@') for body in bodies)
assert sum(bool(sites(n,body,'?MovePoint@MotionMaster@@')) for body in bodies)==1
for body in bodies:
 for forbidden in ('?KilledMonsterCredit@Player@@','?UpdateAchievementCriteria@Player@@','?CompleteQuest@Player@@','?AddItem@Player@@','?TeleportTo@Player@@'):
  assert not sites(n,body,forbidden)
# Independently retain the existing native gossip/weak chamber tokens/497/death
# cleanup verifiers, including their original field offsets and both Unit timers.
runpy.run_path(str(Path(__file__).with_name('verify_hruthnir_cleanup_release.py')))
print('PASS actual registered Hruthnir factory/vtable/lifecycle110/constructor-destructorc0, six owned5/5/4/15/2/5s callback vtables/native ALIVE-CORPSE clock; guarded13/6s Unit timers and original field offsets/497/death cleanup retained. Exact94714 native text without transient AI, ordinary AttackStart/scenery kill/route and movement requests preserved; no direct progression grants. Client/full909 remains separate.')
print(json.dumps({'exe_sha256':hashlib.sha256(n['binary']).hexdigest(),'owned_callbacks':6,'guarded_unit_callbacks':2}))
