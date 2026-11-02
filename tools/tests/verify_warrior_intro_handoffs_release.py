"""Immediate shared-actor calls use original targets and typed finite recovery."""
from pathlib import Path
import hashlib,json,re,runpy
from capstone import Cs,CS_ARCH_X86,CS_MODE_64,CS_OP_IMM
from sharasdal_erunak_entry_pe import sites,values,fields

p=runpy.run_path(str(Path(__file__).with_name('verify_warrior_intro_timers_release.py')))
n,ins,refs,table,resolved,reachable=(p[k] for k in ('n','ins','refs','table','resolved','reachable'))
new_ritual=p.get('new_ritual',False)
def pairs(body):return [(op.mnemonic,op.op_str) for op in body]
maptext=(n['args'].release/'worldserver.map').read_text(errors='replace')
def address(name):
 found=re.findall(r'\s+\w+:\w+\s+'+re.escape(name)+r'\s+([0-9a-fA-F]{16})',maptext)
 assert len(found)==1,('Selected native symbol',name)
 return int(found[0],16)
helper='?AttackBattleTarget@npc_ragnvald_dragonbornAI@'
for actor,target,allies in (('npc_ragnvald_dragonborn',98257,{102990,102995,97166}),('npc_bezzered',97167,{97176})):
 suffix=actor+'AI@'+actor+'@@';body=ins('?DoAction@'+suffix)
 vt,raw=table('??_7'+suffix+'6B@',82)
 assert n['symbol']('?DoAction@'+suffix) in resolved(raw)
 assert vt in refs(ins('?GetAI@'+actor+'@@'))
 at,=sites(n,body,helper)
 assert {target,105419}|allies<=values(body)
 assert 0x9c0 not in fields(body),'Foreign current AI must be resolved by the guarded helper'
 if actor=='npc_ragnvald_dragonborn' and p.get('new_opening'):
  from warrior_opening_pe import verify_second_battle
  verify_second_battle(n,body,reachable)
 else:
  assert {('mov','r9b, 0xa'),('xor','r8d, r8d'),('movups','xmm0, xmmword ptr [rax]'),('movaps','xmmword ptr [rsp + 0x50], xmm0')}<=set(pairs(body[at-12:at]))
  assert len(sites(n,body,'?GetGuidValue@Object@@'))==1
 assert any(op.mnemonic=='call' and op.op_str=='qword ptr [rax + 0x10]' for op in body),'Original own AttackStart lost'

if p.get('new_ritual'):
 from warrior_ritual_pe import verify_presentation_handoff
 verify_presentation_handoff(n,ins,refs,table,resolved,reachable)
 print('PASS actual Ragnvald/Bezzeredes immediate full-GUID combat handoffs; native97166 text, owned spirit departure and actual933 stage3/native Ragnvald+Bezzeredes receipt are now instance-owned and checked against linked ritual code. Legacy AI-owned Danica closure/ResumeBattle no longer selected. Full933/client remains separate.')
