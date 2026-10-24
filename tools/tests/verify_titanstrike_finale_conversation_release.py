"""Inspect actual actor-duration production and per-viewer deadline consumption."""
from pathlib import Path
import runpy

previous = runpy.run_path(str(Path(__file__).with_name('verify_conversation_farewell_release.py')))
native = previous['native']
instructions = previous['instructions']
producer = instructions('?GenerateDynamicFieldData@Conversation@@')
actor_reads = [index for index, entry in enumerate(producer)
               if entry.mnemonic == 'mov' and entry.op_str == 'eax, dword ptr [r14 + 0x20]']
assert any(
    any(entry.mnemonic == 'add' and entry.op_str == 'dword ptr [r13], eax'
        for entry in producer[index + 1:index + 5]) and
    any(entry.mnemonic == 'add' and entry.op_str == 'r14, 0x24'
        for entry in producer[index + 1:index + 6]) and
    any(entry.mnemonic == 'jne' for entry in producer[index + 1:index + 9])
    for index in actor_reads
), 'Native36-byte actor loop no longer accumulates duration field32'
viewer_name = '?CanNeverSee2@Conversation@@'
viewer = instructions(viewer_name)
assert native['symbol']('?GetUInt32Value@Object@@') in native['calls'](viewer_name), 'Per-viewer deadline read missing'
assert any(entry.mnemonic == 'cmp' and entry.op_str == 'byte ptr [rdx + 0x16], 4' for entry in viewer), 'Non-player visibility rejection changed'
assert any(entry.mnemonic == 'mov' and entry.op_str == 'edx, 0xc' for entry in viewer), 'Per-viewer comparison reads wrong field'
comparisons = [index for index, entry in enumerate(viewer) if entry.mnemonic == 'cmp' and entry.op_str == 'edi, eax']
assert any(viewer[index + 1].mnemonic == 'seta' and
           any(entry.mnemonic == 'sub' and entry.op_str == 'edi, ebx' for entry in viewer[max(0, index - 8):index])
           for index in comparisons), 'Per-viewer elapsed/deadline comparison changed'
print('PASS: actual Release sums native actor duration field32 at stride36 and rejects non-player viewers; real per-player elapsed time consumes last-line field12 independently of NPC-caster expiry')
