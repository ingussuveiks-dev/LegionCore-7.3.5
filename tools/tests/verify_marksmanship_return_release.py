"""Verify scoped return hooks and real engine delay-byte handoff in the deployed PE."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM, CS_OP_MEM, CS_OP_REG
from capstone.x86_const import X86_REG_RSP, X86_REG_RBP

previous = runpy.run_path(str(Path(__file__).with_name('verify_marksmanship_report_use_release.py')))
native = previous['native']
instructions = previous['instructions']
hit_name = '?Hit@spell_marksmanship_return@@'
hit = instructions(hit_name)
load = instructions('?Load@spell_marksmanship_return@@')
check = instructions('?Check@spell_marksmanship_return@@')
current_name = '?Current@MarksmanshipReturn@@'
teleport_name = '?EffectTeleportUnits@Spell@@'
handler = native['symbol'](teleport_name)
positions = [index for index, entry in enumerate(hit) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM and native['resolve'](entry.operands[0].imm) == handler]
assert len(positions) == 1, 'Scoped hit must invoke native teleport exactly once'
position = positions[0]


def byte_store(entry):
    return (entry.mnemonic == 'mov' and len(entry.operands) == 2 and entry.operands[0].type == CS_OP_MEM and
            entry.operands[0].size == 1 and entry.operands[0].mem.base not in (X86_REG_RSP, X86_REG_RBP))


stores = [(index, entry) for index, entry in enumerate(hit[:position]) if byte_store(entry) and entry.operands[1].type == CS_OP_IMM and entry.operands[1].imm == 1]
assert len(stores) == 1, 'Missing bounded native delay enable'
enabled_at, enabled = stores[0]
delay = enabled.operands[0].mem
assert any(entry.mnemonic == 'movzx' and entry.operands[1].type == CS_OP_MEM and entry.operands[1].size == 1 and
           entry.operands[1].mem.base == delay.base and entry.operands[1].mem.disp == delay.disp for entry in hit[:enabled_at]), 'Previous delay flag is not captured'
restores = [(index, entry) for index, entry in enumerate(hit[position+1:], position+1) if byte_store(entry) and entry.operands[1].type == CS_OP_REG and entry.operands[0].mem.base == delay.base and entry.operands[0].mem.disp == delay.disp]
assert len(restores) == 1, 'Native delay flag is not restored after teleport'
restored_at, restored = restores[0]
pending = [entry.operands[1].mem.disp for entry in hit[position+1:restored_at] if entry.mnemonic == 'movzx' and entry.operands[1].type == CS_OP_MEM and entry.operands[1].size == 1 and entry.operands[1].mem.base == delay.base]
assert len(pending) == 1 and pending[0] != delay.disp, 'Accepted state does not read native pending flag'
update = instructions('?Update@Player@@UEAAXI@Z')
assert {0, 1} <= {entry.operands[1].imm for entry in update if byte_store(entry) and entry.operands[0].mem.disp == delay.disp and entry.operands[1].type == CS_OP_IMM}, 'Scoped delay uses a different field than native Player::Update'
assert any(operand.type == CS_OP_MEM and operand.size == 1 and operand.mem.disp == pending[0] for entry in update for operand in entry.operands), 'Native update does not consume the captured pending field'
scope_fields = {entry.operands[0].mem.disp for entry in load if byte_store(entry)}
assert any(operand.type == CS_OP_MEM and operand.size == 1 and operand.mem.disp in scope_fields for entry in check for operand in entry.operands), 'CheckCast loses captured origin scope'
assert any(operand.type == CS_OP_MEM and operand.size == 1 and operand.mem.disp in scope_fields for entry in hit for operand in entry.operands), 'Effect hit loses captured origin scope'


def immediate_values(entries):
    return {operand.imm for entry in entries for operand in entry.operands if operand.type == CS_OP_IMM}


assert {100397, 1489} <= immediate_values(load), 'Wrong initial actor/map scope'
assert {40419, 128826, 972, 8} <= immediate_values(instructions(current_name)), 'Wrong active quest/owned item/completed scenario guard'
assert native['symbol'](current_name) in native['calls'](hit_name) & native['calls']('?Check@spell_marksmanship_return@@'), 'Missing cast/hit context revalidation'
assert {252, 90} <= immediate_values(instructions('?Register@spell_marksmanship_return@@')), 'Wrong native effect hook bindings'
assert native['symbol']('?TeleportTo@Player@@QEAA_NIMMMMII@Z') in native['calls'](teleport_name), 'Native teleport no longer delegates actual target transfer'
assert native['symbol']('?RewardPlayerAndGroupAtEvent@Player@@') in native['calls']('?EffectKillCredit@Spell@@'), 'Native effect90 group credit replaced or absent'
assert b'spell_marksmanship_return' in native['binary']
print(f'PASS: actual Release retains scoped Vereesa cast/hit guards and single native teleport with saved/restored delay byte+{delay.disp}, pending byte+{pending[0]} consumed by Player::Update, and unchanged effect90 group bridge')
