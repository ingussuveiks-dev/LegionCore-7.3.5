from linked_player_abi import inherited_operand
"""Instance-owned authored guard identities/death producer and consumer PE proof."""
import re,struct
from capstone import CS_OP_MEM,CS_OP_IMM,CS_OP_REG
from capstone.x86_const import X86_REG_RIP
from sharasdal_erunak_entry_pe import sites,values
INSTANCE='instance_kingslayers_InstanceMapScript@instance_kingslayers@@'
ACTOR='npc_sister_althea_ebonlockeAI@npc_sister_althea_ebonlocke@@'
def verify(n):
 ins=n['instructions'];method=lambda name:'?'+name+'@'+INSTANCE
 def refs(body):return {x.address+x.size+o.mem.disp for x in body for o in x.operands if o.type==CS_OP_MEM and o.mem.base==X86_REG_RIP}
 def pairs(body):return [(x.mnemonic,x.op_str) for x in body]
 def data_values(body):
  found=values(body).copy();addresses=refs(body)
  # MSVC can materialize the PE image base then index a static constexpr array
  # through an image-relative displacement instead of a RIP-relative operand.
  for x in body:
   for o in x.operands:
    if o.type==CS_OP_MEM and o.mem.disp>=0x10000:addresses.add(n['base']+o.mem.disp)
  for at in addresses:
   try:found.update(struct.unpack_from('<3I',n['binary'],n['offset'](at)))
   except (ValueError,struct.error):pass
  return found
 def virtual(body,displacement):
  result=[]
  for i,x in enumerate(body):
   if x.mnemonic in ('call','jmp') and x.operands[0].type==CS_OP_MEM and x.operands[0].mem.disp==displacement:result.append(i)
   if x.mnemonic=='mov' and len(x.operands)==2 and x.operands[0].type==CS_OP_REG and x.operands[1].type==CS_OP_MEM and x.operands[1].mem.disp==displacement:
    reg=x.operands[0].reg
    for j in range(i+1,min(i+6,len(body))):
     if body[j].mnemonic in ('call','jmp') and body[j].operands[0].type==CS_OP_REG and body[j].operands[0].reg==reg:result.append(j);break
  return sorted(set(result))
 scope=ins(method('GuardSlot'));create=ins(method('OnCreatureCreateForScript'));remove=ins(method('OnCreatureRemoveForScript'));death=ins(method('CreatureDiesForScript'));state=ins(method('GetData'));guid=ins(method('GetGuidData'))
 assert {1620,3,360869,360872,360871,108216,108217}<=data_values(scope)
 assert ('cmp','qword ptr [rdi + 8], rax') in pairs(scope) and ('cmp',inherited_operand('qword ptr [rdi + 0x3a88], rax')) in pairs(scope),'Exact map and persistent spawn qualification missing'
 assert sites(n,scope,'?GetUInt32Value@Object@@'),'Actual guard entry qualification missing'
 for body,callback in ((create,'OnCreatureCreateForScript'),(remove,'OnCreatureRemoveForScript'),(death,'CreatureDiesForScript')):
  assert len(sites(n,body,method('GuardSlot')))==1 and len(sites(n,body,'?'+callback+'@InstanceScript@@'))==1
  assert sites(n,body,'?GetGuidValue@Object@@')
 for body in (remove,death):assert len(sites(n,body,'??8ObjectGuid@@'))==1,'Full runtime GUID binding must reject stale actors'
 assert ('mov','byte ptr [rsi + rdi + 0x2f0], 1') in pairs(death)
 assert ('cmp','dword ptr [rbx + 0xa18], 0') in pairs(death),'Alive callbacks must not set a death receipt'
 assert ('mov','byte ptr [rsi + rdi + 0x2f0], 0') in pairs(remove),'A living removal must clear a stale receipt'
 assert ('mov','byte ptr [rdx + rdi + 0x2f0], 0') in pairs(create),'New living incarnation must clear old receipt'
 assert sites(n,state,'?GetCreature@Map@@') and {108218,3}<=values(state)
 assert ('mov','byte ptr [rbx], r12b') in pairs(state) and ('cmp','dword ptr [rax + 0xa18], r12d') in pairs(state),'Same-GUID living respawn must invalidate prior death'
 assert ('add','rbx, 0x11') in pairs(state) and ('imul','r14, rax, 0x11') in pairs(state),'Actual packed three-record instance stride changed'
 assert {360869,360872,360871}<=data_values(state+guid)
 assert sites(n,guid,'?GetGuidData@InstanceScript@@') and any(x.mnemonic=='movups' and x.op_str.startswith('xmm0, xmmword ptr') for x in guid)
 table,=re.findall(r'\s+\w+:\w+\s+\?\?_7'+re.escape(INSTANCE)+r'6B@\s+([0-9a-fA-F]{16})',(n['args'].release/'worldserver.map').read_text(errors='replace'));address=int(table,16)
 slots=struct.unpack_from('<25Q',n['binary'],n['offset'](address));indexes={}
 for name in ('OnCreatureCreateForScript','OnCreatureRemoveForScript','CreatureDiesForScript','GetData','GetGuidData'):
  slot,=[i for i,at in enumerate(slots) if n['resolve'](at)==n['symbol'](method(name))];indexes[name]=slot*8
 factory=ins('?GetInstanceScript@instance_kingslayers@@');installed=refs(factory)
 ctor='??0'+INSTANCE
 if sites(n,factory,ctor):installed |= refs(ins(ctor))
 assert address in installed,'Actual registered instance factory must install new receipt handlers'
 for caller,name in (('?AddToWorld@Creature@@','OnCreatureCreateForScript'),('?RemoveFromWorld@Creature@@','OnCreatureRemoveForScript'),('?Kill@Unit@@','CreatureDiesForScript')):
  assert virtual(ins(caller),indexes[name]),(caller,name,indexes[name])
 tick=ins('?UpdateIntro@'+ACTOR);los=ins('?MoveInLineOfSight@'+ACTOR)
 assert not sites(n,tick,'?FindNearestCreature@WorldObject@@') and not sites(n,los,'?GetCreatureListWithEntryInGrid@WorldObject@@'),'Guard disappearance/radius must not stand in for death'
 query,=virtual(tick,indexes['GetData']);pick,second=sites(n,tick,'?IntroOpponent@'+ACTOR);assert query<pick<second and 108218 in values(tick[:query])
 assert tick[query+1].mnemonic=='test' and tick[query+2].mnemonic=='je' and tick[query+2].operands[0].imm>tick[pick].address,'Unacknowledged death checkpoint must skip release'
 assert virtual(los,indexes['GetGuidData']) and len(virtual(los,indexes['GetData']))==2
 assert {360869,360872,360871,108216,108217}<=data_values(los)
 assert len(sites(n,los,'?GetCreature@ObjectAccessor@@'))==3,'Original guard and companion full identities must be re-resolved across callbacks'
 assert ('call','qword ptr [rax + 0x10]') in pairs(los),'Native guard AttackStart missing'
 for part in (scope,create,remove,death,state,guid,tick):
  for forbidden in ('?UpdateAchievementCriteria@Player@@','?CompleteQuest@Player@@','?AddItem@Player@@','?KilledMonsterCredit@Player@@'):assert not sites(n,part,forbidden)
 print('PASS actual instance_kingslayers factory/vtable and native Creature create/remove/Unit::Kill dispatch, exact1620/three DB spawns+entries/full GUIDs, genuine death-only idempotent receipts, living replacement/respawn/removal invalidation and corpse retention. Actual Althea preflight resolves three original guard roles and re-resolves live guard/companion across callbacks; release requires instance acknowledgement, with no spatial absence, quest/item/victory/door grant or static cross-instance cache. Native retired-AI/pet/guardian/lifecycle cases remain distinct from client combat/traversal.')
