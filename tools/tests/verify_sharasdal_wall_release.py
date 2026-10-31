"""Trace the actual Grash waypoint, specific wall and original quest recipient."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM,CS_OP_MEM,CS_OP_REG

retained=runpy.run_path(str(Path(__file__).with_name('verify_sharasdal_wound_applied_release.py')))
n=retained['n'];instructions=n['instructions'];suffix='@npc_grashAI@npc_grash@@'
table=retained['tables']['npc_grash']
def sites(body,prefix):
 targets={n['resolve'](address) for name,address in n['symbols'].items() if name.startswith(prefix)}
 return [i for i,x in enumerate(body) if x.mnemonic in ('call','jmp') and x.operands[0].type==CS_OP_IMM and n['resolve'](x.operands[0].imm) in targets]
def values(body):return {o.imm for x in body for o in x.operands if o.type==CS_OP_IMM}
def fields(body):return {o.mem.disp for x in body for o in x.operands if o.type==CS_OP_MEM}
def events(body):
 result=[]
 for at in sites(body,'?UpdateAchievementCriteria@Player@@'):
  assets=[x.operands[1].imm for x in body[max(0,at-12):at] if x.mnemonic=='mov' and x.op_str.startswith('r8d, ') and x.operands[1].type==CS_OP_IMM]
  assert assets;result.append(assets[-1])
 return result

world=instructions('?World@SharasdalWall@@')
assert {105298,1600,362066,1066}<=values(world)
assert sites(world,'?IsInWorld@Object@@') and sites(world,'?IsPreDelete@Object@@')
movement=instructions('?MovementInform'+suffix)
slots=[slot for slot in range(0,0x290,8) if struct.unpack_from('<Q',n['binary'],table+slot)[0]==n['symbol']('?MovementInform'+suffix)]
slot,=slots
native_callback=instructions('?OnArrived@?$WaypointMovementGenerator@VCreature@@@@')
dispatch=[i for i,x in enumerate(native_callback) if x.mnemonic=='call' and x.op_str=='r9']
assert len(dispatch)==1
at=dispatch[0]
assert [(x.mnemonic,x.op_str) for x in native_callback[at-3:at]]==[
 ('mov',f'r9, qword ptr [r8 + {slot:#x}]'),('mov','r8d, dword ptr [r15]'),('mov','edx, 2')]
assert sites(instructions('?DoUpdate@?$WaypointMovementGenerator@VCreature@@@@'),'?OnArrived@?$WaypointMovementGenerator@VCreature@@@@')
assert sites(movement,'?World@SharasdalWall@@') and sites(movement,'?GetMotionSlot@MotionMaster@@')
# Path()/Wall() are inlined in this Release: actual IDLE slot0, virtual kind2,
# native _pathId2c, zero-based arrival node and exact three wall GUIDs remain.
path_slot,=sites(movement,'?GetMotionSlot@MotionMaster@@')
assert any(x.mnemonic=='xor' and x.op_str=='edx, edx' for x in movement[max(0,path_slot-6):path_slot])
assert any(x.mnemonic=='mov' and x.op_str=='r14d, dword ptr [r14 + 0x2c]' for x in movement)
assert {10993808,10993810,134291,134287,134290,248514,209904,1,2,3,4}<=values(movement)
assert any(x.mnemonic=='test' and x.op_str=='byte ptr [rbx + 0x10c], 1' for x in movement)
assert any(x.mnemonic=='test' and x.op_str=='byte ptr [rbx + 0x10c], 4' for x in movement)
assert sites(movement,'?GetDBTableGUIDLow@GameObject@@') and sites(movement,'?InSamePhase@WorldObject@@')
opened=sites(movement,'?HandleGameObject@InstanceScript@@')[0]
state,=sites(movement,'?GetGoState@GameObject@@')
assert opened<state
assert any(x.mnemonic=='mov' and x.op_str=='r9, rbp' for x in movement[opened-10:opened])
assert movement[state+1].mnemonic=='test' and movement[state+1].op_str=='eax, eax' and movement[state+2].mnemonic=='jne'
finished,=sites(movement,'?CompleteWallCredit'+suffix)
assert state<finished and any(x.mnemonic=='mov' and x.op_str=='byte ptr [rbx + 0x1b0], 1' for x in movement[state:finished])
assert not sites(movement,'?FindNearestPlayer@WorldObject@@') and not events(movement)

select=instructions('?sGossipSelect'+suffix)
# BindWallRecipient is inlined; it must precede the existing consumed-action bit
# and route command, retaining the complete caller GUID and native quest lease.
leases=sites(select,'?AddQuestDelayedEvent@Player@@');assert len(leases)==2
lease=leases[0] # lower binding still precedes its original route; upper has a separate lease
assert 40341 in values(select) and {0x190,0x1a0,0x1a8}<=fields(select)
assert sites(select,'?World@SharasdalWall@@') and sites(select,'?getQuestStatus@Player@@')
assert struct.unpack_from('<Q',n['binary'],table+0x48)[0]==n['symbol']('?DoAction'+suffix)
assert any(i>lease and x.mnemonic=='call' and x.operands[0].type==CS_OP_MEM and x.operands[0].mem.disp==0x48 for i,x in enumerate(select))
credit=instructions('?CompleteWallCredit'+suffix)
assert events(credit)==[49817] and {40341,47127,2,1000,30}<=values(credit)
assert {0x190,0x1a8,0x1b0,0x1b1,0x1b2,0x1b3,0x1b4,0x1b8}<=fields(credit)
assert sites(credit,'?GetPlayer@ObjectAccessor@@') and sites(credit,'?getQuestStatus@Player@@') and sites(credit,'?Eligible@SharasdalGrash@@')
at,=sites(credit,'?UpdateAchievementCriteria@Player@@')
assert any(i<at for i in sites(credit,'?Receipt@SharasdalGrash@@')) and any(i>at for i in sites(credit,'?Receipt@SharasdalGrash@@'))
assert any(x.mnemonic=='mov' and x.op_str=='byte ptr [rbx], 1' for x in credit[:at])
assert any(x.mnemonic=='mov' and x.op_str=='byte ptr [rbx], 0' for x in credit[at:])
life=instructions('?UpdateLifecycle'+suffix)
assert sites(life,'?CompleteIntroHeal'+suffix)[0]<sites(life,'?CompleteWallCredit'+suffix)[0]
rearm=instructions('?SetGUID'+suffix)
assert 40341 in values(rearm) and {0x190,0x1b0,0x1b2,0x1b4}<=fields(rearm)
assert sites(rearm,'??8ObjectGuid@@') and not events(rearm)
assert any(x.mnemonic=='xor' and x.op_str=='esi, esi' for x in rearm)
assert any(x.mnemonic=='mov' and x.op_str=='byte ptr [rbx + 0x1b2], sil' for x in rearm)
assert any(x.mnemonic=='mov' and x.op_str=='qword ptr [rbx + 0x1b4], rsi' for x in rearm)
assert struct.unpack_from('<Q',n['binary'],table+0x80)[0]==n['symbol']('?SetGUID'+suffix)
entry=instructions('?OnPlayerEnter@instance_sharasdal_InstanceMapScript@instance_sharasdal@@')
assert sites(entry,'?GetCreature@Map@@') and {0x39f,0x9c0,0x9c8}<=fields(entry)
entry_calls=[i for i,x in enumerate(entry) if x.mnemonic=='call' and x.op_str=='rbx']
assert len(entry_calls)==2
for at in entry_calls:
 assert any(x.mnemonic=='mov' and x.op_str=='rbx, qword ptr [rax + 0x80]' for x in entry[max(0,at-9):at])
 assert any(x.mnemonic=='mov' and x.op_str=='r8d, 0x9d95' for x in entry[max(0,at-5):at])
def ai_register(displacement):
 # Restrict this proof to the original Grash loop. A later, separately
 # verified Erunak loop also loads the current/suspended AI pair.
 loads=[x for x in entry[:entry_calls[-1]+1] if x.mnemonic=='mov' and x.operands[0].type==CS_OP_REG and x.operands[1].type==CS_OP_MEM and x.operands[1].mem.disp==displacement]
 assert len(loads)==1
 return loads[0].reg_name(loads[0].operands[0].reg)
current,suspended=ai_register(0x9c0),ai_register(0x9c8)
assert any(x.mnemonic=='cmp' and x.op_str==f'{suspended}, {current}' for x in entry[entry_calls[0]:entry_calls[1]])
for at,receiver in zip(entry_calls,(current,suspended)):
 assert any(x.mnemonic=='mov' and x.op_str==f'rcx, {receiver}' for x in entry[at-5:at])
for body in (credit,select):
 for prefix in ('?AddItem@Player@@','?CompleteQuest@Player@@','?AddDelayedEvent@Unit@@'):
  assert not sites(body,prefix)
for message in (b'Sharasdal wall arrival cancelled after its original quest attempt ended',b'Sharasdal wall arrival could not reach its original recipient/native criterion; retry when that recipient returns'):
 assert message in n['binary']
print('PASS selected Grash/native waypoint vtable, actual IDLE _pathId and zero-based node, issued-role/lower362066/world1066 gates, three specific walls/phase and open-state acknowledgement; only original40341 full GUID/quest lease gets49817 once through bounded/rearmed native receipt helper. Actual instance entry dispatches SetGUID0x80 to current/suspended once, including no-update absence; GUID-matched rearm only resets the earned pending delivery. No nearest-player event or route/cast replay from retry. Existing heal/Brute/guard/rescue/point boss proof retained; no client/full-route certification')
