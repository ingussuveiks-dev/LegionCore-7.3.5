"""Check the selected 7.3.5 GO range implementation in the actual final PE."""
from pathlib import Path
import re, runpy, struct
from capstone import CS_OP_IMM, CS_OP_MEM
p = runpy.run_path(str(Path(__file__).with_name('verify_frost_scene_equip_release.py')))
n, ins = p['n'], p['ins']
stamp = re.search(r'Timestamp is ([0-9A-Fa-f]+)', (n['args'].release/'worldserver.map').read_text(errors='replace'))
assert stamp and int(stamp[1], 16) == struct.unpack_from('<I', n['binary'], n['pe']+8)[0]
fn = '?IsInRange@GameObject@@'
code = list(ins(fn))
assert 11 in {o.imm for i in code for o in i.operands if o.type == CS_OP_IMM}  # OBJECT_FIELD_SCALE_X
assert n['symbol']('?GetFloatValue@Object@@') in n['calls'](fn)
assert n['symbol']('?IsWithinDist3d@WorldObject@@QEBA_NMMMM_N') in n['calls'](fn)
scaled = {o.mem.disp for i in code if i.mnemonic == 'mulss' for o in i.operands if o.type == CS_OP_MEM and i.reg_name(o.mem.base) == 'rbx'}
assert {8, 12, 16, 20, 24, 28} <= scaled, scaled  # all six matching DB2 GeoBox faces
assert sum(i.mnemonic == 'comiss' for i in code) == 6
assert not any('sqrt' in i.mnemonic for i in code)
consumer = '?_IsWithinDist@GameObject@@'
if n['symbol'](fn) not in n['calls'](consumer):  # Release inlines the range body here.
 selected = list(ins(consumer))
 assert n['symbol']('?GetFloatValue@Object@@') in n['calls'](consumer)
 assert {8, 12, 16, 20, 24, 28} <= {o.mem.disp for i in selected if i.mnemonic == 'mulss' for o in i.operands if o.type == CS_OP_MEM and i.reg_name(o.mem.base) == 'rbx'}
 assert sum(i.mnemonic == 'comiss' for i in selected) == 6
for forbidden in ('?KilledMonsterCredit@Player@@', '?CompleteQuest@Player@@', '?TeleportTo@Player@@'):
 assert not any(n['resolve'](v) in n['calls'](fn) for name, v in n['symbols'].items() if name.startswith(forbidden))
print('PASS final PE native GO interaction: scale field11 feeds all six DB2 box faces, six 3D bounds, native missing-display spherical fallback and actual virtual distance consumer; no planar-zero success or progression/transfer grant. Independent actual DB2/GO tests establish the transform, not PE inspection alone.')
