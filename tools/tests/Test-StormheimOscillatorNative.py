"""38558 repair evidence: real chest selector and preserved personal loot."""
import hashlib,json,re,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
archive=ROOT/'.codex/TDB735/TDB_full_735.00_2018_02_19/TDB_world_735.00_2018_02_19.sql'
selected={'quest_template':{38558},'quest_objectives':{278005},'gameobject_template':{240286}}
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
    for mt in patterns[t].finditer(line):original.setdefault(t,[]).append(dict(zip(cols[t],re.findall(r"'(?:\\.|''|[^'\\])*'|[^,]+",mt[0][1:-1]))))
q=original['quest_template'][0];assert q['RewardNextQuest']=='38058' and q['StartItem']=='0'
assert [(r['Type'],r['ObjectID'],r['Amount'],r['StorageIndex'],r['Flags']) for r in original['quest_objectives']]==[('1','123887','1','0','0')]
original['quest_template']=[{k:q[k] for k in ('ID','LogTitle','LogDescription','QuestDescription','Flags','FlagsEx','AllowableRaces','StartItem','RewardNextQuest')}]
go=original['gameobject_template'][0];assert go['type']=='3' and go['Data0']=='1690' and go['displayId']=='8644'
original['gameobject_template']=[{k:go[k] for k in ('entry','name','type','displayId','size','Data0','Data1','Data8','Data15','Data30','Data31','ScriptName')}]
runtime=ROOT/'build-extractors/bin/Release';out=ROOT/'build-extractors/runtime-test/stormheim-oscillator'
read=lambda name:(out/name).read_text(encoding='utf-8-sig').splitlines()
world=read('world-live.tsv');catalog=read('native-lines.txt');floors=read('floors.txt');geometry=read('geometry.txt')
assert 'loot\t240286\t240286\t123887\t100\t1\t1\t0\t1\t1' in world and 'personal-metadata\t0' in world
assert 'item123887 Crystal Oscillator max=0 stack=1 bonding=4 class=12' in catalog
assert 'item123887 use effects 0' in catalog and 'native lock1690 type5 and Open3365 effect33 target23' in catalog
assert sum(s.endswith('PASS') for s in floors)==2 and any('81 native ground' in s for s in geometry)
assert not any(s.startswith(('FAIL:','INCOMPLETE:','EXHAUSTED:')) for s in geometry)
smoke=sorted(out.glob('startup-*'))[-1]
assert (smoke/'DBErrors.log').stat().st_size==0 and (smoke/'stderr.log').stat().st_size==0
assert re.search(r'World initialized|World server is ready|Worldserver is ready',(smoke/'Server.log').read_text(encoding='utf8',errors='replace'))
migration=ROOT/'sql/updates/world/2026_10_17_01_stormheim_crystal_oscillator.sql'
geometry_files=[runtime/'mmaps/1220.mmap',runtime/'vmaps/1220.vmtree']
geometry_files += [runtime/f'mmaps/1220{x:02d}{y:02d}.mmtile' for x in range(24,28) for y in range(25,28)]
geometry_files += [runtime/f'maps/1220_{x}_{y}.map' for x in (25,26) for y in (25,26)]
geometry_files += [runtime/f'vmaps/1220_{y}_{x}.vmtile' for x in (25,26) for y in (25,26)]
record={
 'quest':38558,'title':'See Ya Later, Oscillator','date':'2026-10-17','status':'blocking loot selector repaired/deployed; automated checks passed; client verification pending',
 'original_archive_sha256':hashlib.sha256(archive.read_bytes()).hexdigest(),'original':original,
 'observed_failure':'Installed Crystal Oscillator240286/DB114753 had Data1=0 despite its existing100% quest loot240286->123887. Native GetLootId returned0, so the native SendLoot outer guard skipped even the personal FillLoot branch. questItem1 and personal marker60251 do not supply that selector. The regression reproduces this empty personal chest before the repair.',
 'repair':'Migration2026_10_17_01 sets only240286.Data1=240286 for the recognized type3/lock1690/item123887 chest when its exact existing100% quest row is present. Custom nonzero selectors and missing/wrong loot are refused. Preserve personal marker60251,flags2113540,size0.05,restock/LOS/quest/group/security fields, lock, all spawn/orientation/respawn data and every loot/quest row.',
 'world_live':world,'hotfix':read('hotfix.tsv'),'native_catalog':catalog,
 'native_db2_sha256':{n:hashlib.sha256((runtime/f'dbc/enUS/{n}.db2').read_bytes()).hexdigest() for n in ('Item','ItemSparse','ItemEffect','Lock','SpellEffect','GameObjectDisplayInfo','CreatureDisplayInfo','CreatureModelData','QuestLineXQuest')},
 'floors':floors,'ground_route':geometry,
 'selected_geometry_sha256':{p.relative_to(runtime).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in geometry_files if p.exists()},
 'findings':[
  'Keep original objective278005 ITEM123887 amount1/storage0/Flags0. Crystal Oscillator is a quest-bound,class12 item,stack1,maxCount0, no use spell. Native GetLootId, SendLoot selector/personal-generation guard, IsPersonal, FillLoot processor-selection branch and actual ProcessRareOrGoLoot execute. No kill/GO-open credit can substitute for taking the item.',
  'This chest is personal because Data30=60251 is nonzero. No personal_loot_template metadata row changes its nonboss path, and DungeonEncounter is0. The existing rare/GO personal processor is therefore the actual route; testing only ordinary LootTemplate::Process would miss that distinction. Its ungrouped,nonreference,quest-required mode1/chance100/count1 row stays unchanged. A second player uses separate loot storage on the same public object.',
  'Native lock1690 uses type2/index5/skill0, matched by Open3365 effect33/misc5/target23. Gathering6478/type13 is rejected by the real CanOpenLock. No profession, guessed cast, extra click script or item-use spell is added.',
  'Actual item-added/removed/count-reconstruction/completion and RewardQuest item-consumption handlers execute. Opening without taking grants no item objective; taking completes, bank-only ownership cannot reward, consumption removes the item, and only rewarded38558 admits the existing38058 next quest. Native NPC reward-next dispatch preserves its normal details window and independent reward callback.',
  'Overspark90866 already starts/ends38558 and starts38058. Preserve PrevQuestID39800 for38558 and PrevQuestID38558 for38058, both native next links and live unrestricted race masks. The test deliberately exercises both faction values with forced39800 rewarded history; this does not add an ordinary Horde route or alter faction-specific story admission. Native quest-line128/order3 is confirmed.',
  'Keep chest DB114753 at2962.24,3081.73,420.672 on map1220, display8644/file255125, scale0.05. Native boundsZ[-6.65746,6.65744] exist; no size/art correction is inferred from the tiny scale. Overspark7028 resolves to model182/file119159. Checked sparse/item/lock/effect/display/model overlays are absent.',
  'Native floor checks: shore Overspark DB290453 floor440.215 versus440.295, chest-side floor420.184 versus420.672. Complete ground route from shore giver to conservative5yd chest approach passes81 polygons,23 corners,443.902yd. The phased upper Overspark copy remains at3395.59,3077.83,526.411/phase4589; its actual visibility and later quest behavior are retained, not client-verified.',
  'Opening this outdoor personal quest chest calls native AddPlayerLootCooldown with TYPE_GUID and difficulty0. The actual method writes the spawn GUID114753 and a600-second expiry; native lookup stops blocking after expiry. An earlier empty click can therefore hide the chest temporarily even after data repair. Preserve that bounded existing behavior; no character inventory, quest history or personal cooldown rows are reset.'
 ],
 'checks':[
  'Test-StormheimOscillatorData.ps1 executes the real migration twice in connection-local copies and compares complete protected rows, including the unrelated bones chest, loot, spawn, objectives and handoff settings. Custom selector/settings and absent/wrong loot cases pass; -Live confirms offline deployment and native-reference overlay absence.',
  'Test-StormheimOscillator.ps1 compiles selected production selector/personal/open/dispatch/RareOrGo/lock/item/reward-next/cooldown handlers. Empty chest baseline, actual item-only completion, separate players, wrong lock, conditions/difficulty, bag/bank/retry, consumption and38058 details pass. Full FillLoot setup, world lookup, inventory storage and packet/UI services remain fixtures.',
  'Test-StormheimOscillatorCatalog.ps1 uses native C++ sparse DB2Storage plus Release GridMap/VMap/MMap/Detour libraries. Native item/lock/Open effect/model/quest-line, two floor anchors and complete ground route pass. Existing Helheim supplies/item lifecycle regression and -VerifyReleaseBinary reward-next marker/actual final-link checks pass.',
  'Canonical Release worldserver build succeeds with original game/scripts library paths and no global OutDir override. The data-only migration was deployed transactionally with a private scoped backup and updates SHA1 record. A fresh canonical startup reached ready, shut down with exit0 and produced empty DBErrors/stderr. Runtime configuration/certificates/DLLs/extracted data remain preserved.'
 ],
 'release':{'migration':migration.name,'migration_sha1':hashlib.sha1(migration.read_bytes()).hexdigest().upper(),'worldserver_sha256':hashlib.sha256((runtime/'worldserver.exe').read_bytes()).hexdigest().upper(),'smoke_directory':smoke.relative_to(ROOT).as_posix(),'ready':True,'exit_code':0,'DBErrors_bytes':0,'stderr_bytes':0},
 'limits':[
  'Selected native handlers execute, including the actual personal-loot branch and processor selection. The full FillLoot/session/opcode/loot-permission pipeline, item-template runtime loading, real inventory writes/space/stack handling, conditions/world services, clock, persistence and packets are fixture boundaries. The sparse native catalog independently confirms item limits/class, but the loot fixture does not reproduce the full item subsystem.',
  'The production personal SendLoot fragment substitutes the test player reference for its enclosing this pointer. Its selector and personal-generation guard/fields are retained; the full nonpersonal/battleground/group branch is outside the wrapper. Unexecuted rare/GO grouped branches are fixtures because this real row is ungrouped.',
  'The600-second retry test executes the native cooldown methods against a fixture clock/map and checks the actual GUID key/expiry. It does not exercise the complete visibility dispatcher or persisted character cooldown load/save. Existing affected players may need the ordinary cooldown to expire; no exact client recovery was verified.',
  'Client pending: actual Overspark menu and prerequisites, chest cursor/visibility/tiny model, native Open cast/interruption, loot window and pickup, bag-full behavior, personal cooldown/retry, two players, relog and38558 reward/38058 details. The existing38058 charging/bar behavior retains its separate client checks. No whole quest, visual or client sequence is certified.',
  'Earlier quest/client requirements remain. Helheim bone native interaction audit failures were separately resolved in539e94ca; their client collection checks remain pending.'
 ],
 'next':'38405/38614 To Weather the Storm: faction prerequisites, three real item sources and next-stage handoff; code/data only.'
}
path=ROOT/'docs/audits/stormheim-oscillator-2026-10-17.json'
if '--write-evidence' in sys.argv:path.write_text(json.dumps(record,indent=2,ensure_ascii=False)+'\n',encoding='utf8')
else:assert record==json.loads(path.read_text(encoding='utf8'))
print('PASS:38558 native original/live personal-chest repair, actual item/next quest path and Release deployment evidence; client pending.')
