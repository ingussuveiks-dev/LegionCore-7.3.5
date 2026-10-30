"""Verify native warning speech delivery and retained immediate self-death."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM, CS_OP_MEM, CS_OP_REG
from capstone.x86_const import X86_REG_RDX, X86_REG_RSP

previous = runpy.run_path(str(Path(__file__).with_name('verify_talonclaw_apata_voice_release.py')))
dialogue = previous['previous']
native = previous['native']
assert native['symbol']('?Kill@Unit@@QEAAXPEAV1@_NPEBVSpellInfo@@@Z') in dialogue['calls'](dialogue['branch'](37)), 'Native DIE no longer calls Unit::Kill'
action = dialogue['instructions']('?ProcessAction@SmartScript@@')
operations = {(entry.mnemonic, entry.op_str) for entry in action}
assert {('mov', 'eax, dword ptr [r15 + 0x10]'), ('cmp', 'eax, dword ptr [r15 + 0xc]'),
        ('cmp', 'dword ptr [rdx + 0xc], eax'), ('add', 'rdx, 0xa0')}.issubset(operations), 'Inlined LINK lookup no longer checks actual link/event ID and holder stride'
# Derive event.type from the actual dispatcher's initial active/LINK guard.
# The caller's copied holder can move within its stack frame after recompiling.
dispatcher = native['symbol']('?ProcessEvent@SmartScript@@')
prefix = dialogue['instructions']('?ProcessEvent@SmartScript@@')[:64]
fields = [(i, x.operands[0].mem.disp) for i, x in enumerate(prefix)
          if x.mnemonic == 'cmp' and len(x.operands) == 2
          and x.operands[0].type == CS_OP_MEM and x.operands[0].size == 4
          and x.operands[0].mem.base == X86_REG_RDX
          and x.operands[1].type == CS_OP_IMM and x.operands[1].imm == 61]
assert len(fields) == 1, 'Reinspect native active/LINK event field'
field_at, event_offset = fields[0]
assert prefix[field_at - 2].mnemonic == 'cmp' and prefix[field_at - 2].operands[0].size == 1
assert prefix[field_at - 1].mnemonic == 'jne' and prefix[field_at + 1].mnemonic == 'jne'
assert prefix[field_at - 1].operands[0].imm == prefix[field_at + 2].address

gates = []
for i, entry in enumerate(action):
    if not (entry.mnemonic == 'cmp' and len(entry.operands) == 2
            and entry.operands[0].type == CS_OP_MEM and entry.operands[0].size == 4
            and entry.operands[0].mem.base == X86_REG_RSP
            and entry.operands[1].type == CS_OP_IMM and entry.operands[1].imm == 61):
        continue
    if action[i + 1].mnemonic != 'jne':
        continue
    window = action[i + 2:i + 22]
    calls = [(j, x) for j, x in enumerate(window) if x.mnemonic == 'call'
             and x.operands[0].type == CS_OP_IMM and native['resolve'](x.operands[0].imm) == dispatcher]
    if len(calls) != 1:
        continue
    j, call = calls[0]
    pointers = [(k, x.operands[1].mem.disp) for k, x in enumerate(window[:j])
                if x.mnemonic == 'lea' and x.operands[0].type == CS_OP_REG
                and x.operands[0].reg == X86_REG_RDX and x.operands[1].type == CS_OP_MEM
                and x.operands[1].mem.base == X86_REG_RSP]
    if not pointers:
        continue
    k, holder = pointers[-1]
    assert all(X86_REG_RDX not in x.regs_access()[1] for x in window[k + 1:j]), 'Holder argument overwritten'
    if entry.operands[0].mem.disp != holder + event_offset:
        continue
    assert action[i + 1].operands[0].type == CS_OP_IMM and action[i + 1].operands[0].imm > call.address
    gates.append((holder, event_offset))
assert len(gates) == 1, 'Actual copied holder type61 gate must dominate the same holder passed to native ProcessEvent'
print('PASS: deployed native TALK publishes localized voice/chat and retained DIE37 reaches Unit::Kill through actual LINK resolution; no scene playback or sound duration claim')
