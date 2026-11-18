"""Inspect the final x64 implementation, including the original final-link inputs."""
from pathlib import Path
import runpy
from sharasdal_erunak_entry_pe import sites, values, floats

n = runpy.run_path(str(Path(__file__).with_name('verify_smart_linked_cast_retry_release.py')))
path = n['instructions']('?CalculatePath@PathGenerator@@QEAA_NAEBVVector3@G3D@@0_N@Z')
helper = n['instructions']('?BuildMawShipGroundPath@Movement@@')
query = n['instructions']('?GetModelNavMeshQuery@MMapManager@MMAP@@')
support = n['instructions']('??R<lambda_1>@?1??BuildMawShipGroundPath@Movement@@')
startup = n['instructions']('?SetInitialWorldSettings@World@@')
admission, = sites(n, path, '?IsPathfindingEnabled@DisableMgr@@')
ship, = sites(n, path, '?BuildMawShipGroundPath@Movement@@')
ordinary, = sites(n, path, '?BuildPolyPath@PathGenerator@@')
assert admission < ship < ordinary
assert {1492, 246919} <= values(helper)
assert sites(n, helper, '?IsFalling@Unit@@')
assert any(x.mnemonic == 'test' and '0x1000200' in x.op_str for x in helper) # Flying/disabled gravity.
model, = sites(n, helper, '?GetModelNavMeshQuery@MMapManager@MMAP@@')
nearest = sites(n, helper, '?findNearestPoly@dtNavMeshQuery@@')
corridor, = sites(n, helper, '?findPath@dtNavMeshQuery@@')
corners, = sites(n, helper, '?findStraightPath@dtNavMeshQuery@@')
assert len(nearest) == 2 and model < min(nearest) < max(nearest) < corridor < corners
assert len(sites(n, helper, '?WorldToModel@GameObjectModel@@')) == 2
assert len(sites(n, helper, '?isInLineOfSight@Map@@')) == 2
assert sites(n, support, '?GetHeight@Map@@') and sites(n, support, '?getHeight@GameObjectModel@@')
assert {0.5, 0.6, 1.0, 4.0, 296.0} <= floats(n, helper)
assert 512 in values(helper) and 4096 in values(query)
assert sites(n, startup, '?loadGameObject@MMapManager@MMAP@@')
print('PASS linked Maw model navigation: scoped1492/246919, actual CalculatePath hook before terrain path, model transforms and complete Detour corridor, live Map height/LOS, native point/distance bounds, per-thread query4096 and startup model preload')
