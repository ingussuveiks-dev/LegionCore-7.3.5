from linked_player_abi import inherited_operand
from linked_player_abi import player_offset,player_operand
"""Real smoke SpellScript cast/effect guards before the native scenario event."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
from sharasdal_erunak_entry_pe import sites, values, floats
from merlis_intro_pe import reachable

p = runpy.run_path(str(Path(__file__).with_name('verify_kingslayers_gate_order_release.py')))
n, ins = p['n'], p['ins']
suffix = 'spell_kingslayers_smoke_bomb_SpellScript@spell_kingslayers_smoke_bomb@@'
method = lambda name: '?' + name + '@' + suffix
pairs = lambda rows: {(op.mnemonic, op.op_str) for op in rows}


def refs(rows):
    return {op.address + op.size + v.mem.disp for op in rows for v in op.operands
            if v.type == CS_OP_MEM and v.mem.base == X86_REG_RIP}


factory = ins('?GetSpellScript@spell_kingslayers_smoke_bomb@@')
registered = refs(ins('?AddSC_Kingslayers@@'))
assert any(n['symbol']('?GetSpellScript@spell_kingslayers_smoke_bomb@@') in
           struct.unpack_from('<8Q', n['binary'], n['offset'](at)) for at in registered)
assert any(struct.unpack_from('<10Q', n['binary'], n['offset'](at))[5] == n['symbol'](method('Register'))
           for at in refs(factory))
assert {('mov', 'ecx, 0x210'), ('mov', 'byte ptr [rbx + 0x1f8], 0'),
        ('lea', 'rcx, [rbx + 0x1f9]')} <= pairs(factory)
register, check, guard, context, after = [ins(method(x)) for x in
    ('Register', 'CheckSmoke', 'GuardSmokeEvent', 'SmokePlayer', 'HandleAfterCast')]
assert all(n['symbol'](method(x)) in refs(register) for x in ('CheckSmoke', 'GuardSmokeEvent', 'HandleAfterCast'))
assert {('lea', 'rcx, [rbx + 0xa8]'), ('lea', 'rcx, [rbx + 0x120]'),
        ('lea', 'rcx, [rbx + 0x60]'), ('mov', 'r9d, 0x3d'), ('xor', 'r8d, r8d')} <= pairs(register)
assert {1620, 8228, 1123, 42504, 42627, 48533} <= values(context)
assert {13.0, -8822.94, 657.59} <= floats(n, context)
for target in ('?GetCaster@SpellScript@@', '?CanContact@Player@@', '?IsInWorld@Object@@',
               '?IsPreDelete@Object@@', '?IsQuestObjectExpired@WorldObject@@', '?GetAreaId@WorldObject@@',
               '?GetQuestRewardStatus@Player@@', '?GetQuestStatus@Player@@', '?GetCriteriaProgressMap@',
               '?CanUpdateCriteria@Scenario@@'):
    assert sites(n, context, target), target
assert {('test', inherited_operand('byte ptr [rbx + 0x4ff0], 1')), ('cmp', 'al, 4'),
        ('cmp', player_operand('byte ptr [rbx + 0x6848], ') + 'r14b'), ('cmp', 'qword ptr [rcx + 0x28], r14')} <= pairs(context)
assert sites(n, check, method('SmokePlayer')) and {('mov', 'dl, 1'), ('mov', 'ecx, 0x51')} <= pairs(check)
qualified, = sites(n, guard, method('SmokePlayer'))
prevent, = sites(n, guard, '?PreventHitDefaultEffect@SpellScript@@')
assert {('mov', 'dl, 1'), ('cmp', 'byte ptr [rbx + 0x1f8], 0'),
        ('mov', 'byte ptr [rbx + 0x1f8], 1'), ('movups', 'xmmword ptr [rbx + 0x1f9], xmm0')} <= pairs(guard)
assert guard[prevent].mnemonic == 'jmp'
for at in [i for i, op in enumerate(guard) if (op.mnemonic, op.op_str) in
           {('cmp', 'byte ptr [rbx + 0x1f8], 0'), ('test', 'rax, rax')}]:
    latch, = [op.address for op in guard if (op.mnemonic, op.op_str) == ('mov', 'byte ptr [rbx + 0x1f8], 1')]
    assert latch not in reachable(guard, guard[at + 1].operands[0].imm)
    assert guard[prevent].address in reachable(guard, guard[at + 1].operands[0].imm)
receipt, = sites(n, after, method('SmokePlayer'))
identity, = sites(n, after, '??9ObjectGuid@@')
casts = sites(n, after, '?CastSpell@Unit@@')
assert len(casts) == 3 and receipt < identity < casts[0]
assert {('xor', 'edx, edx'), ('lea', 'rdx, [rbx + 0x1f9]'),
        ('mov', 'byte ptr [rbx + 0x1f8], al')} <= pairs(after[:casts[0]])
assert after[identity + 2].mnemonic == 'jne'
assert after[casts[0]].address not in reachable(after, after[identity + 2].operands[0].imm)
assert {214683, 214734, 214304, 15003, 108057, 2412,
        108159, 108155, 108178, 108158, 108177, 108161, 108179, 108157, 108160, 108107} <= values(after)
assert sites(n, after, '?PlayDirectSound@WorldObject@@') and sites(n, after, '?CreateConversation@Conversation@@')
# The actual native pipeline invokes the registered effect hooks before
# dispatching the unchanged SEND_EVENT implementation and both criteria types.
native = ins('?HandleEffects@Spell@@')
# Release inlines CallScriptEffectHandlers, including its default-prevention
# bit test. Follow that actual dispatch rather than requiring an unused symbol.
prepare, = sites(n, native, '?_PrepareScriptCall@SpellScript@@')
affected, = sites(n, native, '?IsEffectAffected@EffectHook@_SpellScript@@')
invoke, = sites(n, native, '?Call@EffectHandler@SpellScript@@')
assert prepare < affected < invoke
assert {('mov', 'rbx, qword ptr [rcx + 0x120]'), ('mov', 'edx, 6')} <= pairs(native[:prepare])
assert {('movzx', 'edx, byte ptr [rax + 0x29]'), ('test', 'al, dl'),
        ('setne', 'r13b')} <= pairs(native[invoke:])
dispatch, = [i for i, op in enumerate(native) if (op.mnemonic, op.op_str) == ('call', 'r8')]
assert invoke < dispatch
gate = max(i for i, op in enumerate(native[:dispatch]) if (op.mnemonic, op.op_str) == ('test', 'r13b, r13b'))
assert gate > invoke and native[gate + 1].mnemonic == 'jne'
assert native[dispatch].address not in reachable(native, native[gate + 1].operands[0].imm)
load = native[dispatch - 3]
assert load.mnemonic == 'mov' and load.operands[1].type == CS_OP_MEM
table = n['base'] + load.operands[1].mem.disp
assert struct.unpack_from('<Q', n['binary'], n['offset'](table + 61 * 8))[0] == n['symbol']('?EffectSendEvent@Spell@@')
event = ins('?EffectSendEvent@Spell@@')
assert len(sites(n, event, '?UpdateAchievementCriteria@Player@@')) == 2
for body in (check, guard, context, after):
    for target in ('?UpdateAchievementCriteria@Player@@', '?KilledMonsterCredit@Player@@', '?CompleteQuest@Player@@', '?AddItem@Player@@'):
        assert not sites(n, body, target)
print('PASS registered native smoke214645: real cast check plus effect0/SEND_EVENT61 guard before unchanged native credit; Rogue/quest/map/area/radius/current48533 admission, actual receipt/full caster identity before original visuals/sound/Clay/panic, per-cast no replay and no manual progression/item shortcut. Native controlled effect endpoints/client presentation remain separate.')
