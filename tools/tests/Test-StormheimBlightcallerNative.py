"""38361 original/live/native-handler evidence; no client certification."""
import hashlib,json,re,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
archive=ROOT/'.codex/TDB735/TDB_full_735.00_2018_02_19/TDB_world_735.00_2018_02_19.sql'
selected={'quest_template':{38361},'quest_objectives':{277874},'creature_template':{91158,91414,91571,91581},'creature_queststarter':{91158},'creature_questender':{91158}}
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
        r=dict(zip(cols[t],re.findall(r"'(?:\\.|''|[^'\\])*'|[^,]+",mt[0][1:-1])))
        if t in ('creature_queststarter','creature_questender') and r['quest']!='38361':continue
        original.setdefault(t,[]).append(r)
q=original['quest_template'][0]
assert q['LogTitle']=="'Wrath of the Blightcaller'" and q['RewardNextQuest']=='0'
assert [(r['ObjectID'],r['Amount'],r['StorageIndex'],r['Flags']) for r in original['quest_objectives']]==[('91581','15','0','0')]
original['quest_template']=[{k:q[k] for k in ('ID','LogTitle','LogDescription','QuestDescription','StartItem','RewardNextQuest','Flags','FlagsEx','AllowableRaces')}]
original['creature_template']=[{k:r[k] for k in ('entry','name','modelid1','faction','KillCredit1','KillCredit2','AIName','ScriptName')} for r in original['creature_template']]
out=ROOT/'build-extractors/runtime-test/stormheim-blightcaller';runtime=ROOT/'build-extractors/bin/Release'
read=lambda name:(out/name).read_text(encoding='utf-8-sig').splitlines()
world=read('world.tsv');catalog=read('native-lines.txt');geometry=read('geometry.txt');ground=read('geometry-ground-only.txt')
assert 'quest38361 line127 order2' in catalog
assert any(s.endswith('Permanent Feign Death (Stun, Untrackable, Immune)') for s in catalog)
assert sum(s.startswith('PASS:') for s in geometry)==1 and any('20 native ground/water' in s for s in geometry)
assert any('surfaceError=' in s and s.endswith('staticLOS=1 PASS') for s in geometry)
assert any('partial=1' in s and 'out_of_nodes=0' in s for s in ground)
files=[p for folder,pattern in (('mmaps','1220*.mmtile'),('maps','1220_*.map'),('vmaps','1220*.vmtile')) for p in (runtime/folder).glob(pattern) if any(k in p.name for k in ('2426','24_26','26_24','2326','23_26','26_23'))]
files += [runtime/'mmaps/1220.mmap',runtime/'vmaps/1220.vmtree']
record={
 'quest':38361,'title':'Wrath of the Blightcaller','status':'existing required kill/reward route passes selected native checks; no production repair indicated; client pending',
 'original_archive_sha256':hashlib.sha256(archive.read_bytes()).hexdigest(),'original':original,
 'world_live':world,'hotfix':read('hotfix.tsv'),'native_catalog':catalog,
 'native_db2_sha256':{n:hashlib.sha256((runtime/f'dbc/enUS/{n}.db2').read_bytes()).hexdigest() for n in ('QuestLineXQuest','CreatureDisplayInfo','CreatureModelData','FactionTemplate','Spell')},
 'ground_only_diagnostic':ground,'walking_swimming_route':geometry,
 'geometry_sha256':{p.relative_to(runtime).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in files},
 'findings':[
  'Keep the original mandatory MONSTER277874/91581 amount15,storage0,Flags0.91581 is the7th Legion Paratrooper, not a generic gossip marker. Native KilledMonster credits its own entry and both template substitutes:91414 Skyfire Gryphon Rider and91571 Thane Wildsky already link91581. No new death script, acceptance credit, event completion bit, tracking flag or guessed spell is needed.',
  'Installed38361 is offered/ended by Nathanos91158 after38332 is rewarded; no acceptance start scripts or extra source19/20 conditions apply. Native quest line127 order2 is confirmed. Its expanded Horde race mask234881970 has nine allowed bits, including the installed allied-race bits; native SatisfyQuestRace rejects every other tested bit. The narrower original33555378 mask is historical, not a repair target.',
  'Native Unit::Kill owner/reward admission, KillRewarder::_RewardPlayer/_RewardKillCredit, Player::KilledMonster/KilledMonsterCredit, objective/completion/prerequisite/race and request-reward handlers execute. Player/pet final blows, direct/substitute/mixed kills,14-to15, counter cap, duplicate rewarded-victim admission, no quest/rewarded, saved14 reconstruction, abandon/retry and group range/life/contact pass. No generic pet, Warlock, damage-scaling or scenario code changed.',
  'Live map1220 has24 paratroopers and one Thane without attack-protection bits, native hostile faction84 and active98..110 scaling. Existing seven91414 spawns are permanent feign-death/stun/immune decorations: flags0x20048300, addon145363 and native spell name confirm that state. Their substitution path is tested hypothetically, not reported as an executable kill of these protected world copies. The24 ordinary paratroopers already exceed15; preserve the protected decoration and its native credit mapping.',
  'Four Nathanos copies are retained. Unphased DB340422 at4343.36,2834.09,1.83185 is the shore questgiver.6375 copies use existing zone7541/90 conditions for38360 complete OR rewarded, independent of38361; two upper copies overlap and remain a client visibility/duplicate check. There is no evidence here to delete shared actors or gate this kill quest on38360.',
  'Native installed display/model chains are valid: Nathanos66644->10608/file1769126, rider62752->49/file119940, Thane62718->53/file118355 and paratrooper62743->55/file120791. Model metadata has positive radii/reach; checked display/model/faction overlays are absent. Preserve the live Nathanos display rather than restoring original68482 merely because it differs.',
  'Original Flags38797320 differs from installed34603016 by UPDATE_PHASE_SHIFT. This core schedules phase notifications on acceptance/reward without testing that bit. The active6375 condition is for38360, so no38361 phase repair is indicated. All original required objective fields, RewardNextQuest0 and installed faction/prerequisite settings remain unchanged.',
  'A ground-only survey from shore Nathanos to paratrooper DB289012 returned partial paths without node exhaustion. The actual walking/swimming survey passes20 polygons,17 ground/3 water,8 corners and53.043yd, exact5yd approach, valid endpoint floor and static LOS. Preserve the failed conservative query rather than calling this a spawn defect. This is one sampled entry route, not an audit of every target/corpse location.'
 ],
 'checks':[
  'Test-StormheimBlightcallerData.ps1 read-only live objective/relations, counts/positions, active scaling, spell difficulty admission data, protected addons and phase conditions; native-reference overlays absent.',
  'Test-StormheimBlightcaller.ps1 compiles the selected native handlers and current exported quest/template data. Nine allowed race bits, missing/unrewarded prerequisite, player/pet, required counter boundary/cap, duplicate rewarded actor, request-reward distance, abandon/retry, missing victim and party eligibility pass. Upstream Break the Spine and its existing prerequisites regressions pass.',
  'Test-StormheimBlightcallerCatalog.ps1 links original Release DB2/MMap/VMap/GridMap libraries. Native models/factions/spells/quest line and the walking/swimming shore approach pass. Test-StormheimBlightcallerNative.py preserves original/live evidence and hashes.',
  'No production source or migration changed. No new worldserver build, deployment or startup is claimed; canonical Release executable is retained and hashed.'
 ],
 'release':{'worldserver_sha256':hashlib.sha256((runtime/'worldserver.exe').read_bytes()).hexdigest(),'production_change':False,'migration':None},
 'limits':[
  'World lookup, owner resolution service, damage-enough/loot-recipient results, per-creature rewarded storage,50ms scheduling, packets, menu/XP/inventory/persistence writes and unrelated CanTakeQuest predicates remain fixtures. Full native attack-target validation/combat/AI and the full Unit::Kill/Reward dispatcher are not executed. Native faction relations, installed flags/scaling and selected reward handlers are checked independently.',
  'Hypothetical91414 template credit does not make its seven protected feign-death world spawns attackable. Body/corpse visibility and ordinary paratrooper/Thane combat still need the client. No immunity, decorative pose or shared actor is changed.',
  'Saved14 is a counter-reconstruction fixture, not real logout/SQL integration. Abandon/retry resets fixture acceptance and does not claim generic delayed kill callbacks are tied to a quest lifetime. Native Creature::ClearLootList clears rewarded-player state on corpse removal; full respawn timing is not simulated.',
  'The one sampled corridor and endpoint do not prove every NPC floor/path, client collision loading, dynamic phase-visible objects or camera/model behavior. Client pending: actual Horde quest menu/acceptance, paratrooper and pet kills15/15, optional Thane, protected rider props, group sharing, retry/relog, Nathanos copies/phase and actual reward. Alliance rejection is tested from the live race mask; no Alliance quest route is invented.',
  'Prior client requirements and historical failures remain in their respective audits. The three Helheim bone interaction audit failures were separately resolved in539e94ca; their client collection checks remain pending.'
 ],
 'next':'38558 See Ya Later, Oscillator: Alliance source item/drop, actual collection and38058 handoff; code/data only.'
}
path=ROOT/'docs/audits/stormheim-blightcaller-2026-10-16.json'
if '--write-evidence' in sys.argv:path.write_text(json.dumps(record,indent=2,ensure_ascii=False)+'\n',encoding='utf8')
else:assert record==json.loads(path.read_text(encoding='utf8'))
print('PASS:38361 original/live kill, Horde admission, protected actors, native models/factions and shore-route evidence; client pending.')
