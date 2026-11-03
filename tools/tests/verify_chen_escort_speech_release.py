"""Actual registered late-frame Chen speech bound, independent native route."""
from pathlib import Path
import runpy,json,hashlib
from sharasdal_erunak_entry_pe import sites

routes_proof=runpy.run_path(str(Path(__file__).with_name('verify_monk_chen_routes_release.py')))
opening=runpy.run_path(str(Path(__file__).with_name('verify_chen_opening_voice_release.py')))
p=opening['p']
n=p['n'];ins=n['instructions'];early=p['early'];dispatch=p['dispatch'];factory=p['factory']
assert routes_proof['n']['binary']==n['binary']
assert p['escort_speech'] and routes_proof['escort_speech']
pres=p['pres'];ctx=p['ctx'];pairs=p['pairs'];needs=p['needs'];reachable=p['reachable']
body=ins(pres('UpdateEscortSpeech'))
speech,=sites(n,dispatch,pres('UpdateEscortSpeech'));route,=sites(n,dispatch,ctx('Update'))
boss,=sites(n,dispatch,'?Update@State@MonkJorvinax@@')
assert route<speech<boss
assert pairs(dispatch[speech-2:speech])==[('mov','edx, esi'),('lea','rcx, [rdi + 0x418]')]
assert ('mov','qword ptr [rsi + 0x428], rbx') in pairs(factory)
first,students=sites(n,early,'?SendChat@CreatureTextMgr@@')
clocks=sites(n,early,'?getMSTime@@');assert len(clocks)==2
reserve=next(i for i,x in enumerate(pairs(early)) if x==('or','al, 1'))
stamp=next(i for i,x in enumerate(pairs(early)) if x==('mov','dword ptr [r12 + 0x14], eax'))
assert reserve<clocks[1]<stamp<first<students
assert clocks[1]+1==stamp
needs(body,ctx('Progress'),ctx('Actor'),ctx('Leaf'),ctx('Current'),'?GetCurrentStep@Scenario@@',
      '?getMSTime@@','?GetGuidValue@Object@@','?SendChat@CreatureTextMgr@@')
expected={('mov','rdi, rcx'),('cmp','byte ptr [rcx + 0x13], 0'),('cmp','qword ptr [rcx + 8], rax'),
          ('mov','qword ptr [rcx + 8], rax'),('mov','byte ptr [rcx + 0x10], 0'),('mov','byte ptr [rcx + 0x12], 0'),
          ('test','al, 4'),('cmp','byte ptr [rdx + 0x71], 0'),('cmp','byte ptr [rdx + 0xcf], 0'),
          ('cmp','byte ptr [rdx + 0x95], 0'),('or','al, 4'),('mov','byte ptr [rcx + 0x12], al'),
          ('test','al, 1'),('mov','ebx, dword ptr [rcx + 0x14]'),('lea','ecx, [rdx - 1]'),
          ('cmovbe','ecx, edx'),('cmp','ecx, 0x84d'),('cmp','eax, 4'),('add','rdx, 0x48'),
          ('or','byte ptr [rdi + 0x12], 4'),('add','rdi, 0x13'),('mov','byte ptr [rdi], 1'),
          ('mov','byte ptr [rdi], 0'),('movups','xmm0, xmmword ptr [rax]'),('mov','rdx, r14'),('mov','r8b, 5')}
assert expected<=set(pairs(body))
chat,=sites(n,body,'?SendChat@CreatureTextMgr@@');current,=sites(n,body,ctx('Current'))
bound=next(i for i,x in enumerate(pairs(body)) if x==('cmp','ecx, 0x84d'))
assert body[bound+1].mnemonic=='jb' and body[chat].address not in reachable(body,body[bound+1].operands[0].imm)
once=next(i for i,x in enumerate(pairs(body)) if x==('or','byte ptr [rdi + 0x12], 4'))
assert bound<current<once<sites(n,body,'?GetGuidValue@Object@@')[0]<chat
assert pairs(body[chat-4:chat])==[('lea','r9, [rsp + 0x60]'),('mov','r8b, 5'),('mov','rdx, r14'),('mov','rcx, rbx')]
# Arrival consumes the optional line rather than replaying escort instructions
# over the genuine subsequent handoff. No essential action runs here.
arrival=next(i for i,x in enumerate(pairs(body)) if x==('cmp','byte ptr [rdx + 0x95], 0'))
assert body[arrival+1].mnemonic=='je'
assert body[chat].address not in reachable(body,body[arrival+2].address)
for forbidden in ('?MovePath@','?MovePoint@','?UpdateAchievementCriteria@','?KilledMonsterCredit@','?CompleteQuest@',
                  '?AddItem@','?TeleportTo@','?CastSpell@','?SummonCreature@','?AddDelayedEvent@'):
    assert not sites(n,body,forbidden),('Optional speech cannot own progression/movement',forbidden)
print(f"PASS actual registered{n.get('monk_instance_size',0x498):x} instance: after-route optional Chen5 uses actual Talk4 clock in original padding/2125ms native bound; original8s/15s routes, admission/weak owner/full actors, arrival retirement, once/reentry, shared unscoped queues and prior4443ms bound retained. Client audio and other coalesced dialogue remain separate.")
print(json.dumps({'world_sha256':hashlib.sha256(n['binary']).hexdigest(),'state_bytes':24,'state_offset':0x418,'actual_previous_voice_offset':0x14,'minimum_previous_voice_ms':2125,'movement_ms':8000}))
