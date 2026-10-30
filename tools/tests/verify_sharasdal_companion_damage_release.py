"""Verify all selected Sharasdal wound policies after actual native scaling."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM,CS_OP_MEM,CS_OP_REG
from capstone.x86_const import X86_REG_RIP
previous=runpy.run_path(str(Path(__file__).with_name('verify_damage_scaling_bounds_release.py')))
native=previous['native'];instructions=native['instructions']
selected_tables={}
healing=any(name.startswith('?Recover@SharasdalCompanion@@') for name in native['symbols'])
def sites(body,target):
    address=native['symbol'](target)
    return [i for i,x in enumerate(body) if x.mnemonic=='call' and x.operands[0].type==CS_OP_IMM and native['resolve'](x.operands[0].imm)==address]
for script,speech,flag in [('npc_erunak_stonespeaker',(7,8),0x59),('npc_grash',(1,2),0x5a),('npc_adel_waterspeaker',(2,3),0x59)]:
    prefix='?DamageTakenAfterScaling@'+script+'AI@'
    body=instructions(prefix)
    health,=sites(body,'?GetHealth@Unit@@QEBA_KPEAV1@@')
    assert not any(x.operands and x.operands[0].type==CS_OP_REG and x.reg_name(x.operands[0].reg) in ('rdx','edx','dx','dl') for x in body[:health])
    assert [(x.mnemonic,x.op_str) for x in body[health+1:health+6]]==[
        ('cmp','rbx, rax'),('jb',body[health+2].op_str),('xor','eax, eax'),('mov','dword ptr [rsi], eax'),('cmp',f'byte ptr [rdi + {flag:#x}], al')]
    assert body[health+6].mnemonic=='jne' and body[health+6].operands[0].imm==body[health+2].operands[0].imm
    aura,=sites(body,'?AddAura@Unit@@QEAAPEAVAura@@I')
    combined=healing and script!='npc_erunak_stonespeaker'
    claim=next(i for i,x in enumerate(body) if x.mnemonic=='mov' and x.op_str==f'{"word" if combined else "byte"} ptr [rdi + {flag:#x}], 1')
    assert health+6<claim<aura
    for op in ('edx, 0x32cdd','r8, rcx'):assert any(x.op_str==op for x in body[health+6:aura])
    talks=sites(body,'?Talk@CreatureAI@@QEAAXE');assert len(talks)==2 and aura<talks[0]<talks[1]
    for index,group in zip(talks,speech):assert any(x.mnemonic=='mov' and x.op_str==f'dl, {group}' for x in body[index-5:index])
    assert any(x.mnemonic=='or' and x.op_str=='dword ptr [rax + 0x3140], 8' for x in body[talks[-1]:])
    assert body[health+2].operands[0].imm>body[talks[-1]].address
    assert len([x for x in body if x.mnemonic=='call'])==4
    factory=instructions('?GetAI@'+script+'@@');tables=[]
    for x in factory:
        for operand in x.operands:
            if x.mnemonic=='lea' and operand.type==CS_OP_MEM and operand.mem.base==X86_REG_RIP:
                position=native['offset'](x.address+x.size+operand.mem.disp)
                if struct.unpack_from('<Q',native['binary'],position+0xa8)[0]==native['symbol'](prefix):tables.append(position)
    assert len(tables)==1 and struct.unpack_from('<Q',native['binary'],tables[0]+0xa0)[0]==native['symbol']('?DamageTaken@UnitAI@@')
    selected_tables[script]=tables[0]
    assert any(x.mnemonic=='xor' and x.op_str=='ebp, ebp' for x in factory)
    assert any(x.mnemonic=='mov' and x.op_str==('byte ptr [rsi + 0x5a], bpl' if flag==0x5a else 'word ptr [rsi + 0x58], bp') for x in factory)
selector=instructions('?selectAI@FactorySelector@@')
scripted,=sites(selector,'?GetCreatureAI@ScriptMgr@@');named,=sites(selector,'?GetAIName@Creature@@')
assert scripted<named and selector[scripted+1].op_str=='rax, rax' and selector[scripted+1].mnemonic=='test'
assert selector[scripted+2].mnemonic=='jne' and selector[scripted+2].operands[0].imm>selector[named].address
print('PASS three selected Sharasdal factories install final0xa8/empty raw0xa0 and initialized wound latches; original attacker-relative HP/zero/once-before208093/self aura/two speech groups/stun retained. Native ScriptName result returns before AIName, including Grash SmartAI label. No direct progression; healing, routes and client visuals remain separate')
