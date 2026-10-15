"""Replay migrated SQL links and validate their native 7.3.5 criteria order.

This does not emulate the full SmartAI scheduler, client scenes or combat.
"""
import json
from pathlib import Path
import sys
sys.dont_write_bytecode=True
rows=Path(sys.argv[1]).read_text().splitlines()
blocks=[]
for row in rows:
    if row in ('PASS1','PASS2'):blocks.append([])
    elif blocks and not row.startswith('custom_instance'):blocks[-1].append(row)
assert len(blocks)==2 and blocks[0]==blocks[1], 'migration is not repeatable'
data=blocks[1]
sai=[json.loads(r.split('\t',1)[1]) for r in data if r.startswith('smart\t')]
boss={r['id']:r for r in sai if r['entry']==100344 and r['source']==0}
assert 'compass\t245794\t3\t245794\t132377\t100\t40606' in data
assert 'go_preserved\t2' in data
assert 'instance\t1533\tinstance_dark_riders_artifact' in data
for r in boss.values():
    if r['link']:assert r['link'] in boss and boss[r['link']]['event']==61
assert boss[9]['event']==37 and boss[9]['a1']==1
assert boss[10]['action']==11 and boss[10]['a1']==199764 and not boss[10]['link']
assert boss[90]['event']==2 and (boss[90]['e1'],boss[90]['e2'])==(0,1) and boss[90]['flags']&1
assert boss[90]['action']==1 and boss[90]['a1']==3

sys.path.insert(0,str(Path(sys.argv[2])/'tools/research'))
from audit_spell_mechanics import Data
native=Data(Path(sys.argv[2])/'build-extractors/bin/Release/dbc/enUS',Path('__native_only__'))
steps=sorted((s for s in native.rows('ScenarioStep').values() if s['ScenarioID']==988),key=lambda s:s['OrderIndex'])
tree=native.rows('CriteriaTree');criteria=native.rows('Criteria')
leaves={}
for s in steps:
    descendants=[r for r in tree.values() if r['Parent']==s['Criteriatreeid']]
    assert len(descendants)==1
    leaves[s['OrderIndex']]=criteria[descendants[0]['CriteriaID']]
assert [(leaves[i]['Type'],leaves[i]['Asset']) for i in (4,5,6)]==[(92,47988),(92,47989),(36,128942)]
effects=[e for e in native.rows('SpellEffect').values() if e['SpellID']==204819]
assert any(e['Effect']==90 and e['EffectMiscValue1']==103473 for e in effects)
assert any(e['Effect']==249 and e['EffectMiscValue1']==128942 for e in effects)
assert not any(e['Effect'] in (24,157,206) and e['EffectItemType']==128942 for e in effects)

for optional_cast_succeeds in (False,True):
    actions=[]
    def run(event):
        r=boss[event];actions.append((r['action'],r['a1'],r['a2']))
        # Native SMART_ACTION_CAST returns before its LINK on cast failure.
        if r['action']==11 and not optional_cast_succeeds:return
        if r['link']:run(r['link'])
    run(10)  # optional cosmetic cast must not expose the chest or consume speech
    assert not any(a[0] in (50,205) for a in actions)
    run(90)
    assert (1,3,5000) in actions and not any(a[0]==50 for a in actions)
    # Actual TextOver3 starts the defeat completion, after the retained 5s speech.
    assert boss[13]['event']==52 and boss[13]['e1']==3
    run(13)
    kinds=[a[0] for a in actions]
    assert kinds.index(205)<kinds.index(50)<kinds.index(12)<kinds.index(37)
    assert sum(a[0]==50 and a[1]==246021 for a in actions)==1
    assert any(a==(205,92,47989) for a in actions)
    # Aggro moves step4 -> 5; defeat then makes step6 active before the chest.
    assert leaves[5]['Asset']==47989 and leaves[6]['Asset']==128942
    assert any(r['entry']==101282 and r['id']==12 and r['action']==53 for r in sai)
assert any(r['entry']==246021 and r['event']==70 and r['e1']==3 and r['a1']==204819 for r in sai)
print('PASS: native988 criteria/loot spell, compass loot, migration replay, optional-cast failure, delayed chest and defeat -> chest -> guide -> death order')
