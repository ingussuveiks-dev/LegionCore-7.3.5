"""Exact corpse binding in the linked Althea subtype, with ordinary actors retained."""
from pathlib import Path
import runpy,struct,re,json,hashlib
from capstone import CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
from sharasdal_erunak_entry_pe import sites,values
n=runpy.run_path(str(Path(__file__).with_name('verify_smart_linked_cast_retry_release.py')))
ins=n['instructions']
def table(prefix):
    rows=[]
    for line in (n['args'].release/'worldserver.map').read_text(errors='replace').splitlines():
        match=re.match(r'\s+\w+:\w+\s+(\?\S+)\s+([0-9a-fA-F]{16})',line)
        if match and match[1].startswith(prefix):rows.append(int(match[2],16))
    assert len(rows)==1,(prefix,len(rows))
    return rows[0]
def refs(body):
    return {op.address+op.size+operand.mem.disp for op in body for operand in op.operands if operand.type==CS_OP_MEM and operand.mem.base==X86_REG_RIP}
def pairs(body):return [(op.mnemonic,op.op_str) for op in body]
base=table('??_7Conversation@@6BWorldObject@@@');derived=table('??_7AltheaDeathConversation@@6BWorldObject@@@')
raw=struct.unpack_from('<50Q',n['binary'],n['offset'](base))
slot,=[i for i,pointer in enumerate(raw) if n['resolve'](pointer)==n['symbol']('?FindCreatureActor@Conversation@@')]
assert slot*8>0x168 and raw[0x168//8]==n['symbol']('?CanNeverSee2@Conversation@@')
actual=struct.unpack_from('<'+str(slot+1)+'Q',n['binary'],n['offset'](derived))
assert n['resolve'](actual[slot])==n['symbol']('?FindCreatureActor@AltheaDeathConversation@@')
assert n['resolve'](actual[1])==n['symbol']('?AddToWorld@AltheaDeathConversation@@')
for i in range(2,slot):assert n['resolve'](actual[i])==n['resolve'](raw[i]),('Existing Conversation virtual slot changed',i)
producer=ins('?GenerateDynamicFieldData@Conversation@@')
assert len([op for op in producer if op.mnemonic=='call' and op.operands[0].type==CS_OP_MEM and op.operands[0].mem.disp==slot*8])==1
default=ins('?FindCreatureActor@Conversation@@')
assert sites(n,default,'?GetVisibilityRange@WorldObject@@') and sites(n,default,'?FindNearestCreature@WorldObject@@')
assert ('mov','r9b, 1') in pairs(default),'Default actor selection must remain alive-only'
hook=ins('?JustDied@npc_sister_althea_ebonlockeAI@')
assert derived in refs(hook),'Real death hook must install the scoped subtype vtable'
actor=ins('?FindCreatureActor@AltheaDeathConversation@@')
assert {108218,1620}<=values(actor)
assert {('test','edx, edx'),('test','r9, r9'),('movups','xmm0, xmmword ptr [rcx + 0x508]')}<=set(pairs(actor))
for target,count in [('?GetCreature@ObjectAccessor@@',1),('?IsInWorld@Object@@',1),('?IsPreDelete@Object@@',1),('?IsQuestObjectExpired@WorldObject@@',2),('?IsWithinDistInMap@WorldObject@@',1),('?InSamePhase@WorldObject@@',1),('?InSamePhaseId@WorldObject@@Q',1),('?CanTargetQuestObject@WorldObject@@',2)]:
    assert len(sites(n,actor,target))==count,('Missing scoped predicate',target)
assert not sites(n,actor,'?FindNearestCreature@WorldObject@@'),'Exact binding must never fall back to a duplicate'
admit=ins('?AddToWorld@AltheaDeathConversation@@')
lookup,=sites(n,admit,'?GetCreature@ObjectAccessor@@');scope,=sites(n,admit,'?InheritQuestObjectScope@WorldObject@@');publish,=sites(n,admit,'?AddToWorld@Conversation@@')
assert lookup<scope<publish and admit[publish].mnemonic=='jmp'
for index in (lookup,scope):
    assert admit[index+1].mnemonic=='test' and admit[index+2].mnemonic=='je'
    assert admit[index+2].operands[0].imm>admit[publish].address
print('PASS actual Althea2431 full-GUID scoped subtype/vtable, exact original slot/entry/map, world/predelete/range/phases/reciprocal scope; source scope is inherited after GUID creation and before world publication. Default resolver remains alive-only; all earlier non-destructor/non-overridden Conversation virtual slots retained. Native/ASAN evidence differs from client playback/full1123.')
print(json.dumps({'exe_sha256':hashlib.sha256(n['binary']).hexdigest(),'resolver_virtual_slot':slot*8,'source_guid_offset':0x508}))
