"""Verify both deployed consumers reject zero/oversized phases before their bit test."""
from pathlib import Path
import re
import runpy
from capstone import CS_OP_IMM, CS_OP_MEM, CS_OP_REG

previous = runpy.run_path(str(Path(__file__).with_name('verify_orestes_farewell_release.py')))
header = (Path(__file__).resolve().parents[2] / 'src/server/game/AI/SmartScripts/SmartScriptMgr.h').read_text()
count = int(re.search(r'SMART_EVENT_PHASE_COUNT\s*=\s*(\d+)', header)[1])
for name in ['?ProcessEvent@SmartScript@@', '?UpdateTimer@SmartScript@@']:
    body = previous['instructions'](name)[:70]
    positions = [index for index, entry in enumerate(body) if entry.mnemonic == 'mov' and len(entry.operands) == 2 and entry.operands[0].type == CS_OP_REG and entry.operands[1].type == CS_OP_MEM and entry.operands[1].mem.disp == 0xac]
    assert len(positions) == 1, f'Actual phase read missing in {name}'
    position = positions[0]
    register = body[position].operands[0].reg
    decrement, compare, reject, bit_test, no_match = body[position + 1:position + 6]
    assert decrement.mnemonic == 'dec' and decrement.operands[0].reg == register
    assert compare.mnemonic == 'cmp' and compare.operands[0].reg == register and compare.operands[1].type == CS_OP_IMM and compare.operands[1].imm == count - 1, f'Missing unsigned0..{count - 1} phase-index bound'
    assert reject.mnemonic == 'ja' and reject.operands[0].type == CS_OP_IMM, 'Invalid phase is not rejected before bit selection'
    assert bit_test.mnemonic == 'bt' and bit_test.operands[1].reg == register
    assert no_match.mnemonic == 'jae' and no_match.operands[0].imm == reject.operands[0].imm, 'Invalid phase does not reach the same rejection as an unmatched mask'
    assert any(entry.mnemonic == 'je' and entry.operands[0].type == CS_OP_IMM and entry.operands[0].imm == body[position + 6].address for entry in body[max(0, position - 3):position]), 'Native mask0 always-active bypass changed'
print('PASS: both canonical event and timer consumers bound the actual phase index before BT, reject reset/oversized phases and preserve the native mask0 bypass')
