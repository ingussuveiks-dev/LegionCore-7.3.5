"""Prove actual same-Creature reply ownership and live AI selection."""
import struct
from capstone import CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
from sharasdal_erunak_entry_pe import method,sites,values

def verify(n):
 ins=n['instructions'];movement=ins(method('MovementInform'))
 factories=ins('?GetAI@npc_erunak_stonespeaker@@');tables=[]
 for x in factories:
  for o in x.operands:
   if o.type!=CS_OP_MEM or o.mem.base!=X86_REG_RIP:continue
   at=x.address+x.size+o.mem.disp
   try:rows=struct.unpack_from('<82Q',n['binary'],n['offset'](at))
   except (ValueError,struct.error):continue
   if n['symbol'](method('MovementInform')) in rows and n['symbol'](method('SetGUID')) in rows:tables.append(rows)
 table,=tables;slot,=[i*8 for i,value in enumerate(table) if value==n['symbol'](method('MovementInform'))]
 native=ins('?OnArrived@?$WaypointMovementGenerator@VCreature@@@@')
 at,=[i for i,x in enumerate(native) if x.mnemonic=='call' and x.op_str=='r9']
 assert [(x.mnemonic,x.op_str) for x in native[at-3:at]]==[
  ('mov',f'r9, qword ptr [r8 + {slot:#x}]'),('mov','r8d, dword ptr [r15]'),('mov','edx, 2')]
 first,=sites(n,movement,'?FindNearestCreature@WorldObject@@')
 assert any(x.op_str=='edx, 0x19995' for x in movement[first-4:first])
 assert [(x.mnemonic,x.op_str) for x in movement[first+1:first+10]]==[
  ('mov','rbx, rax'),('test','rax, rax'),('je',movement[first+3].op_str),
  ('mov','rcx, qword ptr [rax + 0x9c8]'),('test','rcx, rcx'),('jne',movement[first+6].op_str),
  ('mov','rcx, qword ptr [rax + 0x9c0]'),('test','rcx, rcx'),('je',movement[first+9].op_str)]
 talks=sites(n,movement,'?Talk@CreatureAI@@');first_talk=talks[0]
 assert movement[first_talk-1].op_str=='edx, edx'
 assert movement[first+6].operands[0].imm==movement[first+10].address
 assert movement[first+9].operands[0].imm>movement[first_talk].address
 queued,=sites(n,movement,'?AddDelayedEvent@FunctionProcessor@@')
 frame=movement[queued-2].operands[1].mem.disp
 assert [(x.mnemonic,x.op_str) for x in movement[queued-3:queued]]==[
  ('lea','rcx, [rbx + 0x36a0]'),('lea',f'r8, [rsp + {frame:#x}]'),('mov','edx, 0x1388')]
 assert any(x.op_str==f'qword ptr [rsp + {frame+8:#x}], rbx' for x in movement[first_talk:queued])
 callbacks=[name for name in n['symbols'] if name.startswith('?_Do_call@') and '??MovementInform@npc_erunak_stonespeakerAI@' in name]
 name,=callbacks;reply=ins(name)
 for prefix in ('?IsInWorld@Object@@','?IsPreDelete@Object@@'):assert sites(n,reply,prefix)
 assert any(x.op_str=='dword ptr [rax + 0xa18], 0' for x in reply) # native ALIVE0
 loads=[i for i,x in enumerate(reply) if x.op_str=='rcx, qword ptr [rax + 0x9c8]'];at,=loads
 assert [(x.mnemonic,x.op_str) for x in reply[at+1:at+5]]==[
  ('test','rcx, rcx'),('jne',reply[at+2].op_str),('mov','rcx, qword ptr [rax + 0x9c0]'),('test','rcx, rcx')]
 assert reply[at+5].mnemonic=='je' and reply[at+5].operands[0].imm==reply[-3].address
 assert reply[at+2].operands[0].imm==reply[at+6].address
 talk,=sites(n,reply,'?Talk@CreatureAI@@');assert any(x.op_str=='dl, 1' for x in reply[at+6:talk])
 # Link the actual type-erased function table back to this issuing method.
 references=[x.address+x.size+o.mem.disp for x in movement for o in x.operands if o.type==CS_OP_MEM and o.mem.base==X86_REG_RIP]
 found=[]
 for address in references:
  try:raw=struct.unpack_from('<8Q',n['binary'],n['offset'](address))
  except (ValueError,struct.error):continue
  if n['symbol'](name) in raw:found.append(address)
 assert len(found)==1
 new_ascent=any(name.startswith('?UpdateAscent@instance_sharasdal_') for name in n['symbols'])
 if new_ascent:
  from sharasdal_lower_ascent_pe import verify_movement
  verify_movement(n)
 else:assert {1707,83810,208580}<=values(movement)
 for body in (movement,reply):
  for prefix in ('?UpdateAchievementCriteria@Player@@','?KilledMonsterCredit@Player@@','?AddItem@Player@@','?CompleteQuest@Player@@'):
   assert not sites(n,body,prefix)
 print('PASS selected actual Erunak waypoint vtable/native zero-based dispatch; Adelee current-or-original-suspended AI checked before Talk0 and real same-Creature FunctionProcessor36a0/5000ms callback. Registered callback rechecks world/notdelete/alive and available AI before Talk1; no cached AI capture or new grant. Ascent follows the selected original or separately checked repaired bridge; actual client remains separate.')
