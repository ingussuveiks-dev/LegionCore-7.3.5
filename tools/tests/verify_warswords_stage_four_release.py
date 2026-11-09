"""Selected registered instance and deferred scene guards in the actual x64 PE."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM

retained = runpy.run_path(str(Path(__file__).with_name('verify_warswords_final_visibility_release.py')))
n = retained['native']
ins, symbols, resolve = retained['instructions'], n['symbols'], n['resolve']
suffix = '@instance_warswords_InstanceMapScript@instance_warswords@@'
step = '?onScenarionNextStep' + suffix
def sites(rows, prefix):
    targets = {resolve(v) for k, v in symbols.items() if k.startswith(prefix)}
    assert targets, prefix
    return [i for i, x in enumerate(rows) if x.mnemonic in ('call', 'jmp')
            and x.operands[0].type == CS_OP_IMM and resolve(x.operands[0].imm) in targets]
def immediates(rows):
    return {o.imm for x in rows for o in x.operands if o.type == CS_OP_IMM}

rows = ins(step)
assert sites(rows, '?IsParticipant@WarswordsArrival@@')
assert sites(rows, '?GetQuestCancellationReceipt@Player@@')
assert sites(rows, '?AddDelayedEvent@FunctionProcessor@@')
assert {40043, 3000} <= immediates(rows), 'Original quest or three-second deadline missing'
names = [k for k in symbols if k.startswith('?_Do_call@') and
         'onScenarionNextStep' + suffix in k]
name, = names
body = ins(name)
participant, = sites(body, '?IsParticipant@WarswordsArrival@@')
receipt, = sites(body, '?GetQuestCancellationReceipt@Player@@')
scenario, = sites(body, '?GetScenario@ScenarioMgr@@')
stage, = sites(body, '?GetCurrentStep@Scenario@@')
cast, = sites(body, '?CastSpell@Unit@@')
assert participant < receipt < scenario < stage < cast
assert {40043, 189740} <= immediates(body)
# Weak original-instance lifetime is checked before reading its captured script
# argument; a cancelled quest takes the same refusal path before the gate.
weak = next(i for i, x in enumerate(body[:participant]) if x.mnemonic == 'cmp'
            and x.op_str == 'dword ptr [rax + 8], ebx')
cancel = next(i for i, x in enumerate(body[:participant]) if x.mnemonic == 'test' and x.op_str == 'al, al')
assert body[weak + 1].mnemonic == 'je' and body[cancel + 1].mnemonic == 'jne'
assert body[weak + 1].operands[0].imm == body[cancel + 1].operands[0].imm
identity = next(i for i, x in enumerate(body[receipt:scenario], receipt)
                if x.mnemonic == 'cmp' and x.op_str == 'qword ptr [rdi + 0x20], rax')
assert body[identity + 1].mnemonic == 'jne'
assert body[identity + 1].operands[0].imm == body[weak + 1].operands[0].imm
check = next(i for i in range(stage + 1, cast) if body[i].mnemonic == 'cmp' and body[i].op_str == 'eax, 4')
assert body[check + 1].mnemonic == 'jne'
assert body[check + 1].operands[0].imm > body[cast].address
print('PASS actual registered Warswords instance/deferred189740: original3000ms/40043,initial and late participant gate,weak instance generation before old-script comparison,cancellation and exact quest receipt,live944 stage4 before native self-cast;retained boss visibility and wave/arrival PE chain. Client traversal remains separate.')
