"""Verify native post-application wound release, keeping story branches separate."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_MEM,CS_OP_IMM
from capstone.x86_const import X86_REG_RIP
previous=runpy.run_path(str(Path(__file__).with_name('verify_sharasdal_companion_damage_release.py')))
native,instructions,sites=(previous[k] for k in ('native','instructions','sites'))
recover='?Recover@SharasdalCompanion@@'
body=instructions(recover)
assert body[3].op_str=='byte ptr [rdx], 0' and body[6].mnemonic=='je'
assert body[7].mnemonic=='mov' and body[7].op_str=='byte ptr [rdx], 0'
assert body[8].op_str=='byte ptr [r8], 0' and body[9].mnemonic=='je'
world,=sites(body,'?IsInWorld@Object@@');deleted,=sites(body,'?IsPreDelete@Object@@');hp,=sites(body,'?GetHealthPct@Unit@@');remove,=sites(body,'?RemoveAura@Unit@@QEAAXIVObjectGuid@@')
assert 9<world<deleted<hp<remove
for i,branch in ((world,'je'),(deleted,'jne')):assert body[i+2].mnemonic==branch and body[i+2].operands[0].imm==body[6].operands[0].imm
assert any(x.mnemonic=='cmp' and x.op_str=='dword ptr [rbx + 0xa18], 0' for x in body[deleted:hp])
limit=body[hp+1];assert limit.mnemonic=='movss' and limit.operands[1].mem.base==X86_REG_RIP
assert struct.unpack_from('<f',native['binary'],native['offset'](limit.address+limit.size+limit.operands[1].mem.disp))[0]==40.0
assert body[hp+2].mnemonic=='comiss' and body[hp+3].mnemonic=='ja' and body[hp+3].operands[0].imm==body[6].operands[0].imm
assert body[hp+4].op_str=='byte ptr [rdi], 0'
assert any(x.op_str=='dword ptr [rbx + 0x3140], 0xfffffff7' and x.mnemonic=='and' for x in body[hp:remove])
assert any(x.op_str=='edx, 0x32cdd' for x in body[hp:remove]) and len([x for x in body if x.mnemonic=='call'])==4
for script,flag,pending in [('npc_erunak_stonespeaker',0x59,0x5b),('npc_grash',0x5a,0x5b),('npc_adel_waterspeaker',0x59,0x5a)]:
    table=previous['selected_tables'][script];suffix='@'+script+'AI@'
    life='?UpdateLifecycle'+suffix;heal='?HealReceived'+suffix;reset='?Reset'+suffix
    for method,slot in ((life,0x110),(heal,0xb0),(reset,0x28)):
        assert struct.unpack_from('<Q',native['binary'],table+slot)[0]==native['symbol'](method)
    adapter=instructions(life)
    assert [(x.mnemonic,x.op_str) for x in adapter[:3]]==[
        ('lea',f'r8, [rcx + {flag:#x}]'),('lea',f'rdx, [rcx + {pending:#x}]'),('mov','rcx, qword ptr [rcx + 0x20]')]
    assert adapter[3].mnemonic=='jmp' and native['resolve'](adapter[3].operands[0].imm)==native['symbol'](recover)
    reset_body=instructions(reset);assert reset_body[0].op_str==f'byte ptr [rcx + {pending:#x}], 0'
    assert reset_body[2].mnemonic=='jmp' and native['resolve'](reset_body[2].operands[0].imm)==native['symbol']('?Reset@EventMap@@')
    heal_body=instructions(heal)
    if script=='npc_grash':
        mark=next(i for i,x in enumerate(heal_body) if x.op_str==f'byte ptr [rbx + {pending:#x}], 1' and x.mnemonic=='mov')
        assert heal_body[mark-4].op_str==f'byte ptr [rbx + {flag:#x}], 0' and heal_body[mark-3].mnemonic=='je'
        assert heal_body[mark-2].op_str=='dword ptr [rsi], 0' and heal_body[mark-1].mnemonic=='je'
        assert heal_body[mark-3].operands[0].imm==heal_body[mark-1].operands[0].imm==heal_body[mark+1].address
    else:
        assert [(heal_body[i].mnemonic,heal_body[i].op_str) for i in (0,2,4)]==[
            ('cmp',f'byte ptr [rcx + {flag:#x}], 0'),('cmp','dword ptr [r8], 0'),('mov',f'byte ptr [rcx + {pending:#x}], 1')]
        assert heal_body[1].mnemonic==heal_body[3].mnemonic=='je' and heal_body[1].operands[0].imm==heal_body[5].address==heal_body[3].operands[0].imm
    factory=instructions('?GetAI@'+script+'@@');assert any(x.op_str==f'byte ptr [rsi + {pending:#x}], bpl' for x in factory)
    damage=instructions('?DamageTakenAfterScaling'+suffix)
    if script=='npc_erunak_stonespeaker':assert any(x.op_str==f'byte ptr [rdi + {pending:#x}], al' for x in damage)
    else:assert any(x.op_str==f'word ptr [rdi + {flag:#x}], 1' for x in damage)
heal_body=instructions('?DealHeal@Unit@@')
receiver=[i for i,x in enumerate(heal_body) if x.mnemonic=='call' and x.operands[0].type==CS_OP_MEM and x.operands[0].mem.disp==0xb0]
healer=[i for i,x in enumerate(heal_body) if x.mnemonic=='call' and x.operands[0].type==CS_OP_MEM and x.operands[0].mem.disp==0xb8]
applied,=sites(heal_body,'?ModifyHealth@Unit@@')
assert len(receiver)==len(healer)==1 and receiver[0]<healer[0]<applied
update=instructions('?Update@Creature@@')
dispatch=[i for i,x in enumerate(update) if x.mnemonic=='call' and x.operands[0].type==CS_OP_MEM and x.operands[0].mem.disp==0x110]
assert len(dispatch)==2
assert any(x.op_str=='byte ptr [rsi + 0x3a9d], 1' for x in update[dispatch[0]-12:dispatch[0]])
assert any(x.op_str=='byte ptr [rsi + 0x3a9d], 0' for x in update[dispatch[-1]:dispatch[-1]+4])
assert any(x.op_str=='rcx, rax' and x.mnemonic=='cmp' for x in update[dispatch[0]-6:dispatch[0]])
print('PASS selected three positive incoming-heal markers/reset/new-wound invalidation, native HealReceived->HealDone->ModifyHealth order, locked disabled/current lifecycle0x110 and observed40pct->clear latch/stun->208093 removal. Grash50/90pct legacy story remains separate; no new progression/cast/health grant or client proof')
