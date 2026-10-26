"""Check selected aura registration, original delays and reported-cast retry."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_ashbringer_pickup_admission_release.py')))
native, instructions = previous['native'], previous['instructions']
scope = 'spell_bs_dominate_mind_AuraScript@spell_bs_dominate_mind@@'
update = '?OnUpdate@' + scope
register = '?Register@' + scope
factory = '?GetAuraScript@spell_bs_dominate_mind@@'
body = instructions(update)


def calls(prefix):
    targets = {address for name, address in native['symbols'].items() if name.startswith(prefix)}
    return [index for index, entry in enumerate(body) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM
            and native['resolve'](entry.operands[0].imm) in targets]


timer = next(index for index, entry in enumerate(body) if entry.op_str == 'word ptr [rcx + 0x362], 0')
assert body[timer + 1].mnemonic == 'je'
exit_address = body[timer + 1].operands[0].imm
caster, = calls('?GetCaster@AuraScript@@')
assert body[caster + 2].op_str == 'rax, rax' and body[caster + 3].mnemonic == 'je'
assert body[caster + 3].operands[0].imm == exit_address
assert not calls('?GetTarget@AuraScript@@')
credits = calls('?DoUpdateAchievementCriteria@InstanceScript@@')
assert len(credits) == 3
for position, value in zip(credits, ('r8d, 0xb432', 'r8d, 0xb5aa', 'r8d, 0xcbda')):
    assert all(any(entry.op_str == operand for entry in body[position - 6:position]) for operand in (value, 'edx, 0x5c', 'r9d, r9d'))
stage, = calls('?getScenarionStep@InstanceScript@@')
assert body[stage + 1].op_str == 'eax, 3' and body[stage + 2].mnemonic == 'jne'
claimed = next(index for index, entry in enumerate(body) if entry.op_str == 'byte ptr [rbx + 0x360], 1')
assert body[stage + 2].operands[0].imm == body[claimed].address
assert body[claimed + 1].op_str == 'ecx, 0x1f40'
cast, = calls('?CastSpell@Unit@@QEAA?AW4SpellCastResult@@PEAV1@I_N')
assert all(any(entry.op_str == operand for entry in body[cast - 8:cast]) for operand in ('rcx, rsi', 'rdx, rsi', 'r8d, 0x34fd2', 'r9b, 1'))
assert body[cast + 1].op_str == 'r14d, eax' and body[cast + 2].op_str == 'eax, eax'
assert body[cast + 3].mnemonic == 'jne'
assert body[cast + 4].op_str == 'word ptr [rbx + 0x362], di'
assert any(entry.op_str == 'edi, edi' and entry.mnemonic == 'xor' for entry in body[cast - 8:cast])
assert body[cast + 5].mnemonic == 'jmp' and body[cast + 5].operands[0].imm == exit_address
failed = cast + 6
assert body[cast + 3].operands[0].imm == body[failed].address
assert body[failed].op_str == 'word ptr [rbx + 0x362], 1'
assert body[failed + 1].op_str == 'byte ptr [rbx + 0x361], dil'
assert body[failed + 2].mnemonic == 'jne' and body[failed + 2].operands[0].imm == exit_address
assert body[failed + 3].op_str == 'byte ptr [rbx + 0x361], 1'
assert not any(entry.op_str == 'word ptr [rbx + 0x362], di' for entry in body[failed:])
assert any(entry.op_str == 'dword ptr [rsp + 0x40], r14d' for entry in body[failed:])
diagnostic = b'Ashbringer domination: Unbreakable Will 217042 failed with result %u for caster %u; retain aura-owned retry'
assert any(entry.mnemonic == 'lea' and entry.operands[1].type == CS_OP_MEM and entry.operands[1].mem.base == X86_REG_RIP
           and native['binary'][native['offset'](entry.address + entry.size + entry.operands[1].mem.disp):].startswith(diagnostic)
           for entry in body[failed:])


def references(selected, target):
    return [index for index, entry in enumerate(selected) if entry.mnemonic == 'lea' and entry.operands[1].type == CS_OP_MEM
            and entry.operands[1].mem.base == X86_REG_RIP
            and native['resolve'](entry.address + entry.size + entry.operands[1].mem.disp) == target]


hooks = instructions(register)
for callback in ('?OnApply@' + scope, '?OnRemove@' + scope, update):
    position, = references(hooks, native['symbol'](callback))
    assert any(entry.op_str == 'r9d, 0xf3' for entry in hooks[max(0, position - 5):position + 1])
    assert any(entry.op_str == 'r8d, r8d' for entry in hooks[max(0, position - 5):position + 4])
tables = []
for entry in instructions(factory):
    if entry.mnemonic == 'lea' and entry.operands[1].type == CS_OP_MEM and entry.operands[1].mem.base == X86_REG_RIP:
        address = entry.address + entry.size + entry.operands[1].mem.disp
        if struct.unpack_from('<Q', native['binary'], native['offset'](address) + 0x28)[0] == native['symbol'](register):
            tables.append(address)
assert len(tables) == 1
assert any(entry.op_str == 'dword ptr [rbx + 0x360], 0xfa00000' for entry in instructions(factory))
loaders = []
for entry in instructions('?AddSC_paladin_broken_shore@@'):
    if entry.mnemonic == 'lea' and entry.operands[1].type == CS_OP_MEM and entry.operands[1].mem.base == X86_REG_RIP:
        address = entry.address + entry.size + entry.operands[1].mem.disp
        if struct.unpack_from('<Q', native['binary'], native['offset'](address) + 0x10)[0] == native['symbol'](factory):
            loaders.append(address)
assert len(loaders) == 1
print('PASS installed actual domination loader/factory/Register/effect0-aura243 update:4000ms initial/8000ms second phase retained; self217042 nonzero result writes pending1 and only first failure flag/diagnostic, zero alone clears timer. Missing caster stops, original three stage3 criteria unchanged. No GetTarget in update. Reported OK does not prove override application/button/removal or client/VFX/whole775 success.')
