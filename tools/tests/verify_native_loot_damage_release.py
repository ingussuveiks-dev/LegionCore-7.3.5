from linked_player_abi import inherited_unit_delta
"""Check the emitted 64-bit input clamp at a raid caller and core damage path."""
from pathlib import Path
import re
import runpy

p = runpy.run_path(str(Path(__file__).with_name('verify_tsulong_transition_release.py')))
ins, sites, values = p['ins'], p['sites'], p['values']

def has(rows, mnemonic, operands):
    return any(x.mnemonic == mnemonic and x.op_str == operands for x in rows)

freya = ins('?EnterCombat@boss_freyaAI@boss_freya@@')
stores = [(i, re.fullmatch(r'dword ptr \[rbx \+ (0x[0-9a-f]+)\], eax', x.op_str))
          for i, x in enumerate(freya) if x.mnemonic == 'mov']
stores = [(i, m[1]) for i, m in stores if m and has(freya[max(0, i-9):i], 'cmovb', 'eax, ecx')]
assert len(stores) == 1
at, member = stores[0]
assert int(member, 16) == 14776+inherited_unit_delta(), 'Actual native Creature counter offset; update only with a native layout regression'
window = freya[at-13:at]
assert 0x40 in values(window) and sites(window, '?GetUInt64Value@Object@@'), 'Full native maxhealth argument'
for m, s in [('mov', 'r8d, dword ptr [rbx + '+member+']'), ('not', 'ecx'),
             ('mov', 'edx, ecx'), ('cmp', 'rdx, rax'), ('cmovb', 'eax, ecx'), ('add', 'eax, r8d')]:
    assert has(window, m, s), (m, s)

damage = ins('?DealDamage@Unit@@')
stores = [i for i, x in enumerate(damage) if x.mnemonic == 'mov' and x.op_str == 'dword ptr [rbx + '+member+'], r8d']
assert len(stores) == 1
window = damage[stores[0]-12:stores[0]]
for m, s in [('cmovb', 'r8, r15'), ('mov', 'edx, dword ptr [rbx + '+member+']'),
             ('not', 'eax'), ('mov', 'ecx, eax'), ('cmp', 'rcx, r8'), ('cmovb', 'r8d, eax'), ('add', 'r8d, edx')]:
    assert has(window, m, s), (m, s)
print('PASS linked native loot counter: current native Creature member'+str(int(member,16))+', full64 native maxhealth input at Freya, UINT32_MAX-current remaining room, unsigned64 comparison/clamp before32bit addition at both raid and core min(health,damage) paths. Complete Tsulong/Rook/prior367 checks retained; native API cases and client traversal are separate evidence.')
