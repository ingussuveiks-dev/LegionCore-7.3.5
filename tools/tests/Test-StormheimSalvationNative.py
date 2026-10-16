"""Native40005 evidence and deployed chain/travel repair; no client certification."""
import re,json,hashlib,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
selected={'quest_template':{40005,40072},'quest_objectives':{280182,282130},'creature_template':{92307,97986,97988,98190},'creature_queststarter':{97986},'creature_questender':{97988}}
cols={};table=None;data={}
patterns={t:re.compile(r"\((?:"+'|'.join(map(str,ids))+r"),(?:'(?:\\.|''|[^'\\])*'|[^'()])*\)") for t,ids in selected.items()}
archive=ROOT/'.codex/TDB735/TDB_full_735.00_2018_02_19/TDB_world_735.00_2018_02_19.sql'
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
        r=dict(zip(cols[t],re.findall(r"'(?:\\.|''|[^'\\])*'|[^,]+",mt[0][1:-1])))
        if t in ('creature_queststarter','creature_questender') and int(r['quest'])!=40005:continue
        data.setdefault(t,[]).append(r)

assert len(data['quest_objectives'])==2
assert {(r['ObjectID'],r['Flags'],r['Amount'],r['StorageIndex']) for r in data['quest_objectives']}=={('92307','1','1','3'),('103766','28','1','0')}
q=next(r for r in data['quest_template'] if r['ID']=='40005')
assert 'Defeat God-King Skovald' in q['LogDescription'] and 'portals we destroyed' in q['QuestDescription'] and q['RewardNextQuest']=='40072'
assert any(r['id']=='97988' for r in data['creature_questender'])
data['quest_template']=[{k:r[k] for k in ('ID','LogTitle','LogDescription','QuestDescription','RewardNextQuest','Flags','FlagsEx')} for r in data['quest_template']]
data['creature_template']=[{k:r[k] for k in ('entry','name','modelid1','faction','npcflag','unit_flags','VehicleId','AIName','ScriptName')} for r in data['creature_template']]
out=ROOT/'build-extractors/runtime-test/stormheim-salvation'
world=(out/'world-live.tsv').read_text(encoding='utf-8-sig').splitlines()
actors={int(f[2]):f for line in world if (f:=line.split('\t'))[0]=='actor'}
assert set(actors)=={92307,97986,97988,98190}
for entry,guid,display in ((92307,291417,65873),(97986,338656,68213),(98190,341218,68213),(97988,338669,76630)):
    f=actors[entry];assert f[1]==str(guid) and f[6:11]==['1220','7541','1','','0'] and f[17]==str(display)
    if entry in (97986,98190):assert f[11]=='190' and f[15]=='npc_stormheim_salvation_vethir' and f[16]=='4367'
    if entry==92307:assert f[11:16]==['2780','0','0','','']
