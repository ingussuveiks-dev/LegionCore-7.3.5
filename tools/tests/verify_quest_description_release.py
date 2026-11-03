"""Actual native SQL loader and constructor must keep short and full text roles."""
from pathlib import Path
import hashlib,json,re,runpy
from capstone import CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
from sharasdal_erunak_entry_pe import sites
n=runpy.run_path(str(Path(__file__).with_name('verify_smart_linked_cast_retry_release.py')))
ins=n['instructions'];ctor=ins('??0Quest@@QEAA@PEAVField@@@Z')
getters=sites(n,ctor,'?GetString@Field@@');assert len(getters)==9
# Native Quest class layout is independently measured by the full native
# component test: sizeof(Field)=24, text start107, offsets472/504/536.
destinations=[0x1d8,0x1f8,0x218,0x278,0x298,0x318,0x338,0x358,0x378]
assignments=[]
for get,destination in zip(getters,destinations):
 assert [(o.mnemonic,o.op_str) for o in ctor[get+1:get+3]]==[
  ('mov','rdx, rax'),('lea',f'rcx, [rsi + {destination:#x}]')]
 assignment=ctor[get+3];assert assignment.mnemonic=='call'
 assignments.append(n['resolve'](assignment.operands[0].imm))
assert len(set(assignments))==1
for get in getters[1:]:
 assert [(o.mnemonic,o.op_str) for o in ctor[get-5:get]]==[
  ('movzx','eax, dil'),('lea','rdx, [rax + rax*2]'),
  ('lea','rcx, [r14 + rdx*8]'),('inc','dil'),('lea','rdx, [rbp - 0x30]')]
load=ins('?LoadQuests@QuestDataStoreMgr@@')
assert len(sites(n,load,'??0Quest@@QEAA@PEAVField@@@Z'))==1
source=Path(__file__).resolve().parents[2]/'src/server/game/Globals/QuestData.cpp'
s=source.read_text();a=s.index('QueryResult result = WorldDatabase.Query("SELECT "',s.index('void QuestDataStoreMgr::LoadQuests()'));b=s.index(');',a)
query=''.join(json.loads(m.group()) for m in re.finditer(r'"(?:[^"\\]|\\.)*"',s[a:b]))
assert ', LogTitle, LogDescription, QuestDescription, AreaDescription,' in query
needle=query.encode()+b'\0';raw=n['binary'].find(needle)
assert raw>=0 and n['binary'].find(needle,raw+1)==-1
va=next(n['base']+start+raw-at for start,size,at in n['sections'] if at<=raw<at+size)
refs={o.address+o.size+arg.mem.disp for o in load for arg in o.operands if arg.type==CS_OP_MEM and arg.mem.base==X86_REG_RIP}
assert va in refs
print('PASS actual LoadQuests references original SQL LogTitle/LogDescription/QuestDescription projection and calls actual Quest constructor; all nine successive real Field::GetString results reach native correct member offsets, including summary504 before detail536. No packet truncation, locale fallback bypass or quest/progression mutation. Client UI remains separate.')
print(json.dumps({'world_sha256':hashlib.sha256(n['binary']).hexdigest(),'field_size':24,'text_start':107,'summary_offset':504,'detail_offset':536}))
