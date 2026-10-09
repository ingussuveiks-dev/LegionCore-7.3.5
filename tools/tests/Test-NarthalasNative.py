"""Read-only 7.3.5 evidence for the Nar'thalas academy (ten quests)."""
import hashlib,json,re,struct,sys
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/research'))
from audit_spell_mechanics import Data
quests={37467,37468,37736,37678,37518,42370,42371,37729,37730,37469}
dump=ROOT/'.codex/TDB735/TDB_full_735.00_2018_02_19/TDB_world_735.00_2018_02_19.sql'
objectives=[]
for line in dump.open(encoding='utf8'):
 if line.startswith('INSERT INTO `quest_objectives`'):
  for match in re.finditer(r"\((?:'(?:\\.|''|[^'\\])*'|[^'()])*\)",line):
   parts=match[0][1:].split(',')
   if int(parts[1]) in quests:
    objectives.append(dict(zip(('ID','QuestID','Type','Order','StorageIndex','ObjectID','Amount','Flags','Flags2'),map(int,parts[:9]))))
assert len(objectives)==21
assert {r['ObjectID'] for r in objectives if r['QuestID']==37736}=={120946,120947,120948,120949}
assert {r['ObjectID'] for r in objectives if r['QuestID']==42371}=={107299,107300,107301,107354,107355,107356,137422,137423,137426}
d=Data(ROOT/'build-extractors/bin/Release/dbc/enUS',ROOT/'.codex/progression-audit')
spells={177816,179185,179186,179187,179188,179189,179190,179203,212782,179151,179152,179153,212912,212913,212914}
fx=[r for r in d.rows('SpellEffect').values() if r['SpellID'] in spells]
def effect(s,i):return next(r for r in fx if r['SpellID']==s and r['EffectIndex']==i)
assert effect(177816,0)['EffectMiscValue1']==88889 and effect(177816,0)['EffectMiscValue2']==3385
assert effect(179185,0)['Effect']==157 and effect(179185,0)['ImplicitTarget1']==1
assert effect(179203,1)['EffectTriggerSpell']==108997
assert effect(212782,0)['Effect']==179 and effect(212782,0)['EffectMiscValue1']==7042
assert effect(212782,1)['EffectTriggerSpell']==218570
assert d.rows('OverrideSpellData')[711]['Spells1']==212782
assert not any(r['EffectAura']==293 and r['EffectMiscValue1']==711 for r in d.rows('SpellEffect').values())
assert all(s not in d.rows('Spell') for s in (108997,218570,212924))
for i in range(3):
 assert effect(179151+i,0)['EffectAura']==430 and effect(179151+i,0)['EffectMiscValue1']==935+i
 assert effect(212912+i,0)['Effect']==90 and effect(212912+i,0)['EffectMiscValue1']==107354+i
 assert effect(212912+i,1)['Effect']==198 and effect(212912+i,1)['EffectMiscValue1']==1279+i
# These sparse WDC1 rows contain two inline strings. Read the catalog exactly
# as DB2FileLoaderSparseImpl does; do not mistake the binary index for text.
scene_path=ROOT/'build-extractors/bin/Release/dbc/enUS/SceneScriptText.db2'
b=scene_path.read_bytes();lo,hi=struct.unpack_from('<II',b,28);catalog=struct.unpack_from('<I',b,60)[0]
assert b[:4]==b'WDC1' and struct.unpack_from('<H',b,44)[0]&1 and struct.unpack_from('<I',b,8)[0]==2
scenes={}
for id in (11725,13159,13160,13162,13164,13165,13166,14125,14126,14127,14128):
 assert lo<=id<=hi
 offset,size=struct.unpack_from('<IH',b,catalog+6*(id-lo));assert size and offset+size<=catalog
 values=b[offset:offset+size].split(b'\0')
 scenes[id]={'name':values[0].decode('utf8'),'text':values[1].decode('utf8')}
assert 'scene:TriggerServerEvent("Credit")' in scenes[13162]['text']
assert 'PathCache:Load(13470)' in scenes[13160]['text']
assert 'scene:TriggerServerEvent("phase")' in scenes[14125]['text']
assert 'scene:SetCamera(0,1.5,nil,nil)' in scenes[14125]['text']
node=d.rows('PathNode')[116700];assert node['PathID']==13470 and node['LocationID']==116720
location=d.rows('Location')[116720]
assert abs(location['Pos1']-200.607635)<.001 and abs(location['Pos3']+53.777821)<.001
members=[r for r in d.rows('SceneScriptPackageMember').values() if r['SceneScriptPackageID'] in (820,1378,1379,1380,1628,1629,1630)]
assert all(any(r['SceneScriptPackageID']==1378+i and r['SceneScriptID']==13164+i for r in members) for i in range(3))
visual_ids={58937,58940,58941,45470,45473,45474,45475,45476,45477}
assert visual_ids <= d.rows('SpellVisual').keys()
item_effects=[r for r in d.rows('ItemEffect').values() if r['ItemID'] in (120946,120947,120948,120949)]
assert len(item_effects)==4 and all(r['TriggerType']==5 for r in item_effects)
evidence=dict(quests=sorted(quests),objectives=sorted(objectives,key=lambda r:(r['QuestID'],r['Order'])),
 archive=dump.name,archive_sha256=hashlib.sha256(dump.read_bytes()).hexdigest(),effects=fx,item_effects=item_effects,
 wand_bar=d.rows('OverrideSpellData')[711],summon_properties=d.rows('SummonProperties')[3385],scene_members=members,
 rune_location=location,scene_text_sha256=hashlib.sha256(b).hexdigest(),
 scene_scripts={id:dict(name=r['name'],text_sha256=hashlib.sha256(r['text'].encode()).hexdigest()) for id,r in scenes.items()},
 rune_success_event='Credit',book_events=['phase','phase2'],rune_visual_ids=sorted(visual_ids),runtime_sha256=d.hashes,
 limitations='Not a client playthrough. The native book camera scene is retained, but sketch-world phasing and retail boss abilities are not reconstructed. Book shelf X/Y come from saved quest POIs; height uses the existing lower classroom floor. Save Yourself and The Head of the Snake remain outside this academy repair.')
if '--write-evidence' in sys.argv:
 (ROOT/'docs/audits/narthalas-native-2026-10-09.json').write_text(json.dumps(evidence,indent=2)+'\n',encoding='utf8')
print('PASS: ten quests / 21 native objectives, wand bar and projectile, original robes, three rune success scenes, book scenes, true rune origin and valid rune visuals.')
