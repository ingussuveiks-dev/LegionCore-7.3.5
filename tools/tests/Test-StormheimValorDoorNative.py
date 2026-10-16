"""Original40001 and installed native catalog evidence; no client certification."""
import hashlib,json,re,sys
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2]
archive=ROOT/'.codex/TDB735/TDB_full_735.00_2018_02_19/TDB_world_735.00_2018_02_19.sql'
selected={'quest_template','quest_objectives','creature_template','creature_questender','creature_template_addon'}
cols={};original={};table=None
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
        key=int(next((r[k] for k in ('QuestID','quest','ID','entry') if k in r),'0'))
        if (n.startswith('quest_') and key==40001) or (n=='creature_questender' and key==40001) or (n in('creature_template','creature_template_addon') and key==97979):original.setdefault(n,[]).append(r)
assert len(original['quest_template'])==1 and not original.get('quest_objectives')
quest=original['quest_template'][0]
assert quest['RewardNextQuest']=='40002' and 'Find Vethir' in quest['LogDescription']
assert any(r['id']=='97979' for r in original['creature_questender'])
actor=original['creature_template'][0]
assert actor['modelid1']=='68213' and actor['npcflag']=='2' and actor['faction']=='35'
original['quest_template']=[{k:quest[k] for k in ('ID','LogTitle','LogDescription','QuestDescription','Flags','FlagsEx','RewardNextQuest')}]
original['creature_template']=[{k:actor[k] for k in ('entry','name','modelid1','gossip_menu_id','faction','npcflag','unit_flags','unit_flags2','AIName','ScriptName')}]
out=ROOT/'build-extractors/runtime-test/stormheim-valor-door'
native=(out/'native-lines.txt').read_text(encoding='utf-8-sig').splitlines()
assert 'quest 40002 line 138 order 0' in native and 'quest 40078 line 135 order 11' in native
assert 'quest40001 has no native QuestLineXQuest row; handoff is established by World relations' in native
assert 'Vethir display 68213 scale 0.75 model 8673 file 1280971 modelScale 1' in native
assert 'faction148 canHaveReputation 0' in native
geometry=(out/'geometry.txt').read_text(encoding='utf-8-sig').strip()
assert geometry.startswith('PASS:') and '118 native navmesh polygons' in geometry
record={
    'quest':40001,'title':"Knocking on Valor's Door",'status':'code/data audit passed; no confirmed implementation defect; client pending',
    'archive_sha256':hashlib.sha256(archive.read_bytes()).hexdigest(),'original':original,
    'world_evidence':(out/'world.tsv').read_text(encoding='utf-8-sig').splitlines(),
    'hotfix_evidence':(out/'hotfix.tsv').read_text(encoding='utf-8-sig').splitlines(),
    'native_catalog':native,'native_hashes':json.loads((out/'native-inputs.json').read_text(encoding='utf-8-sig')),'ground_route':geometry,
    'behavior':[
        'Native AddQuestAndCheckCompletion marks this no-objective quest complete on acceptance. This is not reward: the native request-reward handler requires the involved Vethir questgiver and successful interaction admission.40001 has neither AUTOCOMPLETE0x10000 nor exploration/event special flag. No invented proximity, kill, spell or gossip credit is added.',
        'Rewarded40078 is required for fresh acceptance. Havi92539 supplies40001; Vethir97979 is the ender and supplies40002 only after40001 reward.40002 A Familiar Fate keeps both monster objectives97982/97981 and does not complete on acceptance.40001 has no native QuestLineXQuest row; World relations establish this bridge between line135 and line138.',
        'Base Vethir DB291392 has no display override, no phase-ID restriction or spawn addon. Native display68213 resolves to model8673/file1280971; World model metadata exists. Installed template addon bytes1=3 preserves sleeping pose, no aura; npcflag3 remains gossip+questgiver and33536 does not contain NOT_SELECTABLE0x02000000. Native faction190 is non-hostile to human/orc templates1/2 and faction148 has no reputation bar. No Hotfix overlay for the audited references. Original TDB instead has questgiver-only npcflag2, faction35, no gossip menu and no template addon; it confirms display/questgiver/turn-in design, not the provenance or retail accuracy of the installed pose/menu/faction.',
        'The installed ground navmesh connects conservative exact5yd approaches to the native Havi and Vethir coordinates through118 polygons. This does not simulate enemy combat, client model loading, visual occlusion or player navigation. Native interaction additionally includes combat reach; the fixture models6.75+1.5yd sizes, but geometry passes without relying on this extension.'
    ],
    'automated_checks':[
        'Test-StormheimValorDoorData.ps1: read-only live World/Hotfix invariants for chain, zero objectives, exact base actor/model/pose/flags/menu and no quest-specific SmartAI/conditions. No SQL migration.',
        'Test-StormheimValorDoor.ps1: freshly extracts native CanTakeQuest dispatch, status/race/previous/completion/reward/next, AddQuestAndCheckCompletion, HandleQuestGiverRequestReward, CanInteractWithQuestGiver and GetNPCIfCanInteractWith. Both factions; prior merely complete versus rewarded; unaccepted/rewarded quest rejection; wrong giver; dead, out-of-world, flying, charmed, hostile, distant and unflagged actor rejection; abandon/reaccept, saved complete status; exact Havi->Vethir->40002 handoff.',
        'Test-StormheimValorDoorCatalog.ps1: real C++ DB2Storage readers, native faction methods and linked Release navmesh libraries. Test-StormheimValorDoorNative.py: original735 TDB narrative, no-objective design, Vethir ender/template and reproducible evidence.'
    ],
    'limits':[
        'The native CanTakeQuest dispatcher is extracted, but class/level/skill/reputation/timed/group/conditions component predicates are fixture defaults; actual DB restrictions are inspected separately. AddQuest storage, completion persistence, rewarded-set mutation, NPC AI callbacks, packet delivery, world lookup, hostility/reputation and distance are fixtures. Full RewardQuest inventory/currency/XP writes, client packets and database reconnect are not executed. Saved-complete is a reconstructed state test, not a login integration test.',
        'No production source or database mutation; no new Release server build/startup needed. The catalog tests compile standalone native readers using existing Release link inputs. No claim of deployed executable proof for the newly extracted handler tests.',
        'Pending client: Havi offer after40078; travel and Vethir visibility/pose/quest marker, actual click/reward, abandonment/relog, both factions,40002 offer. The full40002 prison/combat/release sequence is outside this audit.',
        'Earlier38347/39848/39857/39849/39850/39851/39853/39855/40078 client checks remain pending. Previous bone navmesh approach failures127694/127702/127704 remain unresolved; this successful40001 route does not clear them.'
    ],
    'next':'40002 A Familiar Fate: Dravax97982, jailer shard and Vethir prison release/credit97981, retry/re-entry and next40003. Code/data only.'
}
path=ROOT/'docs/audits/stormheim-valor-door-2026-10-16.json'
if '--write-evidence' in sys.argv:path.write_text(json.dumps(record,indent=2,ensure_ascii=False)+'\n',encoding='utf8')
else:assert record==json.loads(path.read_text(encoding='utf8'))
print('PASS: original40001 no-objective travel/turn-in design, native Vethir model/faction/pose references, live handoff and118-polygon ground route. No production repair indicated; actual client interaction remains pending.')
