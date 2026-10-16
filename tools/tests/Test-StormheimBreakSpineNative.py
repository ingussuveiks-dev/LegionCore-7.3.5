"""Native40004 evidence and deployed tracking repair; no client certification."""
import re,json,hashlib,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
selected={'quest_template':{40004,40005},'quest_objectives':{280178,280179,280180,280181},'creature_template':{97822,97859,97944,97942},'creature_queststarter':{97979,97986},'creature_questender':{97979,97986}}
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
        if t in ('creature_queststarter','creature_questender') and int(r['quest'])!=40004:continue
        data.setdefault(t,[]).append(r)
assert len(data['quest_objectives'])==4 and all(r['Flags']=='1' and r['Type']=='0' and r['Amount']=='1' for r in data['quest_objectives'])
assert {r['ObjectID'] for r in data['quest_objectives']}=={'97822','97859','97942','97944'}
q=next(r for r in data['quest_template'] if r['ID']=='40004')
assert 'Defeat Garzareth, Karuas, Muorg and Ravathes' in q['LogDescription'] and q['RewardNextQuest']=='0'
assert any(r['id']=='97986' for r in data['creature_questender'])
data['quest_template']=[{k:r[k] for k in ('ID','LogTitle','LogDescription','QuestDescription','RewardNextQuest','Flags','FlagsEx')} for r in data['quest_template']]
data['creature_template']=[{k:r[k] for k in ('entry','name','modelid1','KillCredit1','KillCredit2','faction','unit_flags','AIName','ScriptName')} for r in data['creature_template']]
out=ROOT/'build-extractors/runtime-test/stormheim-break-spine'
world=(out/'world-live.tsv').read_text(encoding='utf-8-sig').splitlines()
expected={97822:(291435,63482),97859:(291507,64046),97942:(291594,64645),97944:(146706021,65514)}
for line in world:
    f=line.split('\t')
    if f[0]!='spawn':continue
    entry=int(f[2]);guid,display=expected[entry]
    assert int(f[1])==guid and f[6:12]==['1220','7541','7643','1','1','']
    assert f[12:19]==['0','0','0','14','32768','',''] and f[19:]==['97948','105123',str(display),'0','0','0']
