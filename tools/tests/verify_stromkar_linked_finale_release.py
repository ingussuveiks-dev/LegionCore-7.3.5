"""Check the existing cast-to-linked-spell bridge, not a new script substitute."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM

previous = runpy.run_path(str(Path(__file__).with_name('verify_stromkar_execute_landing_release.py')))
native = previous['native']
instructions = previous['instructions']
linked = '?LinkedSpell@Spell@@'
assert native['symbol'](linked) in native['calls']('?cast@Spell@@'), 'Ordinary cast omits linked dispatch'
casts = {address for name, address in native['symbols'].items() if name.startswith('?CastSpell@Unit@@')}
assert casts & native['calls'](linked), 'Linked dispatcher does not request Unit spells'
body = instructions('?cast@Spell@@')
immediate = native['symbol']('?_handle_immediate_phase@Spell@@')
bridge = native['symbol'](linked)
immediate_at = [index for index, entry in enumerate(body) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM and native['resolve'](entry.operands[0].imm) == immediate]
linked_at = [index for index, entry in enumerate(body) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM and native['resolve'](entry.operands[0].imm) == bridge]
assert len(immediate_at) == 1 and any(index > immediate_at[0] for index in linked_at), 'Reinspect immediate effects before type0 link dispatch'
assert any(index > immediate_at[0] and any(entry.mnemonic == 'xor' and entry.op_str == 'r9d, r9d' for entry in body[max(0, index - 8):index]) for index in linked_at), 'Post-effect linked dispatch is not type0'
print('PASS: canonical Spell::cast retains immediate effects then linked dispatch, and native LinkedSpell requests Unit casts. Actual201904 binding/positive default branch and target alternatives are separate checks, not a completed hit/list/actor/VFX traversal')
