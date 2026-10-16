"""Original735 source-item/interaction evidence and scoped40002 repair audit."""
import hashlib,json,re,sys
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2]
archive=ROOT/'.codex/TDB735/TDB_full_735.00_2018_02_19/TDB_world_735.00_2018_02_19.sql'
selected={'quest_template','quest_objectives','creature_template','creature_loot_template','gossip_menu_option'}
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
        key=int(next((r[k] for k in ('QuestID','ID','entry','MenuID') if k in r),'0'))
        if (n.startswith('quest_') and key in(40002,40003)) or (n=='creature_template' and key in(91743,97890,97979)) or (n=='creature_loot_template' and key==97890 and r.get('Item')=='129106') or (n=='gossip_menu_option' and key==18828):original.setdefault(n,[]).append(r)
quest=next(r for r in original['quest_template'] if r['ID']=='40002')
assert 'Dravax' in quest['LogDescription'] and quest['ItemDrop1']=='129106' and quest['ItemDropQuantity1']=='1'
objectives=[r for r in original['quest_objectives'] if r['QuestID']=='40002']
assert {r['ObjectID'] for r in objectives}=={'97981','97982'}
original['quest_template']=[{k:r[k] for k in ('ID','LogTitle','LogDescription','QuestDescription','Flags','FlagsEx','StartItem','ItemDrop1','ItemDropQuantity1','RewardNextQuest')} for r in original['quest_template']]
original['creature_template']=[{k:r[k] for k in ('entry','name','modelid1','gossip_menu_id','faction','npcflag','AIName','ScriptName')} for r in original.get('creature_template',[])]
out=ROOT/'build-extractors/runtime-test/stormheim-familiar-fate'
native=(out/'native-lines.txt').read_text(encoding='utf-8-sig').splitlines()
assert 'use item=129106 spell=193253 trigger=5 charges=0' in native
assert "item 129106 Jailer's Shard max=0 stack=1 flags=67648,8192" in native
assert 'spell 193253 Credit - Shard Collected' in native and 'spell 193262 Credit - Vethir Freed' in native
route=(out/'geometry.txt').read_text(encoding='utf-8-sig').strip()
assert route.startswith('PASS:') and '19 native navmesh polygons' in route
migration=ROOT/'sql/updates/world/2026_10_16_06_stormheim_familiar_fate.sql'
smoke=sorted(out.glob('startup-*'))[-1]
assert (smoke/'DBErrors.log').stat().st_size==0 and (smoke/'stderr.log').stat().st_size==0
assert re.search(r'World initialized|World server is ready|Worldserver is ready',(smoke/'Server.log').read_text(encoding='utf8',errors='replace'))
record={
 'quest':40002,'title':'A Familiar Fate','status':'code/data repair passed; client verification pending',
 'archive_sha256':hashlib.sha256(archive.read_bytes()).hexdigest(),'original':original,
 'world_live':(out/'world-live.tsv').read_text(encoding='utf-8-sig').splitlines(),
 'hotfix':(out/'hotfix.tsv').read_text(encoding='utf-8-sig').splitlines(),
 'native_catalog':native,'native_hashes':json.loads((out/'native-inputs.json').read_text(encoding='utf-8-sig')),'ground_route':route,
 'findings':[
  '97982 is Credit - Shard Collected, not Dravax. Actual Dravax97890 DB291388 already drops quest-required129106 (installed75.985% chance). Quest ItemDrop129106/quantity1 makes it eligible via native HasQuestForItem; native ItemEffect193253/trigger5 casts on obtaining/storing it. Initial suspicion of a missing collection implementation was corrected before changing data. No extra shard, kill credit on Dravax death, loot chance change or invented item-use spell is added.',
  'Circle of Binding91743 DB291393 at the native release POI already has menu18828/option0, item condition129106 and SmartAI33/credit97981 followed by close-gossip. Native Spell::EffectKillCredit134 and SmartAI action33 both call RewardPlayerAndGroupAtEvent. Thus one player obtaining a shard and clicking the circle can advance both counters for nearby group members who possess no shard and never click. Menu item conditions are not re-evaluated by that SmartAI action at selection time.',
  'The bound shard193253 handler suppresses native group credit and checks its actual cast item/owner/self target. A scoped PlayerScript reconciles an active quest from an actual carried shard, including after loaded-bag visibility becomes available; it never creates the item. First counter is not a substitute for the item at release. The exact base circle rechecks active quest, collection counter, actual carried shard, current menu/item, own source/map/phase, life/flight/vehicle/combat/LOS/visibility and native NPC interaction distance. Both counters are written individually once; generic kill/talk/group delivery is blocked only for these two40002 objectives.',
  'The successful owner loses only their view of the binding through the existing hide-list/visibility API. Public circle/Vethir objects, NPC flags and Vethir sleeping addon remain unchanged for other players. Tracking immediately records the hide and cleans it on abandonment/logout/map transfer; saved completed/rewarded progress hides the circle again nearby. This is authored personal visibility behavior, not recovered retail animation choreography. No unproven model animation, flight, vehicle, camera, pet or teleport behavior is introduced.',
  'Shard stays until native reward/abandonment ItemDrop cleanup. FlagsEx8192 does not contain KEEP_ADDITIONAL_ITEMS1; addon SpecialFlags0 does not contain NOT_REMOVE_SOURCE16. No character progress or inventory reset is performed. Legacy counters already granted by the old group behavior are retained because their provenance cannot be recovered safely. Vethir97979 remains starter/ender and still supplies40003 Stem the Tide after reward; its Stormforged Horn129161/use193327 remains native.'
 ],
 'automated':[
  'Test-StormheimFamiliarFate.ps1 -VerifyReleaseBinary compiles the complete production module with freshly extracted Player::HasQuestForItem, Spell::EffectKillCredit, SmartAI action33, native group/kill/talk/completion/reward/next handlers. Both factions, independent group owners, old native dispatchers versus unrelated quest999 sharing, actual item versus no/fake cast item, repeat selection, stale menu/item, invalid phase/map/LOS/life/transport/flight/combat/source, combat-reach distance, loaded-bag retry, abandonment and saved completion, owner-only binding visibility cleanup/re-entry, and native next-giver eligibility pass.',
  'Test-StormheimFamiliarFateData.ps1 repeats the real migration against connection-local tables, compares full modified/protected rows, checks custom-script preservation, real source-item eligibility metadata, exact actors/menu/condition/native old action, and unchanged Vethir/next quest. -Live verifies the deployed migration. No Hotfix item/use/credit-effect overrides are present.',
  'Test-StormheimFamiliarFateCatalog.ps1 uses native C++ DB2Storage on sparse ItemSparse and regular item/effect/spell/display/model/quest-line data. Native ground navmesh has a19-polygon route between conservative5yd Dravax and Circle approaches. Spell names are not proof of visual playback.',
  'Previous40001 Valor Door and40078 Heavy Burden native regression suites pass after the shared Player guard change. Release worldserver built with its actual original game/scripts library link inputs and deployed directly to canonical build-extractors/bin/Release; no global OutDir override. Registration/guard markers and actual link-input timestamps are verified. Canonical executable startup reached ready and exited0 with fresh DBErrors/stderr empty.'
 ],
 'release':{'migration':migration.name,'migration_sha1':hashlib.sha1(migration.read_bytes()).hexdigest().upper(),'worldserver_sha256':hashlib.sha256((ROOT/'build-extractors/bin/Release/worldserver.exe').read_bytes()).hexdigest().upper(),'smoke_directory':smoke.relative_to(ROOT).as_posix(),'DBErrors_bytes':0,'stderr_bytes':0},
 'limits':[
  'C++ tests execute extracted production handlers but inventory storage/count visibility, character persistence/reward mutation, world/grid/network/NPC invocation and menu condition admission are fixtures. Actual loot RNG/loot window, pet killing blow/loot rights, database login and reward item deletion are not end-to-end client tests. Generic source-item cleanup was inspected in the native core and preserved; tests model abandonment storage/item deletion. The native eligibility function does execute against fixture inventory state.',
  'Client pending: Dravax combat and loot (player and pet kills, both factions), actual obtain trigger and each player item/counters, Circle menu and stale-menu rejection, binding disappearance only for its owner, Vethir visibility/pose/reward, abandon/retry/relog and full40003 handoff. No whole-quest or visual client verification is claimed.',
  'Previous38347/39848/39857/39849/39850/39851/39853/39855/40078/40001 client checks remain pending. Earlier bone navmesh failures127694/127702/127704 remain unresolved; this successful ground route does not clear them.'
 ],
 'next':'40003 Stem the Tide: native Stormforged Horn129161/193327, three Felbound Portals and Vethir air strikes, retry/cleanup and next handoff. Code/data only.'
}
path=ROOT/'docs/audits/stormheim-familiar-fate-2026-10-16.json'
if '--write-evidence' in sys.argv:path.write_text(json.dumps(record,indent=2,ensure_ascii=False)+'\n',encoding='utf8')
else:assert record==json.loads(path.read_text(encoding='utf8'))
print('PASS: original40002 source-item design, native sparse item/trigger/credit/model references, installed personal repair and canonical Release/startup evidence. Actual client interaction/visuals remain pending.')
