"""Prove the three optional combat messages use an available content AI."""
from sharasdal_erunak_entry_pe import sites,values

def guard(body,at,receiver):
 expected=[('mov',f'rcx, qword ptr [{receiver} + 0x9c8]'),('test','rcx, rcx'),
           ('jne',body[at+2].op_str),('mov',f'rcx, qword ptr [{receiver} + 0x9c0]'),
           ('test','rcx, rcx'),('je',body[at+5].op_str)]
 assert [(x.mnemonic,x.op_str) for x in body[at:at+6]]==expected
 assert body[at+2].operands[0].imm==body[at+6].address
 return body[at+5].operands[0].imm

def verify(n):
 ins=n['instructions'];lady=ins('?UpdateAI@npc_lady_zitrinAI@npc_lady_zitrin@@')
 assert {105298,209983,25000,209919,2500,209707,16000,209934,28000}<=values(lady)
 cast,=[i for i in sites(n,lady,'?CastSpell@Unit@@') if any(x.op_str=='r8d, 0x3343f' for x in lady[i-8:i])]
 assert lady[cast+1].op_str=='rcx, qword ptr [rsi + 0x9c8]'
 skipped=guard(lady,cast+1,'rsi')
 talks=sites(n,lady,'?Talk@CreatureAI@@');assert len(talks)==2 and talks[0]<cast<talks[1]
 assert any(x.op_str=='dl, 2' for x in lady[talks[0]-5:talks[0]])
 assert any(x.op_str=='dl, 9' for x in lady[talks[1]-5:talks[1]])
 assert skipped==lady[talks[1]+1].address
 reschedule=sites(n,lady,'?ScheduleEvent@EventMap@@');assert reschedule and any(x.op_str=='r8d, 0x61a8' for x in lady[talks[1]+1:reschedule[-1]])
 move=ins('?MovementInform@npc_grashAI@npc_grash@@')
 loads=[i for i,x in enumerate(move) if x.op_str=='rcx, qword ptr [rax + 0x9c8]'];assert len(loads)==2
 found=sites(n,move,'?FindNearestCreature@WorldObject@@')
 for at,entry in zip(loads,(102826,104853)):
  search=max(i for i in found if i<at)
  assert any(x.op_str==f'edx, {entry:#x}' for x in move[search-5:search])
  skip=guard(move,at,'rax')
  dispatch,=[i for i,x in enumerate(move[at+6:],at+6) if x.mnemonic in ('call','jmp') and x.op_str=='qword ptr [rax + 0x10]'] if at==loads[-1] else [i for i,x in enumerate(move[at+6:loads[-1]],at+6) if x.mnemonic=='call' and x.op_str=='qword ptr [rax + 0x10]']
  assert skip>move[dispatch].address or skip<move[at].address # final null branch returns through the earlier common epilogue
  assert move[at+6].op_str=='rax, qword ptr [rcx]' and move[at+7].op_str=='rdx, rdi'
 for body in (lady,move):
  for forbidden in ('?UpdateAchievementCriteria@Player@@','?KilledMonsterCredit@Player@@','?AddItem@Player@@','?CompleteQuest@Player@@'):
   assert not sites(n,body,forbidden),forbidden
 print('PASS selected real Lady UpdateAI: original25s209983 target cast then available original-suspended/current Talk9, null branch rejoins unchanged reschedule. Selected Grash point handler checks both original/current companions before actual virtual AttackStart10 to same Lady. Retained factory/native movement/event/credit/loot chain is checked separately; no native243-causes-charm, live crash or client claim.')
