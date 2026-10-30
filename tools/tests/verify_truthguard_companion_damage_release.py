"""Verify both template bindings select final-damage companion protection."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM,CS_OP_MEM,CS_OP_REG
from capstone.x86_const import X86_REG_RIP
previous=runpy.run_path(str(Path(__file__).with_name('verify_damage_scaling_bounds_release.py')))
native=previous['native'];instructions=native['instructions']
prefix='?DamageTakenAfterScaling@npc_sr_orik_cato_scriptAI@'
body=instructions(prefix)
def sites(target):
    address=native['symbol'](target)
    return [i for i,x in enumerate(body) if x.mnemonic=='call' and x.operands[0].type==CS_OP_IMM and native['resolve'](x.operands[0].imm)==address]
health,=sites('?GetHealth@Unit@@QEBA_KPEAV1@@')
threat,=sites('?modifyThreatPercent@ThreatManager@@')
assert not any(x.operands and x.operands[0].type==CS_OP_REG and x.reg_name(x.operands[0].reg) in ('rdx','edx','dx','dl') for x in body[:health])
assert [(x.mnemonic,x.op_str) for x in body[health+1:health+5]]==[('cmp','rbx, rax'),('jb',body[health+2].op_str),('mov','dword ptr [rsi], 0'),('test','rdi, rdi')]
assert body[health+5].mnemonic=='je' and body[health+5].operands[0].imm==body[health+2].operands[0].imm
assert body[health+2].operands[0].imm>body[threat].address
assert [(x.mnemonic,x.op_str) for x in body[threat-3:threat]]==[('mov','rdx, qword ptr [rbp + 0x20]'),('lea','rcx, [rdi + 0x2ff0]'),('mov','r8d, 0xffffffa6')]
assert len([x for x in body if x.mnemonic=='call'])==2
tables=[]
for x in instructions('?GetAI@npc_sr_orik_cato_script@@'):
    for operand in x.operands:
        if x.mnemonic=='lea' and operand.type==CS_OP_MEM and operand.mem.base==X86_REG_RIP:
            position=native['offset'](x.address+x.size+operand.mem.disp)
            if struct.unpack_from('<Q',native['binary'],position+0xa8)[0]==native['symbol'](prefix):tables.append(position)
assert len(tables)==1
assert struct.unpack_from('<Q',native['binary'],tables[0]+0xa0)[0]==native['symbol']('?DamageTaken@UnitAI@@')
print('PASS selected Orik/Cato factory installs slot0xa8 after bounded native scaling; original attacker reaches target-relative HP, lethal zero precedes nullable attacker check and original -90 threat adjustment; empty raw hook, no progression/summon/health grant. Client scenario traversal remains separate')
