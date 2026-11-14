"""Exercise the actual retained report-use proof with immutable PE byte-copy controls."""
from pathlib import Path
import ast
source_path=Path(__file__).with_name('verify_marksmanship_report_use_release.py')
from five_ten_20261111_pe import ins,n
from capstone import CS_OP_MEM,CS_OP_REG,CS_OP_IMM
source=Path(source_path).read_text(encoding='utf-8-sig')
node=next(x for x in ast.parse(source).body if isinstance(x,ast.FunctionDef) and x.name=='report_gossip_dispatch')
namespace=dict(CS_OP_MEM=CS_OP_MEM,CS_OP_REG=CS_OP_REG,CS_OP_IMM=CS_OP_IMM)
exec(compile(ast.Module(body=[node],type_ignores=[]),source_path,'exec'),namespace)
check=namespace['report_gossip_dispatch']
prefix='?HandleGameobjectReportUse@WorldSession@@'
def rows():return ins(prefix)
def matches():return check(rows(),n['symbol']('?AI@GameObject@@'),n['resolve'])
found=matches();assert len(found)==1
load_at,call_at=found[0];original_rows=rows();load=original_rows[load_at];call=original_rows[call_at]
assert call_at-load_at>3,'Must reproduce old scheduling blind spot'
setup=original_rows[load_at+1:call_at]
zero=next(x for x in setup if x.mnemonic=='xor' and x.op_str=='r8d, r8d')
this=next(x for x in setup if x.mnemonic=='mov' and x.op_str=='rcx, rax')
get_ai=original_rows[load_at-2];vptr=original_rows[load_at-1]
original=n['binary'];cases=1
for address,replacement,label in [
 (load.address+load.size-1,b'\x40','wrong slot'),
 (zero.address,b'\x45\x31\xc9','clobbered target'),
 (get_ai.address,b'\x90'*get_ai.size,'missing AI accessor'),
 (this.address,b'\x48\x89\xd9','wrong AI this'),
 (call.address,b'\x90'*call.size,'missing dispatch'),
 (zero.address,b'\x41\xb0\x01','wrong report flag'),
 (vptr.address,b'\x48\x8b\x0b','wrong vtable owner')]:
 try:
  damaged=bytearray(original);offset=n['offset'](address);damaged[offset:offset+len(replacement)]=replacement
  n['binary']=bytes(damaged);ins.cache_clear();assert not matches(),label;cases+=1
 finally:n['binary']=original;ins.cache_clear()
assert len(matches())==1 and n['binary']==original;cases+=1
print(f'PASS {cases} actual report-use dispatch controls:scheduled slot9/AI/this/false flag and seven corrupt byte-copy refusals; original PE unchanged')
