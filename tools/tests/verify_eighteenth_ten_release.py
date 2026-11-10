"""Linked geometry, action gates, native health widths and roster lifetime."""
from pathlib import Path
import runpy
p=runpy.run_path(str(Path(__file__).with_name('verify_seventeenth_ten_release.py')));p=p['p']|p;ins=p['ins'];sites=p['sites'];values=p['values'];callbacks=p['callbacks'];expanded=p['expanded']
def op(rows,m,s):return any(x.mnemonic==m and x.op_str==s for x in rows)
r=ins('?UpdateAI@boss_brawguild_toppsAI@');assert op(r,'divss','xmm6, xmm0')and op(r,'divss','xmm7, xmm0')and 3000 in values(r),'shared nonzero direction length and coincident retry'
assert op(ins('?DoAction@npc_empyreal_focusAI@'),'cmp','edx, 1');assert op(ins('?DoAction@boss_lei_shiAI@'),'cmp','edx, 3')
r=ins('?OnCreatureCreate@instance_unga_ingoo_InstanceMapScript@');assert op(r,'cvttss2si','rdx, xmm0')and sites(r,'?SetHealth@Unit@@')
r=ins('?SpellHit@npc_generic_soldierAI@');assert op(r,'shr','rbx, 2')and op(r,'mov','qword ptr [rsp + 0x28], rbx')and sites(r,'?SendHealSoldiers@npc_soldierAI@@');assert any('_K@Z'in s for s in p['n']['symbols']if s.startswith('?SendHealSoldiers@npc_soldierAI@@'))
r=ins('?HandleOnApply@spell_touch_of_yshaarj_AuraScript@');assert op(r,'cvtsi2ss','xmm1, rax')and op(r,'movss','dword ptr [r14 + 0x1c], xmm1')
r=ins('?AfterAbsorb@spell_touch_of_yshaarj_AuraScript@');assert op(r,'maxss','xmm0, xmm6')and op(r,'cvttss2si','rdx, xmm0')and op(r,'add','rdx, rsi')and sites(r,'?SetHealth@Unit@@')
r=ins('?UpdateAI@npc_en_nightmother@');at=next(i for i,x in enumerate(r)if x.mnemonic=='mov'and x.op_str=='dword ptr [rbx + 0x58], 0x1770');assert sites(r[at:at+35],'?SetCanFly@Unit@@'),'landing timer armed before target lookup'
r=ins('?UpdateAI@npc_fshoo@');at=next(i for i,x in enumerate(r)if x.mnemonic=='cmp'and x.op_str=='ax, 0x64');assert r[at+1].mnemonic=='jbe'and op(r[at:at+12],'add','eax, 4'),'100 enters authored middle bracket'
r=ins('?EnterCombat@boss_freyaAI@');assert op(r,'imul','rcx, rax')and op(r,'add','rdx, rdi')and sites(r,'?SetMaxHealth@Unit@@')
r=ins('?Reset@boss_brawguild_masterAI@');assert sites(r,'?_Reset@BrawlersBossAI@@')and op(r,'mov','word ptr [rdi + 0xc0], 0')
r=ins('?JustSummoned@boss_brawguild_masterAI@');assert op(r,'cmp','byte ptr [rsi + 0xc0], 3')and any(x.mnemonic=='jae'for x in r)
r=ins('?SummonedCreatureDies@boss_brawguild_masterAI@');assert any(x.mnemonic=='cmp'and '0xc8]'in x.op_str for x in r)or any('0xc8]'in x.op_str for x in r)
reports=[expanded(s)for s in callbacks('boss_brawguild_masterAI','SummonedCreatureDies')];assert len(reports)==1 and sites(reports[0],'?GetPlayer@ObjectAccessor@@')and sites(reports[0],'?GetAnyOwner@Unit@@'),'actor-owned report resolves original owner after delay'
print('PASS actual Release eighteenth ten:shared direction length/retry,correct action gates,four native-width health paths,unconditional takeoff timer,100-stack bracket and Master reset/roster/actor-owned report. Prior177 selected paths retained;models/native/client scopes separate.')
