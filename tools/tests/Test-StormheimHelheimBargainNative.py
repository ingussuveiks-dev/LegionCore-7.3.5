"""Native Helheim bargain/effect/conversation evidence; no client playback claim."""
import re,json,hashlib,sys
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/research'))
from audit_spell_mechanics import Data
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
  if (n.startswith('quest_') and key in (39848,39857)) or (n in ('creature_queststarter','creature_questender','gameobject_queststarter','gameobject_questender') and key in(39848,39857)) or (n=='creature_template' and key in(91387,97479,97695,97696)) or (n=='gameobject_template' and key==244559):records.setdefault(n,[]).append(r)
approach=next(r for r in records['quest_objectives'] if r['QuestID']=='39848' and r['ObjectID']=='97696')
assert all(approach[k]==v for k,v in {'ID':'280092','Type':'0','StorageIndex':'0','ObjectID':'97696','Amount':'1','Flags':'0'}.items())
obj=next(r for r in records['quest_objectives'] if r['QuestID']=='39848' and r['ObjectID']=='97479')
assert all(obj[k]==v for k,v in {'ID':'280017','Type':'0','StorageIndex':'2','ObjectID':'97479','Amount':'1','Flags':'2'}.items())
records['quest_template']=[{k:r[k] for k in ('ID','LogTitle','LogDescription','QuestDescription','Flags','FlagsEx','RewardNextQuest')} for r in records['quest_template']]
records['creature_template']=[{k:r[k] for k in ('entry','name','modelid1','faction','npcflag','AIName','ScriptName')} for r in records['creature_template']]
d=Data(ROOT/'build-extractors/bin/Release/dbc/enUS',ROOT/'.codex/stormheim-no-overlays')
ids={212898,192518}
native={t:[r for r in d.rows(t).values() if r.get('SpellID') in ids] for t in ('SpellEffect','SpellMisc','SpellXSpellVisual')}
assert any(r['SpellID']==212898 and r['Effect']==219 and r['EffectMiscValue1']==2251 and r['ImplicitTarget1']==18 for r in native['SpellEffect'])
assert any(r['SpellID']==192518 and r['Effect']==203 and r['EffectTriggerSpell']==192299 for r in native['SpellEffect'])
assert not native['SpellXSpellVisual']
records.update(archive_sha256=hashlib.sha256(archive.read_bytes()).hexdigest(),native=native,runtime_hashes=d.hashes,limits='Native212898 summons conversation2251, not2252, and has no direct SpellXSpellVisual. The native C++ reader checks its sparse ConversationLine4747/broadcast, actor display65043 and altar display7786. World conversation actor50552/91387 is supplied directly, not resolved to an ambient creature GUID. Base BroadcastText98487 is absent; the installed Hotfix supplies Helya female text and voice52998, independently checked by the catalog wrapper. Native records do not certify rendered speech/actor playback. Preserve original objectives including sequenced flag2; installed quest Flags0 differ from original38797320 and remain unchanged.')
path=ROOT/'docs/audits/stormheim-helheim-bargain-native-2026-10-16.json'
if '--write-evidence' in sys.argv:path.write_text(json.dumps(records,indent=2)+'\n')
else:assert json.loads(json.dumps(records))==json.loads(path.read_text())
print('PASS: original39848 objectives/relations and native212898 effect219 -> conversation2251, destination18;192518 removes legacy192299. No direct spell visuals or client-playback assertion.')
