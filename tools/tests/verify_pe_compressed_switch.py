"""Prove the emitted inline switch boundary and reject a corrupt third RVA.

The counterfactual modifies an in-memory byte copy only; no executable is
written or launched. Invoke with the Release directory containing its map.
"""
from five_ten_20261111_pe import ins, n, sites
from capstone import CS_OP_MEM
import struct

prefix = '?Apply@PetBattleAura@@'
rows = ins(prefix)
assert len(sites(rows, '?RefreshPetTypeOverride@PetBattle@@')) == 1
tables = [n['base'] + operand.mem.disp for row in rows
          for operand in row.operands if operand.type == CS_OP_MEM
          and operand.mem.scale == 4 and operand.mem.index
          and n['base'] + operand.mem.disp > rows[-1].address]
assert len(tables) == 1, 'Expected the actual compressed Apply switch'
table, = tables
assert rows[-1].address + rows[-1].size == table
assert rows[-2].mnemonic == 'jmp', 'This regression exercises a tail-call epilogue'
assert rows[-2].operands[0].imm - n['base'] in n['starts']
original = n['binary']
try:
    damaged = bytearray(original)
    struct.pack_into('<I', damaged, n['offset'](table) + 8, table - n['base'])
    n['binary'] = bytes(damaged)
    ins.cache_clear()
    try:
        ins(prefix)
    except AssertionError as error:
        assert 'instruction boundary' in str(error), str(error)
    else:
        raise AssertionError('A switch target pointing into data was accepted')
finally:
    n['binary'] = original
    ins.cache_clear()
assert len(sites(ins(prefix), '?RefreshPetTypeOverride@PetBattle@@')) == 1
print('PASS actual compressed switch/tail-call code boundary; expected-broken in-memory third target rejected; original PE unchanged')
