"""Native40003 evidence; keep the failed approach and missing client checks."""
import hashlib,json,re,sys
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/research'))
from audit_spell_mechanics import Data
from audit_spell_visuals import Table
d=Data(ROOT/'build-extractors/bin/Release/dbc/enUS',ROOT/'.codex/stormheim-no-overlays')
ids={193327,193328,193329,193330,196084}
native={t:[r for r in d.rows(t).values() if r.get('SpellID') in ids] for t in('SpellEffect','SpellMisc','SpellXSpellVisual','SpellCooldowns','SpellCastingRequirements')}
assert next(r for r in native['SpellCastingRequirements'] if r['SpellID']==193327)['RequiresSpellFocus']==1887
assert any(r['SpellID']==193327 and r['Effect']==3 and r['ImplicitTarget1']==38 for r in native['SpellEffect'])
assert any(r['SpellID']==193327 and r['RecoveryTime']==10000 for r in native['SpellCooldowns'])
assert {(r['SpellID'],r['EffectMiscValue1']) for r in native['SpellEffect'] if r['Effect']==61}=={(193328,46837),(193329,46839),(193330,46840)}
assert next(r for r in native['SpellXSpellVisual'] if r['SpellID']==196084)['SpellVisualID']==50884
def raw(name,fields):
    meta=re.split('struct '+name+'Meta\n',d.meta,flags=re.IGNORECASE)[1].split('\n};')[0]
    index,layout=re.search(r'DB2Meta instance\((-?\d+), \d+, (0x[\dA-Fa-f]+)',meta).groups()
    t=Table(d.dbc/(name+'.db2'),int(index),int(layout,16));d.hashes[name]=t.sha256
    return [dict(ID=t.ids[i],parent=t.parents.get(i,0),**{k:t.value(i,f) for k,f in fields.items()}) for i in range(t.count)]
events=[r for r in raw('SpellVisualEvent',{'start':0,'end':3,'target':6,'kit':7}) if r['parent']==50884]
kits={r['kit'] for r in events};effects=[r for r in raw('SpellVisualKitEffect',{'type':0,'effect':1}) if r['parent'] in kits]
a={r['ID']:r for r in raw('SpellVisualKitModelAttach',{'effect_name':3,'attachment':4,'start_anim':15,'anim':16,'end_anim':17,'anim_kit':18})}
attachments=[a[r['effect']] for r in effects if r['type']==2]
names={r['ID']:r for r in raw('SpellVisualEffectName',{'model_file':10,'texture_file':8})}
assert kits=={60744} and {names[r['effect_name']]['model_file'] for r in attachments}=={1110065,1040934}
native.update(impact_events=events,impact_kit_effects=effects,impact_attachments=attachments,impact_models=[names[r['effect_name']] for r in attachments])
archive=ROOT/'.codex/TDB735/TDB_full_735.00_2018_02_19/TDB_world_735.00_2018_02_19.sql'
selected={'quest_template','quest_objectives','creature_template','gameobject_template'};cols={};original={};table=None
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
        key=int(next((r[k] for k in('QuestID','ID','entry') if k in r),'0'))
        if (n=='quest_template' and key in(40003,40004)) or (n=='quest_objectives' and key==40003) or (n=='creature_template' and key in(98039,98040,98041,97986)) or (n=='gameobject_template' and key==244681):original.setdefault(n,[]).append(r)
