"""Inspect actual linked native jump descriptor, launch and arrival guards."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM, CS_OP_MEM

n = runpy.run_path(str(Path(__file__).with_name('verify_smart_linked_cast_retry_release.py')))
ins = n['instructions']
jump = '?EffectJumpDest@Spell@@'
move = '?MoveJump@MotionMaster@@QEAAXMMMMMMI_NPEBUJumpArrivalCastArgs@@'
arrival = '?MovementInform@EffectMovementGenerator@@'

def values(body):
    return {o.imm for x in body for o in x.operands if o.type == CS_OP_IMM}

def call_sites(body, prefix):
    target = n['symbol'](prefix)
    return [i for i, x in enumerate(body) if x.mnemonic == 'call'
            and x.operands[0].type == CS_OP_IMM
            and n['resolve'](x.operands[0].imm) == target]

j, m, a = (ins(name) for name in (jump, move, arrival))
assert {1600,209683,209684,1539,206455,206456,206360,206361,1479} <= values(j)
# The scoped pair/map branches set AL, which feeds the Player-only common
# native self-GUID assignment. Inspect the full128-bit descriptor copy.
scope = next(i for i,x in enumerate(j) if x.mnemonic == 'cmp' and x.op_str == 'r9d, 0x640')
assert [x.op_str for x in j[scope:scope+9] if x.mnemonic == 'cmp'] == [
    'r9d, 0x640','dword ptr [rax], 0x33313','r8d, 0x33314']
assert j[scope+7].op_str == 'al, 1'
entry = next(i for i,x in enumerate(j) if x.address == j[scope+8].operands[0].imm)
assert j[entry].op_str == 'byte ptr [r11 + 0x16], 4'
assert any(x.mnemonic == 'test' and x.op_str == 'al, al' for x in j[entry:entry+10])
guid = call_sites(j, '?GetGuidValue@Object@@')[0]
assert entry < guid < entry+15
assert j[guid-2].op_str == 'edx, edx' and j[guid-1].op_str == 'rcx, r11'
assert [(x.mnemonic,x.op_str) for x in j[guid+1:guid+3]] == [
    ('movups','xmm0, xmmword ptr [rax]'),('movups','xmmword ptr [rsp + 0x74], xmm0')]
assert call_sites(j, move) and call_sites(j, '?CalculateJumpSpeeds@Spell@@')
assert call_sites(m, '?Launch@MoveSplineInit@Movement@@')
scope = next(i for i,x in enumerate(m) if x.mnemonic == 'cmp' and x.op_str == 'edi, 0x33313')
assert [x.op_str for x in m[scope:scope+12] if x.mnemonic == 'cmp'] == [
    'edi, 0x33313','dword ptr [rbx], 0x33314','byte ptr [r11 + 0x16], 4',
    'dword ptr [r11 + 0xe0], 0x640']
assert m[scope+9].op_str == 'r12b, 1'
gate = next(i for i,x in enumerate(m) if x.address == m[scope+10].operands[0].imm)
assert m[gate].op_str == 'r13d, r13d' and m[gate+1].mnemonic == 'jg'
refusal = m[gate:gate+30]
assert any(x.mnemonic == 'test' and x.op_str == 'r12b, r12b' for x in refusal)
assert any(x.mnemonic == 'and' and x.op_str == 'dword ptr [rax + 0x3140], 0xbfffffff' for x in refusal)
# The same Sharasdal flag selects actual map for constructor+0x24, as opposed
# to the UINT32_MAX sentinel used by ordinary unscoped movement generators.
store = next(i for i,x in enumerate(m) if x.mnemonic == 'mov' and x.op_str == 'dword ptr [rdx + 0x24], eax')
window = m[store-25:store]
for mnemonic,operand in [('test','r12b, r12b'),('mov','eax, 0xffffffff'),('mov','eax, dword ptr [rax + 0xe0]')]:
    assert any(x.mnemonic == mnemonic and x.op_str == operand for x in window)
gate = next(i for i,x in enumerate(a) if x.mnemonic == 'cmp' and x.op_str == 'r10d, -1')
assert [(a[gate+i].mnemonic,a[gate+i].op_str) for i in (2,4,6,7)] == [
    ('cmp','byte ptr [rdx], bpl'),('cmp','dword ptr [rdi + 0xa18], ebp'),
    ('mov','eax, dword ptr [rsi + 0x24]'),('cmp','dword ptr [rcx], eax')]
assert sum(x.mnemonic == 'jne' or x.mnemonic == 'je' for x in a[gate:gate+9]) == 4
assert call_sites(a, '?GetUnit@ObjectAccessor@@') and call_sites(a, '?CastSpell@Unit@@QEAA?AW4SpellCastResult@@PEAV1@I_N')
cast = ins('?CastSpell@Unit@@QEAA?AW4SpellCastResult@@PEAV1@I_N')
assert any(x.mnemonic == 'test' and x.op_str == 'rdx, rdx' for x in cast)
print('PASS actual linked Player/map1600/209683->209684 full self-GUID descriptor; positive native launch/state cleanup, captured actual map constructor and arrived/alive/same-map arrival guard; native Unit null target rejection retained. World destination, claim/equip/scene and client remain separate.')
