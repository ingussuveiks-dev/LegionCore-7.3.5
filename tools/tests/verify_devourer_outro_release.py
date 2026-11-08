"""Select the actual registered1607 Update; retain acquisition/recovery proofs."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM

p = runpy.run_path(str(Path(__file__).with_name('verify_devourer_graveyard_release.py')))
n, ins = p['n'], p['ins']
name = '?Update@instance_devourer_InstanceMapScript@instance_devourer@@'
body = ins(name)

def sites(prefix):
    targets = {n['resolve'](v) for k, v in n['symbols'].items() if k.startswith(prefix)}
    return [i for i, x in enumerate(body) if x.mnemonic == 'call'
            and x.operands[0].type == CS_OP_IMM and n['resolve'](x.operands[0].imm) in targets]

def find(mnemonic, operands):
    return next(i for i, x in enumerate(body) if (x.mnemonic, x.op_str) == (mnemonic, operands))

assert sites('?IsCompleted@Scenario@@'), 'Release lacks Devourer actual-completion outro gate'
assert p['table'].count(n['symbol'](name)) == 1, 'Registered instance misses repaired Update'
get = sites('?GetScenario@ScenarioMgr@@')
ids = sites('?GetScenarioId@Scenario@@')
done = sites('?IsCompleted@Scenario@@')
assert len(get) == len(ids) == len(done) == 2
end = body[get[0]+3].operands[0].imm
cleanup = body[get[1]+3].operands[0].imm
for g, identity, complete, target in zip(get, ids, done, [end, cleanup]):
    assert g < identity < complete
    assert [(x.mnemonic, x.op_str) for x in body[g+1:g+4]][:2] in [
        [('mov', 'rbx, rax'), ('test', 'rax, rax')],
        [('mov', 'rdi, rax'), ('test', 'rax, rax')]]
    assert body[g+3].mnemonic == 'je' and body[g+3].operands[0].imm == target
    assert body[identity+1].op_str == 'eax, 0x436'
    assert body[identity+2].mnemonic == 'jne' and body[identity+2].operands[0].imm == target
    assert body[complete-2].op_str == 'edx, edx'  # non-bonus completion
    assert body[complete+1].op_str == 'al, al'
    assert body[complete+2].mnemonic == 'je' and body[complete+2].operands[0].imm == target
timer = find('mov', 'eax, dword ptr [r13 + 0x308]')
assert done[0] < timer < get[1]
assert [(x.mnemonic, x.op_str) for x in body[timer+1:timer+7]][:5] == [
    ('cmp', 'eax, edi'), ('jbe', hex(body[timer+6].address)),
    ('sub', 'eax, edi'), ('mov', 'dword ptr [r13 + 0x308], eax'), ('jmp', hex(end))]
assert body[timer+6].op_str == 'dword ptr [r13 + 0x308], 0x7d0'
guid, = sites('?GetGuidValue@Object@@')
lookup, = sites('?GetCreature@Map@@')
world, = sites('?IsInWorld@Object@@')
deleted, = sites('?IsPreDelete@Object@@')
random, = sites('?urand@@')
cast, = sites('?CastSpell@Unit@@')
assert timer < guid < get[1] < done[1] < lookup < world < deleted < random < cast
assert body[guid-1].op_str == 'edx, edx'
assert find('movups', 'xmm0, xmmword ptr [r12]') < get[1]
assert find('movups', 'xmmword ptr [rax + 0x1a], xmm0') < get[1]  # full128-bit snapshot
assert body[lookup-2].op_str == 'rdx, [rbx + 0x1a]'
skip = body[world+2].operands[0].imm
assert body[lookup+2].op_str == 'rax, rax' and body[lookup+3].operands[0].imm == skip
assert body[world+2].mnemonic == 'je'
assert body[deleted+2].mnemonic == 'jne' and body[deleted+2].operands[0].imm == skip
for value in ['eax, 0x18004', 'eax, 0x18003', 'eax, 0xd56e']:
    assert deleted < find('cmp', value) < random  # exactly the original three entries
membership = find('cmp', 'qword ptr [rax + 0x10], rdi')
assert deleted < membership < random
assert body[membership+1].mnemonic == 'je' and body[membership+1].operands[0].imm == body[random-2].address
assert body[random-2].op_str == 'edx, 1' and body[random-1].op_str == 'ecx, ecx'
assert body[random+1].op_str == 'eax, 1' and body[random+2].operands[0].imm == skip
assert find('sete', 'r8b') < find('add', 'r8d, 0x33395') < cast  #209814 for98308, otherwise209813
assert [(x.mnemonic, x.op_str) for x in body[cast-3:cast]] == [
    ('xor', 'r9d, r9d'), ('mov', 'rdx, rdi'), ('mov', 'rcx, rdi')]  # ordinary self cast
refresh = sites('?instance@ScenarioMgr@@')[1]
assert any(x.mnemonic == 'je' and x.operands[0].imm == body[refresh].address for x in body[cast:]), 'No per-target scenario refresh'
for forbidden in ('?GetCurrentStep@Scenario@@', '?getScenarionStep@InstanceScript@@',
                  '?UpdateAchievementCriteria@Player@@', '?CompleteQuest@Player@@',
                  '?AddItem@Player@@', '?TeleportTo@Player@@', '?SetCurrentStep@Scenario@@'):
    assert not sites(forbidden), forbidden
print('PASS linked Devourer outro:registered1607 primary Update,actual1078/nonbonus completion before timer and per target;two-second rearm before callbacks,full128-bit identity snapshot/map lookup/live+predelete+entry+tracking guards,native50-percent selection/ordinary self209813+209814,no cached-step/item/progress grants. Client visual/full-chain checks remain separate.')
