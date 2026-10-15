"""Native TDB735 supply objectives plus actual 26972 Gathering lock/effect."""
import re,json,hashlib,sys
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/research'))
from audit_spell_mechanics import Data
archive=ROOT/'.codex/TDB735/TDB_full_735.00_2018_02_19/TDB_world_735.00_2018_02_19.sql'
selected={'quest_template','quest_template_addon','quest_objectives','gameobject_template','gameobject_loot_template','creature_template','creature_loot_template'}
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
  key=int(next((r[k] for k in ('QuestID','ID','entry','Entry') if k in r),'0'))
  if (n.startswith('quest_') and key in (38324,38339,38347)) or (n.startswith('gameobject_') and key==240586) or (n.startswith('creature_') and key in(91575,93377)):records.setdefault(n,[]).append(r)
objectives={int(r['QuestID']):r for r in records['quest_objectives']}
for q,item,amount in [(38324,122611,8),(38339,122619,20)]:
 assert all(objectives[q][k]==v for k,v in {'Type':'1','StorageIndex':'0','ObjectID':str(item),'Amount':str(amount),'Flags':'0'}.items())
pile,=records['gameobject_template']
assert pile['type']=='3' and pile['displayId']=='6328' and pile['Data0']=='2160' and pile['Data8']=='38324'
for r in records['quest_template']:
 if r['ID'] in ('38324','38339'):assert r['RewardNextQuest']=='38347' and r['Flags']=='37748744'
records['quest_template']=[{k:r[k] for k in ('ID','LogTitle','LogDescription','QuestDescription','Flags','FlagsEx','AllowableRaces','RewardNextQuest')} for r in records['quest_template']]
records['creature_template']=[{k:r[k] for k in ('entry','name','modelid1','faction','lootid')} for r in records['creature_template']]
d=Data(ROOT/'build-extractors/bin/Release/dbc/enUS',ROOT/'.codex/stormheim-no-overlays')
effects=[r for r in d.rows('SpellEffect').values() if r['SpellID']==6478]
assert len(effects)==1 and all(effects[0][k]==v for k,v in {'Effect':33,'EffectMiscValue1':13,'ImplicitTarget1':23}.items())
lock=d.rows('Lock')[2160];assert lock['Type2']==2 and lock['Index2']==13 and lock['Skill2']==0
records.update(archive_sha256=hashlib.sha256(archive.read_bytes()).hexdigest(),native={'Lock':lock,'Gathering6478':effects,'Map1463':d.rows('Map')[1463]},runtime_hashes=d.hashes,limits='Native objectives and Gathering are confirmed, including sparse items/displays in the separate C++ DB2Storage catalog. Original TDB does not establish the installed custom loot chances or hostile faction16: preserve those working rows. Original pile Data1=0/Data30=60523/Data31=1 differs from installed personal settings0/0; only restore the missing existing240586 selector, without inventing a60523 loot mapping. Original objective Flags2=1 differs from installed0; no objective type/slot/flag or character progress rewrite is made. Native records are not rendered visual/client-loot evidence.')
path=ROOT/'docs/audits/stormheim-helheim-supplies-native-2026-10-15.json'
if '--write-evidence' in sys.argv:path.write_text(json.dumps(records,indent=2)+'\n')
else:assert json.loads(json.dumps(records))==json.loads(path.read_text())
print('PASS: original8 bones/20 seaweed item objectives and38347 handoff; native6478 effect33/type13/target23 and Gathering lock2160. Sparse items/models checked by C++ catalog; client interactions pending.')
