"""Verify the actual registered Faronaar update invokes bounded own-companion recovery."""
from pathlib import Path
import re
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
p=runpy.run_path(str(Path(__file__).with_name('verify_warswords_final_credit_release.py')))
n, instructions=p['native'],p['instructions']
update='?OnUpdate@player_faronaar_chain@@'
recovery='?RecoverCompanion@Faronaar@@'
assert n['symbol'](recovery) in n['calls'](update), 'Selected update lacks new recovery helper'
body=instructions(recovery)
values={o.imm for i in body for o in i.operands if o.type==CS_OP_IMM}
assert {6000,178860,90474,300000}<=values
cast='?CastSpell@Unit@@QEAA?AW4SpellCastResult@@MMMI_N'
assert n['symbol'](cast) in n['calls'](recovery), 'Must retain actual destination summon overload'
assert n['symbol']('?DespawnOrUnsummon@Creature@@') in n['calls'](recovery)
assert n['symbol']('?MoveFollow@MotionMaster@@') in n['calls'](recovery)
for forbidden in ('?KilledMonsterCredit@Player@@','?QuestObjectiveSatisfy@Player@@','?CompleteQuest@Player@@'):
    assert n['symbol'](forbidden) not in n['calls'](recovery), 'Recovery must not grant progression'
mapping=(n['args'].release/'worldserver.map').read_text(errors='replace')
table=re.search(r'\s[0-9a-f]+:[0-9a-f]+\s+\?\?_7player_faronaar_chain@@6B@\s+([0-9a-fA-F]{16})',mapping)
assert table
address=int(table[1],16)
header=(Path(__file__).resolve().parents[2]/'src/server/game/Scripting/ScriptMgr.h').read_text()
part=header[header.index('class TC_GAME_API PlayerScript'):]
slot=1+len(re.findall(r'\bvirtual\b',part[:part.index('virtual void OnUpdate(Player*')]))
assert struct.unpack_from('<Q',n['binary'],n['offset'](address)+slot*8)[0]==n['symbol'](update), 'Wrong actual PlayerScript hook slot'
assert any(i.mnemonic=='lea' and o.type==CS_OP_MEM and o.mem.base==X86_REG_RIP and i.address+i.size+o.mem.disp==address for i in instructions('?AddSC_faronaar_chain@@') for o in i.operands), 'Unregistered player script table'
print(f'PASS {n["args"].release}: exact registered PlayerScript OnUpdate slot{slot} calls6000ms own90474 recovery, real178860 destination summon/300000ms lifetime/follow, no progression grants. Client cave traversal remains pending.')