catalog=(out/'native-lines.txt').read_text(encoding='utf-8-sig').splitlines()
assert 'quest 40004 line 138 order 2' in catalog and 'quest 40005 line 138 order 3' in catalog
assert sum(s.startswith('display ') for s in catalog)==4 and sum(s.startswith('spell ') for s in catalog)==10
geometry=(out/'geometry.txt').read_text(encoding='utf-8-sig').splitlines()
assert [int(re.search(r'through (\d+) native',s)[1]) for s in geometry if s.startswith('PASS:')]==[100,21,16,31,60]
assert not any(s.startswith(('FAIL:','INCOMPLETE:')) for s in geometry)
smoke=sorted(out.glob('startup-*'))[-1]
assert (smoke/'DBErrors.log').stat().st_size==0 and (smoke/'stderr.log').stat().st_size==0
assert re.search(r'World initialized|World server is ready|Worldserver is ready',(smoke/'Server.log').read_text(encoding='utf8',errors='replace'))
migration=ROOT/'sql/updates/world/2026_10_16_13_stormheim_break_spine.sql'
record={
    'quest':40004,'title':'Break the Spine','status':'scoped objective tracking repair and native code/data checks passed; client verification pending',
    'original_archive_sha256':hashlib.sha256(archive.read_bytes()).hexdigest(),'original':data,
    'world_live':world,'hotfix':(out/'hotfix.tsv').read_text(encoding='utf-8-sig').splitlines(),
    'native_catalog':catalog,'native_inputs_sha256':json.loads((out/'native-inputs.json').read_text(encoding='utf-8-sig')),'ground_routes':geometry,
    'findings':[
        'Original735 narrative names Garzareth97822, Karuas97859, Muorg97944 and Ravathes97942; original objectives280178..280181 require one each at storage3..6, all Flags1. Installed rows had Flags0 before this repair. Native QuestDef identifies bit1 as TRACKED_ON_MINIMAP, not optional or sequenced. Migration13 restores only that bit on the four exact quest/objective/entry/type/amount matches, retaining all other fields and bits. Client minimap output is still unverified.',
        'All four live base spawns exist in map1220/zone7541/area7643, phaseMask1 and no PhaseId restriction. Their creature.modelid overrides are0; native displays63482/64046/64645/65514 resolve to model/file68/1115848,8162/1109066,8246/1119496,8420/1240190 and each has World model metadata. Installed hostile faction14 is hostile to native human/orc factions1/2. No NON_ATTACKABLE, IMMUNE_TO_PC or NOT_SELECTABLE bits are set. Flags32768 is the unchanged unknown bit15. Native template scaling is active98..110, not0..0.',
        'Blank AIName/ScriptName and no SmartAI rows do not imply missing combat AI. The ordinary CreatureAI selector can choose AggressorAI for hostile non-civilian actors; its generic scheduler reads CreatureSpells. All10 installed creature_template_spell IDs exist in native Spell.db2. Their empty Difficulties lists admit outdoor difficulty0 through the extracted production CreatureSpell predicate. Full AI selection, spell scheduling, combat scaling and damage are not executed by this audit.',
        'Native Unit::Kill owner resolution attributes pet blows to the owner. KillRewarder schedules50ms credit; the extracted callback resolves the actual creature, Player::KilledMonster delivers its own entry plus existing97948/105123 secondary credits, and KilledMonsterCredit updates each required objective once. Secondary entries do not match40004 objectives. Completion requires all four in any order and reward offer requires interaction with involved Vethir97986; unrelated talk/credit does not advance the test. Native TalkedToCreature can credit MONSTER objectives generically, so this audit does not claim a new global talk-credit restriction. These hostile lieutenants have npcflag0 and no gossip/script interaction route.',
        'Starter97979 and ender97986 are retained. Installed40004 has PrevQuestID0, no conditions and no automatic next quest; native line138 orders40002/40003/40004/40005 as0/1/2/3 but ordering alone does not enforce prerequisites. Sleeping-Vethir early offer and40005 prerequisite/ascension behavior are recorded for the next audit, not silently changed. Original TDB confirms narrative/objectives/ender/model/secondary-credit design, while its faction35 differs from the installed hostile data. It does not establish provenance of current spawn coordinates; in particular Muorg DB146706021 is retained without a claim of retail positioning.',
        'All five conservative exact5yd ground routes pass with complete status, exact destination polygon and untruncated straight-path endpoint: starting Vethir->Ravathes100 polygons, Ravathes->Garzareth21, Garzareth->Karuas16, Karuas->Muorg31, Muorg->turn-in Vethir60. Independent audit query budget8192 follows the resolved40003 node-exhaustion diagnosis; runtime query budget and navmesh data remain unchanged.'
    ],
    'checks':[
        'Test-StormheimBreakSpineData.ps1 repeats the actual migration in connection-local MySQL copies, compares full rows against expectations including adjacent40003/40005, preserves an unrelated flag bit and a custom target, then -Live validates deployed objectives/relations/actors/scaling/spells. Native references have no checked display/model/faction/quest-line Hotfix overlays.',
        'Test-StormheimBreakSpine.ps1 freshly extracts production quest acceptance/status/race/prerequisite/completion/reward/NPC interaction, Unit kill-owner prefix, KillRewarder callback/group selection, Player template/monster-credit handlers and creature spell difficulty predicate.24 orders x2 factions x2 player/pet blows, required fourth kill, repeated cap, unrelated entries, missing actor, no quest, abandonment, reaccept, rewarded state and group range/life/contact branches pass. This also runs the upstream Valor Door native regression.',
        'Test-StormheimBreakSpineCatalog.ps1 compiles actual DB2Storage readers and links installed Release MMap/Detour libraries; all native model/spell/faction/quest-line references and five complete routes pass. Test-StormheimBreakSpineNative.py checks original TDB evidence and reproduces this audit.',
        'Migration13 deployed transactionally after private scoped backup and recorded in updates with its SHA1. Canonical Release startup reached ready and exited0 with empty new DBErrors/stderr. SQL-only change: no C++ server implementation rebuild required.'
    ],
    'deployment':{'migration':migration.name,'sha1':hashlib.sha1(migration.read_bytes()).hexdigest().upper(),'worldserver_sha256':hashlib.sha256((ROOT/'build-extractors/bin/Release/worldserver.exe').read_bytes()).hexdigest().upper(),'smoke_directory':smoke.relative_to(ROOT).as_posix(),'DBErrors_bytes':0,'stderr_bytes':0},
    'limits':[
        'World lookup, actual damage/kill invocation,50ms scheduler clock, group services, inventory/storage, packets and persistence are fixtures. Native CanTakeQuest dispatch is extracted, but unrelated component gates use fixture defaults. The native template loader derived KILL special flag is represented explicitly. Full RewardQuest inventory/XP/currency writes and actual character login are not executed. No deployed native handler binary certification is claimed from source extraction or startup.',
        'Client pending: actual Vethir offer after prison release, four minimap areas/markers, all four spawn appearances/combat/spells/scaled damage, direct and pet blows, group proximity, both factions, abandon/retry/relog, all ground approaches and97986 reward. Navmesh connectivity does not verify renderer/collision or combat path quality.',
        'Earlier quest/client limitations remain.40003 starting navmesh issue is resolved as diagnostic node exhaustion in commit6ca7ba29; earlier bone approach failures127694/127702/127704 remain unresolved. No whole quest or visual is described as client-verified.'
    ],
    'next':'40005 Stormheim\'s Salvation: prerequisite/offer, Vethir ascent, Skovald92307 and credit103766, completion and40072 handoff. Also examine early40004 offer in that chain audit. Code/data only.'
}
path=ROOT/'docs/audits/stormheim-break-spine-2026-10-16.json'
if '--write-evidence' in sys.argv:path.write_text(json.dumps(record,indent=2,ensure_ascii=False)+'\n',encoding='utf8')
else:assert record==json.loads(path.read_text(encoding='utf8'))
print('PASS: native40004 narrative/tracking/model/spell/credit evidence and five complete ground routes; client checks remain pending.')
