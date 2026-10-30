"""Inspect the linked native projected-crack consumer, stores and selected AI."""
from pathlib import Path
import bisect,runpy,re,struct
from capstone import Cs,CS_ARCH_X86,CS_MODE_64,CS_OP_IMM,CS_OP_MEM,CS_OP_REG
from capstone.x86_const import X86_REG_RIP,X86_REG_RCX,X86_REG_EAX
n=runpy.run_path(str(Path(__file__).with_name('verify_scenario_lfg_release.py')))
d=Cs(CS_ARCH_X86,CS_MODE_64);d.detail=True
def ins(prefix):
 a=n['symbol'](prefix);i=bisect.bisect_right(n['starts'],a-n['base'])-1
 begin,end,unwind=n['functions'][i]
 if not begin<=a-n['base']<end:
  end=min(v for v in n['symbols'].values() if v>a)-n['base']
  assert n['base']+end-a<128
 else:
  origin=n['root'](begin,unwind)
  for b,e,u in n['functions'][i+1:]:
   if b!=end or n['root'](b,u)!=origin:break
   end=e
 return list(d.disasm(n['binary'][n['offset'](a):n['offset'](a)+n['base']+end-a],a))
def values(p):return {o.imm for x in ins(p) for o in x.operands if o.type==CS_OP_IMM}
def call(p,q):assert n['symbol'](q) in n['calls'](p),(p,q)
def refs(p):
 return {x.address+x.size+o.mem.disp for x in ins(p) for o in x.operands if o.type==CS_OP_MEM and o.mem.base==X86_REG_RIP}
update='?Update@StromkarGroundCracks@@';clear='?Clear@StromkarGroundCracks@@'
assert {103398,370991,1539,1037,7,500,900001,10000}<=values(update)
call(update,'?CreateAreaTrigger@AreaTrigger@@');call(update,clear);call(clear,'?Remove@AreaTrigger@@')
for p in ('?SpellHit@npc_stromkar_awakening@@','?UpdateAI@npc_stromkar_awakening@@'):call(p,update)
for p in ('?Abort@npc_stromkar_awakening@@','?UpdateAI@npc_stromkar_awakening@@'):call(p,clear)
duration='?CalculateDuration@at_stromkar_ground_cracks@@'
assert 2000 in values(duration)
factory='?GetAI@?$GenericAreaTriggerEntityScript@Uat_stromkar_ground_cracks@@@@'
selected=False
for a in refs(factory):
 try:table=struct.unpack_from('<40Q',n['binary'],n['offset'](a))
 except (ValueError,struct.error):continue
 if n['symbol'](duration) in table and n['symbol']('?CreateUnitList@at_stromkar_ground_cracks@@') in table:selected=True
assert selected,'Selected AT duration/unit-list vtable missing'
assert b'at_stromkar_ground_cracks\0' in n['binary']
# Data symbols are not code and cannot be read through the function-only map.
text=(n['args'].release/'worldserver.map').read_text(errors='replace')
# LookupEntry is inlined. Derive its bounds field from the actual native getter.
size_offsets={x.operands[1].mem.disp for x in ins('?GetNumRows@DB2StorageBase@@')
 if x.mnemonic=='mov' and len(x.operands)==2 and x.operands[0].type==CS_OP_REG
 and x.operands[0].reg==X86_REG_EAX and x.operands[1].type==CS_OP_MEM
 and x.operands[1].mem.base==X86_REG_RCX}
assert len(size_offsets)==1,size_offsets
size_offset=size_offsets.pop()
for prefix in ('?sDecalPropertiesStore@@','?sTextureBlendSetStore@@'):
 matches=re.findall(r'\s+\w+:\w+\s+('+re.escape(prefix)+r'\S*)\s+([0-9a-fA-F]{16})\s',text)
 assert len(matches)==1,matches
 address=int(matches[0][1],16)
 assert address in refs('?LoadStores@DB2Manager@@'),prefix+' not loaded/registered'
 assert address+size_offset in refs(update),prefix+' not checked by actual controller'
assert 797909 in values(update) or 797909 in values('?Supported@StromkarGroundCrackData@@')
print('PASS linked real burst/update/abort projection hooks, exact context/data guard, native AT creation/removal, selected2s/no-units AI, loaded and consumed native decal/blend stores. Client WMO projection remains pending.')
