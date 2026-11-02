"""Actual registered Chen queue, immutable players and original native NPC text."""
from pathlib import Path
import hashlib
import json
import re
import runpy
import struct
from capstone import CS_OP_MEM,CS_OP_IMM
from sharasdal_erunak_entry_pe import sites, values
from monk_intro_clock_pe import verify_clock

p=runpy.run_path(str(Path(__file__).with_name('verify_monk_nine_czi_timers_release.py')))
n,refs,table,resolved=p['n'],p['refs'],p['table'],p['resolved']
ins=n['instructions'];suffix='@npc_monk_chen_porterAI@npc_monk_chen_porter@@'
def method(name): return '?'+name+suffix
actor_table,raw=table('??_7npc_monk_chen_porterAI@npc_monk_chen_porter@@6B@',82)
script_table,script_raw=table('??_7npc_monk_chen_porter@@6BScriptObject@@@',16)
factory=ins('?GetAI@npc_monk_chen_porter@@')
assert actor_table in refs(factory) and sites(n,factory,'??0FunctionProcessor@@')
assert n['symbol']('?GetAI@npc_monk_chen_porter@@') in resolved(script_raw)
assert script_table in refs(ins('?AddSC_monk_intro_scenario@@'))
slots=resolved(raw)
assert slots[0]==n['symbol']('??_Gnpc_monk_chen_porterAI@npc_monk_chen_porter@@')
assert slots[0x110//8]==n['symbol'](method('UpdateLifecycle'))
assert all(n['symbol'](method(name)) in slots for name in ('MovementInform','MoveInLineOfSight','SetData','UpdateAI'))
life=verify_clock(n,method('UpdateLifecycle'),0x40);destroy=ins('??_Gnpc_monk_chen_porterAI@npc_monk_chen_porter@@')
destroy_site,=sites(n,destroy,'??1FunctionProcessor@@')
assert (destroy[destroy_site-1].mnemonic,destroy[destroy_site-1].op_str)==('add','rcx, 0x40')
assert not sites(n,ins(method('UpdateAI')),'?Update@FunctionProcessor@@')
guid_callbacks=0;native_text_calls=0
def reaches_without(body,target,blocked):
    by_address={op.address:i for i,op in enumerate(body)};pending=[body[0].address];seen=set()
    while pending:
        address=pending.pop()
        if address in seen or address in blocked or address not in by_address:continue
        if address==target:return True
        seen.add(address);op=body[by_address[address]]
        if op.mnemonic in ('ret','int3','ud2'):continue
        if op.mnemonic.startswith('j') and op.operands[0].type==CS_OP_IMM:pending.append(op.operands[0].imm)
        if op.mnemonic!='jmp':pending.append(op.address+op.size)
    return False
for name,delays in (('MovementInform',[3000,7000,11000,15000,15000,16000]),('SetData',[4000,5000,8000])):
    body=ins(method(name));queued=sites(n,body,'?AddDelayedEvent@FunctionProcessor@@')
    assert len(queued)==len(delays) and not sites(n,body,'?AddDelayedEvent@Unit@@')
    for at,delay in zip(queued,delays):
        previous=[(op.mnemonic,op.op_str) for op in body[at-6:at]]
        direct=any(m=='lea' and re.fullmatch(r'rcx, \[r(?:di|bx) \+ 0x40\]',s) for m,s in previous)
        if not direct:
            # The scoped presentation successor retains this exact queue in
            # callee-saved RBX across the four personal unscoped callbacks.
            assert name=='MovementInform' and ('mov','rcx, rbx') in previous
            clocks=[i for i,op in enumerate(body[:at]) if (op.mnemonic,op.op_str)==('lea','rbx, [rdi + 0x40]')]
            assert clocks and not reaches_without(body,body[at].address,{body[i].address for i in clocks})
            assert not any(op.op_str.startswith('rbx, ') and op.mnemonic not in ('cmp','test') and i not in clocks for i,op in enumerate(body[:at]) if i>min(clocks))
        assert ('mov',f'edx, {delay:#x}') in previous
    callbacks=[key for key in n['symbols'] if key.startswith('?_Do_call@') and '??'+name+suffix in key]
    assert len(callbacks)==len(delays)
    for callback in callbacks:
        assert any(n['symbol'](callback) in resolved(struct.unpack_from('<8Q',n['binary'],n['offset'](address))) for address in refs(body))
        cb=ins(callback);pairs=[(op.mnemonic,op.op_str) for op in cb]
        assert not sites(n,cb,'?AddDelayedEvent@Unit@@')
        if name=='MovementInform' and any('<lambda_'+str(i)+'>' in callback for i in (2,3,4,5)):
            assert ('movups','xmm0, xmmword ptr [rcx + 0x10]') in pairs or any(op.mnemonic=='movups' and op.op_str.endswith(' + 0x10]') for op in cb)
            assert not sites(n,cb,'?GetGUID@Object@@') and not sites(n,cb,'?FindNearestPlayer@WorldObject@@')
            guid_callbacks+=1
        if (name=='MovementInform' and any('<lambda_'+str(i)+'>' in callback for i in (1,3,4,5))) or (name=='SetData' and '<lambda_2>' in callback):
            assert len(sites(n,cb,'?SendChat@CreatureTextMgr@@'))==1
            assert not sites(n,cb,'?Talk@CreatureAI@@') and not any(arg.type==CS_OP_MEM and arg.mem.disp==0x9c0 for op in cb for arg in op.operands),'Native text retains the actual source even without target AI'
            native_text_calls+=1
        if name=='MovementInform' and '<lambda_5>' in callback:
            assert sites(n,cb,'?MovePath@MotionMaster@@') and 9859006 in values(cb)
        if name=='MovementInform' and '<lambda_6>' in callback:
            assert sites(n,cb,'?MovePoint@MotionMaster@@')
        if name=='SetData' and '<lambda_2>' in callback:
            assert sites(n,cb,'?MoveFollow@MotionMaster@@')
        if name=='SetData' and '<lambda_3>' in callback:
            assert sites(n,cb,'?MovePath@MotionMaster@@') and 9859005 in values(cb)
assert guid_callbacks==4 and native_text_calls==5
print('PASS actual registered Chen factory/vtable/lifecycle110/destructor40, nine owned exact timers/callback vtables, four immutable full GUID captures and five original native CreatureTextMgr text calls without target-AI dereference; independent original follow/path calls retained. Nine Czi/native lifecycle/retry PE retained; client/admission/full943 remain separate.')
print(json.dumps({'exe_sha256':hashlib.sha256(n['binary']).hexdigest(),'selected_callbacks':9,'guid_captures':4,'native_text_calls':5}))
