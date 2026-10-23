"""Reject stale exact-step admission after the native deferred pickup cast."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM

previous = runpy.run_path(str(Path(__file__).with_name('verify_talonclaw_dakarr_release.py')))
instructions = previous['instructions']('?AfterApply@spell_talonclaw_presentation@@')
bounds = {}
for current, following in zip(instructions, instructions[1:]):
    if current.mnemonic != 'cmp' or len(current.operands) != 2 or current.operands[1].type != CS_OP_IMM:
        continue
    expected = {12: 'jb', 13: 'ja'}.get(current.operands[1].imm)
    if expected and following.mnemonic == expected and following.operands[0].type == CS_OP_IMM:
        bounds[current.operands[1].imm] = following.operands[0].imm
assert bounds.keys() == {12, 13} and bounds[12] == bounds[13], 'Linked scene admission does not accept exactly native pickup/completed steps12/13'
print('PASS: actual Release admits deferred Talonclaw scene at12/13 and rejects both earlier and invalid later stages through the same guard')
