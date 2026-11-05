"""Actual shared aborted-event lifetime gate and unchanged native spell callers."""
from pathlib import Path
import re
import runpy
import struct
from capstone import CS_OP_MEM
from sharasdal_erunak_entry_pe import sites

p=runpy.run_path(str(Path(__file__).with_name('verify_devourer_escape_release.py')))
n,ins=p['n'],p['ins']
pairs=lambda rows:{(x.mnemonic,x.op_str) for x in rows}
body=ins('?Update@EventProcessor@@')
virtual=lambda offset:[i for i,x in enumerate(body) if x.mnemonic=='call' and x.operands[0].type==CS_OP_MEM and x.operands[0].mem.disp==offset]
execute,=virtual(8);abort,=virtual(24);deletable,=virtual(16)
destroy=virtual(0)
assert len(destroy)==2 and execute<destroy[0]<abort<deletable<destroy[1]
assert body[deletable+1].op_str=='al, al' and body[deletable+2].mnemonic=='je'
retry_address=body[deletable+2].operands[0].imm
retry=next(i for i,x in enumerate(body) if x.address==retry_address)
assert retry>destroy[1]
# AddEvent is inlined here: the same event enters m_events_queue at current+1,
# retaining its original m_addTime. Its current-spell reference can then clear.
retry_rows=body[retry:]
assert {('mov','rbp, qword ptr [rsi]'),('inc','rbp'),('mov','qword ptr [rdi + 0x18], rbp'),('mov','qword ptr [rsp + 0x28], rdi'),('lea','rcx, [rsi + 0x18]')} <= pairs(retry_rows)
assert not any(x.mnemonic=='mov' and x.op_str.startswith('qword ptr [rdi + 0x10],') for x in retry_rows)
assert not any(x.mnemonic=='call' and x.operands[0].type==CS_OP_MEM for x in retry_rows)
assert ('cmp','byte ptr [rdi + 8], 0') in pairs(body[:abort])
assert sites(n,body,'?AddEventsFromQueue@EventProcessor@@')
text=(n['args'].release/'worldserver.map').read_text(errors='replace')
def table(name):
    rows=re.findall(r'\s+\w+:\w+\s+'+re.escape(name)+r'\s+([0-9a-fA-F]{16})',text)
    assert len(rows)==1,name
    return struct.unpack_from('<4Q',n['binary'],n['offset'](int(rows[0],16)))
assert n['resolve'](table('??_7SpellEvent@@6B@')[2])==n['symbol']('?IsDeletable@SpellEvent@@')
assert n['resolve'](table('??_7VehicleJoinEvent@@6B@')[2])==n['symbol']('?IsDeletable@BasicEvent@@')
# This three-byte leaf has no x64 unwind entry.
basic=n['offset'](n['symbol']('?IsDeletable@BasicEvent@@'))
assert n['binary'][basic:basic+3]==b'\xb0\x01\xc3' # mov al,1; ret
unit=ins('?Update@Unit@@')[:256]
events,=sites(n,unit,'?Update@EventProcessor@@');spells,=sites(n,unit,'?_UpdateSpells@Unit@@')
assert events<spells
print('PASS actual shared EventProcessor.Update: canceled flag reaches Abort then real virtual IsDeletable before destruction; held SpellEvent queues same event at current+1 without rewriting add-time or Execute, native Unit updates spells after events. SpellEvent override and ordinary immediately deletable VehicleJoinEvent vtables retained. No logger/SpellEvent destructor suppression or ABI field change. Native capture/client scope separate.')
