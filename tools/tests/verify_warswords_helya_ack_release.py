"""Real queued normal-channel acceptance and actual target-aura acknowledgement."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM,CS_OP_MEM
import verify_warswords_runestone_release
from warswords_helya_ack_scope import verify
verify()
p=runpy.run_path(str(Path(__file__).with_name('verify_warswords_final_intro_release.py')))
n,ins=p['native'],p['instructions'];actor=p['actor'];scope='@'+actor
helper=ins('?UpdateBlessing'+scope);outer=ins(p['outer']);poll=ins('?UpdateIntro'+scope)
def sites(rows,prefix):
 targets={n['resolve'](address)for name,address in n['symbols'].items()if name.startswith(prefix)}
 assert targets,prefix
 return[i for i,x in enumerate(rows)if x.mnemonic in('call','jmp')and x.operands[0].type==CS_OP_IMM and n['resolve'](x.operands[0].imm)in targets]
def pairs(rows):return{(x.mnemonic,x.op_str)for x in rows}
request,=sites(outer,'?CastSpell@Unit@@');ack,=sites(outer,'?UpdateBlessing'+scope)
assert request<ack and {('mov','r8d, 0x2f9f6'),('xor','r9d, r9d')}<=pairs(outer[request-12:request]),'Keep actual normal195062 channel request'
assert not sites(outer,'?HasAura@Unit@@'),'Request callback must not reject the aura before its native event'
for offset in (0x129,0x139):
 assert any(x.mnemonic=='movups'and x.operands[0].type==CS_OP_MEM and x.operands[0].mem.disp==offset and x.operands[0].size==16 for x in outer[:request]),'Capture complete original Player/Helya GUID before normal cast'
assert ('mov','byte ptr [rax + 0x128], 1')in pairs(outer[:request]),'Request must arm its exact pending acknowledgement before reentrant cast callbacks'
aura,=sites(helper,'?HasAura@Unit@@');pending,=sites(helper,'?FindCurrentSpellBySpellId@Unit@@')
timer,=[i for i,x in enumerate(helper)if x.mnemonic=='mov'and x.op_str=='edx, 0x1b58']
assert aura<pending<timer
assert helper[aura+1].mnemonic=='test'and helper[aura+2].mnemonic=='jne'
assert helper[pending+1].mnemonic=='test'and helper[pending+2].mnemonic=='jne'
queued_flow=p['reachable'](helper,helper[pending+2].operands[0].imm)
assert helper[timer].address not in queued_flow,'Queued normal cast must not arm combat before actual aura'
for prefix in ('?CancelIntro'+scope,'?RemoveIntroConversation'+scope):
 assert all(helper[i].address not in queued_flow for i in sites(helper,prefix)),'Healthy pending native cast must preserve its original conversation'
assert ('mov','edx, 0x2f9f6')in pairs(helper[pending-4:pending]),'Only actual195062 pending cast may defer acknowledgement'
ack_flow=p['reachable'](helper,helper[aura+2].operands[0].imm)
assert helper[timer].address in ack_flow
assert ('mov','byte ptr [rdi + 0x128], sil')in pairs(helper[:timer]),'Acknowledgement must be claimed once before scheduling7000ms'
assert {('cmp','byte ptr [rcx + 0xc7], 0'),('cmp','byte ptr [rcx + 0x128], 0')}<=pairs(helper[:aura]),'Inactive or already-acknowledged intro must not rearm release'
for prefix in ('?GetPlayer@ObjectAccessor@@','?GetCreature@ObjectAccessor@@','?IsIntroTarget'+scope,'?IsIntroHelya'+scope):
 assert sites(helper,prefix),prefix
poll_ack,=sites(poll,'?UpdateBlessing'+scope)
assert poll[poll_ack+1].mnemonic=='cmp'and poll[poll_ack+1].op_str=='byte ptr [rsi + 0xc6], 0','Polling cancellation must recheck the intro claim before removing communion'
constructor=n['symbol']('??0ObjectGuid@@QEAA@XZ');factory=ins('?GetAI@npc_vigfus_bladewind_last@@')
for offset in (0x129,0x139):
 assert any(x.mnemonic=='call'and x.operands[0].type==CS_OP_IMM and n['resolve'](x.operands[0].imm)==constructor and factory[i-1].op_str==f'rcx, [rsi + {offset:#x}]'for i,x in enumerate(factory)if i),'Selected factory must initialize both native128-bit GUIDs'
 for method in ('Reset','CancelIntro'):
  rows=ins('?'+method+scope)
  assert any(x.mnemonic=='movups'and x.operands[0].type==CS_OP_MEM and x.operands[0].size==16 and x.operands[0].mem.disp==offset for x in rows),'Reset/cancellation must clear complete pending identities'
for rows in (outer,helper,ins(p['callback'])):
 for forbidden in ('?SetHealth@Unit@@','?Kill@Unit@@','?CompleteQuest@Player@@','?UpdateAchievementCriteria@Player@@','?TeleportTo@Player@@'):
  assert not sites(rows,forbidden),forbidden
print('PASS actual final factory/poll/queued normal195062 lifecycle:original128-bit Player+Helya identities,pending native channel preserves843,actual aura alone arms once-only7000ms release,reset/cancel guards and retained full intro/rune/bonfire/earlier linked checks pass. No health/death/credit bypass;client completion remains separate.')
