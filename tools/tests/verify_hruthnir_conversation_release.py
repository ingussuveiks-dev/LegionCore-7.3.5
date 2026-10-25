"""Check the installed creature-bound duration accumulator and viewer deadlines."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_titanstrike_finale_conversation_release.py')))
producer = previous['instructions']('?GenerateDynamicFieldData@Conversation@@')
reads = [index for index, entry in enumerate(producer)
         if entry.mnemonic == 'mov' and entry.op_str == 'eax, dword ptr [r14 + 0x20]']
creature_reads = [index for index in reads
                  if any(entry.mnemonic == 'add' and entry.op_str == 'r14, 0x28'
                         for entry in producer[index + 1:index + 6])]
assert len(creature_reads) == 1, 'Reinspect native40-byte ConversationCreature duration loop'
position = creature_reads[0]
assert producer[position + 1].mnemonic == 'test' and producer[position + 1].op_str == 'eax, eax'
assert producer[position + 2].mnemonic == 'je'
assert producer[position + 3].mnemonic == 'add' and producer[position + 3].op_str == 'dword ptr [r13], eax'
assert any(entry.mnemonic == 'jmp' for entry in producer[position + 5:position + 10]), 'Creature accumulation no longer repeats through its native vector'
assert any(entry.mnemonic == 'mov' and entry.op_str == 'eax, dword ptr [r14 + 0x18]'
           for entry in producer[max(0, position - 45):position]), 'Native creature flags not serialized before duration'
native = previous['native']
constructor = previous['instructions']('??0Conversation@@')
tables = [entry.address + entry.size + entry.operands[1].mem.disp
          for index, entry in enumerate(constructor[:-1])
          if entry.mnemonic == 'lea' and entry.op_str.startswith('rax, ')
          and entry.operands[1].type == CS_OP_MEM and entry.operands[1].mem.base == X86_REG_RIP
          and constructor[index + 1].op_str == 'qword ptr [rdi], rax']
assert len(tables) == 1, 'Actual primary Conversation constructor table changed'
target = struct.unpack_from('<Q', native['binary'], native['offset'](tables[0] + 0x168))[0]
assert native['resolve'](target) == native['symbol']('?CanNeverSee2@Conversation@@'), 'Constructed Conversation does not select the audited viewer deadline'
visibility = previous['instructions']('?canSeeOrDetect@WorldObject@@')
dispatch = [index for index, entry in enumerate(visibility)
            if entry.mnemonic == 'call' and entry.op_str == 'qword ptr [rax + 0x168]']
assert len(dispatch) == 1
position = dispatch[0]
assert [entry.op_str for entry in visibility[position - 3:position]] == [
    'rax, qword ptr [rsi]', 'rdx, rdi', 'rcx, rsi'], 'Visibility no longer dispatches object.CanNeverSee2(seer)'
assert visibility[position + 1].mnemonic == 'test' and visibility[position + 1].op_str == 'al, al'
assert visibility[position + 2].mnemonic == 'jne'
rejection = next(entry for entry in visibility if entry.address == visibility[position + 2].operands[0].imm)
assert rejection.mnemonic == 'xor' and rejection.op_str == 'al, al', 'Expired viewer does not return false from real visibility admission'
print('PASS: actual Release creature-bound slot duration field32/stride40 accumulates into factory budget; native30s fallback/5s padding, per-player deadline and scope-expiry consumers verified; no NPC-caster cleanup repair asserted')
print('PASS: constructed Conversation primary slot0x168 selects CanNeverSee2; actual WorldObject visibility dispatch passes obj/seer and returns false on an expired viewer')
