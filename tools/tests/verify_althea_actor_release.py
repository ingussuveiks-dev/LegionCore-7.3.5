"""Exact corpse binding in the linked Althea subtype, with ordinary actors retained."""
from pathlib import Path
import runpy,struct,re,json,hashlib
from capstone import CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
from sharasdal_erunak_entry_pe import sites,values
from merlis_intro_pe import reachable
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
assert {('test','edx, edx'),('test','r9, r9')}<=set(pairs(actor))
assert any(operand.type==CS_OP_MEM and operand.mem.disp==0x508 for op in actor for operand in op.operands), 'Captured full source GUID not used'
for target,count in [('?GetCreature@ObjectAccessor@@',1),('?IsInWorld@Object@@',1),('?IsPreDelete@Object@@',1),('?IsQuestObjectExpired@WorldObject@@',2),('?IsWithinDistInMap@WorldObject@@',1),('?InSamePhase@WorldObject@@',1),('?InSamePhaseId@WorldObject@@Q',1),('?CanTargetQuestObject@WorldObject@@',2)]:
    assert len(sites(n,actor,target))==count,('Missing scoped predicate',target)
assert not sites(n,actor,'?FindNearestCreature@WorldObject@@'),'Exact binding must never fall back to a duplicate'
identity,=sites(n,actor,'??9ObjectGuid@@')
guid,=sites(n,actor,'?GetGuidValue@Object@@')
assert sites(n,actor,'?GetCreature@ObjectAccessor@@')[0]<guid<identity
assert pairs(actor[guid-2:guid])==[('xor','edx, edx'),('mov','rcx, rax')], 'Actual actor OBJECT_FIELD_GUID required'
assert pairs(actor[guid+1:identity])==[('mov','rdx, rsi'),('mov','rcx, rax')], 'Compare actual full field with captured GUID'
assert actor[identity+1].op_str=='al, al' and actor[identity+2].mnemonic=='jne'
success,=[op.address for op in actor if (op.mnemonic,op.op_str)==('mov','rax, rbx')]
assert success not in reachable(actor,actor[identity+2].operands[0].imm), 'Mismatched fullGUID reaches actor return'
admit=ins('?AddToWorld@AltheaDeathConversation@@')
lookup,=sites(n,admit,'?GetCreature@ObjectAccessor@@');identity,=sites(n,admit,'??8ObjectGuid@@');scope,=sites(n,admit,'?InheritQuestObjectScope@WorldObject@@');publish,=sites(n,admit,'?AddToWorld@Conversation@@')
assert lookup<identity<scope<publish and 108218 in values(admit)
assert sites(n,admit,'?IsInWorld@Object@@') and sites(n,admit,'?IsPreDelete@Object@@')
assert pairs(admit[lookup+1:lookup+3])==[('mov','rbx, rax'),('test','rax, rax')]
assert admit[lookup+3].mnemonic=='je' and admit[publish].address not in reachable(admit,admit[lookup+3].operands[0].imm)
for index in (identity,scope):
    assert admit[index+1].mnemonic=='test' and admit[index+2].mnemonic=='je'
    assert admit[publish].address not in reachable(admit,admit[index+2].operands[0].imm)
print('PASS actual Althea2431 scoped subtype/vtable: explicit full128-bit source GUID comparisons before actor return and world publication, original slot/entry/map, world/predelete/range/phases/reciprocal scope; source scope inherited after GUID creation and before publication. Default resolver remains alive-only; all other Conversation virtual slots retained. Controlled native identity collisions are separate from actual client/full1123 behavior.')
print(json.dumps({'exe_sha256':hashlib.sha256(n['binary']).hexdigest(),'resolver_virtual_slot':slot*8,'source_guid_offset':0x508}))
