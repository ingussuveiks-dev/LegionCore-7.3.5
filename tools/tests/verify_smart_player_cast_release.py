"""Check the opt-in caster and delayed eligibility paths in the deployed PE."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM, CS_OP_MEM, CS_OP_REG

previous = runpy.run_path(str(Path(__file__).with_name('verify_smart_invoker_player_release.py')))
native = previous['native']
instructions = previous['instructions']
action = instructions('?ProcessAction@SmartScript@@')
legacy = native['symbol']('?GetScriptCreditCaster@')
calls = [index for index, entry in enumerate(action) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM and native['resolve'](entry.operands[0].imm) == legacy]
assert len(calls) == 1, 'Legacy credit-caster branch changed'
branch = action[calls[0] - 35:calls[0]]
assert any(entry.mnemonic == 'test' and any(operand.type == CS_OP_IMM and operand.imm == 128 for operand in entry.operands) for entry in branch), 'Explicit0x80 opt-in gate missing'
assert any(entry.mnemonic == 'cmp' and any(operand.type == CS_OP_IMM and operand.imm == 130 for operand in entry.operands) for entry in branch), 'Player caster accepts an unbounded target'
assert any(entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM and native['resolve'](entry.operands[0].imm) == native['symbol']('?GetScriptCreditPlayer@') for entry in branch), 'Opt-in caster does not resolve the player/owner'
mapping = (native['args'].release / 'worldserver.map').read_text(errors='replace')
callbacks = []
for row in mapping.splitlines():
    fields = row.split()
    if len(fields) > 3 and fields[1].startswith('?_Do_call') and 'ProcessAction@SmartScript' in fields[1]:
        if native['symbol']('?GetWorldObject@ObjectAccessor@@') in native['calls'](fields[1]):
            callbacks.append(fields[1])
assert len(callbacks) == 1, 'Missing or ambiguous deferred source-GUID callback'
callback = callbacks[0]
for prefix in ['?GetUnit@ObjectAccessor@@', '?GetWorldObject@ObjectAccessor@@', '?IsInWorld@Object@@',
               '?InSamePhase@WorldObject@@', '?IsWithinDistInMap@WorldObject@@', '?CanTargetQuestObject@WorldObject@@',
               '?CastSpell@Unit@@QEAA?AW4SpellCastResult@@PEAV1@I_NPEAVItem@@PEBVAuraEffect@@VObjectGuid@@']:
    assert native['symbol'](prefix) in native['calls'](callback), f'Missing deferred cast validation {prefix}'
body = instructions(callback)
assert any(entry.mnemonic == 'cmp' and any(operand.type == CS_OP_MEM and operand.mem.disp == 8 for operand in entry.operands) for entry in body), 'Deferred map equality missing'
assert any(entry.mnemonic == 'cmp' and len(entry.operands) == 2 and all(operand.type == CS_OP_REG for operand in entry.operands) for entry in body), 'Deferred self-target identity guard missing'
assert b'player invoker cast requires a bounded player-invoker target' in native['binary'], 'Loader rejection diagnostic missing'
validation = instructions('?IsEventValid@SmartAIMgr@@')
assert any(entry.mnemonic == 'test' and any(operand.type == CS_OP_IMM and operand.imm == 128 for operand in entry.operands) for entry in validation), 'Loader opt-in validation missing'
print('PASS: actual Release retains legacy caster branch, explicit0x80 player/owner normalization with target130 gate, and deferred source GUID/world/map/phase/distance/scope/self-target checks before native CastSpell')
