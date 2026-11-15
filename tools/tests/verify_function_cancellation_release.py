"""Actual scheduler cancellation/rearm branches, not a source-only check."""
from pathlib import Path
import hashlib
reader=Path(__file__).with_name('verify_pending_ai_release.py');source=reader.read_text();ctx={'__file__':str(reader)};exec(compile(source[:source.index('plan=json.loads')],str(reader),'exec'),ctx)
n=ctx['n'];ins=ctx['instructions']
def sites(rows,prefix):
 targets={n['resolve'](v) for k,v in n['symbols'].items() if k.startswith(prefix)}
 return [i for i,x in enumerate(rows) if x.mnemonic in ('call','jmp') and x.op_str.startswith('0x') and n['resolve'](int(x.op_str,16)) in targets]
rows=ins('?Update@FunctionProcessor@@');merge,=sites(rows,'?AddFunctionsFromQueue@FunctionProcessor@@')
assert any(x.mnemonic=='cmp' and 'byte ptr [rcx + 0x78]' in x.op_str for x in rows[:merge]),'cancelled active generation not checked before fresh queue admission'
assert any(x.mnemonic=='mov' and x.op_str=='byte ptr [rsi + 0x78], bpl' for x in rows[:merge]),'old cancellation flag not cleared before fresh queue admission'
dispatch=[i for i,x in enumerate(rows) if x.mnemonic=='call' and x.op_str=='qword ptr [rax + 0x10]'];assert len(dispatch)==1
at=dispatch[0];assert any(x.mnemonic=='mov' and x.op_str=='qword ptr [rbx + 0x60], rbp' for x in rows[merge:at]),'executing function not moved out of its map node'
extracts=[i for i,x in enumerate(rows) if x.mnemonic=='call' and x.op_str.startswith('0x') and any('_Extract@' in k and n['resolve'](v)==n['resolve'](int(x.op_str,16)) for k,v in n['symbols'].items())]
assert any(merge<i<at for i in extracts),'map node not removed before callback invocation'
assert rows[at+1].mnemonic=='cmp' and rows[at+1].op_str=='byte ptr [rsi + 0x78], 0' and rows[at+2].mnemonic=='jne','callback cancellation not checked before another due event'
kill=ins('?KillAllFunctions@FunctionProcessor@@');assert any(x.mnemonic=='lea' and x.op_str=='rdx, [rbp + 0x18]' for x in kill),'pending queue not selected for cancellation'
assert any(x.mnemonic=='mov' and x.op_str=='byte ptr [rbp + 0x78], 1' for x in kill),'active cancellation request missing'
assert len([x for x in kill if x.mnemonic=='call'])>=2,'pending cancellation lacks mutex/clear path'
print('PASS linked FunctionProcessor cancellation: old generation cleared before fresh admission,map callback moved/removed before dispatch,post-callback cancel branch,pending queue mutex/clear path; SHA256='+hashlib.sha256(n['binary']).hexdigest().upper())
