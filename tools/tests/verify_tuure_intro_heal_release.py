"""Follow the actual Boros first-heal lease, native95pct completion and factory."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM,CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

p=runpy.run_path(str(Path(__file__).with_name('verify_tuure_ally_damage_release.py')))
n,instructions,references,values=p['n'],p['instructions'],p['references'],p['values']
scope='npc_vindicator_borosAI@npc_vindicator_boros@@'
heal,finish,reset,life,cancel=('?'+method+'@'+scope for method in ('HealReceived','CompleteIntroHeal','Reset','UpdateLifecycle','CancelIntroHeal'))
for prefix in ('?GetCharmerOrOwnerPlayerOrPlayerItself@Unit@@','?Eligible@TuureIntroHeal@@','?AddQuestDelayedEvent@Player@@','?getQuestStatus@Player@@'):
    assert n['symbol'](prefix) in n['calls'](heal), f'Missing actual heal lease dependency {prefix}'
assert 41993 in values(heal)
assert n['symbol']('?UpdateAchievementCriteria@Player@@') not in n['calls'](heal), 'Pre-heal progress remains'
for prefix in ('?GetPlayer@ObjectAccessor@@','?getQuestStatus@Player@@','?Eligible@TuureIntroHeal@@','?GetHealthPct@Unit@@','?UpdateAchievementCriteria@Player@@'):
    assert n['symbol'](prefix) in n['calls'](finish), f'Missing actual completion dependency {prefix}'
assert 50563 in values(finish) and 92 in values(finish)
assert {106134,1489,41993,1085}<=values('?Eligible@TuureIntroHeal@@')
body=instructions(finish)
site=next(i for i,x in enumerate(body) if x.mnemonic=='call' and x.operands[0].type==CS_OP_IMM and n['resolve'](x.operands[0].imm)==n['symbol']('?GetHealthPct@Unit@@'))
assert any(x.mnemonic in ('comiss','ucomiss') for x in body[site+1:site+5])
constants=[]
for x in body[site+1:site+5]:
    for op in x.operands:
        if op.type==CS_OP_MEM and op.mem.base==X86_REG_RIP:
            constants.append(struct.unpack_from('<f',n['binary'],n['offset'](x.address+x.size+op.mem.disp))[0])
assert 95.0 in constants, 'Completion misses actual95pct boundary'
assert n['symbol'](finish) in n['calls'](life)
assert not {n['resolve'](a) for name,a in n['symbols'].items() if name.startswith('?FindNearestPlayer@')} & n['calls'](finish)
assert not any(name.startswith('?SetHealth@') and n['resolve'](a) in n['calls'](finish) for name,a in n['symbols'].items())
print('PASS actual Boros HealReceived records native owner/fullGUID/quest lease without pre-heal credit; selected locked lifecycle resolves original/rechecks actual95pct and native50563/movement, with no nearest player or forced health. Native full-handler host covers reset/retry/stale lease/recipient/AI/failed heal; client first heal/follow pending.')
