"""Verify actual Tuure wound, positive-heal pending and lifecycle implementations."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

p=runpy.run_path(str(Path(__file__).with_name('verify_tuure_defeat_release.py')))
n,instructions,references,values=p['n'],p['instructions'],p['references'],p['values']
for script in ('npc_vindicator_boros','npc_tuure_small_events'):
    scope=script+'AI@'+script+'@@'
    post,heal,reset,life=('?'+method+'@'+scope for method in ('DamageTakenAfterScaling','HealReceived','Reset','UpdateLifecycle'))
    tables=[]
    for address in references('?GetAI@'+script+'@@'):
        table=struct.unpack_from('<80Q',n['binary'],n['offset'](address))
        if table[0xa8//8]==n['symbol'](post):tables.append(table)
    assert len(tables)==1
    table=tables[0]
    assert table[0xa0//8]==n['symbol']('?DamageTaken@UnitAI@@')
    assert table[0xb0//8]==n['symbol'](heal) and table[0x110//8]==n['symbol'](life)
    assert n['symbol'](reset) in table
    assert n['symbol']('?GetHealth@Unit@@QEBA_KPEAV1@@') in n['calls'](post)
    assert n['symbol']('?AddAura@Unit@@QEAAPEAVAura@@I') in n['calls'](post)
    assert 204237 in values(post) and 10 in values(post)
    assert n['symbol']('?Recover@TuureCompanion@@') in n['calls'](life)
    assert not n['symbol']('?UpdateAchievementCriteria@Player@@') in n['calls'](life)
    assert not n['symbol']('?GetHealthPct@Unit@@') in n['calls'](heal) or script=='npc_vindicator_boros' # initial95pct is separate
recovery='?Recover@TuureCompanion@@'
body=instructions(recovery)
for prefix in ('?IsInWorld@Object@@','?IsPreDelete@Object@@','?GetHealthPct@Unit@@','?RemoveAura@Unit@@QEAAXIVObjectGuid@@'):
    assert n['symbol'](prefix) in n['calls'](recovery)
assert 204237 in values(recovery)
site=next(i for i,x in enumerate(body) if x.mnemonic=='call' and x.operands[0].type==CS_OP_IMM and n['resolve'](x.operands[0].imm)==n['symbol']('?GetHealthPct@Unit@@'))
threshold=body[site+1]
assert threshold.mnemonic=='movss' and threshold.operands[1].mem.base==X86_REG_RIP
assert struct.unpack_from('<f',n['binary'],n['offset'](threshold.address+threshold.size+threshold.operands[1].mem.disp))[0]==40.0
assert body[site+2].mnemonic=='comiss' and body[site+3].mnemonic=='ja'
# Verify the actual shared core dispatch as well as both selected script slots.
heal_body=instructions('?DealHeal@Unit@@')
received=next(i for i,x in enumerate(heal_body) if x.mnemonic=='call' and x.operands[0].type==CS_OP_MEM and x.operands[0].mem.disp==0xb0)
done=next(i for i,x in enumerate(heal_body) if x.mnemonic=='call' and x.operands[0].type==CS_OP_MEM and x.operands[0].mem.disp==0xb8)
applied=next(i for i,x in enumerate(heal_body) if x.mnemonic=='call' and x.operands[0].type==CS_OP_IMM and n['resolve'](x.operands[0].imm)==n['symbol']('?ModifyHealth@Unit@@'))
assert received<done<applied
assert len([x for x in instructions('?Update@Creature@@') if x.mnemonic=='call' and x.operands[0].type==CS_OP_MEM and x.operands[0].mem.disp==0x110])==2
print('PASS selected Tuure ally factories: native final-scaled wound204237/Talk10, positive-heal pending/Reset and locked lifecycle0x110 with actual40pct recovery; native HealReceived/HealDone/application order retained. Initial Boros95pct/arrival and actual client spell/healing remain separate checks.')
