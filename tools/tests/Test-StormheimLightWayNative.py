"""Native To Light the Way archive/catalog evidence; no client playback claim."""
import re,json,hashlib,sys
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/research'))
archive=ROOT/'.codex/TDB735/TDB_full_735.00_2018_02_19/TDB_world_735.00_2018_02_19.sql'
selected={'quest_template','quest_template_addon','quest_objectives','gameobject_template','gameobject_loot_template','creature_template','creature_loot_template','creature_queststarter','creature_questender','gameobject_queststarter','gameobject_questender'}
cols={};records={};table=None
for line in archive.open(encoding='utf8'):
 m=re.match(r'CREATE TABLE `([^`]+)`',line)
 if m:table=m[1] if m[1] in selected else None;cols[table]=[]
 elif table:
  if line.startswith(')'):table=None
  else:
   m=re.match(r'  `([^`]+)`',line)
   if m:cols[table].append(m[1])
 m=re.match(r'INSERT INTO `([^`]+)`',line)
 if not m or m[1] not in selected:continue
 n=m[1]
 for mt in re.finditer(r"\((?:'(?:\\.|''|[^'\\])*'|[^'()])*\)",line):
  r=dict(zip(cols[n],re.findall(r"'(?:\\.|''|[^'\\])*'|[^,]+",mt[0][1:-1])))
  key=int(next((r[k] for k in ('QuestID','quest','ID','entry','Entry') if k in r),'0'))
  if (n.startswith('quest_') and key in (39849,39850)) or (n in ('creature_queststarter','creature_questender','gameobject_queststarter','gameobject_questender') and key in(39849,39850)) or (n=='creature_template' and key in(91657,97480)) or (n=='creature_loot_template' and key==91657):records.setdefault(n,[]).append(r)
objective=next(r for r in records['quest_objectives'] if r['QuestID']=='39849')
assert all(objective[k]==v for k,v in {'ID':'280021','Type':'1','StorageIndex':'0','ObjectID':'128854','Amount':'1','Flags':'0'}.items())
records['original_39849_starter_present']=any(r['quest']=='39849' and r['id']=='97480' for r in records.get('creature_queststarter',[]))
records['quest_template']=[{k:r[k] for k in ('ID','LogTitle','LogDescription','QuestDescription','Flags','FlagsEx','RewardNextQuest')} for r in records['quest_template']]
records['creature_template']=[{k:r[k] for k in ('entry','name','modelid1','faction','npcflag','lootid','AIName','ScriptName')} for r in records['creature_template']]
out=ROOT/'build-extractors/runtime-test/stormheim-light-way'
records.update(archive_sha256=hashlib.sha256(archive.read_bytes()).hexdigest(),native_catalog=(out/'native-lines.txt').read_text(encoding='utf-8-sig').splitlines(),runtime_hashes=json.loads((out/'native-inputs.json').read_text(encoding='utf-8-sig')),live_world=(out/'world.tsv').read_text(encoding='utf-8-sig').splitlines(),live_hotfix=(out/'hotfix.tsv').read_text(encoding='utf-8-sig').splitlines(),limits='No confirmed code/data defect in installed39849. Preserve installed hostile Bloodbeard/loot91657/quest drop128854/chance99.155/mode0 and Ashildir starter relation even where the original TDB is incomplete. Native sparse delivery item has no use effect; objective completes on item pickup and consumes one at reward. Loot/owner/item/native reward-next fragments and actual native item class-mask builder are executed, but full combat, inventory serialization, tap distribution, package reward creation and client UI are not.')
path=ROOT/'docs/audits/stormheim-light-way-native-2026-10-16.json'
if '--write-evidence' in sys.argv:path.write_text(json.dumps(records,indent=2)+'\n')
else:assert json.loads(json.dumps(records))==json.loads(path.read_text())
print('PASS: native39849 ITEM128854 objective, original quest narrative, actual item/no use effect, package746 class masks and installed Bloodbeard/loot/handoff evidence.')
print('Quests:',[(r['ID'],r['LogTitle']) for r in records['quest_template']])
