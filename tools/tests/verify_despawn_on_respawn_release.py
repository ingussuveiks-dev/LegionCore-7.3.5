"""Verify actor capture, original100ms queue and respawn branches in the PE."""
from pathlib import Path
import hashlib,re
# Reuse the matching PE/unwind/thunk reader and outlined-lambda expansion,
# stopping before that verifier's unrelated per-encounter assertions.
reader=Path(__file__).with_name('verify_pending_ai_release.py')
ctx={'__file__':str(reader)};source=reader.read_text();exec(compile(source[:source.index('plan=json.loads')],str(reader),'exec'),ctx)
n=ctx['n'];ins=ctx['instructions'];expanded=ctx['expanded']
def targets(prefix):return {n['resolve'](v) for k,v in n['symbols'].items() if k.startswith(prefix)}
def site(rows,prefix):
 t=targets(prefix);return [i for i,x in enumerate(rows) if x.mnemonic in ('call','jmp') and x.op_str.startswith('0x') and n['resolve'](int(x.op_str,16)) in t]
names=[s for s in n['symbols'] if s.startswith('?_Do_call@') and 'DespawnOnRespawn@CreatureAI@@' in s and s.endswith('EEAAXXZ')];assert len(names)==1
rows=expanded(names[0]);at,=site(rows,'?DespawnOrUnsummon@Creature@@')
# std::function supplies its own closure pointer. The native despawn argument
# must be the directly captured actor, with no second read through retired AI.
state={'rcx':'closure'}
for x in rows[:at]:
 if x.mnemonic=='mov':
  m=re.fullmatch(r'(r\w+), qword ptr \[(r\w+)(?: \+ (0x[0-9a-f]+|\d+))?\]',x.op_str)
  if m:state[m[1]]='capture' if state.get(m[2])=='closure' else 'indirect';continue
  m=re.fullmatch(r'(r\w+), (r\w+)',x.op_str)
  if m:state[m[1]]=state.get(m[2],'other');continue
 if x.mnemonic in ('lea','xor'):
  state[x.op_str.split(',')[0]]='other'
assert state.get('rcx')=='capture','despawn still dereferences captured AI to reach actor'
assert len(site(rows,'?GetGameTime@GameTime@@'))==1
assert any(x.mnemonic=='test' and x.op_str=='ebx, ebx' for x in rows),'zero respawn branch missing'
assert any(x.mnemonic=='add' and x.op_str=='rax, rbx' for x in rows),'nonzero original seconds missing'
stores=[x for x in rows if x.mnemonic=='mov' and x.op_str.startswith('qword ptr [rdi + ') and (x.op_str.endswith(', rax') or x.op_str.endswith(', 0'))]
assert len(stores)==2 and stores[0].op_str.split('],')[0]==stores[1].op_str.split('],')[0],'zero/nonzero respawn no longer share actor field'
helper=ins('?DespawnOnRespawn@CreatureAI@@');queue,=site(helper,'?AddDelayedEvent@FunctionProcessor@@')
assert any(x.mnemonic=='mov' and x.op_str=='edx, 0x64' for x in helper[max(0,queue-5):queue]),'original100ms Unit queue delay missing'
assert any(x.mnemonic=='mov' and x.op_str=='rcx, qword ptr [rcx + 0x10]' for x in helper[:8]),'CreatureAI actor not captured at scheduling time'
stamp=re.search(r'Timestamp is ([0-9a-fA-F]+)',(n['args'].release/'worldserver.map').read_text(errors='replace'));assert stamp and int(stamp[1],16)==n['u32'](n['pe']+8)
print('PASS linked complete DespawnOnRespawn: direct actor capture/no retired-AI dereference,native100ms queue/despawn/zero-nonzero respawn branches,matching PE-map; SHA256='+hashlib.sha256(n['binary']).hexdigest().upper())
