"""Bind earned-return recovery to selected Release code, not registration text.

Selected linked paths only: this is not a saved-character/client cast traversal.
"""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

checked = runpy.run_path(str(Path(__file__).with_name('verify_acquisition_login_recovery_release.py')))
assert checked['earned_marksman'], 'Release lacks earned Marksmanship recovery'
n, ins, sites, find = (checked[k] for k in ('n', 'ins', 'sites', 'find'))
checked['verify_earned'](ins('?HasEarnedMarksmanshipReturn@Player@@'))
retained = runpy.run_path(str(Path(__file__).with_name('verify_marksmanship_return_release.py')))


def value(body, immediate):
    return any(a.type == CS_OP_IMM and a.imm == immediate for o in body for a in o.operands)


def selected(constructor, methods):
    targets = {name: n['symbol'](prefix) for name, prefix in methods.items()}
    candidates = []
    for o in ins(constructor):
        if o.mnemonic != 'lea':
            continue
        for a in o.operands:
            if a.type != CS_OP_MEM or a.mem.base != X86_REG_RIP:
                continue
            address = o.address + o.size + a.mem.disp
            table = struct.unpack_from('<48Q', n['binary'], n['offset'](address))
            if all(target in table for target in targets.values()):
                candidates.append({name: table.index(target) for name, target in targets.items()})
    assert len(candidates) == 1, f'Missing/ambiguous selected recovery vtable: {constructor}'
    return candidates[0]


factory = ins('?AddSC_artifact_acquisition@@')
for constructor in ('??0condition_marksmanship_recovered_arrival@@', '??0player_marksmanship_return_recovery@@'):
    assert len(sites(factory, constructor)) == 1
condition_slots = selected('??0condition_marksmanship_recovered_arrival@@', {'condition': '?OnConditionCheck@condition_marksmanship_recovered_arrival@@'})
player_slots = selected('??0player_marksmanship_return_recovery@@', {name: f'?{name}@player_marksmanship_return_recovery@@' for name in ('OnUpdate', 'OnLogout', 'OnMapChanged')})
assert condition_slots == {'condition': 1}
assert player_slots == {'OnUpdate': 33, 'OnLogout': 25, 'OnMapChanged': 31}
condition = ins('?OnConditionCheck@condition_marksmanship_recovered_arrival@@')
proof, = sites(condition, '?HasEarnedMarksmanshipReturn@Player@@')
assert condition[proof + 1].op_str == 'al, al' and condition[proof + 2].mnemonic == 'je'
assert condition[proof + 3].op_str == 'al, al' and condition[proof + 3].mnemonic == 'xor'
assert condition[proof + 2].operands[0].imm == condition[proof + 6].address

actor = ins('?Native@MarksmanshipRecovery@@')
assert value(actor, 100397) and value(actor, 370758)
predelete, = sites(actor, '?IsPreDelete@Object@@')
custom, = sites(actor, '?GetScriptId@Creature@@')
assert predelete < custom
assert {0x72616d53, 0x4174, 0x49} <= {a.imm for o in actor for a in o.operands if a.type == CS_OP_IMM}, 'Selected AIName is not SmartAI'
assert any(o.mnemonic == 'lea' and any(a.type == CS_OP_MEM and a.mem.base == X86_REG_RIP and b'.?AVSmartAI@@' in n['binary'][n['offset'](o.address+o.size+a.mem.disp):n['offset'](o.address+o.size+a.mem.disp)+80] for a in o.operands) for o in actor), 'Missing actual SmartAI RTTI guard'

ready = ins('?Ready@MarksmanshipRecovery@@')
proof, = sites(ready, '?HasEarnedMarksmanshipReturn@Player@@')
scenario, = sites(ready, '?GetScenario@ScenarioMgr@@')
native, = sites(ready, '?Native@MarksmanshipRecovery@@')
phase, = sites(ready, '?InSamePhase@WorldObject@@')
distance, = sites(ready, '?IsWithinDistInMap@WorldObject@@')
script, = sites(ready, '?GetScript@SmartAI@@')
assert proof < scenario < native < phase < distance < script and value(ready[scenario:native], 972)
step, = sites(ready, '?GetCurrentStep@Scenario@@')
assert ready[step + 1].op_str == 'eax, eax' and ready[step + 2].mnemonic == 'jne'
constant = next(o for o in ready[phase:distance] if o.mnemonic == 'movss' and any(a.type == CS_OP_MEM and a.mem.base == X86_REG_RIP for a in o.operands))
assert struct.unpack_from('<f', n['binary'], n['offset'](checked['rip'](constant)))[0] == 20.0
assert find(ready, 'dword ptr [rcx + 0xc], 0', script, 'cmp') < find(ready, 'dword ptr [rcx + 0x30], 0xa', script, 'cmp') < find(ready, 'byte ptr [rcx + 0x99], 0', script, 'cmp'), 'Original one-shot intro consumption guard missing'

update = ins('?OnUpdate@player_marksmanship_return_recovery@@')
assert len(sites(update, '?HasEarnedMarksmanshipReturn@Player@@')) == 1
cast, = sites(update, '?CastSpell@Unit@@')
assert update[cast-4].mnemonic == 'xor' and update[cast-4].op_str == 'r9d, r9d'
assert update[cast-3].op_str == 'r8d, 0x320f0' and update[cast-2].op_str == 'rdx, r12' and update[cast-1].op_str == 'rcx, r13'
assert value(update, 1000) and value(update, 5000) and value(update, 12)
assert value(update, 100397) and len(sites(update, '?Native@MarksmanshipRecovery@@')) >= 1
# Same CRT mutex-unlock target immediately precedes Ready/Cast; reacquire only
# after the cast result. Source ASAN exercises reentrant cancellation separately.
ready_call, = sites(update, '?Ready@MarksmanshipRecovery@@')
unlock = update[ready_call-3]
assert unlock.mnemonic == 'call' and unlock.operands[0].type == CS_OP_IMM
assert any(o.mnemonic == 'call' and o.operands[0].type == CS_OP_IMM and o.operands[0].imm == unlock.operands[0].imm for o in ins('?OnLogout@player_marksmanship_return_recovery@@'))
hit = ins('?Hit@spell_marksmanship_return@@')
teleport, = sites(hit, '?EffectTeleportUnits@Spell@@')
cleanup, = sites(hit, '?RemoveAurasDueToSpell@Unit@@')
recovered, = sites(hit, '?Ready@MarksmanshipRecovery@@')
assert recovered < teleport < cleanup and hit[cleanup-2].op_str == 'edx, 0x30e57'
pending_test = find(hit, 'cl, cl', teleport, 'test')
recovered_test = find(hit, 'bpl, bpl', pending_test, 'test')
for at in (pending_test, recovered_test):
    assert hit[at+1].mnemonic == 'je' and hit[at+1].operands[0].imm > hit[cleanup].address
for body in (actor, ready, update, condition):
    for forbidden in ('?AddItem@Player@@', '?KilledMonsterCredit@Player@@', '?CompleteQuest@Player@@', '?TeleportTo@Player@@', '?SetCurrentStep@Scenario@@'):
        assert not sites(body, forbidden), forbidden
print(f'PASS linked earned Marksmanship recovery: actual five-goal owned loading-bow proof and late seed guard; selected condition{condition_slots}/player{player_slots}; real DB370758/SmartAI, fresh972/0,20yd/unconsumed intro, normal205040 bounded retry; accepted-pending-only200279 cleanup and native group credit retained. Full character/client traversal remains separate.')