catalog=(out/'native-lines.txt').read_text(encoding='utf-8-sig').splitlines()
assert 'quest 40005 line 138 order 3' in catalog and 'quest 40072 line 138 order 4' in catalog
assert ' effect=28 aura=0 misc=92302,1162 trigger=0 target=46,0 base=0' in catalog
assert ' effect=28 aura=0 misc=105489,1162 trigger=0 target=46,0 base=0' in catalog
assert ' effect=6 aura=12 misc=0,0 trigger=0 target=22,15 base=0' in catalog
floors=(out/'floors.txt').read_text(encoding='utf-8-sig').splitlines()
assert sum(s.endswith('PASS') for s in floors)==5 and '0 failures' in floors[-1]
geometry=(out/'geometry.txt').read_text(encoding='utf-8-sig').splitlines()
assert [int(re.search(r'through (\d+) native',s)[1]) for s in geometry if s.startswith('PASS:')]==[5,3]
assert not any(s.startswith(('FAIL:','INCOMPLETE:')) for s in geometry)
runtime=ROOT/'build-extractors/bin/Release'
meshfiles=[runtime/'mmaps/1220.mmap',runtime/'maps/1220_27_30.map',runtime/'vmaps/1220.vmtree',runtime/'vmaps/1220_27_30.vmtile']
meshfiles += [runtime/f'mmaps/1220{x:02d}{y:02d}.mmtile' for x in range(26,29) for y in range(29,32)]
meshhashes={p.relative_to(runtime).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in meshfiles if p.exists()}
smoke=sorted(out.glob('startup-*'))[-1]
assert (smoke/'DBErrors.log').stat().st_size==0 and (smoke/'stderr.log').stat().st_size==0
assert re.search(r'World initialized|World server is ready|Worldserver is ready',(smoke/'Server.log').read_text(encoding='utf8',errors='replace'))
migration=ROOT/'sql/updates/world/2026_10_16_14_stormheim_salvation.sql'
record={
    'quest':40005,'title':"Stormheim's Salvation",'status':'scoped chain/travel repair and native code/data checks passed; client verification pending',
    'original_archive_sha256':hashlib.sha256(archive.read_bytes()).hexdigest(),'original':data,
    'observed_before_repair':{'40004':{'PrevQuestID':0,'ExclusiveGroup':0},'40005':{'PrevQuestID':0,'Skovald_objective_flags':0},'40003':{'ExclusiveGroup':0},'travel':'97986 menu18843/0 and98190 menu18852/0 used unguarded SmartAI action62 teleports. Menu preparation conditions checked incomplete/complete40005, while the SmartAI callback preceded Player::OnGossipSelect and did not recheck those conditions.'},
    'world_live':world,'hotfix':(out/'hotfix.tsv').read_text(encoding='utf-8-sig').splitlines(),
    'native_catalog':catalog,'native_input_hashes':json.loads((out/'native-inputs.json').read_text(encoding='utf-8-sig')),'floor_checks':floors,'ground_routes':geometry,'selected_geometry_inputs_sha256':meshhashes,
    'findings':[
        'Both portal40003 and lieutenant40004 quests now require rewarded40002, keeping them parallel. Their negative exclusive group-40003 contains exactly those two tasks;40005 PrevQuestID40003 invokes native each-from-all admission, requiring both rewarded. Merely accepted or complete-but-unrewarded prerequisites do not suffice. Already accepted40005 is preserved: travel validates current quest status without imposing the new acceptance gates on legacy characters. No character counters or rewarded state are reset.',
        'This prerequisite repair is an implementation choice supported by the current common Vethir turn-in, original40005 narrative referring to portals already destroyed, and native line138 ordering40002/3/4/5/72 as0/1/2/3/4. QuestLineXQuest is display ordering, not admission logic. The selected original TDB evidence does not recover retail quest-addon prerequisite rows; the new negative-group metadata is not presented as recovered retail data. The prior40004 audit retains its original early-offer finding; this follow-up resolves that admission gap.',
        'Original objective280182 requires one92307 at storage3 with Flags1; restore TRACKED_ON_MINIMAP only. Hidden optional103766 at storage0 retains Flags28. Neither dialog selection nor teleport attempt proves a Skovald defeat; no travel credit is invented. Native completion succeeds after the real92307 kill with optional counter0, while optional-only progress cannot complete the quest.',
        'Use exact base Vethir actors:97986 DB338656 at2490.15,948.437,241.446 with menu18843/0 for incomplete40005, and98190 DB341218 at2488.31,945.134,596.224 with menu18852/0 for complete40005. The production SmartAI subclass revalidates map1220/zone7541/instance0, quest definition/status, live source/player, base coordinates, phase/visibility/LOS, actual native NPC interaction/flags/range, and the current menu/item. Dead, combat, taxi, vehicle, transport or teleporting players cannot use it. Invalid exact base actors never fall through to the old action62; unrelated spawns continue to SmartAI.',
        'Keep the existing direct platform teleport to2488.31,945.13,596.224,3.88255 and return to2492.08,957.37,241.44,4.29. This is not a reconstructed flight. Public Vethir models/poses, vehicle4367/seat15925 and SmartAI rows/menu conditions remain unchanged. Failed TeleportTo can be retried; no credit or script-owned movement/camera/pet state is created. The unchanged menu still offers return only with complete40005; leaving an incomplete/abandoned run uses existing game escape options, not a newly invented return condition.',
        'Native Ride Vethir182921 and209076 are SUMMON28 for92302 and105489 with properties1162, not native rides for this97986/98190 pair. Adjacent193336/193341/193342 trigger the portal-call spells.193975/201938 create conversations550/1159, but their names do not prove the outdoor92307 choreography. None is attached as a guessed cinematic or vehicle sequence. Actual flight/dialogue/visual timing is not verified or newly implemented.',
        'Skovald92307 DB291417 exists on the platform at2449.32,907.818,596.224, no model override, hostile native faction2780, template unit_flags0, active scaling98..110. Display65873 resolves to model8518/file1255702; Vethir68213 to8673/1280971, Havi76630 to8323/1132140, all with World model metadata. Generic creature combat AI remains; installed214298 Demonic Bindings is a native area stun (effect6/aura12, targets22/15). The loader reads Difficulties, not the obsolete difficultyMask16 column; its empty list admits outdoor0. Combat scheduling/damage/voice/retreat are not simulated.',
        'The actual native Unit kill-owner prefix attributes a pet blow to its owner. KillRewarder50ms callback resolves92307 and invokes Player::KilledMonster/KilledMonsterCredit; the required counter completes once. Return Vethir then accepts the complete quest; Havi97988 DB338669 is the involved ender. Native reward-request admission and GetNextQuest resolve40072 at Havi, and40072 remains unavailable until40005 is rewarded. Full RewardQuest currency/item/XP persistence is outside the fixture.',
        'Native terrain/VMap floors pass for both Vethirs, Skovald, lower return and Havi; top platform floor596.141 and lower floor241.445. Complete ground corridors connect arrival->Skovald through5 polygons and lower return->Havi through3. This validates installed static geometry/navmesh anchors, not cold-client model loading, visibility, renderer timing or real combat pathfinding.'
    ],
    'checks':[
        'Test-StormheimSalvationData.ps1 repeats the actual migration in connection-local MySQL table copies, compares full changed/protected rows (including unchanged SmartAI and conditions), preserves custom script/prerequisite/group/flag bits and validates exact group membership. -Live verifies deployed state. Checked display/model/faction/quest-line/spell/vehicle/seat Hotfix overlays are absent.',
        'Test-StormheimSalvation.ps1 -VerifyReleaseBinary compiles the full production travel class and freshly extracted prerequisite/completion/status/race/reward/NPC admission, kill-owner prefix, real50ms kill callback, template/monster credit and spell difficulty handlers. Both factions: all16 combinations of two prerequisite states;25 travel branches per direction; stale, duplicate and failed teleport attempts; other-spawn delegation; definition mismatch; player/pet kills; optional credit independence; complete ascent->kill->return->Havi reward offer and40072 gate. Native request/reward/previous dispatch and production guard bodies execute; clock/world/movement/storage/network are fixtures.',
        'Updated40004 regression follows its new prerequisite/group and retains all24 kill orders/player-pet cases.40003 production regression and canonical registration/actual game/scripts link-input checks pass after the shared Vethir binding and metadata changes. Original audits remain historical evidence; this audit records the follow-up.',
        'Test-StormheimSalvationCatalog.ps1 compiles native DB2Storage and linked Release terrain/VMap/MMap readers. Models/factions/spells/vehicle/quest line, five floor anchors and two ground routes pass. Test-StormheimSalvationNative.py checks original native narrative/objectives/ender and reproducible evidence.',
        'Release worldserver rebuilt directly into canonical build-extractors/bin/Release with the original final-link scripts/game libraries, no OutDir override. The unique Salvation registration exists in that executable and both final-link inputs predate it. Migration14 deployed transactionally after scoped private backup and updates SHA1 registration. Fresh canonical startup reached ready, exited0 and produced empty DBErrors/stderr; runtime configuration/data/DLLs/certificates were preserved.'
    ],
    'release':{'migration':migration.name,'migration_sha1':hashlib.sha1(migration.read_bytes()).hexdigest().upper(),'worldserver_sha256':hashlib.sha256((runtime/'worldserver.exe').read_bytes()).hexdigest().upper(),'smoke_directory':smoke.relative_to(ROOT).as_posix(),'DBErrors_bytes':0,'stderr_bytes':0},
    'limits':[
        'The full C++ production guard is compiled, but its base SmartAI callback service and TeleportTo are fixture implementations, not a client/network integration run. Native NPC interaction uses fixture hostility, phase and object-distance services including creature reach. Actual AI selection, fight scheduling, final damage, group XP admission, teleport ACKs, persistence and movement/camera/pet lifecycle are not executed. Native CanTakeQuest dispatch runs with unrelated component predicates at fixture defaults; selected source19 conditions are absent.',
        'Client pending: both parallel offers after40002 reward,40005 only after both turn-ins; lower and upper menus and duplicate/stale selections; platform arrival/collision, movement release, camera, permanent pet/portrait; actual Skovald combat/spell/voice behavior and direct/pet/group kills; minimap tracking; return selection, Havi reward and40072 offer; abandon/retry/relog and both factions. A successful build, startup or native geometry test does not establish whole-quest/client verification.',
        'Earlier quest/client limitations remain.40003 starting navmesh issue was resolved as audit node exhaustion in6ca7ba29; earlier bone navmesh approach failures127694/127702/127704 remain unresolved. No new flight/cinematic choreography or character-state repair is claimed.'
    ],
    'next':'40072 Halls of Valor: Securing the Aegis: Havi110425 interaction and dungeon95676 objective, loot/claim/completion and chain end. Code/data only.'
}
path=ROOT/'docs/audits/stormheim-salvation-2026-10-16.json'
if '--write-evidence' in sys.argv:path.write_text(json.dumps(record,indent=2,ensure_ascii=False)+'\n',encoding='utf8')
else:assert record==json.loads(path.read_text(encoding='utf8'))
print('PASS: native40005 objectives/chain/two-way travel/model/floor/kill/handoff evidence; client verification remains pending.')
