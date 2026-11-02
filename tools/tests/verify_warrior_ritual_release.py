"""Native933 real ascension receipts and original instance-owned ritual."""
from pathlib import Path
import hashlib,json,runpy
from warrior_ritual_pe import verify_state
p=runpy.run_path(str(Path(__file__).with_name('verify_warrior_opening_release.py')))
n,ins,refs,table,resolved,reachable=(p[key] for key in ('n','ins','refs','table','resolved','reachable'))
verify_state(n,ins,refs,table,resolved,reachable)
print('PASS actual registered Danica native205287 hit bridge plus linked instance-owned genuine death/original weak quest/native933 phase2 prerequisites, original corpses and normal205295/205296 true owned full-GUID spirit receipts. Native1405 true factory/two original actors, normal5s self cast, missing-only two-leaf49203/49262, actual2s departure/owned ghosts/native97166 text and original Ragnvald+Bezzeredes stage3 handoff retained. No nearest-player or synthetic quest/item/teleport grant. Native/component evidence differs from client/full933 traversal.')
print(json.dumps({'exe_sha256':hashlib.sha256(n['binary']).hexdigest(),'native_events':[49203,49262],'normal_spells':[205287,205295,205296],'conversation':1405,'instance_offset':0x3d8}))
