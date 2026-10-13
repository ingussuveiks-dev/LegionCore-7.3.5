"""Native rounds objectives, bars, spell targets and visual references."""
import sys,runpy,json,hashlib
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2]
arguments=sys.argv[:];sys.argv=[str(ROOT/'tools/tests/Test-StormheimIntroNative.py')]
native=runpy.run_path(sys.argv[0]);sys.argv=arguments
d=native['d'];objectives=[r for r in native['objectives'] if r['QuestID'] in(38206,39698)]
for q,expected in ((38206,[(0,96662,0),(0,96661,2),(0,96663,2)]),(39698,[(2,247491,0),(0,96688,2),(0,96689,2)])):
 rows=sorted((r for r in objectives if r['QuestID']==q),key=lambda r:r['StorageIndex'])
 assert [(r['Type'],r['ObjectID'],r['Flags']) for r in rows]==expected
 assert all(r['Amount']==1 and r['Flags2']==0 for r in rows)
bars=[d.rows('OverrideSpellData')[id] for id in(753,754)]
assert [(r['Spells1'],r['Flags']) for r in bars]==[(220291,8),(220525,8)]
spells={69333,69334,220521,220291,220525,220293,220526,57764}
effects=[r for r in d.rows('SpellEffect').values() if r['SpellID'] in spells]
for spell,credit,event,bar in((220291,96688,53167,220293),(220525,96661,53130,220526)):
 assert any(r['SpellID']==spell and r['Effect']==90 and r['EffectMiscValue1']==credit and r['ImplicitTarget1']==1 for r in effects)
 assert any(r['SpellID']==spell and r['Effect']==61 and r['EffectMiscValue1']==event for r in effects)
 assert any(r['SpellID']==spell and r['Effect']==164 and r['EffectTriggerSpell']==bar for r in effects)
assert any(r['SpellID']==220521 and r['Effect']==3 and r['ImplicitTarget1']==25 for r in effects)
for spell,bar in((220293,753),(220526,754)):
 assert any(r['SpellID']==spell and r['EffectAura']==293 and r['EffectMiscValue1']==bar and r['ImplicitTarget1']==1 for r in effects)
misc=[r for r in d.rows('SpellMisc').values() if r['SpellID'] in spells]
assert all(r['RangeIndex']==1 for r in misc if r['SpellID'] in(220291,220525,220293,220526))
assert any(r['SpellID']==220521 and r['RangeIndex']==12 for r in misc)
links=[r for r in d.rows('SpellXSpellVisual').values() if r['SpellID'] in spells]
visuals=[r for id,r in d.rows('SpellVisual').items() if id in {r['SpellVisualID'] for r in links}]
assert len(visuals)==len({r['SpellVisualID'] for r in links})
salute=d.rows('Emotes')[66]
out={'archive':native['p'].name,'archive_sha256':hashlib.sha256(native['p'].read_bytes()).hexdigest(),
 'objectives':objectives,'bars':bars,'effects':effects,'misc':misc,'visual_links':links,'visuals':visuals,'salute_emote':salute,'runtime_hashes':d.hashes,
 'limits':'Native references are evidence of what is sent; client animation, model attachments, bar rendering and dialogue still require a client test.'}
evidence=ROOT/'docs/audits/stormheim-rounds-native-2026-10-13.json'
if '--write-evidence' in arguments:evidence.write_text(json.dumps(out,indent=2)+'\n')
else:assert json.loads(json.dumps(out))==json.loads(evidence.read_text())
print('PASS: six required native objectives, sequenced stages, both self-targeted bars/salutes, gun ANY target and native visual references.')
