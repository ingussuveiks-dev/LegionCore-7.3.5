"""Verify rejected native210133 casts cannot finalize the Truthguard pickup."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM

previous = runpy.run_path(str(Path(__file__).with_name('verify_truthguard_equipped_pair_release.py')))
native = previous['native']
instructions = previous['instructions']
body = instructions('?CastCredit@TruthguardAcquisition@@')
cast = next(index for index, entry in enumerate(body) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM
            and native['resolve'](entry.operands[0].imm) == native['symbol']('?CastSpell@Unit@@QEAA?AW4SpellCastResult@@PEAV1@I_N'))
assert any(entry.op_str == 'r8d, 0x334d5' for entry in body[:cast]), 'Wrong credit spell'
assert any(entry.op_str == 'r9b, 1' for entry in body[:cast]) and any(entry.op_str == 'rdx, rcx' for entry in body[:cast]), 'Native triggered player-self cast changed'
assert body[cast + 1].op_str == 'esi, eax' and body[cast + 2].op_str == 'eax, eax'
assert body[cast + 3].mnemonic == 'je'
accepted = body[cast + 3].operands[0].imm
accepted_index = next(index for index, entry in enumerate(body) if entry.address == accepted)
assert body[accepted_index].op_str == 'al, 1'
denied = next(index for index in range(cast + 4, accepted_index)
              if body[index].op_str == 'al, al' and body[index].mnemonic == 'xor')
assert body[denied + 1].mnemonic == 'jmp' and body[denied + 1].operands[0].imm > accepted, 'Rejection falls through to true'
for forbidden in ('?DoUpdateAchievementCriteria@InstanceScript@@', '?AddItem@Player@@', '?StoreNewItem@Player@@'):
    assert native['symbol'](forbidden) not in native['calls']('?CastCredit@TruthguardAcquisition@@')
callback = instructions('?OnLootStateChanged@go_sr_truthguard@@')
credit_calls = [entry for entry in callback if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM
                and native['resolve'](entry.operands[0].imm) == native['symbol']('?CastCredit@TruthguardAcquisition@@')]
assert len(credit_calls) == 1
direct_casts = [index for index, entry in enumerate(callback) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM
                and native['resolve'](entry.operands[0].imm) == native['symbol']('?CastSpell@Unit@@QEAA?AW4SpellCastResult@@PEAV1@I_N')]
assert len(direct_casts) == 1 and any(entry.op_str == 'r8d, 0x334d4' for entry in callback[direct_casts[0] - 5:direct_casts[0]]), 'Credit was duplicated or scene ordering changed'
assert b'Truthguard pickup: credit cast 210133 rejected with result' in native['binary'], 'Rejection diagnostic missing'
print('PASS: canonical210133 player-self triggered cast checks native zero/OK result before50545/event7/scene/despawn; rejection logs result and shares scoped READY retry with equip failure. Accepted effects, quest/scene completion and client remain separate.')
