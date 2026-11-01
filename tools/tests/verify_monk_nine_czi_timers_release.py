"""Selected registered Nine Czi AI, owned timers and immutable target in linked PE."""
from pathlib import Path
import hashlib
import json
import re
import runpy
import struct
from capstone import CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
from sharasdal_erunak_entry_pe import sites, values
from monk_intro_clock_pe import verify_clock

n=runpy.run_path(str(Path(__file__).with_name('verify_smart_linked_cast_retry_release.py')))
ins=n['instructions'];suffix='@npc_nine_cziAI@npc_nine_czi@@'
def method(name): return '?'+name+suffix
def refs(body):
    return {op.address+op.size+arg.mem.disp for op in body for arg in op.operands
            if arg.type==CS_OP_MEM and arg.mem.base==X86_REG_RIP}
def table(name,count):
    text=(n['args'].release/'worldserver.map').read_text(errors='replace')
    matches=re.findall(r'\s+\w+:\w+\s+'+re.escape(name)+r'\s+([0-9a-fA-F]{16})',text)
    assert len(matches)==1,('Selected vtable',name)
    address=int(matches[0],16)
    return address,struct.unpack_from('<'+str(count)+'Q',n['binary'],n['offset'](address))
def resolved(raw):
    return [n['resolve'](at) if n['base']<=at<n['base']+0x10000000 else at for at in raw]

actor_table,actor_slots=table('??_7npc_nine_cziAI@npc_nine_czi@@6B@',82)
factory=ins('?GetAI@npc_nine_czi@@')
assert actor_table in refs(factory) and sites(n,factory,'??0FunctionProcessor@@')
script_table,script_slots=table('??_7npc_nine_czi@@6BScriptObject@@@',16)
assert n['symbol']('?GetAI@npc_nine_czi@@') in resolved(script_slots)
assert script_table in refs(ins('?AddSC_monk_intro_scenario@@'))
slots=resolved(actor_slots)
assert slots[0]==n['symbol']('??_Gnpc_nine_cziAI@npc_nine_czi@@')
assert slots[0x110//8]==n['symbol'](method('UpdateLifecycle'))
assert all(n['symbol'](method(name)) in slots for name in ('MovementInform','MoveInLineOfSight','SetData','UpdateAI'))
assert any(op.mnemonic in ('call','jmp') and op.operands[0].type==CS_OP_MEM and op.operands[0].mem.disp==0x110 for op in ins('?Update@Creature@@'))
life=verify_clock(n,method('UpdateLifecycle'),0x40);destroy=ins('??_Gnpc_nine_cziAI@npc_nine_czi@@')
destroy_site,=sites(n,destroy,'??1FunctionProcessor@@')
assert (destroy[destroy_site-1].mnemonic,destroy[destroy_site-1].op_str)==('add','rcx, 0x40')
assert not sites(n,ins(method('UpdateAI')),'?Update@FunctionProcessor@@')

for name,delays in (('MovementInform',[4000]),('MoveInLineOfSight',[4000]),('SetData',[3000,7000])):
    body=ins(method(name));queued=sites(n,body,'?AddDelayedEvent@FunctionProcessor@@')
    assert len(queued)==len(delays) and not sites(n,body,'?AddDelayedEvent@Unit@@')
    for site,delay in zip(queued,delays):
        previous=[(x.mnemonic,x.op_str) for x in body[site-6:site]]
        assert any(m=='lea' and re.fullmatch(r'rcx, \[r(?:di|bx) \+ 0x40\]',s) for m,s in previous)
        assert ('mov',f'edx, {delay:#x}') in previous
    callbacks=[key for key in n['symbols'] if key.startswith('?_Do_call@') and '??'+name+suffix in key]
    assert len(callbacks)==len(delays)
    for callback in callbacks:
        assert any(n['symbol'](callback) in resolved(struct.unpack_from('<8Q',n['binary'],n['offset'](at))) for at in refs(body))
        cb=ins(callback)
        assert not sites(n,cb,'?AddDelayedEvent@Unit@@')
        if name=='MoveInLineOfSight':
            assert ('movups','xmm0, xmmword ptr [rcx + 0x10]') in [(x.mnemonic,x.op_str) for x in cb]
            assert not sites(n,cb,'?GetGUID@Object@@') and not sites(n,cb,'?FindNearestPlayer@WorldObject@@')
            assert sites(n,cb,'?Talk@CreatureAI@@') and sites(n,cb,'?DespawnOrUnsummon@Creature@@') and 3000 in values(cb)
            pairs=[(x.mnemonic,x.op_str) for x in body]
            assert ('movups','xmm0, xmmword ptr [rax]') in pairs and ('movaps','xmmword ptr [rsp + 0x60], xmm0') in pairs
        elif name=='MovementInform':
            assert sites(n,cb,'?FindNearestPlayer@WorldObject@@') and sites(n,cb,'?SetFlag@Object@@') and sites(n,cb,'?DoCast@UnitAI@@') and 193721 in values(cb)
        elif '<lambda_1>' in callback:
            assert sites(n,cb,'?Talk@CreatureAI@@') and sites(n,cb,'?UpdateAchievementCriteria@Player@@') and 46734 in values(cb)
        else:
            assert sites(n,cb,'?MovePoint@MotionMaster@@') and 1 in values(cb)
print('PASS actual registered Nine Czi factory/vtable/lifecycle110/destructor40, four owned timers3s/4s/4s/7s and actual callback vtables; departure captures full128-bit GUID without later Player dereference. Native Creature lifecycle dispatch retained, no UpdateAI double clock; original spell/credit/motion paths. Client/admission/whole intro remain separate.')
print(json.dumps({'exe_sha256':hashlib.sha256(n['binary']).hexdigest(),'selected_callbacks':4}))
