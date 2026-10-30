"""Verify the registered native ritual controller and cleanup in the linked PE."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM, CS_OP_MEM, CS_OP_REG
from capstone.x86_const import X86_REG_RCX, X86_REG_EAX

previous = runpy.run_path(str(Path(__file__).with_name('verify_stromkar_ritual_beam_release.py')))
n, values = previous['n'], previous['values']
call = previous['p']['calls_prefix']
references = previous['p']['references']
receiver = '?Receiver@StromkarRitualTorture@@'
assert {201032, 1037, 103144, 370922}.issubset(values(receiver))
# GetCurrentSpell is an inline native field read. The source regression checks
# the actual current201032 channel; the linked receiver retains that ID gate.
call(receiver, '?FindNearestCreature@WorldObject@@')
for name in ('spell_stromkar_ritual_channel', 'spell_stromkar_ritual_torture'):
    assert (name+'\0').encode() in n['binary']
    update = '?Update@'+name+'@@'
    register = '?Register@'+name+'@@'
    assert n['symbol'](update) in references(register), (name, 'registered update')
    call(update, receiver)
channel = 'spell_stromkar_ritual_channel@@'
call('?Update@'+channel, '?CastSpell@Unit@@')
call('?Update@'+channel, '?Clear@'+channel)
assert {204125, 500}.issubset(values('?Update@'+channel))
call('?Clear@'+channel, '?RemoveAurasDueToSpell@Unit@@')
# PreventDefaultAction is inlined. Resolve the actual native boolean through
# its getter, then require this callback to store true into that same field.
ins = previous['p']['ins']
flag_reads = {x.operands[1].mem.disp for x in ins('?_IsDefaultActionPrevented@AuraScript@@')
              if x.mnemonic == 'movzx' and len(x.operands) == 2
              and x.operands[0].type == CS_OP_REG and x.operands[0].reg == X86_REG_EAX
              and x.operands[1].type == CS_OP_MEM and x.operands[1].size == 1
              and not x.operands[1].mem.index and x.operands[1].mem.disp > 8}
assert len(flag_reads) == 1, flag_reads
flag_offset = flag_reads.pop()
aliases = {X86_REG_RCX}
stores = []
for instruction in ins('?Periodic@'+channel):
    operands = instruction.operands
    if instruction.mnemonic == 'mov' and len(operands) == 2:
        if operands[0].type == CS_OP_REG and operands[1].type == CS_OP_REG and operands[1].reg in aliases:
            aliases.add(operands[0].reg)
        if (operands[0].type == CS_OP_MEM and operands[0].size == 1
                and operands[0].mem.base in aliases and operands[0].mem.disp == flag_offset
                and operands[1].type == CS_OP_IMM and operands[1].imm == 1):
            stores.append(instruction.address)
assert len(stores) == 1, 'Missing native default-periodic suppression flag store'
assert {n['symbol']('?Periodic@'+channel), n['symbol']('?Remove@'+channel)} <= references('?Register@'+channel)
# The source/native-aura regression checks the precise existing callback's
# per-caster bit. This verifies that its newly added ID is in the linked loader.
assert 204125 in values('?LoadSpellCustomAttr@SpellMgr@@')
print('PASS registered scoped ritual controllers, native204125 cast, missile replacement, own-aura cleanup and linked spell correction ID; continuous rendering and world target selection remain client checks')
