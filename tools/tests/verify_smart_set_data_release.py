"""Follow the real action45 switch to both guarded current-AI virtual calls.

The native ASAN fixture separately proves the baseline failures, lifecycle,
payloads, diagnostics, selector lifetime and existing LINK policy.
"""
from pathlib import Path
import hashlib,json,runpy,struct
from capstone import CS_OP_IMM,CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

n=runpy.run_path(str(Path(__file__).with_name('verify_smart_linked_cast_retry_release.py')))
a=n['instructions']('?ProcessAction@SmartScript@@')
m=next(i for i,x in enumerate(a) if x.mnemonic=='cmp' and any(o.type==CS_OP_IMM and o.imm==223 for o in x.operands))
assert a[m-1].mnemonic=='dec'
t=[o.mem for x in a[m+1:m+10] for o in x.operands if o.type==CS_OP_MEM and o.mem.index and o.mem.disp>0]
assert len(t)==2 and t[0].scale==1 and t[1].scale==4
def case(number):
    index=n['binary'][n['offset'](n['base']+t[0].disp+number-1)]
    return n['base']+struct.unpack_from('<I',n['binary'],n['offset'](n['base']+t[1].disp+index*4))[0]
first=case(45);end=min(x for x in {case(i) for i in range(1,225)} if x>first)
b=[x for x in a if first<=x.address<end]
pairs=[(x.mnemonic,x.op_str) for x in b]
def exact(sequence):
    found=[i for i in range(len(b)-len(sequence)+1) if pairs[i:i+len(sequence)]==sequence]
    assert len(found)==1,(sequence,found)
    return found[0]
def call(prefix):
    found=[i for i,x in enumerate(b) if x.mnemonic=='call' and x.operands[0].type==CS_OP_IMM and n['resolve'](x.operands[0].imm)==n['symbol'](prefix)]
    assert len(found)==1,(prefix,found)
    return found[0]
selector=call('?GetTargets@SmartScript@@')
creature_type=call('?IsCreature@SmartScript@@');go_type=call('?IsGameObject@SmartScript@@')
creature=exact([('mov','rcx, qword ptr [rax + 0x9c0]'),('test','rcx, rcx')])
assert b[creature+2].mnemonic=='je'
cvirtual=exact([('mov','rax, qword ptr [rcx]'),('mov','r9, qword ptr [rax + 0x78]'),
                ('mov','r8d, dword ptr [r15 + 0x5c]'),('mov','edx, dword ptr [r15 + 0x58]'),('call','r9')])
assert cvirtual==creature+3 and b[cvirtual+5].mnemonic=='jmp'
gameobject=call('?AI@GameObject@@')
assert pairs[gameobject+1]==('test','rax, rax') and b[gameobject+2].mnemonic=='je'
gvirtual=exact([('mov','rcx, qword ptr [rax]'),('mov','r9, qword ptr [rcx + 0x98]'),
                ('mov','r8d, dword ptr [r15 + 0x5c]'),('mov','edx, dword ptr [r15 + 0x58]'),
                ('mov','rcx, rax'),('call','r9')])
assert gvirtual==gameobject+3 and b[gvirtual+6].mnemonic=='jmp'
assert selector<creature_type<creature<cvirtual<go_type<gameobject<gvirtual
warning=b[creature+2].operands[0].imm
assert warning==b[gameobject+2].operands[0].imm and b[gvirtual+7].address==warning
assert b[cvirtual+5].operands[0].imm==b[gvirtual+6].operands[0].imm
assert pairs[creature_type+1]==('test','al, al') and b[creature_type+2].mnemonic=='je'
assert b[creature_type+2].operands[0].imm==b[go_type-2].address
assert pairs[go_type+1]==('test','al, al') and b[go_type+2].mnemonic=='je'
assert b[go_type+2].operands[0].imm==b[cvirtual+5].operands[0].imm
assert pairs[selector+1]==('mov','rsi, rax') and pairs[selector+3]==('test','rax, rax') and b[selector+4].mnemonic=='je'
messages=[]
for x in b:
    if x.mnemonic=='lea' and x.op_str.startswith('r9, ') and x.operands[1].type==CS_OP_MEM and x.operands[1].mem.base==X86_REG_RIP:
        at=n['offset'](x.address+x.size+x.operands[1].mem.disp)
        message=n['binary'][at:n['binary'].index(b'\0',at)]
        if b'SET_DATA' in message:messages.append(message)
assert len(messages)==1 and all(part in messages[0] for part in (
    b'SourceType %u Event %u',b'SET_DATA %u/%u not delivered',b'receiver %u (%s): no current AI.'))
assert call('?GetGUID@Object@@')>gvirtual and call('?GetEntry@Object@@')>gvirtual
print('PASS actual action45: current Creature/GO AI null guards precede the two native SetData virtual slots; both absent-AI branches reach explicit not-delivered source/event/payload/full-GUID warning; native target types, payload fields and list continuation retained')
print(json.dumps({'world_sha256':hashlib.sha256(n['binary']).hexdigest(),'action':45,'creature_set_data_slot':15,'gameobject_set_data_slot':19,'baseline':'9099990c6: two separate ASAN null-dispatch counterfactuals'}))
