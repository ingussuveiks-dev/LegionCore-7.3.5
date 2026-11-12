from linked_player_abi import inherited_operand
from linked_player_abi import player_offset,player_operand
"""Inspect the actual qualified single shared Althea victory event in Release."""
from pathlib import Path
import runpy,json,hashlib
from sharasdal_erunak_entry_pe import sites,values,floats
p=runpy.run_path(str(Path(__file__).with_name('verify_althea_actor_release.py')))
n=p['n'];ins=n['instructions'];pairs=lambda body:[(x.mnemonic,x.op_str) for x in body]
suffix='npc_sister_althea_ebonlockeAI@npc_sister_althea_ebonlocke@@'
selection='?GetVictoryPlayer@'+suffix;predicate='??R<lambda_1>@?1??GetVictoryPlayer@'+suffix
body=ins('?JustDied@'+suffix);choose,=sites(n,body,selection);award,=sites(n,body,'?UpdateAchievementCriteria@Player@@')
assert choose<award and {51613,92}<=values(body[choose:award])
assert body[choose+1].mnemonic=='test' and body[choose+2].mnemonic=='je'
assert body[choose+2].operands[0].imm==body[award+1].address,'No qualified recipient must skip the sole event'
assert not sites(n,body,'?FindNearestPlayer@WorldObject@@')
helper=ins(selection);eligible=ins(predicate)
def reachable(rows,start):
    indexed={op.address:i for i,op in enumerate(rows)};pending=[start];seen=set()
    while pending:
        at=pending.pop()
        if at in seen or at not in indexed:continue
        seen.add(at);i=indexed[at];op=rows[i]
        if op.mnemonic.startswith('ret'):continue
        if op.mnemonic=='jmp':pending.append(op.operands[0].imm);continue
        if op.mnemonic.startswith('j'):pending.append(op.operands[0].imm)
        if i+1<len(rows):pending.append(rows[i+1].address)
    return seen
lookup,=sites(n,helper,'?GetScenario@ScenarioMgr@@');membership,=sites(n,helper,'?CanUpdateCriteria@Scenario@@')
history,=sites(n,helper,'?GetCriteriaProgressMap@');nearby,=sites(n,helper,'?GetPlayerListInGrid@WorldObject@@')
qualify=sites(n,helper,predicate);assert len(qualify)==2 and lookup<membership<history<qualify[0]<nearby<qualify[1]
assert {1620,1123,48585}<=values(helper) and 100.0 in floats(n,helper)
count,=[i for i,op in enumerate(helper) if (op.mnemonic,op.op_str)==('cmp','qword ptr [rcx + 0x28], 0')]
assert history<count<qualify[0] and helper[count+1].mnemonic=='jne'
rejected=reachable(helper,helper[count+1].operands[0].imm)
assert helper[qualify[0]].address not in rejected and helper[nearby].address not in rejected
assert helper[qualify[0]+1].mnemonic=='test' and helper[qualify[0]+2].mnemonic=='jne'
assert helper[nearby].address not in reachable(helper,helper[qualify[0]+2].operands[0].imm),'Valid killer/owner must win before nearby fallback'
for target,count in [('?IsInWorld@Object@@',1),('?IsPreDelete@Object@@',1),('?IsQuestObjectExpired@WorldObject@@',1)]:assert len(sites(n,helper,target))==count
for target,count in [('?IsInWorld@Object@@',1),('?IsPreDelete@Object@@',1),('?CanContact@Player@@',1),('?InSamePhase@WorldObject@@',1),('?IsWithinDistInMap@WorldObject@@',1),('?IsQuestObjectExpired@WorldObject@@',1),('?CanTargetQuestObject@WorldObject@@',2),('?GetQuestRewardStatus@Player@@',1),('?GetQuestStatus@Player@@',2)]:
    assert len(sites(n,eligible,target))==count,('Missing eligibility predicate',target)
assert {42504,42627,1,3}<=values(eligible) and 100.0 in floats(n,eligible)
assert {('test',inherited_operand('byte ptr [rbx + 0x4ff0], 1')),('cmp','al, 4'),('cmp','qword ptr [rbx + 8], rax')}<=set(pairs(eligible))
for offset in (player_offset(0x6848),player_offset(0x6849),player_offset(0x6851)):assert ('cmp',f'byte ptr [rbx + {hex(offset)}], 0') in pairs(eligible)
assert 51654 not in values(body+helper+eligible),'Victory must not grant the independent door event'
for target in ('?KilledMonsterCredit@Player@@','?RewardPlayerAndGroupAtEvent@Player@@','?CompleteQuest@Player@@','?AddItem@Player@@'):
    assert not any(sites(n,part,target) for part in (body,helper,eligible))
print('PASS actual registered death hook selects one eligible Player before sole51613; original killer/owner wins, NPC/null fallback skips GM/offline/wrong class/quest/map/phase/range/teleport/cancelled scope. Native1123/48585 admission and existing nonzero Counter refuse replay, including partial stage. Both active quest variants retained; no independent51654, per-member grant, item or forced quest completion. Native/ASAN and client traversal remain distinct.')
print(json.dumps({'exe_sha256':hashlib.sha256(n['binary']).hexdigest(),'event':51613,'leaf':48585,'independent_door':51654}))
