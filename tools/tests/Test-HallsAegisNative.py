"""Native40072 evidence and deployed Aegis claim repair; no client certification."""
import re,json,hashlib,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
selected={'quest_template':{40072,43349},'quest_objectives':{280268,285235},'creature_template':{95675,95676,97988},'gameobject_template':{251991},'gameobject_queststarter':{251991},'gameobject_questender':{251991}}
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
        if t in ('gameobject_queststarter','gameobject_questender') and int(r['quest']) not in(40072,43349):continue
        data.setdefault(t,[]).append(r)


assert {(r['ObjectID'],r['Flags'],r['Amount'],r['StorageIndex']) for r in data['quest_objectives']}=={('110425','0','1','0'),('95676','3','1','1')}
assert any(r['quest']=='40072' for r in data['gameobject_questender']) and any(r['quest']=='43349' for r in data['gameobject_queststarter'])
q=next(r for r in data['quest_template'] if r['ID']=='40072');assert q['RewardNextQuest']=='0' and 'claim the Aegis' in q['LogDescription']
go=data['gameobject_template'][0];assert go['type']=='2' and go['displayId']=='35704' and go['Data1']=='20352' and go['ScriptName']=="''"
data['quest_template']=[{k:r[k] for k in ('ID','LogTitle','LogDescription','QuestDescription','RewardNextQuest','StartItem','QuestPackageID','Flags','FlagsEx')} for r in data['quest_template']]
data['gameobject_template']=[{k:go[k] for k in ('entry','name','type','displayId','size','Data0','Data1','Data2','AIName','ScriptName')}]
data['creature_template']=[{k:r[k] for k in ('entry','name','modelid1','faction','npcflag','unit_flags','AIName','ScriptName')} for r in data['creature_template']]
out=ROOT/'build-extractors/runtime-test/halls-aegis';runtime=ROOT/'build-extractors/bin/Release'
world=(out/'world-live.tsv').read_text(encoding='utf-8-sig').splitlines()
for expected in ('phase-condition\t14\t0\t40072\t1','phase-condition\t14\t0\t43349\t0','scaling\t95675\t98\t110','scaling\t95676\t98\t110'):
    assert expected.encode().decode('unicode_escape') in world
