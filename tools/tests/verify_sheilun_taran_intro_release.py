"""Prove the registered Taran AI consumes the native final-heal notification."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
from sharasdal_erunak_entry_pe import sites, values, fields, floats

retained = runpy.run_path(str(Path(__file__).with_name('verify_creature_heal_applied_release.py')))
n = retained['n']; ins = n['instructions']
suffix = '@npc_tazhan_chuAI@npc_tazhan_chu@@'
method = lambda name: '?' + name + suffix
wounds=any(name.startswith(method('DamageTakenAfterScaling')) for name in n['symbols'])
if wounds:
    from sheilun_companion_wounds_pe import verify as verify_wounds
    verify_wounds(n)
registered=ins('?AddSC_Sheylun@@')
assert n['symbol']('?AddSC_Sheylun@@') in n['calls']('?AddScenarioScripts@@')
name=registered[12]; address=name.address+name.size+name.operands[1].mem.disp
assert n['binary'][n['offset'](address):n['offset'](address)+14]==b'npc_tazhan_chu'
assert 20 in sites(n,registered,'??0CreatureScript@@')
primary=registered[21]; address=primary.address+primary.size+primary.operands[1].mem.disp
assert registered[22].op_str=='qword ptr [rbx], rax'
assert n['resolve'](struct.unpack_from('<Q',n['binary'],n['offset'](address+0x50))[0])==n['symbol']('?GetAI@npc_tazhan_chu@@')
dispatch=ins('?GetCreatureAI@ScriptMgr@@')
assert sites(n,dispatch,'?GetScriptId@Creature@@')
assert any(x.mnemonic=='mov' and x.operands[-1].type==CS_OP_MEM and x.operands[-1].mem.disp==0x50 for x in dispatch)
factory = ins('?GetAI@npc_tazhan_chu@@')
references = [x.address+x.size+o.mem.disp for x in factory for o in x.operands
              if x.mnemonic == 'lea' and o.type == CS_OP_MEM and o.mem.base == X86_REG_RIP]
assert len(references)==1, 'Reinspect actual Taran factory table reference'
table,=references
assert factory[15].mnemonic=='mov' and factory[15].op_str=='qword ptr [rsi], rax'
for name,slot in [('OnHealApplied',0x288),('UpdateLifecycle',0x110),('HealReceived',0xb0),('Reset',0x28)]:
    expected='?HealReceived@UnitAI@@' if wounds and name=='HealReceived' else method(name)
    assert n['resolve'](struct.unpack_from('<Q',n['binary'],n['offset'](table+slot))[0]) == n['symbol'](expected)
assert all((op in [(x.mnemonic,x.op_str) for x in factory]) for op in [
    ('mov','qword ptr [rsi + 0xf8], rbp'),('mov','qword ptr [rsi + 0x100], rbp'),
    ('mov','dword ptr [rsi + 0x108], ebp'),('mov','word ptr [rsi + 0x10c], bp')])
scope=ins(method('IntroHealScenario')); player=ins(method('IntroHealPlayer')); receipt=ins(method('IntroHealReceipt'))
assert {101881,362837,1541,1007} <= values(scope)
for target in ('?IsInWorld@Object@@','?IsPreDelete@Object@@','?GetScenario@ScenarioMgr@@','?GetScenarioId@Scenario@@'):
    assert sites(n,scope,target)
assert {41003,10} <= values(player)
for target in ('?CanContact@Player@@','?InSamePhase@WorldObject@@','?GetQuestRewardStatus@Player@@','?GetQuestStatus@Player@@'):
    assert sites(n,player,target)
assert 46576 in values(receipt) and any(x.mnemonic=='cmp' and x.op_str=='qword ptr [rcx + 0x28], 1' for x in receipt)
assert sites(n,receipt,'?GetCriteriaProgressMap@')
heal=ins(method('OnHealApplied')); life=ins(method('UpdateLifecycle')); old=[] if wounds else ins(method('HealReceived'))
for target in (method('IntroHealScenario'),method('IntroHealPlayer'),'?GetCharmerOrOwnerPlayerOrPlayerItself@Unit@@','?AddQuestDelayedEvent@Player@@','?getQuestStatus@Player@@','?GetGuidValue@Object@@'):
    assert sites(n,heal,target),target
assert 95.0 in floats(n,heal) and {0xe1,0xf8,0x100,0x108,0x10c,0x10d} <= fields(heal)
assert any(x.mnemonic=='movups' and x.op_str=='xmmword ptr [rbx + 0xe1], xmm0' for x in heal), 'Missing full original healer GUID'
for target in (method('IntroHealScenario'),method('IntroHealPlayer'),'?GetPlayer@ObjectAccessor@@','?getQuestStatus@Player@@'):
    assert len(sites(n,life,target))==2,(target,sites(n,life,target))
assert {0xf8,0x100} <= fields(life) and 95.0 in floats(n,life)
credit,=sites(n,life,'?UpdateAchievementCriteria@Player@@')
assert 48371 in values(life[credit-5:credit]) and 92 in values(life[credit-5:credit])
receipts=sites(n,life,method('IntroHealReceipt'));assert len(receipts)==2
assert receipts[0]<credit<sites(n,life,'?GetPlayer@ObjectAccessor@@')[1]<receipts[1]
latch,=[i for i,x in enumerate(life) if x.mnemonic=='mov' and x.op_str=='byte ptr [rdi + 0x59], 1']
move,=sites(n,life,'?MovePoint@MotionMaster@@'); talk,=sites(n,life,'?Talk@CreatureAI@@')
assert receipts[1]<latch<talk<move and life[receipts[1]+2].mnemonic=='je' and life[receipts[1]+2].operands[0].imm>life[move].address
assert {-1020.62,-3135.73,26.91} <= floats(n,life)
assert any(x.mnemonic=='cmp' and x.op_str=='byte ptr [rdi + 0x10c], 0x1e' for x in life)
assert any(x.mnemonic=='mov' and x.op_str=='dword ptr [rdi + 0x108], 0x3e8' for x in life)
assert sites(n,ins(method('Reset')),method('CancelIntroHeal'))
if not wounds:assert 40.0 in floats(n,old) and 204237 in values(old)
for body in (heal,life,old):
    for forbidden in ('?AddItem@Player@@','?CompleteQuest@Player@@','?KilledMonsterCredit@Player@@','?CastSpell@Unit@@'):
        assert not sites(n,body,forbidden)
assert not sites(n,old,'?UpdateAchievementCriteria@Player@@') and not sites(n,old,'?MovePoint@MotionMaster@@')
assert b'Sheilun Taran Zhu: applied opening heal48371 rejected after thirty attempts' in n['binary']
print('PASS actual Taran factory/table0x288+0x110/native heal pipeline: real362837/map1541/scenario1007/Monk41003, post-applied95 full healer GUID/weak quest attempt, two owner/quest/context resolutions, actual48371 and46576 before once speech/pose/original point1, finite30/1s and Reset cancellation. Shared wounds are separately inspected when installed; route/finale/client remain separate. No item/quest/teleport grant.')
