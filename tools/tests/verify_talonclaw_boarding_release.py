"""Verify the installed click/cast consumers and quest-removal flag refresh."""
from pathlib import Path
import re
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_huey_return_admission_release.py')))
native = previous['native']
instructions = previous['previous']['instructions']
cast = '?CheckCast@Spell@@'
assert native['symbol']('?GetConditionsForNotGroupedEntry@ConditionMgr@@') in native['calls'](cast)
assert native['symbol']('?IsObjectMeetToConditions@ConditionMgr@@QEBA_NAEAUConditionSourceInfo@@') in native['calls'](cast)
callback = '?_Do_call@?$_Func_impl_no_alloc@V<lambda_1>@?1??RemoveActiveQuest@Player@@QEAAXI@Z@X$$V@std@@EEAAXXZ'
body = instructions(callback)
assert any(entry.mnemonic == 'cmp' and any(operand.type == CS_OP_IMM and operand.imm == 40385 for operand in entry.operands)
           for entry in body), '40385 removal does not refresh the cached boarding flag'
refresh = '?UpdateForQuestWorldObjects@Player@@'
assert native['symbol'](refresh) in native['calls'](callback), 'Removal callback no longer publishes quest object updates'
assert native['symbol']('?GetConditionsForSpellClickEvent@ConditionMgr@@') in native['calls'](refresh), 'Refresh does not inspect actual source18 quest conditions'
assert native['symbol']('?AddDelayedEvent@FunctionProcessor@@') in native['calls']('?RemoveActiveQuest@Player@@'), 'Removal bypasses the real inlined Unit delayed callback queue'
name = '??_7?$_Func_impl_no_alloc@V<lambda_1>@?1??RemoveActiveQuest@Player@@QEAAXI@Z@X$$V@std@@6B@'
pattern = re.compile(r'\s+\w+:\w+\s+' + re.escape(name) + r'\s+([0-9a-fA-F]{16})\s')
tables = [int(match[1],16) for line in (Path(native['args'].release) / 'worldserver.map').read_text().splitlines()
          if (match := pattern.match(line))]
assert len(tables) == 1, 'Missing removal callback vtable'
table = tables[0]
slots = [struct.unpack_from('<Q',native['binary'],native['offset'](table)+slot)[0] for slot in range(0,64,8)]
assert native['symbol'](callback) in slots, 'Removal callback is not installed in the actual function vtable'
remove = instructions('?RemoveActiveQuest@Player@@')
assert any(operand.type == CS_OP_MEM and operand.mem.base == X86_REG_RIP and entry.address+entry.size+operand.mem.disp == table
           for entry in remove for operand in entry.operands), 'RemoveActiveQuest does not install the verified callback'
print('PASS: deployed click visibility/cast and independent SmartAI conditions remain linked; actual source17 lookup and queued RemoveActiveQuest callback refresh40385 feed native source18 quest-flag publication; no SQL-only or client-flight claim')
