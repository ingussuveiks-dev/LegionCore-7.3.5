"""Validate a real scheduled compressed switch and reject unsafe byte copies.

No executable is written or launched. The original retained PetBattle switch
regression remains separate; this covers intervening register saves in Wind Stone.
"""
from five_ten_20261111_pe import ins, n, sites
from capstone import CS_OP_MEM
import struct

prefix = '?OnGossipSelect@go_wind_stone@'
rows = ins(prefix)
assert len(sites(rows, '?SummonNPC@')) == 1
table, = {n['base'] + o.mem.disp for x in rows for o in x.operands
          if o.type == CS_OP_MEM and o.mem.scale == 4 and o.mem.index
          and n['base'] + o.mem.disp > rows[-1].address}
assert rows[-1].address + rows[-1].size == table
remap, = [x for x in rows if x.mnemonic == 'movzx' and any(
    o.type == CS_OP_MEM and o.size == 1 and o.mem.index and n['base'] + o.mem.disp > table
    for o in x.operands)]
bound, = [x for x in rows if x.mnemonic == 'cmp' and x.op_str == 'eax, 0x6c']
assert rows.index(remap) - rows.index(bound) > 4, 'Must exercise the old four-instruction blind spot'
save = rows[rows.index(bound) + 2]
assert save.mnemonic == 'mov' and save.op_str.startswith('qword ptr [rsp') and save.size >= 2
original = n['binary']

def refuse(offset, replacement, reason):
    damaged = bytearray(original)
    damaged[offset:offset+len(replacement)] = replacement
    n['binary'] = bytes(damaged)
    ins.cache_clear()
    try:
        ins(prefix)
    except AssertionError as error:
        assert reason in str(error), str(error)
    else:
        raise AssertionError('Invalid switch accepted: ' + reason)
    finally:
        n['binary'] = original
        ins.cache_clear()

refuse(n['offset'](bound.address), b'\x90' * bound.size, 'unique nearby index bound')
refuse(n['offset'](save.address), b'\x31\xc0' + b'\x90' * (save.size - 2), 'checked index changed')
refuse(n['offset'](table) + 8, struct.pack('<I', table - n['base']), 'instruction boundary')
assert len(sites(ins(prefix), '?SummonNPC@')) == 1
assert n['binary'] == original
print('PASS scheduled Wind Stone switch; missing bound, clobbered index and data-target counterfactuals refused; original PE unchanged')
