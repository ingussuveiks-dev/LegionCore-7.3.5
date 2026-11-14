"""Inspect all23 native summon table entries; corrupt byte copies must fail."""
from five_ten_20261111_pe import ins, n
from sheilun_map_summon_pe import instructions
from capstone import CS_OP_MEM
import struct

rows = instructions(n)
tables = sorted({n['base'] + o.mem.disp for row in rows for o in row.operands
                 if o.type == CS_OP_MEM and o.mem.scale == 4 and o.mem.index
                 and n['base'] + o.mem.disp >= rows[-1].address + rows[-1].size})
assert len(tables) == 2
bound, = [x for x in rows if x.mnemonic == 'cmp' and x.op_str == 'eax, 5']
original = n['binary']
controls = [(n['offset'](table) + 8, struct.pack('<I', table - n['base'])) for table in tables]
controls.append((n['offset'](bound.address) + bound.size - 1, b'\x04'))
for offset, replacement in controls:
    try:
        damaged = bytearray(original)
        damaged[offset:offset+len(replacement)] = replacement
        n['binary'] = bytes(damaged)
        ins.cache_clear()
        try:
            instructions(n)
        except AssertionError:
            pass
        else:
            raise AssertionError('Invalid native summon table accepted')
    finally:
        n['binary'] = original
        ins.cache_clear()
assert instructions(n) and n['binary'] == original
print('PASS both native summon RVA tables/all23 targets; two data-target and one truncated-bound counterfactuals refused; original PE unchanged')