q=next(r for r in original['quest_template'] if r['ID']=='40003')
assert q['StartItem']=='129161' and 'three Felbound Portals' in q['LogDescription']
original['quest_template']=[{k:r[k] for k in('ID','LogTitle','LogDescription','QuestDescription','StartItem','ItemDrop1','ItemDropQuantity1','RewardNextQuest')} for r in original['quest_template']]
original['creature_template']=[{k:r[k] for k in('entry','name','modelid1','npcflag','VehicleId','AIName','ScriptName')} for r in original['creature_template']]
original['gameobject_template']=[{k:r[k] for k in('entry','type','displayId','name','size','Data4','ScriptName')} for r in original['gameobject_template']]
out=ROOT/'build-extractors/runtime-test/stormheim-stem-tide'
catalog=(out/'native-lines.txt').read_text(encoding='utf-8-sig').splitlines()
assert 'quest40003 line=138 order=1' in catalog and 'horn cast0ms range20 native nearby-entry/conditions; explicit mask0' in catalog
geometry=(out/'geometry.txt').read_text(encoding='utf-8-sig').splitlines()
assert sum(s.startswith('PASS:') for s in geometry)==3 and sum(s.startswith('FAIL:') for s in geometry)==1
migration=ROOT/'sql/updates/world/2026_10_16_07_stormheim_stem_tide.sql'
smoke=sorted(out.glob('startup-*'))[-1]
assert (smoke/'DBErrors.log').stat().st_size==0 and (smoke/'stderr.log').stat().st_size==0
assert re.search(r'World initialized|World server is ready|Worldserver is ready',(smoke/'Server.log').read_text(encoding='utf8',errors='replace'))
record={
 'quest':40003,'title':'Stem the Tide','status':'scoped code/data repair passed; one ground-navmesh approach unresolved; client verification pending',
 'original_archive_sha256':hashlib.sha256(archive.read_bytes()).hexdigest(),'original':original,
 'world_live':(out/'world-live.tsv').read_text(encoding='utf-8-sig').splitlines(),'hotfix':(out/'hotfix.tsv').read_text(encoding='utf-8-sig').splitlines(),
 'native':native,'native_hashes':d.hashes,'native_cpp_catalog':catalog,'catalog_hashes':json.loads((out/'native-inputs.json').read_text(encoding='utf-8-sig')),'geometry':geometry,
 'findings':[
  'Original40003 asks for three Felbound Portals using Stormforged Horn129161. Native use193327 is an instant nearby-entry dummy with20yd range and10s recovery. It also requires spell focus1887; the actual portal type8 has Data0=1887/Data1=10, and native GameObjectFocusCheck checks that type/ID/radius plus object sizes before casting. The20yd script bound does not bypass this focus requirement. Installed source13 conditions retain all three97983/97984/97985 alternatives. Native WorldObjectSpellTargetCheck evaluates these conditions before its fallback rejection; no global immunity/selection flag fix is introduced. The actual implicit search precedes the target-selection hook; no claim is made that a hook can repair an absent native search result.',
  'Original World markers291595/291537/291523 responded through shared SmartAI timed lists: wait5000ms, optional196084, then action33. Those lists did not hide any GO, isolate each player acceptance, or drive a Vethir flyby; action33 distributed progress to the group. Repeated callers shared the same marker action list. Native193328/193329/193330 are SEND_EVENT61 to46837/46839/46840, not scene playback; installed event_scripts has none and no matching source handler was found. Adjacent193331/193332 actually summon Dreadstalker98035 and must not be used as guessed Vethir spells.',
  'The native horn item/use/conditions and public marker flags remain. New spell handlers require the actual owned cast item, held horn, exact base marker/GO, phase/visibility/LOS and active quest. The first captured marker and acceptance lifetime cannot be replaced by a later CheckCast/selection call. Each player gets a separate cast ticket; only that ticket can write one portal counter after the installed5s delay, then derive optional aggregate98186 from the three distinct counters. Direct/group/talk credit is blocked only for these40003 objective entries. Other quest999 keeps normal group credit in the test.',
  'Optional authored flybys use native Vethir templates98039/98040/98041, display68213 -> model8673/file1280971, with personal wild summons, no vehicle or guardian. Approach starts45yd west/20yd south/35yd above the marker, goes to25yd above at12yd/s; after response it departs65yd east/20yd north/45yd above at20yd/s. These offsets/timing are authored, not recovered retail choreography. Failed flyby/impact summons and failed optional casts do not gate progression. Pending runs cancel on death, quest replacement, missing horn/marker/GO, invalid phase/LOS/context or leaving the80yd response radius; cleanup/lifetime guards prevent late credit. Summons have native10s/3s lifetimes; an old-map actor unreachable through current-map lookup is bounded by that lifetime.',
  'Portal Explosion196084 has native visual50884 -> kit60744 -> attachments312598/312599 (world attachment127), effect names24093/22925 and files1110065/1040934, plus sound effect22802. It is cast by an owner-only helper at the marker, independently of progression. This is actual native asset evidence, not proof of explosion playback or a breath beam connecting the flyby to the portal.',
  'Each owner hides only their exact public GO115097/115080/115070. Public portals, other owners, model1114444/scale0.75, factions, spawn positions and original lists remain. A500ms nearby-visibility pass restores on abandonment/logout/transfer and re-hides from saved complete/rewarded state; no new kill/spawn counter is awarded on login. Old counters are retained rather than erased. Vethir97986 DB338656 remains the ender; RewardNextQuest0 remains0, so there is no invented automatic next-quest window. Native horn source-item reward/abandon cleanup is preserved.'
 ],
 'checks':[
  'Test-StormheimStemTide.ps1 -VerifyReleaseBinary compiles the full production module, fresh native quest lifetime/completion/reward/group/kill/talk handlers, native spell134 group delivery, GameObjectFocusCheck and the actual condition-admission prefix. Tests cover three portals in a different order,4999/5000ms boundary, derived count1/2/3, two simultaneous owners, repeated use, stale acceptance/check/callback, decoy private helpers, optional summon/cast failure, missing source/item, phase/LOS/range/life/transfer/taxi/vehicle changes, owner hide cleanup and saved/rewarded visibility. Focus predicate rejects wrong type/ID and distance. Full spell engine, cast scheduler clock, inventory, grid/lookup, conditions, motion spline and networking are fixture services; focus distance/object-size service uses a conservative fixture approximation.',
  'Test-StormheimStemTideData.ps1 uses connection-local MySQL copies to repeat the real migration, compare full modified/protected rows and preserve custom bindings. -Live checks deployed item/quest/objective/marker/GO/condition/list/Vethir invariants. Native sparse ItemSparse and regular item/effect/spell/model/QuestLine tables are read by C++ DB2Storage. Visual references are decoded from installed26972 regular WDC1 tables; tested Hotfix overlays are absent.',
  'Release worldserver built into canonical build-extractors/bin/Release with the original final-link game/scripts libraries and no OutDir override. Binary registration/guard markers and link-input timestamps pass. Fresh canonical startup reached ready and exited0 with empty DBErrors/stderr.40002 Familiar Fate and40078 Heavy Burden production/native regressions pass after the shared Player guard addition.'
 ],
 'release':{'migration':migration.name,'migration_sha1':hashlib.sha1(migration.read_bytes()).hexdigest().upper(),'worldserver_sha256':hashlib.sha256((ROOT/'build-extractors/bin/Release/worldserver.exe').read_bytes()).hexdigest().upper(),'smoke_directory':smoke.relative_to(ROOT).as_posix(),'DBErrors_bytes':0,'stderr_bytes':0},
 'limits':[
  'Installed ground navmesh could not connect the exact5yd approaches from starting Vethir2875.26,1208.98,188.763 to the north marker2713.4,970.118,205.88 (one approach each). Keep this unresolved result. North->east23 polygons, east->south29 and south->turn-in37 pass. This diagnostic does not establish that client movement/collision cannot traverse the first route, nor verify the authored flight clearance. No navmesh regeneration or speculative actor relocation was done.',
  'Client pending: accepting from Vethir and receiving/using the horn, actual native implicit-target search/cast, both factions and two players, all three approaches, flyby arrival/departure/height/animation, sound/impact, portal disappearance only for its owner, abandon/retry/relog and actual turn-in at97986. Motion tests record requested spline endpoints, not execution of the native motion/renderer. No whole quest or visual client verification is claimed.',
  'Previous38347/39848/39857/39849/39850/39851/39853/39855/40078/40001/40002 client checks remain pending. Earlier bone navmesh failures127694/127702/127704 remain unresolved.40003 RewardNextQuest0 and the later40005 prerequisite/menu flow are recorded, not repaired in this scope.'
 ],
 'next':next(r['ID']+' '+r['LogTitle'].strip("'") for r in original['quest_template'] if r['ID']=='40004')+'; parallel quest at starting Vethir, then audit40005 prerequisite/ascension. Code/data only.'
}
path=ROOT/'docs/audits/stormheim-stem-tide-2026-10-16.json'
if '--write-evidence' in sys.argv:path.write_text(json.dumps(record,indent=2,ensure_ascii=False)+'\n',encoding='utf8')
else:assert record==json.loads(path.read_text(encoding='utf8'))
print('PASS: native40003 item/target/visual/model and live/deployed repair evidence; preserves the failed first navmesh approach and pending client checks.')
