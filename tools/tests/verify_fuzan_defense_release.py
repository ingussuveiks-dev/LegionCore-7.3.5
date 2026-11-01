"""Verify the linked temple factory no longer installs the raw death callback."""
from pathlib import Path
import runpy
import struct

from capstone import CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
from sharasdal_erunak_entry_pe import sites, values

n = runpy.run_path(str(Path(__file__).with_name('verify_smart_linked_cast_retry_release.py')))
prefix = '?GetInstanceScript@instance_temple_of_the_jade_serpent@@'
factory = n['instructions'](prefix)
assert len(sites(n, factory, '??0InstanceScript@@')) == 1
assert not sites(n, factory, '?SetData@SmartAI@@')
assert not any('instance_temple_of_the_jade_serpent_InstanceMapScript' in name for name in n['symbols']), 'Obsolete derived instance still linked'
registration = n['instructions']('?AddSC_instance_temple_of_the_jade_serpent@@')
assert 1625 in values(registration)
assert len(sites(n, registration, '??0InstanceMapScript@@')) == 1
# The actual registered object's installed vtable must contain this factory.
installed = []
for instruction in registration:
    if instruction.mnemonic != 'lea':
        continue
    for operand in instruction.operands:
        if operand.type == CS_OP_MEM and operand.mem.base == X86_REG_RIP:
            address = instruction.address + instruction.size + operand.mem.disp
            at = n['offset'](address)
            installed.extend(struct.unpack_from('<16Q', n['binary'], at))
assert n['symbol'](prefix) in {n['resolve'](address) for address in installed if n['base'] <= address < n['base'] + 0x10000000}
print('PASS linked Fu Zan1625 factory: registered vtable constructs native InstanceScript; obsolete raw32-death derived callback absent. Native51-row SmartAI/weighted criteria dialogue and client traversal are separate checks.')
