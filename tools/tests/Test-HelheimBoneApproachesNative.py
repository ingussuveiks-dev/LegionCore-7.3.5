"""Retain the failed conservative surveys and verify native bone approaches."""
import hashlib,json,re,sys
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/research'))
from audit_spell_mechanics import Data
runtime=ROOT/'build-extractors/bin/Release';out=ROOT/'build-extractors/runtime-test/stormheim-helheim-supplies'
rows=json.loads((out/'geometry-player.json').read_text(encoding='utf-8-sig'))
assert len(rows)==16 and all(r['connected'] and r['native_exit']==0 for r in rows)
assert {r['spawn'] for r in rows if not r['ground_connected']}=={127694,127702,127704}
assert {r['spawn'] for r in rows if not r['center_connected']}=={127694,127704}
assert (out/'bone-display-hotfix.txt').read_text(encoding='utf-8-sig').strip()=='0'
model=Data(runtime/'dbc/enUS',ROOT/'.codex/stormheim-no-overlays').rows('GameObjectDisplayInfo')[6328]
assert model['FileDataID']==190437
definitions=(ROOT/'src/server/game/Entities/Object/ObjectDefines.h').read_text(encoding='utf8')
assert re.search(r'#define INTERACTION_DISTANCE\s+5\.0f',definitions)
source=(ROOT/'src/server/game/Entities/GameObject/GameObject.cpp').read_text(encoding='utf8')
assert 'return INTERACTION_DISTANCE;' in source.split('float GameObject::GetInteractionDistance() const')[1].split('void GameObject::UpdateModelPosition')[0]
routes=[]
for r in rows:
    evidence=r['evidence'];assert any(s.startswith('PASS:') for s in evidence)
    assert not any(s.startswith(('FAIL:','INCOMPLETE:','EXHAUSTED:')) for s in evidence)
    assert any('magma/slime=0' in s for s in evidence) and any('staticLOS=1 PASS' in s for s in evidence)
    distance=float(next(s for s in evidence if s.startswith('endpoint centerDistance=')).split('=')[1])
    if r['spawn'] in (127694,127704):assert distance>5
    routes.append({**{k:r[k] for k in ('spawn','position','orientation','ground_connected','center_connected')},
        'native_connected':True,'native_evidence':evidence,
        'original_ground_diagnostic':r['ground_evidence'] if not r['ground_connected'] else [r['ground_evidence'][0]],
        'conservative_center_diagnostic':r['center_evidence'] if not r['center_connected'] else [r['center_evidence'][0]]})
