"""Follow selected instance vtable and scoped final-damage policy in linked PE."""
from pathlib import Path
import re
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM, CS_OP_REG
from capstone.x86_const import X86_REG_RIP

p = runpy.run_path(str(Path(__file__).with_name('verify_warswords_final_credit_release.py')))
n, ins = p['native'], p['instructions']
policy = '?ProtectUnholyFinale@DarkRidersArtifact@@'
callback = '?OnCreatureDamageTaken@Instance@instance_dark_riders_artifact@@'
factory = '?GetInstanceScript@instance_dark_riders_artifact@@'


def refs(method):
    return {x.address+x.size+o.mem.disp for x in ins(method) for o in x.operands
            if o.type == CS_OP_MEM and o.mem.base == X86_REG_RIP}


mapping = (n['args'].release/'worldserver.map').read_text(errors='replace')
vtable = re.search(r'\?\?_7Instance@instance_dark_riders_artifact@@6B@\s+([0-9a-fA-F]{16})', mapping)
assert vtable, 'Missing actual Dark Riders instance vtable'
address = int(vtable.group(1),16)
slots = struct.unpack_from('<128Q',n['binary'],n['offset'](address))
assert n['symbol'](callback) in slots, 'Actual instance does not select repaired damage callback'
assert address in refs(factory), 'Map script factory does not construct that instance vtable'
slot = slots.index(n['symbol'](callback))*8
damage_body = ins('?DealDamage@Unit@@')
loads = [i for i,x in enumerate(damage_body)
         if x.mnemonic=='mov' and len(x.operands)==2 and x.operands[0].type==CS_OP_REG
         and x.operands[1].type==CS_OP_MEM and x.operands[1].mem.disp==slot]
assert any(any(y.mnemonic=='call' and y.operands[0].type==CS_OP_REG
               and y.operands[0].reg==damage_body[i].operands[0].reg for y in damage_body[i+1:i+8])
           and any(y.mnemonic=='call' and y.operands[0].type==CS_OP_IMM
                   and n['resolve'](y.operands[0].imm)==n['symbol']('?GetInstanceScript@WorldObject@@')
                   for y in damage_body[max(0,i-10):i]) for i in loads), 'Native damage consumer does not dispatch instance callback slot'
for target in (policy,'?ProtectFinale@DarkRidersArtifact@@','?GetScenarioId@Scenario@@','?GetCurrentStep@Scenario@@'):
    assert n['symbol'](target) in n['calls'](callback), ('Missing actual damage dependency',target)
body = ins(policy)
values = {o.imm for x in body for o in x.operands if o.type==CS_OP_IMM}
assert {1026,102532,-4,1} <= values, 'Policy lacks exact scenario/actor or4..5 stage bounds'
assert any(x.mnemonic=='add' and x.op_str=='eax, -4' for x in body)
assert any(x.mnemonic=='cmp' and x.op_str=='eax, 1' and body[i+1].mnemonic=='ja' for i,x in enumerate(body[:-1]))
assert n['symbol']('?GetHealth@Unit@@QEBA_KPEAV1@@Z') in n['calls'](policy), 'Policy must use attacker-relative final health'
assert n['symbol']('?SetHealth@Unit@@') in n['calls'](policy), 'Missing native one-health hold'
assert any(x.mnemonic=='cmp' and x.op_str=='rbx, rax' and body[i+1].mnemonic=='jb' for i,x in enumerate(body[:-1])), 'Missing nonlethal damage refusal'
# Follow the third argument's saved pointer, rather than assuming MSVC keeps
# it in r14 (the added zero-damage guard moves it to rsi).
aliases = {'r8'}
for x in body:
    if x.mnemonic=='call':
        break
    if x.mnemonic=='mov' and all(o.type==CS_OP_REG for o in x.operands):
        destination,source = (x.reg_name(o.reg) for o in x.operands)
        if source in aliases:
            aliases.add(destination)
        else:
            aliases.discard(destination)
stores = [i for i,x in enumerate(body)
          if x.mnemonic=='mov' and x.operands[0].type==CS_OP_MEM and x.operands[0].size==4
          and x.reg_name(x.operands[0].mem.base) in aliases
          and not x.operands[0].mem.disp and not x.operands[0].mem.index]
assert any(body[i].operands[1].type==CS_OP_REG
           and any(y.mnemonic=='xor' and len(y.operands)==2
                   and all(o.type==CS_OP_REG and o.reg==body[i].operands[1].reg for o in y.operands)
                   for y in body[max(0,i-4):i]) for i in stores), 'Missing zero store through actual final-damage pointer'
assert any(x.mnemonic=='cmp' and x.operands[0].type==CS_OP_MEM
           and x.reg_name(x.operands[0].mem.base) in aliases and not x.operands[0].mem.disp
           and x.operands[1].type==CS_OP_IMM and x.operands[1].imm==0
           and body[i+1].mnemonic=='je' for i,x in enumerate(body[:-1])), 'Missing zero-damage early refusal'
for forbidden in ('?CompleteQuest@Player@@','?KilledMonsterCredit@Player@@','?RewardPlayerAndGroupAtEvent@Player@@','?UpdateAchievementCriteria@Player@@','?AddItem@Player@@'):
    assert not {n['resolve'](a) for name,a in n['symbols'].items() if name.startswith(forbidden)} & n['calls'](policy), ('Unexpected progression API',forbidden)
print(f'PASS actual Dark Riders factory/vtable/Unit damage slot0x{slot:x} -> callback -> existing Warlock policy plus Apocalypse102532/1026 stage4..5 final-damage policy; target-relative health, nonlethal refusal, damage cancellation/native SetHealth1, no direct progression. Native host separately verifies scaling/null/AI/player/pet/guardian paths; actual client/finale remains separate.')