else:
 danica=ins('?IsSummonedBy@npc_danica_the_reclaimerAI@');send,=sites(n,danica,'?SendChat@CreatureTextMgr@@')
 assert not sites(n,danica,'?Talk@CreatureAI@@') and 0x9c0 not in fields(danica)
 assert pairs(danica[send-4:send])==[('lea','r9, [rbp - 0x80]'),('xor','r8d, r8d'),('mov','rdx, rbx'),('mov','rcx, rax')]
 closest=[at for at in sites(n,danica,'?GetClosestCreatureWithEntry@@') if at<send][-1]
 assert ('mov','edx, 0x17b8e') in pairs(danica[closest-4:closest]) and danica[closest+1].op_str=='rbx, rax'
 branch=danica[closest+3];assert branch.mnemonic=='je' and danica[send].address not in reachable(danica,branch.operands[0].imm)
 empty=next(op for op in danica[closest:send] if op.mnemonic=='movups' and op.op_str.startswith('xmm0, xmmword ptr [rip'))
 at=empty.address+empty.size+empty.operands[1].mem.disp
 assert at==address('?Empty@ObjectGuid@@2V1@B'),'Whisper must use the actual native ObjectGuid::Empty global'
 # Empty is in zero-initialized virtual storage; raw bytes at a synthetic
 # section offset do not represent its runtime contents. Check its native
 # default constructor and exact global identity instead.
 ctor=n['symbol']('??0ObjectGuid@@QEAA@XZ');pos=n['offset'](ctor)
 guid_ctor=list(Cs(CS_ARCH_X86,CS_MODE_64).disasm(n['binary'][pos:pos+16],ctor))
 assert pairs(guid_ctor[:5])==[('xor','eax, eax'),('mov','qword ptr [rcx], rax'),('mov','qword ptr [rcx + 8], rax'),('mov','rax, rcx'),('ret','')]

 resume='?ResumeBattle@npc_ragnvald_dragonbornAI@';body=ins(resume)
 queue,=sites(n,body,'?AddDelayedEvent@FunctionProcessor@@')
 cast_target=address('__RTDynamicCast')
 cast,=[i for i,op in enumerate(body) if op.mnemonic=='call' and op.operands[0].type==CS_OP_IMM and op.operands[0].imm==cast_target]
 assert {address('??_R0?AVCreatureAI@@@8'),address('??_R0?AUnpc_ragnvald_dragonbornAI@npc_ragnvald_dragonborn@@@8')}<=refs(body[:cast])
 assert pairs(body[cast-2:cast])==[('xor','edx, edx'),('mov','rcx, qword ptr [rbx + 0x9c0]')]
 assert pairs(body[cast+1:cast+2])==[('test','rax, rax')] and body[cast+2].mnemonic=='je'
 call,=[i for i,op in enumerate(body) if op.mnemonic=='call' and op.op_str=='r8']
 assert pairs(body[call-4:call])==[('mov','rcx, qword ptr [rax]'),('mov','r8, qword ptr [rcx + 0x48]'),('mov','edx, 1'),('mov','rcx, rax')]
 vt,raw=table('??_7npc_ragnvald_dragonbornAI@npc_ragnvald_dragonborn@@6B@',82)
 assert resolved(raw)[0x48//8]==n['symbol']('?DoAction@npc_ragnvald_dragonbornAI@')
 assert body[queue].address not in reachable(body,body[call+1].address)
 for condition in ('dword ptr [rbx + 0xa18], 0','eax, 0x17b8f','dword ptr [rbx + 0xe0], 0x5dc','eax, 0x3a5'):
  i=next(i for i,op in enumerate(body[:cast]) if op.mnemonic=='cmp' and op.op_str==condition)
  assert body[i+1].mnemonic=='jne'
  rejected=reachable(body,body[i+1].operands[0].imm)
  assert body[queue].address not in rejected and body[call].address not in rejected and body[cast].address not in rejected
 for dependency,jump in (('?IsInWorld@Object@@','je'),('?IsPreDelete@Object@@','jne')):
  checked,=sites(n,body,dependency)
  assert body[checked+1].op_str=='al, al' and body[checked+2].mnemonic==jump
  rejected=reachable(body,body[checked+2].operands[0].imm)
  assert body[queue].address not in rejected and body[call].address not in rejected
 scenario,=sites(n,body,'?GetScenario@ScenarioMgr@@')
 assert ('mov','edx, dword ptr [rbx + 0x3b8]') in pairs(body[scenario-3:scenario])
 assert body[scenario+1].op_str=='rdi, rax' and body[scenario+2].op_str=='rax, rax' and body[scenario+3].mnemonic=='je'
 missing=reachable(body,body[scenario+3].operands[0].imm)
 assert body[queue].address in missing and body[cast].address not in missing
 steps=sites(n,body,'?GetCurrentStep@Scenario@@');assert len(steps)==3
 for checked,value,jump in zip(steps,(2,3,3),('jb','ja','jne')):
  assert pairs(body)[checked+1]==('cmp',f'eax, {value}') and body[checked+2].mnemonic==jump
  rejected=reachable(body,body[checked+2].operands[0].imm)
  assert body[cast].address not in rejected and body[call].address not in rejected
  assert (body[queue].address in rejected)==(jump=='jne')
 remaining=next(i for i,op in enumerate(body) if op.mnemonic=='test' and op.op_str=='sil, sil')
 assert body[remaining+1].mnemonic=='jne' and body[queue].address in reachable(body,body[remaining+1].operands[0].imm)
 assert body[queue].address not in reachable(body,body[remaining+2].address)
 assert {('mov','rbx, rcx'),('lea','rcx, [rbx + 0x36a0]'),('mov','edx, 0x1f4'),('mov','qword ptr [rsp + 0xa8], rbx'),('mov','byte ptr [rsp + 0xb0], sil'),('mov','eax, dword ptr [rbx + 0x3b8]'),('mov','dword ptr [rsp + 0xb4], eax')}<=set(pairs(body))
 assert sites(n,body,'?ShouldLog@Log@@') and sites(n,body,'?outMessage@Log@@')
 assert b'Warrior intro: Ragnvald %s is missing native933 stage3 or matching AI after 10 battle handoff retries' in n['binary']
 retry,=[name for name in n['symbols'] if name.startswith('?_Do_call@') and 'ResumeBattle@npc_ragnvald_dragonbornAI@' in name]
 vtable,raw=table('??_7'+retry[len('?_Do_call@'):].removesuffix('EEAAXXZ')+'6B@',7)
 assert vtable in refs(body) and n['symbol'](retry) in resolved(raw)
 decoded=ins(retry)
 assert pairs(decoded)[:3]==[('mov','r8, qword ptr [rcx + 8]'),('mov','eax, dword ptr [rcx + 0x14]'),('cmp','dword ptr [r8 + 0x3b8], eax')]
 assert decoded[3].mnemonic=='jne' and decoded[3].operands[0].imm==decoded[8].address and decoded[8].mnemonic=='ret'
 assert pairs(decoded)[4:7]==[('movzx','edx, byte ptr [rcx + 0x10]'),('mov','rcx, r8'),('dec','dl')]
 assert sites(n,decoded,resume)==[7] and decoded[7].mnemonic=='jmp'
 update=ins('?UpdateAI@npc_danica_the_reclaimerAI@');handoff,=sites(n,update,resume)
 assert {('mov','dl, 0xa'),('mov','rcx, rax')}<=set(pairs(update[handoff-5:handoff]))
 assert 0x9c0 not in fields(update) and ('mov','byte ptr [rbx + 0x38], 0') in pairs(update[:handoff])
 for decoded in (body,ins(retry)):
  for forbidden in ('?GetClosestCreatureWithEntry@@','?FindNearestPlayer@WorldObject@@','?UpdateAchievementCriteria@Player@@','?KilledMonsterCredit@Player@@','?CompleteQuest@Player@@','?AddItem@Player@@','?TeleportTo@Player@@'):
   assert not sites(n,decoded,forbidden)
 print('PASS selected actual immediate Ragnvald/Bezzeredes original-target/native AttackStart handoffs, native97166 group0/native Empty-whisper SendChat without target AI, actual Danica event-clear/original receiver/ten-attempt request. Actual map1500/live-world/entry/933 context and GetCurrentStep3 precede RTTI Ragnvald AI/DoAction slot48; missing native scenario or stage2 waits, wrong/earlier/later phase refuses. Receiver-owned500ms retry captures original instanceId and rejects transfer before resolving current AI, with bounded diagnostic; no sender/AI capture, retarget or progress grants. Existing two source clocks/three delayed attacks retained; full933/client remains separate.')
 print(json.dumps({'exe_sha256':hashlib.sha256(n['binary']).hexdigest(),'owned_callbacks':2,'ally_retry_callbacks':2}))