hashes={p.relative_to(runtime).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for folder,pattern in (('mmaps','1463*'),('maps','1463_*'),('vmaps','1463*')) for p in (runtime/folder).glob(pattern) if p.is_file()}
old=json.loads((ROOT/'docs/audits/stormheim-helheim-supplies-2026-10-15.json').read_text(encoding='utf8'))
record={
 'quests':[38324,38339],'date':'2026-10-16','status':'all16 native interaction approaches pass; former three audit failures resolved; client verification pending',
 'original_failed_spawns':[r['spawn'] for r in old['geometry'] if not r['connected']],
 'resolution':[
  'The old survey required a complete NAV_GROUND corridor and a spherical5yd target-center approach. Those are conservative audit assumptions, not the actual player and GameObject admission rules. Keep its13/16 observation as historical evidence. A65535-node ground-only replay still returns partial corridors without node exhaustion for127694/127702/127704; raising the runtime node budget does not solve this.',
  'Native player PathGenerator::CreateFilter admits ground and liquids. The new explicit walk-swim audit permits ground plus ordinary water, excludes magma/slime, and counts the actual corridor flags. Every route requires complete Detour status, the exact target polygon and an untruncated straight path with its end flag. Default ground-only mode and1220 map remain unchanged.',
  'A mixed-navmesh path alone initially appeared to clear all three, but raw endpoint127694 atZ1.39606 lies under terrain4.32933 and127704 atZ-2.33727 under terrain0.0453949. Neither has usable local liquid there. The strengthened check rejects these points: native GridMap/VMap floor/liquid evidence and static LOS must agree. Merely admitting NAV_WATER would have produced false positives.',
  'The conservative center-distance survey then passes14/16, including127702. For127694/127704, execute the actual production GameObject::IsInRange body against native display6328 bounds/file190437 and each installed orientation. GameObject::_IsWithinDist delegates to that rotated axis-bound test; the radius expands each native bound, rather than imposing a5yd center sphere. Do not rescale the bounds by template size1.75: production does not. The new valid shore endpoints pass this native test, supporting surface and static LOS even though their center distances exceed5yd.',
  'All16 existing Cursed Bones240586 spawns pass the native interaction survey from freed Colborn. Positions, orientations, model, scale, flags, quest/loot selectors, all loot rows, server query budgets and extracted runtime data remain unchanged. This resolves the geometry audit assumptions, not a demonstrated in-game spawn defect.'
 ],
 'native_model':model,'gameobject_display_hotfix_rows':0,'routes':routes,
 'runtime_geometry_sha256':hashes,'GameObjectDisplayInfo_sha256':hashlib.sha256((runtime/'dbc/enUS/GameObjectDisplayInfo.db2').read_bytes()).hexdigest(),
 'checks':[
  'Audit-StormheimHelheimSuppliesGeometry.ps1 exports live positions/orientations and confirms no model Hotfix override. It compiles the native Release-linked readers, extracts the current IsInRange handler, retains13/16 ground and14/16 center baselines, then requires16/16 native-bound walking/swimming approaches with endpoint surface and static LOS checks.',
  'Test-StormheimHelheimSupplies.ps1 passes the actual Gathering6478/lock2160, chest loot selector, native loot processing,8 bones/20 seaweed lifecycle, both factions and both reward orders. Data replay and live assertions pass with orientation export added.',
  'Shared-helper regressions: Test-StormheimBreakSpineCatalog.ps1, Test-HallsAegisCatalog.ps1 and Test-AegisReturnCatalog.ps1 pass with unchanged default ground behavior. The40003 initial1024-node exhausted route and all four8192-node complete routes are checked separately.',
  'No production server source or migration changed. No new worldserver build/startup/deployment is claimed. Canonical executable and runtime data hashes remain recorded; private generator output was not deployed.'
 ],
 'release':{'worldserver_sha256':hashlib.sha256((runtime/'worldserver.exe').read_bytes()).hexdigest(),'runtime_data_changed':False,'migration':None},
 'limits':[
  'This is an executable native distance predicate plus installed navmesh/terrain/static-collision survey, not an integrated client movement/loot session. It establishes complete mixed-terrain corridors and valid interaction endpoints; it does not sample every corridor point against collision, run native movement splines or client swimming/jumping physics, or include phase-dependent dynamic GameObject collisions.',
  'Static LOS uses the native+2 height convention to the object center and retains M2 collisions. Full WorldObject::IsWithinLOSInMap hit-sphere/phase/dynamic-tree dispatch is not executed. Native range uses display bounds and real orientations, with no extra reach from template scale or player hitbox.',
  'Client pending: cursor/model visibility, actual land/water traversal and Gathering/loot at all three former failures, cast interruption, bag-space errors, both factions, concurrent players and retry/relog. No whole quest, client collision, model or collection interaction is described as client-verified.',
  'Original supply/seaweed and subsequent quest client requirements remain. The previous observed ground-only failures remain in the original audit with this linked resolution; they must no longer be described as unresolved native interaction approaches.'
 ],
 'next':'Continue with the next unaudited quest branch from native quest-line records; all previous client checks remain separate.'
}
path=ROOT/'docs/audits/helheim-bone-approaches-2026-10-16.json'
trial=ROOT/'.codex/helheim-tile-rebuild'
if (trial/'mmaps/14633131.mmtile').exists():
    record['isolated_regeneration']={'command':'mmaps_generator.exe 1463 --tile 31,31 --silent (private working directory)',
        'source_maps_vmaps_dbc':'junctions to canonical extracted inputs; private mmaps copies only',
        'original_tile_sha256':hashes['mmaps/14633131.mmtile'],
        'private_tile_sha256':hashlib.sha256((trial/'mmaps/14633131.mmtile').read_bytes()).hexdigest(),
        'same_map_header':(trial/'mmaps/1463.mmap').read_bytes()==(runtime/'mmaps/1463.mmap').read_bytes(),
        'result':'Private one-tile rebuild reproduced the same conservative endpoint-height failures; it did not repair the audit criterion and was not deployed.'}
elif path.exists():
    # Retain this historical experiment when its private output is cleaned up;
    # ordinary survey checks do not claim to rerun extraction.
    previous=json.loads(path.read_text(encoding='utf8'))
    if 'isolated_regeneration' in previous:record['isolated_regeneration']=previous['isolated_regeneration']
if '--write-evidence' in sys.argv:path.write_text(json.dumps(record,indent=2,ensure_ascii=False)+'\n',encoding='utf8')
else:assert record==json.loads(path.read_text(encoding='utf8'))
print('PASS:16 native bone interaction approaches; ground/center failures retained and resolved; client checks pending.')
