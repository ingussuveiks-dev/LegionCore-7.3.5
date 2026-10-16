"""Read-only43349 evidence: preserve its existing portal credit/questgiver route."""
import hashlib,json,re,sys
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/research'))
from audit_spell_mechanics import Data
archive=ROOT/'.codex/TDB735/TDB_full_735.00_2018_02_19/TDB_world_735.00_2018_02_19.sql'
selected={'quest_template':{43349},'quest_objectives':{285226},'gameobject_template':{251992},'gameobject_queststarter':{251991},'gameobject_questender':{251992}}
cols={};table=None;original={}
patterns={t:re.compile(r"\((?:"+'|'.join(map(str,ids))+r"),(?:'(?:\\.|''|[^'\\])*'|[^'()])*\)") for t,ids in selected.items()}
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
    t=m[1]
    for mt in patterns[t].finditer(line):
        row=dict(zip(cols[t],re.findall(r"'(?:\\.|''|[^'\\])*'|[^,]+",mt[0][1:-1])))
        if t in ('gameobject_queststarter','gameobject_questender') and row['quest']!='43349':continue
        original.setdefault(t,[]).append(row)
q=original['quest_template'][0]
assert 'Chamber of the Guardian' in q['LogDescription'] and q['StartItem']=='0' and q['RewardNextQuest']=='0'
assert [(r['ObjectID'],r['Amount'],r['StorageIndex'],r['Flags']) for r in original['quest_objectives']]==[('110409','1','0','0')]
original['quest_template']=[{k:q[k] for k in ('ID','LogTitle','LogDescription','QuestDescription','StartItem','RewardNextQuest','Flags','FlagsEx')}]
go=original['gameobject_template'][0]
assert go['type']=='2' and go['displayId']=='35704' and go['Data1']=='20353'
original['gameobject_template']=[{k:go[k] for k in ('entry','name','type','displayId','size','Data0','Data1','Data2','AIName','ScriptName')}]
out=ROOT/'build-extractors/runtime-test/aegis-return';runtime=ROOT/'build-extractors/bin/Release'
read=lambda name:(out/name).read_text(encoding='utf-8-sig').splitlines()
catalog=read('native-lines.txt');assert 'native direct/group spell credit effects for110409 0' in catalog
floor=read('floors.txt');assert sum(s.endswith('PASS') for s in floor)==2
geometry=read('geometry.txt');assert len([s for s in geometry if s.startswith('PASS:')])==1 and 'through 17 native ground' in geometry[0]
data=Data(runtime/'dbc/enUS',ROOT/'.codex/stormheim-no-overlays')
assert 37794 in data.rows('SpellVisual')
inputs={name:hashlib.sha256((runtime/f'dbc/enUS/{name}.db2').read_bytes()).hexdigest() for name in ('GameObjectDisplayInfo','Phase','SpellEffect','SpellVisual')}
geometry_paths=[runtime/'mmaps/1220.mmap',runtime/'vmaps/1220.vmtree',runtime/'maps/1220_33_23.map',runtime/'vmaps/1220_23_33.vmtile']
geometry_paths += [runtime/f'mmaps/1220{x:02d}{y:02d}.mmtile' for x in range(32,35) for y in range(22,25)]
record={
 'quest':43349,'title':'The Aegis of Aggramar','status':'existing code/data route passes selected native checks; client verification pending; no production repair needed',
 'original_archive_sha256':hashlib.sha256(archive.read_bytes()).hexdigest(),'original':original,
 'world_live':read('world.tsv'),'hotfix':read('hotfix.tsv'),'native_catalog':catalog,'native_db2_sha256':inputs,
 'floor_checks':floor,'ground_route':geometry,
 'geometry_inputs_sha256':{p.relative_to(runtime).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in geometry_paths if p.exists()},
 'findings':[
  '43349 is the Dalaran safekeeping handoff after40072, not a second dungeon claim. Preserve its existing251991 starter, PrevQuestID40072, required monster credit110409 and outdoor251992 questgiver ender. There is no item objective or start item and no automatic next quest; installed addon has no direct PrevQuestID43349 successor.',
  'Credit110409 is named Credit - Portal Taken. Existing EventObject510/DB410008 at map1220,-785.162,4419.39,602.464, radius5, SmartEventObject, flags0 dispatches ONTRIGGER89. Existing LINK0->1->2->3 grants107587,110409,109750,106815 via action33/invoker7. No negative-GUID override or source13/source22 conditions block this route. Preserve other Pillar credits, shared native group rewards and the existing entry event; do not invent a pedestal click-credit script.',
  'Selected real EventObject::Update, ActivatedForPlayer, SmartEventObject::OnTrigger, SmartScript base-object selection, invoker target branch and action33 execute against the exported chain. Native player/group credit, objectives, completion and questgiver request-reward admission execute. Both factions, duplicate entry, distance/phase/death/no quest, abandon/retry, nearby/far group members and saved objective counter pass.',
  'An ordinary type2 Aegis Use prepares the quest menu and allows the existing43349 reward request. PrepareGossipMenu filters menu20353 option0/OptionNpc28 for GameObjects; this unrelated armament option does not suppress the independently prepared quest menu. Test executes the actual GO option-admission branch and native GO Use prologue/questgiver branch; no menu repair is indicated.',
  'Phase7502/216 grants4592 while43349 is not rewarded, including NONE/incomplete/complete.7502/217 grants4591 after reward. Dungeon phase7672/7 requires40072 not-NONE and43349 NONE. Native rewarded-condition and phase-update type handlers pass. RewardQuest schedules NotifyConditionChanged after the rewarded-set insertion without a quest flag gate. Original Flags37748744 versus installed33554440 lacks UPDATE_PHASE_SHIFT, but this server notification is unconditional; do not change that bit merely from the archive difference.',
  'Outdoor251992/DB25178660 stays at-854.191,4456.02,592.614, phase4592, display35704, scale0.75, existing SpellStateVisualID37794. Native model file1361813 boundsZ[-2.55803,2.59331], phases3554/4591/4592 exist. The regular26972 SpellVisual table reader finds37794; no visual assignment changed or playback verified.',
  'Native floor at chamber entry602.464 and pedestal-side588.957 pass; elevated object center is3.657yd above supporting floor, within conservative5yd center reach. The ground-only8192-node query connects entry to an exact5yd object approach through17 polygons,3 corners,81.703yd. Terrain area8445 differs from stored area8045; the connected native collision/ground surface is recorded without rewriting spawn area metadata.'
 ],
 'checks':['Test-AegisReturnData.ps1 exports and checks the installed full route and native-reference overlays. It performs no DB writes.','Test-AegisReturn.ps1 compiles selected production/native handlers plus exported live quest/event data, including prerequisite/reward and phase conditions; upstream40072/40005/40004 regressions pass.','Test-AegisReturnCatalog.ps1 compiles against original Release DB2/GridMap/VMap/MMap/Detour libraries. Native catalog, both supporting floors and complete ground route pass.','No implementation or migration changed, so no new Release build/deployment or startup is claimed. Canonical executable is retained from8a5729b2; audit records its hash.'],
 'release':{'worldserver_sha256':hashlib.sha256((runtime/'worldserver.exe').read_bytes()).hexdigest().upper(),'production_change':False,'migration':None},
 'limits':[
  'SmartAI event selection/LINK traversal is a fixture scheduler constrained to the four exported rows; it does not execute the entire loader/dispatcher. World/player enumeration, phase assignment, visibility/LOS, PlayerCondition, packet/menu preparation, unrelated CanTakeQuest predicates and persistence writes remain fixtures. Reward request acceptance is checked, not a complete reward opcode/SQL round trip.',
  'Native group sharing can credit a nearby eligible party member without that member entering the5yd event. Preserve this existing action33 behavior. EventObject flags0 allows exit/re-entry retries. If acceptance or abandon/reaccept occurs while already inside an activated event, leave its radius and re-enter; the generic EventObject does not automatically reset affectedPlayers on quest changes. A dead entrant can likewise need exit/re-entry. No new automatic stationary recovery is claimed.',
  'A standalone native SpellVisual DB2Storage load in the initial diagnostic reader faulted before catalog output. It was removed from this scoped helper; the established regular-table reader confirms37794 instead. No live server/client visual fault was reproduced and no core DB2 change was made.',
  'Client pending: actual Halls pedestal offer/accept, route/portal to the Dalaran chamber, entry credit and group behavior, outdoor phase4592 Aegis visibility/selection, reward and4591 refresh, both factions and abandon/relog. The existing state visual and reward package presentation remain unverified. No whole quest, visual, persistence or client sequence is certified.',
  'Earlier client requirements remain.40003 starting route was resolved as audit-node exhaustion; bone approaches127694/127702/127704 remain recorded as unresolved and are not cleared by this Dalaran route.'
 ],
 'next':'The direct40072->43349 chain ends here. Next code/data audit: revisit the three retained38324 bone approach failures127694/127702/127704, beginning with exact source geometry and Detour diagnostics; do not move spawns without evidence.'
}
path=ROOT/'docs/audits/aegis-return-2026-10-16.json'
if '--write-evidence' in sys.argv:path.write_text(json.dumps(record,indent=2,ensure_ascii=False)+'\n',encoding='utf8')
else:assert record==json.loads(path.read_text(encoding='utf8'))
print('PASS:43349 original/live portal-credit, native interaction/phase, collision and terminal handoff evidence; client pending.')
