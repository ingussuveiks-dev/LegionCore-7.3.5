"""Inspect native ritual M2 assets and the native-reader export; no render claim.

Input models are read-only CASC exports from client 7.3.5.26972. Neither those
assets nor runtime exports belong in Git. Invoke through runtime diagnostics.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--models', type=Path, required=True)
p.add_argument('--native', type=Path, required=True)
p.add_argument('--output', type=Path, required=True)
a = p.parse_args()
rows = [line.split('\t') for line in a.native.read_text(encoding='utf-8-sig').splitlines()]

def has(*values):
    return [str(v) for v in values] in rows

assert has('EFFECT', 201031, 0, 3, 0, 0, 0, 106, 0)
assert has('EFFECT', 201032, 0, 6, 23, 0, 201031, 1, 0)
assert has('RITUAL_PERIOD', 201032, 5000)
assert has('VISUAL', 201031, 52341, 0, 0)
assert has('VISUAL', 201032, 48829, 0, 0)
assert has('EVENT', 48829, 11, 12, 57703)
assert has('MISSILE_MODEL', 52341, 386, 387, 516, 2042, 167229, 255, 34)
assert has('VISUAL', 204093, 53374, 0, 0)
assert has('EVENT', 53374, 7, 8, 64474)

expected = {
    167229: ('Zig_Missile', 4, 0),
    953311: ('Void_Precast_Hand', 0, 5),
    953324: ('Void_SummonPet_Impact_Base', 0, 3),
    953314: ('Void_Shield_State_Base', 0, 2),
}
models = []
for file_id, (name, ribbons, particles) in expected.items():
    raw = (a.models / f'File{file_id:08X}.bin').read_bytes()
    if raw[:4] == b'MD21':
        chunk_size, = struct.unpack_from('<I', raw, 4)
        assert 304 <= chunk_size <= len(raw) - 8
        data = raw[8:8+chunk_size]
    else:
        data = raw
    assert data[:4] == b'MD20' and len(data) >= 304
    def array(offset, size):
        count, start = struct.unpack_from('<II', data, offset)
        assert not count or (304 <= start and start + count*size <= len(data))
        return count, start
    count, start = array(8, 1)
    actual_name = data[start:start+count].rstrip(b'\0').decode('ascii')
    assert actual_name == name, (file_id, actual_name)
    count, start = array(28, 64)
    sequences = []
    for i in range(count):
        anim, variation, duration, speed, flags = struct.unpack_from('<HHIfI', data, start+i*64)
        sequences.append(dict(animation=anim, variation=variation, duration_ms=duration,
                              movement_speed=speed, flags=flags))
    # Only read counts here: emitter record layouts are not interchangeable.
    actual_ribbons, = struct.unpack_from('<I', data, 288)
    actual_particles, = struct.unpack_from('<I', data, 296)
    assert (actual_ribbons, actual_particles) == (ribbons, particles), (file_id, actual_ribbons, actual_particles)
    models.append(dict(file_data_id=file_id, name=name, ribbons=ribbons, particles=particles,
                       sequences=sequences, sha256=hashlib.sha256(raw).hexdigest()))

report = dict(build='7.3.5.26972', models=models,
              native_export_sha256=hashlib.sha256(a.native.read_bytes()).hexdigest(),
              result='PASS native metadata and asset topology', client_rendering_verified=False,
              limits='The authored periodic missile, hand/base effects and receiver shield are identified. '
                     'This does not establish the missing continuous channel producer, exact rendering, '
                     'or that another native visual is a compatible substitute. No production fix follows '
                     'from model names or the absence of a direct BeamEffect alone.')
a.output.write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
print('PASS native ritual missile/hand/base/shield identities and bounded M2 topology; rendering remains unverified')
