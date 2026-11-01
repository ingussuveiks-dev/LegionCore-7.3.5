"""Verify the linked temple factory no longer installs the raw death callback."""
from pathlib import Path
import runpy
import re
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
maptext=(n['args'].release/'worldserver.map').read_text(errors='replace')
name='??_7Instance@FuZanWaveEntry@@6BInstanceScript@@@'
tables=re.findall(r'\s+\w+:\w+\s+'+re.escape(name)+r'\s+([0-9a-fA-F]{16})',maptext)
if tables:
    assert len(tables)==1
    table=int(tables[0],16)
    assert any(op.mnemonic=='lea' and any(operand.type==CS_OP_MEM and operand.mem.base==X86_REG_RIP and
        op.address+op.size+operand.mem.disp==table for operand in op.operands) for op in factory),'Factory must install the actual wave-entry vtable'
    slots=[n['resolve'](address) for address in struct.unpack_from('<51Q',n['binary'],n['offset'](table))]
    assert slots[5]==n['symbol']('?CreatureDies@InstanceScript@@')
    assert slots[15]==n['symbol']('?OnUnitDeath@ZoneScript@@')
    assert not any(name.startswith(('?OnUnitDeath@Instance@FuZanWaveEntry@@','?CreatureDies@Instance@FuZanWaveEntry@@')) for name in n['symbols'])
    print('PASS linked Fu Zan1625 registered factory installs wave-entry Instance; actual vtable retains native CreatureDies and ZoneScript OnUnitDeath slots, with no obsolete raw32-death callback. Native51-row SmartAI/weighted criteria and client traversal are separate checks.')
else:
    print('PASS linked Fu Zan1625 registered factory constructs native InstanceScript; obsolete raw32-death callback absent. Native51-row SmartAI/weighted criteria and client traversal are separate checks.')