catalog=(out/'native-lines.txt').read_text(encoding='utf-8-sig').splitlines()
assert 'quest 40072 line 138 order 4' in catalog and 'native direct/group spell credit effects for110425 0' in catalog
assert {int(s.split()[-1]) for s in catalog if s.startswith('map1477 difficulty ')}=={1,2,8,23}
assert 'Aegis display35704 file 1361813 minZ -2.55803 maxZ 2.59331' in catalog
floors=(out/'floors.txt').read_text(encoding='utf-8-sig').splitlines();assert sum(s.endswith('PASS') for s in floors)==2
geometry=(out/'geometry.txt').read_text(encoding='utf-8-sig').splitlines();assert sum(s.startswith('PASS:') for s in geometry)==1 and 'through 4 native ground' in geometry[1]
assert not any(s.startswith(('FAIL:','INCOMPLETE:')) for s in geometry)
smoke=sorted(out.glob('startup-*'))[-1]
assert (smoke/'DBErrors.log').stat().st_size==0 and (smoke/'stderr.log').stat().st_size==0
assert re.search(r'World initialized|World server is ready|Worldserver is ready',(smoke/'Server.log').read_text(encoding='utf8',errors='replace'))
files=[runtime/'mmaps/1477.mmap',runtime/'vmaps/1477.vmtree']
files += [runtime/f'maps/1477_27_{y}.map' for y in (30,31)] + [runtime/f'vmaps/1477_{y}_27.vmtile' for y in (30,31)]
files += [runtime/f'mmaps/1477{x:02d}{y:02d}.mmtile' for x in range(26,29) for y in range(29,33)]
geometry_hashes={p.relative_to(runtime).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in files if p.exists()}
migration=ROOT/'sql/updates/world/2026_10_16_15_halls_of_valor_aegis.sql'
record={
 'quest':40072,'title':'Halls of Valor: Securing the Aegis','status':'scoped progression/data repair and native checks passed; client verification pending',
 'original_archive_sha256':hashlib.sha256(archive.read_bytes()).hexdigest(),'original':data,
 'observed_before_repair':['110425 is Credit - Aegis of Aggramar, not an actual Havi talk target. The required first counter had no interaction writer. Gameobject251991 was already a native questgiver/40072 ender/43349 starter, but had no script; the original native Use branch only prepared its quest menu. The regression reproduces an incomplete quest after an earned95676 counter and a baseline pedestal click.','Installed285235 had Flags2, while original735 has Flags3 (tracked plus sequenced). Its entry95676 is Odyn; it must not be replaced with Skovald95675 merely because the quest summary names Skovald.','A generic standing-object2yd floor diagnostic initially rejected the Aegis center754.515 over floor750.567. This was an inappropriate model-center predicate, not a confirmed spawn error. The corrected conservative native5yd interaction approach and complete navmesh route pass without moving the object.'],
 'world_live':world,'hotfix':(out/'hotfix.tsv').read_text(encoding='utf-8-sig').splitlines(),'native_catalog':catalog,'native_input_hashes':json.loads((out/'native-inputs.json').read_text(encoding='utf-8-sig')),
 'floor_checks':floors,'ground_route':geometry,'selected_geometry_inputs_sha256':geometry_hashes,
 'findings':[
  'Keep both original mandatory monster objectives:280268/110425 amount1 storage0 Flags0 and285235/95676 amount1 storage1 Flags3. Migration15 restores only the missing tracked bit and binds the existing questgiver pedestal. No physical item, chest loot link, new spawn or guessed visual/scene is added: native Aegis251991 is type2, not a loot chest, and the quest requires credit rather than an item objective.',
  'The actual public pedestal DB25178659 remains at2375.19,559.552,754.515 in map1477/zone7672, phase3554, display35704/scale1, spawnMask8388870. Native dungeon difficulties1/2/8/23 are normal/heroic/keystone/mythic. Phase7672/7 remains conditional on40072 not-NONE and43349 NONE; native Phase3554 exists. Template relations and all phase/source rows are preserved.',
  'Production GameObjectAI::GossipUse validates exact entry/spawn/map/instance/coordinates, active exact quest definition, alive/world/source spawn state, selectable flags, same live map and phase3554, visibility/LOS, native questgiver interaction admission and PlayerCondition. Require both instance Skovald and Odyn states DONE plus this acceptance already holding the earned95676 counter. A saved DONE instance alone cannot mint a new boss credit for a late joiner or a freshly reaccepted quest.',
  'Only an actual successful pedestal use writes110425 directly for that player, sends one objective update and runs native completion. Repeated use, another party member, no quest, wrong/later Aegis251992, another spawn/instance/map, absent phase, death, combat/taxi/vehicle/transport/teleport, distance/LOS/condition or protected object flags cannot grant the claim. Public object and other players remain usable. Native SpellEffect contains no direct/group credit90/134 effect for110425; adding a random Aegis combat spell is not the missing interaction.',
  'Return false from GossipUse to continue the actual GameObject::Use questgiver branch. The completed40072 can be turned in at the native Aegis ender. After reward, this same object offers43349 under its existing PrevQuestID40072.40072 RewardNextQuest0 stays0, so there is no invented automatic next window. The installed additional Havi97988 ender is retained; original TDB confirms the pedestal ender/43349 starter, not a new Havi talk-credit step.',
  'Odyn95676 survives his trial. Separate commit e8344013 repairs his threshold-crossing hit with the native post-scaling hook and target-relative health, including an initial spell before CombatStart. It preserves the ordinary encounter transition and map-wide95676 delivery. Skovald95675 retains his normal BossAI death transition. This claim fix does not replace either encounter with proximity/spawn/loot credit.',
  'Model evidence: native display35704 references file1361813, bounding Z[-2.55803,2.59331]. Skovald65873 resolves to8518/file1255702, Odyn67230 to8695/file1284603, Havi76630 to8323/file1132140. Both bosses have active98..110 scaling. Checked model/display/map/difficulty/phase/quest-line Hotfix overlays are absent.',
  'Native terrain/VMap arena floor749 and pedestal-side floor750.567 pass. The object center is3.948yd above that floor; conservative exact5yd center approaches connect from Odyn arena2399.3,528.85,749 to2375.19,559.552,750.658 through4 ground polygons, two corners and39.0666yd. This does not depend on extra model bounding-box reach and does not prove client visual placement. The shared audit helper now accepts an optional map ID while retaining1220 by default;40003 regression still reproduces its1024-node exhaustion and passes all four8192-node routes.'
 ],
 'checks':[
  'Test-HallsAegisData.ps1 repeats the real migration in connection-local copies and compares full changed/protected rows, including the other Aegis, all quest relations and phase conditions. Custom script and flag bits survive. -Live checks deployed object, objectives, prerequisites, relations, instance binding, phase, bosses and active scaling; native-reference Hotfix checks pass.',
  'Test-HallsAegis.ps1 -VerifyReleaseBinary compiles the complete production GO class plus fresh GameObject::Use prologue/questgiver branch, Player::GetGameObjectIfCanInteractWith, objective/completion/status/race/prerequisite/reward and native request-reward/NPC/GO admission. Baseline missing claim reproduced. Both factions,28 guard cases, duplicate claims, actual party isolation and separate member use, no quest, saved counter re-entry, abandon/reaccept and43349 reward gate pass. Production callback returns into the real questgiver Use branch.',
  'Test-HallsOdynSurrender.ps1 -VerifyReleaseBinary executes the actual old/new surrender handlers, native EnterCombat body, post-scaling dispatcher and real target health/max/multiplier getters. First lethal/scaled/player-pet, first spell bootstrap, normal boundary, large raw/small final, repeat/dormant/protected/achievement/party credit cases pass; its dedicated audit records limits.',
  'Test-HallsAegisCatalog.ps1 reads native26972 DB2 stores and links Release GridMap/VMap/MMap/Detour libraries. Native models, virtual-credit spell scan, quest line, phase/map/difficulties, two floor/interaction anchors and complete ground route pass. Test-HallsAegisNative.py reproduces original archive and installed evidence. Upstream Salvation40005 and Break the Spine40004 native regressions pass.',
  'Release worldserver built directly in canonical build-extractors/bin/Release, original actual scripts/game link paths, no OutDir override. New Aegis registration and Odyn final-damage marker are present; actual scripts final-link timestamp check passes. Migration15 deployed transactionally after private scoped backup and updates SHA1 registration. Fresh final canonical startup reached ready, exited0, and left new DBErrors/stderr empty. Runtime configuration/certificates/DLLs/extracted data remain preserved.'
 ],
 'release':{'migration':migration.name,'migration_sha1':hashlib.sha1(migration.read_bytes()).hexdigest().upper(),'worldserver_sha256':hashlib.sha256((runtime/'worldserver.exe').read_bytes()).hexdigest().upper(),'smoke_directory':smoke.relative_to(ROOT).as_posix(),'DBErrors_bytes':0,'stderr_bytes':0},
 'limits':[
  'Production GO code and selected native engine handlers execute. Actual encounter save/load, world lookup/map enumeration, phasing/visibility/LOS/PlayerCondition services, menu preparation, packets/inventory/XP writes and real combat are fixture services. GO interaction uses a conservative center-distance fixture instead of the native rotated model-box calculation; installed geometry independently supplies an exact5yd approach. Native CanTakeQuest dispatch uses fixture defaults for unrelated component predicates. No full opcode/client or dungeon integration run is claimed.',
  'An abandoned/reaccepted quest loses its original native95676 objective. A completed instance does not re-award that kill at the pedestal; another qualifying Odyn encounter is required. Ordinary relog/re-entry with the saved counter1 can claim in the matching completed instance. This is an explicit boundary, not exact-stage scenario recovery or new character persistence.',
  'Client pending: Havi offer after40005 reward, dungeon/phase3554 object visibility/model/click, normal/heroic/mythic/keystone encounters including first high-damage spell and pet blows, Odyn survival/achievement/loot cleanup, actual Aegis tracking/claim/turn-in, both factions and two players, abandon/retry/relog, and43349 offer. No full quest, dungeon, animation, scene or visual is client-verified.',
  'Prior quest/client limitations remain.40003 starting navmesh issue is resolved as audit node exhaustion in6ca7ba29; earlier bone approach failures127694/127702/127704 remain unresolved. This repair changes no other Aegis or scenario object.'
 ],
 'next':'43349 '+next(r['LogTitle'].strip("'") for r in data['quest_template'] if r['ID']=='43349')+': pedestal acceptance, outdoor Aegis251992/phase4592, credit110409 and next handoff. Code/data only.'
}
path=ROOT/'docs/audits/halls-aegis-2026-10-16.json'
if '--write-evidence' in sys.argv:path.write_text(json.dumps(record,indent=2,ensure_ascii=False)+'\n',encoding='utf8')
else:assert record==json.loads(path.read_text(encoding='utf8'))
print('PASS: native40072 objectives/pedestal/model/phase/dungeon/claim/handoff evidence; client checks remain pending.')
