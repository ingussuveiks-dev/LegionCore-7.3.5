"""Check the deployed owner selector and Player-only scenario-credit dispatcher."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM

previous = runpy.run_path(str(Path(__file__).with_name('verify_smart_invoker_player_release.py')))
native = previous['native']
instructions = previous['instructions']
selector = '?GetTargets@SmartScript@@'
owner_address = previous['target_case'](selector, 23)
nearest_address = previous['target_case'](selector, 21)
assert owner_address != nearest_address, 'Owner target aliases nearest-player target'
body = [entry for entry in instructions(selector) if owner_address <= entry.address < owner_address + 96]
calls = {native['resolve'](entry.operands[0].imm) for entry in body
         if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM}
assert native['symbol']('?GetAnyOwner@Unit@@') in calls, 'Target23 does not resolve actual owner/summoner'
assert native['symbol']('?SelectNearestPlayer@Creature@@') not in calls, 'Owner selector falls back to a nearby player'
assert native['symbol']('?GetSummoner@TempSummon@@') in native['calls']('?GetAnyOwner@Unit@@'), 'Temporary summon owner path missing'

action = instructions('?ProcessAction@SmartScript@@')
maximum = next(index for index, entry in enumerate(action) if entry.mnemonic == 'cmp'
               and any(operand.type == CS_OP_IMM and operand.imm == 223 for operand in entry.operands))
tables = [operand.mem for entry in action[maximum + 1:maximum + 10] for operand in entry.operands
          if operand.type == CS_OP_MEM and operand.mem.index and operand.mem.disp > 0]
assert len(tables) == 2 and tables[0].scale == 1 and tables[1].scale == 4
destinations = []
for number in range(223):
    case_index = native['binary'][native['offset'](native['base'] + tables[0].disp + number)]
    destinations.append(native['base'] + struct.unpack_from('<I', native['binary'], native['offset'](native['base'] + tables[1].disp + case_index * 4))[0])
credit_address = destinations[204]
credit_end = min(address for address in destinations if address > credit_address)
credit = [entry for entry in action if credit_address <= entry.address < credit_end]
calls = {native['resolve'](entry.operands[0].imm) for entry in credit
         if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM}
for prefix in [selector, '?GetScriptCreditPlayer@', '?UpdateAchievementCriteria@Player@@']:
    assert native['symbol'](prefix) in calls, f'Native action205 omits {prefix}'
assert native['symbol']('?GetCharmerOrOwnerPlayerOrPlayerItself@Unit@@') in native['calls']('?GetScriptCreditPlayer@'), 'Pet/guardian ownership lost'
print('PASS: deployed target23 is distinct from nearest21 and uses actual TempSummon owner resolution; action205 consumes selected targets through Player/owner normalization and native achievement/scenario credit, not a SQL-only claim')
