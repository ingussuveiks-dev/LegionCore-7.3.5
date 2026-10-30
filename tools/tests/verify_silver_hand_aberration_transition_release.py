"""Check the selected aberration policy/lifecycle, not merely embedded spell IDs."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

n=runpy.run_path(str(Path(__file__).with_name('verify_smart_linked_cast_retry_release.py')))
ins,symbol,calls=n['instructions'],n['symbol'],n['calls']
suffix='@npc_horrific_aberrationAI@npc_horrific_aberration@@'
damage='?DamageTakenAfterScaling'+suffix
lifecycle='?UpdateLifecycle'+suffix
reset='?Reset'+suffix
factory=ins('?GetAI@npc_horrific_aberration@@')
tables=[]
for x in factory:
    if x.mnemonic=='lea' and len(x.operands)==2 and x.operands[1].type==CS_OP_MEM and x.operands[1].mem.base==X86_REG_RIP:
        at=x.address+x.size+x.operands[1].mem.disp
        slots=struct.unpack_from('<80Q',n['binary'],n['offset'](at))
        if slots[0xa8//8]==symbol(damage): tables.append(slots)
slots,=tables
assert slots[0x110//8]==symbol(lifecycle) and slots[0x28//8]==symbol(reset)
assert slots[0xa0//8]!=symbol(damage), 'Final protection incorrectly installed in raw slot'
body=ins(damage)
def at(body,op,mnemonic=None):
    return next(i for i,x in enumerate(body) if x.op_str==op and (mnemonic is None or x.mnemonic==mnemonic))
for prefix in ('?GetHealth@Unit@@QEBA_KPEAV1@','?GetMaxHealth@Unit@@QEBA_KPEAV1@','?GetScenario@ScenarioMgr@@','?GetScenarioId@Scenario@@','?GetCurrentStep@Scenario@@'):
    assert symbol(prefix) in calls(damage)
for op in ('eax, 0x444','eax, 5'):
    i=at(body,op,'cmp');assert body[i+1].mnemonic=='jne'
health=next(i for i,x in enumerate(body) if x.mnemonic=='call' and x.operands[0].imm==symbol('?GetHealth@Unit@@QEBA_KPEAV1@'))
maximum=next(i for i,x in enumerate(body) if x.mnemonic=='call' and x.operands[0].imm==symbol('?GetMaxHealth@Unit@@QEBA_KPEAV1@'))
assert health<maximum and body[health-1].op_str==body[maximum-2].op_str=='rdx, rbp'
threshold=at(body,'rax, 1','shr')
assert [(x.mnemonic,x.op_str) for x in body[threshold:threshold+5]]==[
    ('shr','rax, 1'),('and','r9d, 1'),('add','rax, r9'),('cmp','rdx, rax'),('jae',hex(body[threshold+4].operands[0].imm))]
start=at(body,'dword ptr [rbx + 0x5c], 0x3e8','mov')
assert threshold<start and body[start-1].op_str=='byte ptr [rbx + 0x58], 1'
cap=at(body,'r8d, [rdi - 1]','lea')
assert body[cap-4].op_str=='rax, rdi' and body[cap-3].mnemonic=='jb'
assert body[cap+1].op_str=='dword ptr [rsi], r8d'

body=ins(lifecycle)
assert not any(n['resolve'](address) in calls(lifecycle) for name,address in n['symbols'].items() if name.startswith('?UpdateVictim@'))
timer=at(body,'esi, eax','cmp');assert body[timer+1].mnemonic=='jae'
assert any(x.op_str=='dword ptr [rbx + 0x5c], eax' for x in body[timer:timer+8])
cast=next(i for i,x in enumerate(body) if x.mnemonic=='call' and x.operands[0].imm==symbol('?DoCast@UnitAI@@QEAAXI@Z'))
assert any(x.op_str=='edx, 0x2978f' for x in body[cast-5:cast])
assert any(x.op_str=='dword ptr [rbx + 0x5c], edi' for x in body[cast-5:cast])
epochs=[i for i,x in enumerate(body) if x.mnemonic=='cmp' and x.op_str=='rsi, qword ptr [rbx + 0x68]']
assert len(epochs)==4 and all(body[i+1].mnemonic=='jne' for i in epochs)
kill=[i for i,x in enumerate(body) if x.mnemonic=='call' and x.operands[0].imm==symbol('?Kill@Unit@@')]
assert len(kill)==2 and cast<epochs[0]<kill[0]<epochs[1]<kill[1]<epochs[2]
for op in ('edx, 0x19f83','edx, 0x19f82','edx, 0x1a0b4'):
    assert any(x.op_str==op for x in body)
assert sum(x.op_str=='edx, 0xbb8' for x in body)==2
finished=at(body,'byte ptr [rbx + 0x60], 1','mov')
assert epochs[-1]<finished
assert any(x.op_str=='byte ptr [rax + 0x858], dil' for x in body[epochs[-2]:finished])
assert not any(symbol(p) in calls(lifecycle) for p in ('?UpdateAchievementCriteria@Player@@','?KilledMonsterCredit@Player@@'))
body=ins(reset)
for op in ('word ptr [rbx + 0x58], 0','dword ptr [rbx + 0x5c], 0','byte ptr [rbx + 0x60], 0'):
    assert any(x.op_str==op for x in body)
assert any(x.mnemonic=='inc' and x.op_str=='qword ptr [rbx + 0x68]' for x in body)
assert sum(x.mnemonic=='call' and x.operands[0].type==CS_OP_MEM and x.operands[0].mem.disp==0x110 for x in ins('?Update@Creature@@'))>=2
print('PASS actual selected aberration final-damage0xa8/lifecycle0x110: native target-relative HP,1092/5,half-health crossing,1000ms retained,lethal cap,epoch checks,original two kills/despawns/Talk,reset and native primary/suspended lifecycle dispatch. Client choreography remains separate.')
