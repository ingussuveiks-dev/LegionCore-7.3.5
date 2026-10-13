"""Read native TDB735 quests and installed DB2 item/spell data without overlays."""
import sys,re,json,hashlib
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tools/research'))
from audit_spell_mechanics import Data
IDS={39735,39864,38035,38307,38206,39698,39800,39801}
p=ROOT/'.codex/TDB735/TDB_full_735.00_2018_02_19/TDB_world_735.00_2018_02_19.sql'
names={'quest_template','quest_objectives','creature_queststarter','creature_questender','gameobject_queststarter','gameobject_questender'}
cols={};data={};table=None
for line in p.open(encoding='utf8'):
 m=re.match(r'CREATE TABLE `([^`]+)`',line)
 if m:table=m[1] if m[1] in names else None;cols[table]=[]
 elif table:
  if line.startswith(')'):table=None
  else:
   m=re.match(r'  `([^`]+)`',line)
   if m:cols[table].append(m[1])
 m=re.match(r'INSERT INTO `([^`]+)`',line)
 if not m or m[1] not in names:continue
 name=m[1]
 for m in re.finditer(r"\((?:'(?:\\.|''|[^'\\])*'|[^'()])*\)",line):
  r=dict(zip(cols[name],re.findall(r"'(?:\\.|''|[^'\\])*'|[^,]+",m[0][1:-1])))
  if int(r.get('QuestID',r.get('quest',r.get('ID',0)))) in IDS:data.setdefault(name,[]).append(r)
quests={int(r['ID']):r for r in data['quest_template']};assert set(quests)==IDS
objectives=[{k:int(r[k]) for k in ('ID','QuestID','Type','StorageIndex','ObjectID','Amount','Flags','Flags2')} for r in data['quest_objectives']]
for q,expected in ((39735,[(97377,0)]),(39864,[(97377,0)]),(38035,[(96644,0),(96663,2)]),(38307,[(96683,0),(96686,2)])):
 rows=sorted((r for r in objectives if r['QuestID']==q),key=lambda r:r['StorageIndex'])
 assert [(r['ObjectID'],r['Flags']) for r in rows]==expected
 assert all(r['Type']==0 and r['Amount']==1 and r['Flags2']==0 for r in rows)
assert quests[38035]['StartItem']=='128687' and quests[38307]['StartItem']=='128692'
d=Data(ROOT/'build-extractors/bin/Release/dbc/enUS',ROOT/'.codex/stormheim-no-overlays')
items=[r for r in d.rows('ItemEffect').values() if r['ItemID'] in(128687,128692)]
assert {(r['ItemID'],r['SpellID'],r['TriggerType']) for r in items}=={(128687,191405,0),(128692,191467,0)}
effects=[r for r in d.rows('SpellEffect').values() if r['SpellID'] in(191405,191467,192465,192477)]
for spell,credit,conversation in ((191405,96644,1417),(191467,96683,1409)):
 assert any(r['SpellID']==spell and r['Effect']==90 and r['EffectMiscValue1']==credit and r['ImplicitTarget1']==1 for r in effects)
 assert any(r['SpellID']==spell and r['Effect']==219 and r['EffectMiscValue1']==conversation for r in effects)
for spell in(192465,192477):
 assert any(r['SpellID']==spell and r['Effect']==252 and r['ImplicitTarget2']==17 for r in effects)
 assert any(r['SpellID']==spell and r['Effect']==90 and r['EffectMiscValue1']==97549 for r in effects)
assert not any(r['Effect']==16 and r['EffectMiscValue1'] in(39735,39864,38035,38307) for r in d.rows('SpellEffect').values())
out={'archive':p.name,'archive_sha256':hashlib.sha256(p.read_bytes()).hexdigest(),
 'quests':{q:{k:r[k] for k in ('ID','LogTitle','LogDescription','Flags','FlagsEx','StartItem','ItemDrop1','ItemDropQuantity1','RewardNextQuest')} for q,r in quests.items()},
 'objectives':objectives,'starters':data.get('creature_queststarter',[]),'enders':data.get('creature_questender',[]),
 'item_effects':items,'spell_effects':effects,'runtime_hashes':d.hashes,
 'note':'Effect 219 is SUMMON_CONVERSATION, not a SceneScriptPackage. Spell_scene rows are not used by these letter effects.'}
evidence=ROOT/'docs/audits/stormheim-intro-native-2026-10-13.json'
if '--write-evidence' in sys.argv:evidence.write_text(json.dumps(out,indent=2)+'\n')
else:assert json.loads(json.dumps(out))==json.loads(evidence.read_text())
print('PASS: native faction introductions, letter + sequenced reporting objectives, item spells/conversations, native teleports and absence of QUEST_COMPLETE event spells.')
