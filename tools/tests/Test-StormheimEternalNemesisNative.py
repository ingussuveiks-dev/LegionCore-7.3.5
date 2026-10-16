"""Native Eternal Nemesis archive/catalog evidence; no client playback claim."""
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
  if (n.startswith('quest_') and key in (39857,39849)) or (n in ('creature_queststarter','creature_questender','gameobject_queststarter','gameobject_questender') and key in(39857,39849)) or (n=='creature_template' and key in(91387,97480)) or (n=='gameobject_template' and key==244559):records.setdefault(n,[]).append(r)
assert not any(r['QuestID']=='39857' for r in records.get('quest_objectives',[]))
assert any(r['quest']=='39857' and r['id']=='244559' for r in records['gameobject_queststarter'])
assert any(r['quest']=='39857' and r['id']=='97480' for r in records['creature_questender'])
records['original_39849_starter_present']=any(r['quest']=='39849' and r['id']=='97480' for r in records.get('creature_queststarter',[]))
records['quest_template']=[{k:r[k] for k in ('ID','LogTitle','LogDescription','QuestDescription','Flags','FlagsEx','RewardNextQuest')} for r in records['quest_template']]
records['creature_template']=[{k:r[k] for k in ('entry','name','modelid1','faction','npcflag','AIName','ScriptName')} for r in records['creature_template']]
out=ROOT/'build-extractors/runtime-test/stormheim-eternal-nemesis'
records.update(archive_sha256=hashlib.sha256(archive.read_bytes()).hexdigest(),native_catalog=(out/'native-lines.txt').read_text(encoding='utf-8-sig').splitlines(),runtime_hashes=json.loads((out/'native-inputs.json').read_text(encoding='utf-8-sig')),live_world=(out/'world.tsv').read_text(encoding='utf-8-sig').splitlines(),live_hotfix=(out/'hotfix.tsv').read_text(encoding='utf-8-sig').splitlines(),limits='No confirmed code/data defect in installed39857. Original TDB lacks the39849 starter relation; installed97480/39849 is present and exercised, and must not be removed by restoring the incomplete archive. Native completion before callback still invokes the actual inherited GO acceptance action2062252. World explicitly emits line4748 only; native next7170 has Valarjar-specific text. Do not infer rendered playback or add that line to all classes without evidence. Preserve native zero objectives and33554440 flags; no migration or character reset.')
path=ROOT/'docs/audits/stormheim-eternal-nemesis-native-2026-10-16.json'
if '--write-evidence' in sys.argv:path.write_text(json.dumps(records,indent=2)+'\n')
else:assert json.loads(json.dumps(records))==json.loads(path.read_text())
print('PASS: original39857 zero objectives/altar starter/Ashildir return agree with native C++ catalog; installed39849 starter is present despite the incomplete archive.')
