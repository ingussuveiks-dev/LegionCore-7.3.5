"""Follow the selected Zakajz AI's native pose lifecycle in the linked PE."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

p = runpy.run_path(str(Path(__file__).with_name('verify_stromkar_execute_jump_release.py')))
n, ins, values = (p[k] for k in ('n', 'instructions', 'values'))
ai = 'npc_stromkar_zakajz@@'
update, hit, reset, clear = ('?' + name + '@' + ai for name in ('UpdateAI', 'SpellHit', 'Reset', 'ClearDefeatPose'))
for method, targets in (
    (update, ('?UpdateAI@SmartAI@@', '?GetScenario@ScenarioMgr@@', '?GetCurrentStep@Scenario@@', '?AddAura@Unit@@QEAA', clear)),
    (hit, (clear, '?SpellHit@SmartAI@@')),
    (reset, (clear, '?Reset@SmartAI@@')),
    (clear, ('?RemoveAurasDueToSpell@Unit@@', '?GetGuidValue@Object@@')),
):
    for target in targets:
        matches = {n['resolve'](a) for s, a in n['symbols'].items() if s.startswith(target)}
        assert matches & n['calls'](method), (method, target)
assert {1539, 104276, 1037, 9, 130966, 2000} <= values(update)
assert {201904, 104276, 1539} <= values(hit)
assert 130966 in values(clear)
calls = [(i, n['resolve'](x.operands[0].imm)) for i, x in enumerate(ins(hit))
         if x.mnemonic in ('call', 'jmp') and x.operands[0].type == CS_OP_IMM]
assert next(i for i, a in calls if a == n['symbol'](clear)) < next(i for i, a in calls if a == n['symbol']('?SpellHit@SmartAI@@'))
selected = []
for x in ins('?GetAI@?$GenericCreatureScript@Unpc_stromkar_zakajz@@@@'):
    for op in x.operands:
        if x.mnemonic != 'lea' or op.type != CS_OP_MEM or op.mem.base != X86_REG_RIP:
            continue
        at = n['offset'](x.address + x.size + op.mem.disp)
        if at + 640 <= len(n['binary']):
            table = struct.unpack_from('<80Q', n['binary'], at)
            if all(n['symbol'](method) in table for method in (update, hit, reset)):
                selected.append(table)
assert len(selected) == 1, 'Native AI factory does not install all pose lifecycle overrides'
assert n['symbol']('?DamageTakenAfterScaling@npc_stromkar_zakajz@@') in selected[0]
assert n['symbol']('?JustSummoned@npc_stromkar_zakajz@@') in selected[0]
print('PASS actual selected Zakajz AI pose UpdateAI/SpellHit/Reset and retained damage/summon slots; scoped130966 retry and own-caster cleanup before original SmartAI hit dispatch. Native catalog/host and client rendering are separate checks.')
