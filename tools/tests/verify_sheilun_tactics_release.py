"""Native chosen aura, acknowledgement and original real damage consumers."""
from pathlib import Path
import runpy,struct,re
from capstone import CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
from sharasdal_erunak_entry_pe import sites,values,fields
retained=runpy.run_path(str(Path(__file__).with_name('verify_sheilun_plan_release.py')))
n=retained['n'];ins=n['instructions'];helper=lambda name:'?'+name+'@SheilunPlan@@'
selected=ins(helper('Select'));apply=ins(helper('ApplyTactic'));live=ins(helper('ChoiceLive'))
before,=sites(n,selected,helper('ApplyTactic'));credit,=sites(n,selected,'?UpdateAchievementCriteria@Player@@')
assert before<credit
cast,=sites(n,apply,'?CastSpell@Unit@@');checks=sites(n,apply,helper('ChoiceLive'))
auras=sites(n,apply,'?HasAura@Unit@@');remove,=sites(n,apply,'?RemoveAurasDueToSpell@Unit@@')
assert len(checks)==2 and len(auras)==3
assert cast<checks[0]<auras[0]<remove<checks[1]<auras[1]<auras[2]
assert 204654 in values(apply)
assert [(x.mnemonic,x.op_str) for x in apply[21:25]]==[
 ('mov','eax, r9d'),('neg','eax'),('sbb','edi, edi'),('add','edi, 0x31f6e')]
assert apply[cast-3].op_str=='r9b, 1' and apply[cast-2].op_str=='r8d, edi' and apply[cast-1].op_str=='rdx, rcx'
assert apply[55].op_str=='r14d, r14d' and apply[56].mnemonic=='jne'
assert apply[auras[0]+2].mnemonic=='je'
assert apply[remove-2].op_str=='edx, [rsi + 0x31f6d]'
assert apply[auras[1]+2].mnemonic=='je' and apply[auras[2]+2].mnemonic=='jne'
assert apply[auras[2]+3].op_str=='al, 1'
for target in ('?FindPlayer@ObjectAccessor@@','?GetCreature@Map@@','?getQuestStatus@Player@@','?GetScenario@ScenarioMgr@@','?GetScenarioId@Scenario@@',helper('PlayerContext'),helper('Receipt')):
 assert len(sites(n,live,target))==1,target
assert {1541,1007,41003,2}<=values(live)
assert {0x18,0x3a88}<=fields(live)
assert b'Sheilun plan: native tactic %u not applied' in n['binary']
assert b'Sheilun plan: native tactic %u did not remain exclusive' in n['binary']
for body in (apply,live):
 for target in ('?UpdateAchievementCriteria@Player@@','?AddItem@Player@@','?CompleteQuest@Player@@','?KilledMonsterCredit@Player@@','?AddDelayedEvent@Unit@@'):
  assert not sites(n,body,target)
# No new damage coefficient is embedded in this producer. Verify the linked
# existing native aura79 table entry and real physical/stat and taken consumers.
data_symbols=re.findall(r'^\s+\w+:\w+\s+\?AuraEffectHandler@@\S+\s+([0-9a-fA-F]{16})\s+',n['maptext'],re.M)
assert len(data_symbols)==1
table=int(data_symbols[0],16)
handler=n['symbol']('?HandleModDamagePercentDone@AuraEffect@@')
assert n['resolve'](struct.unpack_from('<Q',n['binary'],n['offset'](table+79*8))[0])==handler
dispatch=ins('?HandleEffect@AuraEffect@@QEAAXPEAVAuraApplication@@E_N@Z')
assert any(x.address+x.size+o.mem.disp==table for x in dispatch for o in x.operands
           if o.type==CS_OP_MEM and o.mem.base==X86_REG_RIP)
native=ins('?HandleModDamagePercentDone@AuraEffect@@')
stats=sites(n,native,'?HandleStatModifier@Unit@@');assert len(stats)==3
assert native[0].op_str=='r8b, 0x15'
for at,kind in zip(stats,[0x22,0x23,0x24]):
 assert native[at-4].op_str==f'edx, {kind:#x}'
 assert native[at-3].op_str=='r8d, 4'
 assert native[at-5].op_str=='xmm3, dword ptr [rbx + 0x1c]'
taken=ins('?MeleeDamageBonusTaken@Unit@@')
masked,=[i for i in sites(n,taken,'?GetTotalAuraMultiplier@Unit@@') if taken[i-2].op_str=='edx, 0x57']
assert taken[masked-9].mnemonic=='call' and taken[masked-9].operands[0].type==CS_OP_MEM and taken[masked-9].operands[0].mem.disp==0x280
predicate=taken[masked-8]
assert predicate.mnemonic=='lea' and predicate.operands[1].mem.base==X86_REG_RIP
predicate_table=predicate.address+predicate.size+predicate.operands[1].mem.disp
callback,=[name for name in n['symbols'] if name.startswith('?_Do_call@') and '??GetTotalAuraMultiplierByMiscMask@Unit@@' in name]
raw=struct.unpack_from('<8Q',n['binary'],n['offset'](predicate_table))
assert any(n['resolve'](at)==n['symbol'](callback) for at in raw[:5])
spell=ins('?SpellDamageBonusDone@Unit@@')
assert any(any(x.op_str=='edx, 0x4f' for x in spell[max(0,i-8):i])
           for i in sites(n,spell,'?GetTotalNotStuckAuraEffectByType@Unit@@'))
print('PASS registered plan->acknowledged native tactic before credit: exact0 Balanced204654/1 Reckless204653 self triggered cast, resultOK/actual aura, opposite-only removal and exclusive recheck, two original player/source/quest/role/scenario re-resolutions. Linked native aura79 table/stat34-36/TOTAL_PCT, ordinary spell79 and melee taken87 consumers retained. Numeric25/50/mask127 from separate fresh native catalog; no custom damage/quest/item grant. Client tactic/battle fidelity remains separate.')
