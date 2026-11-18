"""Verify nearest-hit model and object-tree traversal in the final x64 PE."""
from pathlib import Path
import runpy
from sharasdal_erunak_entry_pe import sites

n = runpy.run_path(str(Path(__file__).with_name('verify_smart_linked_cast_retry_release.py')))
closest = n['instructions']('??R?$MDLCallback@UDynamicTreeIntersectionCallback@@')
los = n['instructions']('??R?$MDLCallback@UDynamicTreeisInLineOfSightCallback@@')
ray, = sites(n, closest, '?intersectRay@GameObjectModel@@')
any_hit, = sites(n, los, '?intersectRay@GameObjectModel@@')
assert any(x.mnemonic == 'xor' and x.op_str == 'r9d, r9d' for x in closest[ray-8:ray])
assert any(x.mnemonic == 'mov' and x.op_str == 'r9b, 1' for x in los[any_hit-8:any_hit])

height = n['instructions']('?getHeight@DynamicMapTree@@')
node, = sites(n, height, '??$intersectRay@U?$MDLCallback@UDynamicTreeIntersectionCallback@@')
assert any(x.mnemonic == 'mov' and x.op_str == 'byte ptr [rsp + 0x20], 0' for x in height[node-8:node])

contact = n['instructions']('?getObjectHitPos@DynamicMapTree@@')
grid, = sites(n, contact, '??$intersectRay@UDynamicTreeIntersectionCallback@@@?$RegularGrid2D@')
assign, = [i for i in range(grid-8, grid) if contact[i].mnemonic == 'mov' and contact[i].op_str == 'byte ptr [rsp + 0x28], al']
# The lock succeeds with EAX=0. That tested value supplies the false traversal
# flag; the error branch leaves this ordinary nearest-contact path.
test, branch = contact[assign-4:assign-2]
assert test.mnemonic == 'test' and test.op_str == 'eax, eax' and branch.mnemonic == 'jne'
print('PASS linked nearest-surface collision: closest triangle for height/contact, false first-object traversal at height and contact callers, unchanged any-hit LOS')
