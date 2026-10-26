"""Verify selected ritual pulse admission without changing the native cadence."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous=runpy.run_path(str(Path(__file__).with_name('verify_silver_hand_wave_deaths_release.py')))
native,instructions,sites=previous['native'],previous['instructions'],previous['sites']
name='?UpdateAI@npc_travar_lastAI@npc_travar_last@@'
body=instructions(name)
predicate,=sites(body,'?CanStart@SilverHandFinalEntry@@')
credit,=sites(body,'?UpdateAchievementCriteria@Player@@')
victim,=sites(body,'?UpdateVictim@CreatureAI@@')
assert victim<predicate<credit
assert not sites(body,'?FindNearestPlayer@WorldObject@@')
assert any(entry.op_str=='rax, qword ptr [rax + 0x768]' for entry in body[victim:predicate])
assert body[predicate-2].op_str=='rcx, qword ptr [rbx + 0x20]' and body[predicate-1].op_str=='rdx, rsi'
assert body[predicate+1].op_str=='al, al' and body[predicate+2].mnemonic=='jne'
accepted=body[predicate+2].operands[0].imm
assert accepted==next(entry.address for entry in body[predicate+3:credit] if entry.op_str=='rcx, qword ptr [rbx + 0x20]')
assert body[predicate+3].op_str=='rbx, qword ptr [rbx + 8]'
assert any(entry.mnemonic=='jne' and entry.operands[0].imm==body[predicate-2].address for entry in body[predicate+3:credit])
assert any(entry.op_str=='edx, 0x5c' for entry in body[predicate:credit])
assert any(entry.op_str=='r8d, 0xca7e' for entry in body[predicate:credit])
reset=next(index for index,entry in enumerate(body) if entry.op_str=='dword ptr [rdi + 0x3c], 0x3e8')
assert credit<reset
stunned=next(index for index,entry in enumerate(body) if entry.op_str=='byte ptr [rax + 0x3140], 8')
assert body[stunned+1].mnemonic=='jne' and body[stunned+1].operands[0].imm>body[reset].address
for operands in ('rsi, rsi','rax, rax'):
    guard=next(index for index,entry in enumerate(body[:predicate]) if entry.mnemonic=='test' and entry.op_str==operands)
    assert body[guard+1].mnemonic=='je' and body[credit].address<body[guard+1].operands[0].imm<=body[reset].address
assert any(entry.op_str=='eax, dword ptr [rdi + 0x3c]' for entry in body[:predicate])
remaining=next(index for index,entry in enumerate(body) if entry.op_str=='eax, ebx' and entry.mnemonic=='cmp')
assert body[remaining+1].mnemonic=='ja' and body[remaining+1].operands[0].imm>body[reset].address
assert any(entry.mnemonic=='sub' and entry.op_str=='eax, ebx' for entry in body[reset:])
enabled=next(index for index,entry in enumerate(body) if entry.op_str=='byte ptr [rdi + 0x39], al')
assert body[enabled+1].mnemonic=='je' and body[enabled+1].operands[0].imm>body[reset].address
assert body[victim+1].op_str=='al, al' and body[victim+2].mnemonic=='jne' and body[victim+2].operands[0].imm>body[reset].address
tables=[]
for entry in instructions('?GetAI@npc_travar_last@@'):
    if entry.mnemonic!='lea' or len(entry.operands)!=2 or entry.operands[1].type!=CS_OP_MEM or entry.operands[1].mem.base!=X86_REG_RIP:
        continue
    slots=struct.unpack_from('<80Q',native['binary'],native['offset'](entry.address+entry.size+entry.operands[1].mem.disp))
    if slots[0x270//8]==native['symbol']('?MoveInLineOfSight@npc_travar_lastAI@npc_travar_last@@'):
        tables.append(slots)
assert len(tables)==1 and tables[0][0x18//8]==native['symbol'](name)
assert any(entry.mnemonic=='call' and entry.operands[0].type==CS_OP_MEM and entry.operands[0].mem.disp==0x18
           for entry in instructions('?Update@Creature@@'))
assert len([entry for entry in body if entry.mnemonic=='call'])==3
print('PASS selected canonical UpdateAI inlines actual-map participant iteration through unchanged full CanStart gate, bypasses invalid references and dispatches one92/51838 only after success. Original start/stun/victim and1000ms reset/remainder retained; no nearest substitute. Not a client ritual/timing/VFX proof.')
