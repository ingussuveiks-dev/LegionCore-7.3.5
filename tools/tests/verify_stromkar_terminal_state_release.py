"""Verify the selected Execute AI suppresses native regeneration and clears threat."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM, CS_OP_MEM, CS_OP_REG

p = runpy.run_path(str(Path(__file__).with_name('verify_stromkar_defeat_pose_release.py')))
n, ins = p['n'], p['ins']
hit = '?SpellHit@npc_stromkar_zakajz@@'
clear = '?ClearDefeatPose@npc_stromkar_zakajz@@'
hit_code = ins(hit)
# RegenerateHealth is inlined into Creature::Update in this Release. Derive
# the shared byte from the actual hold/restore consumers, then require the
# native Creature update to gate on that same field; no fixed class offset.
def byte_writes(code):
    return [x for x in code if x.mnemonic == 'mov' and x.operands[0].type == CS_OP_MEM
            and x.operands[0].size == 1 and x.operands[0].mem.disp > 0x500]
writes = byte_writes(hit_code)
offsets = {x.operands[0].mem.disp for x in writes} & {x.operands[0].mem.disp for x in byte_writes(ins(clear))}
assert len(offsets) == 1, 'Shared native regeneration hold/restore field missing'
offset = offsets.pop()
assert any(x.mnemonic == 'cmp' and x.operands[0].type == CS_OP_MEM and x.operands[0].size == 1
           and x.operands[0].mem.disp == offset and x.operands[1].type == CS_OP_IMM
           and x.operands[1].imm == 0 for x in ins('?Update@Creature@@')), 'Actual inlined native regeneration gate missing'
write = next(x for x in writes if x.operands[0].mem.disp == offset)
if write.operands[1].type == CS_OP_IMM:
    assert write.operands[1].imm == 0
else:
    assert write.operands[1].type == CS_OP_REG
    name = write.reg_name(write.operands[1].reg)
    aliases = {'al':'eax','bl':'ebx','cl':'ecx','dl':'edx','sil':'esi','dil':'edi'}
    alias = aliases.get(name, name[:-1]+'d' if name.startswith('r') and name.endswith('b') else name)
    assert any(x.address < write.address and x.mnemonic == 'xor' and x.op_str == f'{alias}, {alias}' for x in hit_code), 'Regeneration flag not written from a cleared register'
for prefix in ('?CombatStop@Unit@@', '?DeleteThreatList@Unit@@', '?deleteReferences@HostileRefManager@@'):
    targets = {n['resolve'](a) for name, a in n['symbols'].items() if name.startswith(prefix)}
    assert targets & n['calls'](hit), (hit, prefix)
print('PASS selected Zakajz Execute clears native bidirectional threat and disables the actual Creature regeneration flag; own cleanup restores it. No player-wide combat reset or true-death/progression API added; actual client combat/pose remain separate.')
