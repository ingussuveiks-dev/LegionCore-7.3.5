"""Verify native warning speech delivery and retained immediate self-death."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM

previous = runpy.run_path(str(Path(__file__).with_name('verify_talonclaw_apata_voice_release.py')))
dialogue = previous['previous']
native = previous['native']
assert native['symbol']('?Kill@Unit@@QEAAXPEAV1@_NPEBVSpellInfo@@@Z') in dialogue['calls'](dialogue['branch'](37)), 'Native DIE no longer calls Unit::Kill'
action = dialogue['instructions']('?ProcessAction@SmartScript@@')
operations = {(entry.mnemonic, entry.op_str) for entry in action}
assert {('mov', 'eax, dword ptr [r15 + 0x10]'), ('cmp', 'eax, dword ptr [r15 + 0xc]'),
        ('cmp', 'dword ptr [rdx + 0xc], eax'), ('add', 'rdx, 0xa0')}.issubset(operations), 'Inlined LINK lookup no longer checks actual link/event ID and holder stride'
gate = next(index for index, entry in enumerate(action) if entry.mnemonic == 'cmp' and entry.op_str == 'dword ptr [rsp + 0x7d0], 0x3d')
assert action[gate + 1].mnemonic == 'jne', 'Inlined lookup does not reject non-LINK61 events'
assert any(entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM
           and native['resolve'](entry.operands[0].imm) == native['symbol']('?ProcessEvent@SmartScript@@')
           for entry in action[gate + 2:gate + 18]), 'Admitted LINK no longer reaches actual event dispatcher'
print('PASS: deployed native TALK publishes localized voice/chat and retained DIE37 reaches Unit::Kill through actual LINK resolution; no scene playback or sound duration claim')
