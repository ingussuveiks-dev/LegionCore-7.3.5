"""Bound only the known stack-free original-journal copy leaf."""
from five_ten_20261111_pe import n,decoder
def original_instance():
 start=n['symbol']('?UpdateOriginalInstance@BattlePetInstance@@')
 end=min(a for a in n['symbols'].values()if a>start)
 assert 0<end-start<=512,'Journal leaf needs independent reinspection'
 at=n['offset'](start);rows=list(decoder.disasm(n['binary'][at:at+end-start],start))
 assert rows and rows[-1].address+rows[-1].size==end,'Journal leaf decoding must be contiguous to the next independent symbol'
 while rows and rows[-1].mnemonic in('int3','nop'):rows.pop()
 assert rows[-1].mnemonic=='ret'
 assert not any(x.mnemonic in('call','push','pop')or x.op_str.startswith(('rsp,','rbp,'))for x in rows),'Known leaf must not acquire a frame/call'
 addresses={x.address for x in rows};sender=n['symbol']('?SendBattlePetUpdates@WorldSession@@')
 for x in rows:
  if not x.mnemonic.startswith('j'):continue
  assert x.op_str.startswith('0x'),'Journal leaf has an unproved indirect jump'
  target=x.operands[0].imm
  assert target in addresses or(x.mnemonic=='jmp'and n['resolve'](target)==sender),'Journal leaf branch leaves its independent bound'
 return rows
