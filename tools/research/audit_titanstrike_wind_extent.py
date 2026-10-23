"""Inspect native wind mesh/attachment bounds; this is not a render test.

The two gameplay force areas overlap, but kit58494 already contains a complete
corridor. A second full assembly is not a second short section of wind.
"""
import argparse
import csv
import hashlib
import itertools
import json
import math
from pathlib import Path
import re
import struct

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--catalog', type=Path, required=True, help='TitanstrikeTheftCatalog native TSV')
p.add_argument('--model', type=Path, required=True, help='Read-only CASC FileDataID1133115 export')
p.add_argument('--output', type=Path, required=True)
args = p.parse_args()
rows = list(csv.reader(args.catalog.read_text(encoding='utf-8-sig').splitlines(), delimiter='\t'))
floors = [dict(id=int(r[1]), x=float(r[2]), y=float(r[3]), z=float(r[4]),
               scale=float(r[5]), yaw=float(r[6]), pitch=float(r[7]), roll=float(r[8]))
          for r in rows if r and r[0] == 'WIND_FLOOR']
assert len(floors) == 8
assert {r['x'] for r in floors} == {0, 15, 25, 35}
assert all(abs(r['yaw']-math.pi/2) < 1e-5 and not r['pitch'] and not r['roll'] for r in floors)
raw = args.model.read_bytes()
m = raw[8:] if raw[:4] == b'MD21' else raw
assert m[:4] == b'MD20'
n, off = struct.unpack_from('<II', m, 8)
name = m[off:off+n].rstrip(b'\0').decode()
assert name == 'FelArakkoa_Windwrap_State'
minimum = struct.unpack_from('<3f', m, 160)
maximum = struct.unpack_from('<3f', m, 172)
assert -86 < minimum[0] < -85 and 66 < maximum[1] < 67
points = []
for a in floors:
    for x, y, z in itertools.product(*zip(minimum, maximum)):
        c, s = math.cos(a['yaw']), math.sin(a['yaw'])
        points.append((a['x']+a['scale']*(c*x-s*y),
                       a['y']+a['scale']*(s*x+c*y), a['z']+a['scale']*z))
span = (min(v[0] for v in points), max(v[0] for v in points))
assert -17 < span[0] < -16 and 51 < span[1] < 52
# Native two DB emitter positions and front-target radius15, not invented
# lengths of the visible mesh. Exact wall-clipped destinations/rendering remain
# a separate client check; these are nominal static local-space projections.
angle = 4.54211
emitters = [(365439, 4833.87, 293.2), (365440, 4840.41, 330.457)]
end_header = Path('src/server/scripts/Scenario/Artifacts/Paladin/ShieldsRest/shields_rest.h').read_text()
block = end_header.split('Position const windEndPos[3] =', 1)[1].split('};', 1)[0]
ends = [tuple(map(float, v)) for v in re.findall(r'\{([\d.]+)f, ([\d.]+)f, (-?[\d.]+)f\}', block)]
assert len(ends) == 3
endpoint_y = sum(v[1] for v in ends)/3
actor_block = end_header.split('Position const windshaperPos[3] =', 1)[1].split('};', 1)[0]
actors = [tuple(map(float, v)) for v in re.findall(r'\{([\d.]+)f, ([\d.]+)f, (-?[\d.]+)f\}', actor_block)]
assert len(actors) == 3
assemblies = []
for guid, x, y in emitters:
    center = (x+15*math.cos(angle), y+15*math.sin(angle))
    world = [(center[0]+px*math.cos(angle)-py*math.sin(angle),
              center[1]+px*math.sin(angle)+py*math.cos(angle)) for px, py, _ in points]
    assemblies.append(dict(guid=guid, nominal_front_target=center,
                           y_bounds=(min(v[1] for v in world), max(v[1] for v in world))))
assert assemblies[0]['y_bounds'][0] < endpoint_y-25
assert assemblies[1]['y_bounds'][0] > assemblies[0]['y_bounds'][0]+30
dx, dy = math.cos(angle), math.sin(angle)
_, primary_x, primary_y = emitters[1]
front_edge = 15+span[0]
npc_gaps = [front_edge-((x-primary_x)*dx+(y-primary_y)*dy) for x, y, _ in actors]
assert all(3 < gap < 6 for gap in npc_gaps), 'Upper native assembly must leave a small NPC-side gap'
result = dict(build='7.3.5.26972', model_file=1133115, model_name=name,
              model_sha256=hashlib.sha256(raw).hexdigest(), mesh_minimum=minimum, mesh_maximum=maximum,
              floor_attachments=floors, kit58494_local_x_bounds=span,
              nominal_assemblies=assemblies, tornado_endpoints=ends, npc_to_mesh_gaps=npc_gaps,
              repair='Keep upper365440 assembly; clear only lower365439 client visual fields before publication. Retain both gameplay force objects.',
              limits='Static native attachment/M2 bounds and nominal front-target projection; transparent mesh edges, collision-clipped cast destinations, visual fallback, final stair edge and NPC gap need client verification. No claim of exact pixel clipping or entire scenario verification.')
args.output.write_text(json.dumps(result, indent=2)+'\n', encoding='utf-8')
print('PASS: native full-corridor wind assembly; lower duplicate projects beyond tornado endpoints toward entrance stairs')
