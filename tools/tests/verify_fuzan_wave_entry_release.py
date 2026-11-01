"""Selected linked wave-entry hooks; native/client coverage is reported separately."""
from pathlib import Path
import hashlib
import json
import runpy

prior=runpy.run_path(str(Path(__file__).with_name('verify_fuzan_summerpetal_release.py')))
n,require=prior['n'],prior['require']
sites,values=prior['sites'],prior['values']
suffix='@Instance@FuZanWaveEntry@@'
world=require('?World'+suffix,('?GetScenario@ScenarioMgr@@','?GetScenarioId@Scenario@@'),(1625,1137))
victory=require('?RecordFuZanVictory'+suffix,('?World'+suffix,'?GetCurrentStep@Scenario@@'),(109397,373405))
admission=require('?CanEnter'+suffix,('?Catalog'+suffix,'?CanContact@Player@@','?InSamePhase@WorldObject@@',
    '?Receipt@FuZanSummerpetal@@','?CanUpdateCriteria@Scenario@@','?FindNearestCreature@WorldObject@@'),
    (103,400106,1625,42765,49356,49176,109391,373396,109465,373425))
enter=require('?Enter'+suffix,('?CanEnter'+suffix,'?AdoptQuestCancellationReceipt@Player@@','?Credit'+suffix),
    (200,42765,217817,217659,217667))
credit=require('?Credit'+suffix,('?CanUpdateCriteria@Scenario@@','?Dispatch'+suffix,'?Receipt@FuZanSummerpetal@@'),(49176,200))
dispatch=require('?Dispatch'+suffix,('?OnInitialize@SmartScript@@','?InitTimer@SmartScript@@','?ProcessEvent@SmartScript@@'))
update=require('?Update'+suffix,('?AdoptQuestCancellationReceipt@Player@@','?GetQuestCancellationReceipt@Player@@',
    '?CastSpell@Unit@@','?Cleanup'+suffix,'?CleanupReceiver'+suffix),
    (200,500,42765,217817,217659,217667))
# Release may inline the complete Minion scan and omit its public symbol. In
# that case verify the scan itself, including the full owner GUID and both
# native entry/created-by-spell pairs; do not require a nonexistent call.
if not sites(n,update,'?Minion'+suffix):
    assert {109350,109352,217666,217669} <= values(update)
    scans=sites(n,update,'?GetSummonList@Unit@@')
    assert len(scans)==1,'One native entry-list scan in the role loop'
    scan=update[scans[0]:scans[0]+48]
    assert sites(n,scan,'?GetCreature@ObjectAccessor@@') and sites(n,scan,'??8ObjectGuid@@')
    pairs=[(op.mnemonic,op.op_str) for op in scan]
    for guard in (('cmp','dword ptr [rax + 0xa18], 0'),
                  ('test','byte ptr [r14 + 0x30e8], 1'),
                  ('movups','xmm0, xmmword ptr [r14 + 0x3b64]'),
                  ('lea','rdx, [rsi + 0x638]'),('mov','edx, 0x7c')):
        assert guard in pairs,('Inlined native alive/summon/128-bit-owner/created-spell guard',guard)
    alive=pairs.index(('cmp','dword ptr [rax + 0xa18], 0'))
    summon=pairs.index(('test','byte ptr [r14 + 0x30e8], 1'))
    owner=sites(n,scan,'??8ObjectGuid@@')[0]
    reject=[scan[alive+1],scan[summon+1],scan[owner+2]]
    assert [op.mnemonic for op in reject]==['jne','je','je'] and len({op.operands[0].imm for op in reject})==1
    reads=sites(n,scan,'?GetUInt32Value@Object@@')
    assert len(reads)==2 and pairs[reads[0]+1]==('cmp','eax, r12d') and pairs[reads[1]+1]==('cmp','eax, r13d')
reconnect=require('?OnPlayerEnter'+suffix,('?AdoptQuestCancellationReceipt@Player@@',),(42765,))
tickets=require('?Spell'+suffix,('?Allow'+suffix,'?Owns@FuZanWaveEntry@@'),(217817,218883,217659,217667,217666,217669))
scene=require('?Create@spell_fuzan_wave_conversation@@',('?PreventHitDefaultEffect@SpellScript@@',
    '?CreateConversation@Conversation@@','?BindQuestObject@Player@@','?CleanupReceiver'+suffix),(2779,42765))
aura=require('?Apply@aura_fuzan_wave_controller@@',('?GetUnitOwner@AuraScript@@','?Allow'+suffix,'?PreventDefaultAction@AuraScript@@'))
assert sites(n,aura,'?PreventDefaultAction@AuraScript@@')[0]<sites(n,aura,'?GetAura@AuraScript@@')[0]
retire=require('?Retire@spell_fuzan_wave_retrieval@@',('?GetCaster@SpellScript@@','?GetInstanceScript@WorldObject@@'),(1625,))
assert not sites(n,retire,'?PreventHitDefaultEffect@SpellScript@@'),'Retain native retrieval effects'
adopt=require('?AdoptQuestCancellationReceipt@Player@@',('?GetQuestCancellationReceipt@Player@@','?Link@QuestCancellationReceipt@@'))
registration=n['instructions']('?AddSC_instance_temple_of_the_jade_serpent@@')
installed=prior['prior']['installed_functions'](registration)
for factory in ('?GetSpellScript@?$GenericSpellScriptLoader@Vspell_fuzan_wave_attempt@@',
                '?GetSpellScript@?$GenericSpellScriptLoader@Vspell_fuzan_wave_conversation@@',
                '?GetSpellScript@?$GenericSpellScriptLoader@Vspell_fuzan_wave_retrieval@@',
                '?GetAuraScript@?$GenericAuraScriptLoader@Vaura_fuzan_wave_controller@@',
                '?IsTriggerMeets@eventobject_fuzan_wave_entry@@'):
    assert n['symbol'](factory) in installed,('Actual registration absent',factory)
for body in (victory,admission,enter,credit,dispatch,update,reconnect,tickets,scene,aura,retire):
    for target in ('?UpdateAchievementCriteria@Player@@','?UpdateAchievementCriteria@Scenario@@','?SetCurrentStep@Scenario@@',
                   '?SummonCreature@WorldObject@@','?AddItem@Player@@','?KilledMonsterCredit@Player@@','?CompleteQuest@Player@@'):
        assert not sites(n,body,target),('Preserve native progress/summon dispatch',target)
print('PASS linked wave-entry: actual five registrations, original winner/attempt and source/receivers, native SmartScript root, ordinary casts/tickets/minions, owned conversation/aura cleanup, early reconnect adoption and unchanged retrieval defaults; no direct progression or summon grants.')
print(json.dumps({'exe_sha256':hashlib.sha256(n['binary']).hexdigest(),'wave_entry_selected_functions':13}))
