"""Verify actual native aura compatibility delegates to existing seat admission."""
from pathlib import Path
import runpy,struct,re
from capstone import CS_OP_IMM,CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
n=runpy.run_path(str(Path(__file__).with_name('verify_smart_linked_cast_retry_release.py')))
ins=n['instructions'];body=ins('?CanStackWith@Aura@@')
stamp=re.search(r'Timestamp is ([0-9a-fA-F]+)',(n['args'].release/'worldserver.map').read_text(errors='replace'));assert stamp and int(stamp[1],16)==struct.unpack_from('<I',n['binary'],n['pe']+8)[0]
def sites(rows,prefix):
 targets={n['resolve'](v) for k,v in n['symbols'].items() if k.startswith(prefix)}
 return [i for i,x in enumerate(rows) if x.mnemonic in ('call','jmp') and x.operands[0].type==CS_OP_IMM and n['resolve'](x.operands[0].imm) in targets]
checks=[i for i in sites(body,'?HasEffectType@Aura@@') if body[i-2].mnemonic=='mov' and body[i-2].op_str=='edx, 0xec']
a,b=checks
assert b==a+5 and body[a-1].op_str=='rcx, rdi' and body[b-1].op_str=='rcx, rbp'
assert body[a+1].op_str=='al, al' and body[a+2].mnemonic=='je' and body[a+2].operands[0].imm==body[b+3].address,'Release lacks non-evicting native vehicle aura compatibility'
assert body[b+1].op_str=='al, al' and body[b+2].mnemonic=='jne','Release lacks non-evicting native vehicle aura compatibility'
target=body[b+2].operands[0].imm
at=next(i for i,x in enumerate(body) if x.address==target)
assert (body[at].mnemonic,body[at].op_str)==('mov','al, 1') and body[at+1].mnemonic=='jmp'
remove=ins('?_RemoveNoStackAurasDueToAura@Unit@@');stack,=sites(remove,'?CanStackWith@Aura@@')
assert remove[stack+1].op_str=='al, al' and remove[stack+2].mnemonic=='jne'
assert sites(ins('?_AddAura@Unit@@'),'?_RemoveNoStackAurasDueToAura@Unit@@')
assert sites(ins('?HandleAuraControlVehicle@AuraEffect@@'),'?_EnterVehicle@Unit@@')
assert sites(ins('?_EnterVehicle@Unit@@'),'?AddPassenger@Vehicle@@')
event_table,=[int(v,16) for v in re.findall(r'^\s+\w+:\w+\s+\?\?_7VehicleJoinEvent@@6B@\s+([0-9a-fA-F]{16})\s',(n['args'].release/'worldserver.map').read_text(errors='replace'),re.M)]
add=ins('?AddPassenger@Vehicle@@')
assert any(x.mnemonic=='lea' and any(o.type==CS_OP_MEM and o.mem.base==X86_REG_RIP and x.address+x.size+o.mem.disp==event_table for o in x.operands) for x in add)
assert struct.unpack_from('<4Q',n['binary'],n['offset'](event_table))[1]==n['symbol']('?Execute@VehicleJoinEvent@@')
assert sites(add,'?AddEvent@EventProcessor@@')
assert sites(ins('?Abort@VehicleJoinEvent@@'),'?RemoveAurasByType@Unit@@') or sites(ins('?Abort@VehicleJoinEvent@@'),'?RemoveAurasDueToSpell@Unit@@')
print('PASS selected native Unit aura insertion/nonstack dispatch->actual Aura::CanStackWith:both CONTROL_VEHICLE236 checks join immediate true compatibility,not capacity-driven existing-aura removal;native control aura->_EnterVehicle->AddPassenger/queued join+abort retained. Actual seat admission/refusal/explicit replacement behavior verified by separate native16;client traversal separate.')
