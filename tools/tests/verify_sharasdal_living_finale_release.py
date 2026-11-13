"""Check the selected reconstructed finale and retain acquisition/death PE proofs.

Native regression covers actual casts/boarding/publication/cleanup alternatives.
This check establishes their presence in the final executable, not client fidelity.
"""
from pathlib import Path
import runpy
import struct

r = runpy.run_path(str(Path(__file__).with_name('verify_sharasdal_acquisition_release.py')))
n, ins, sites, values = r['n'], r['ins'], r['sites'], r['values']
refs, needs = r['refs'], r['needs']
ai = '@npc_lady_zitrinAI@npc_lady_zitrin@@'
controller = '@Controller@SharasdalFinale@@'
story = '@DefeatConversation@SharasdalFinale@@'
cleanup = '@spell_sharasdal_follower_cleanup@@'

def method(name, suffix):
    return '?' + name + suffix

def selected_table(factory, methods):
    expected = {n['symbol'](m) for m in methods}
    for at in refs(factory):
        try:
            rows = struct.unpack_from('<100Q', n['binary'], n['offset'](at))
        except (ValueError, struct.error):
            continue
        resolved = set()
        for row in rows:
            try:
                resolved.add(n['resolve'](row))
            except (ValueError, struct.error):
                pass
        if expected <= resolved:
            return
    raise AssertionError((factory, methods))

selected_table('?GetAI@npc_lady_zitrin@@', [method(m, ai) for m in
    ['UpdateAI', 'Reset', 'DamageTakenAfterScaling', 'SpellHit', 'SpellHitTarget', 'GetGUID', 'UpdateLifecycle']])
damage = method('DamageTakenAfterScaling', ai)
needs(damage, method('World', controller), '?GetHealth@Unit@@', method('Recipient', controller),
      '?AddQuestDelayedEvent@Player@@', '?getQuestStatus@Player@@', '?SetHealth@Unit@@', '?CombatStop@Unit@@')
assert 40341 in values(damage)
needs(method('World', controller), '?GetScenarioId@Scenario@@', '?GetCurrentStep@Scenario@@',
      '?GetCriteriaProgressMap@?$AchievementMgr@VScenario@@@@')
assert {104856, 362095, 1600, 1066, 4, 47133} <= values(method('World', controller))
needs(method('Recipient', controller), '?GetLootRecipient@Creature@@',
      '?GetCharmerOrOwnerPlayerOrPlayerItself@Unit@@', '?IsInSameGroupWith@Player@@')
assert {105298, 102826, 104853, 362092, 362093, 362094} <= values(method('Recipient', controller))
needs(method('Owner', controller), '?GetPlayer@ObjectAccessor@@', '?getQuestStatus@Player@@', '?PlayerReady@SharasdalFinale@@')

update = method('Update', controller)
needs(update, method('Owner', controller), method('World', controller), method('Actor', controller),
      '?GetVehicleBase@Unit@@', '?CastSpell@Unit@@', '?CreateConversation@Conversation@@',
      '?BindQuestObject@Player@@', '?GetGameObjectByEntry@InstanceScript@@', '?SetFlag@Object@@',
      method('Reached', story), method('Cancel', controller))
assert {209457, 209487, 209552, 1906, 4030, 249342, 134299, 1000, 30} <= values(update)
needs(method('UpdateLifecycle', ai), update, method('CompleteGauntletArrival', ai))
needs(method('SpellHitTarget', ai), '?RemoveAurasDueToSpell@Unit@@')
assert {209457, 105298} <= values(method('SpellHitTarget', ai))
hit = method('SpellHit', ai)
needs(hit, method('World', controller), method('Owner', controller), '?GetVehicleBase@Unit@@',
      method('Reached', story), method('ReleaseCarry', controller), method('RestoreAllies', controller), '?Kill@Unit@@')
assert {209487, 362092, 208093, 4030} <= values(hit)
assert len(sites(ins(hit), '?Kill@Unit@@')) == 1
assert not sites(ins(update), '?Kill@Unit@@'), 'No elapsed timer may directly kill Lady'
needs(method('Cancel', controller), '?Remove@Conversation@@', method('ReleaseCarry', controller), method('RestoreAllies', controller))
needs(method('ReleaseCarry', controller), '?ExitVehicle@Unit@@', '?RemoveAurasDueToSpell@Unit@@')

selected_table(update, [method(m, story) for m in ['FindCreatureActor', 'Update', 'RemoveFromWorld']])
needs(method('FindCreatureActor', story), '?GetCreature@ObjectAccessor@@', '?InSamePhase@WorldObject@@', '?IsWithinDistInMap@WorldObject@@')
needs(method('Reached', story), '?GetLine@Conversation@@', '?IsQuestObjectExpired@WorldObject@@', '?getMSTime@@')
needs(method('Update', story), method('Reached', story), method('UnlockWeapon', story), '?Update@Conversation@@')
assert {4, 4032} <= values(method('Update', story))
needs(method('UnlockWeapon', story), '?GetGameObject@ObjectAccessor@@', '?RemoveFlag@Object@@')
assert {249342, 134299} <= values(method('UnlockWeapon', story))
needs(method('RemoveFromWorld', story), method('UnlockWeapon', story), '?RemoveFromWorld@Conversation@@')

loader = '?GetSpellScript@?$GenericSpellScriptLoader@Vspell_sharasdal_follower_cleanup@@@@'
selected_table('?AddSC_Sharasdal@@', [loader])
selected_table(loader, [method('Register', cleanup)])
assert n['symbol'](method('Complete', cleanup)) in r['fn_refs'](method('Register', cleanup))
needs(method('Complete', cleanup), '?GetSpellInfo@SpellScript@@', '?CleanupFollowers@SharasdalFinale@@')
assert {210045, 210247} <= values(method('Complete', cleanup))
followers = '?CleanupFollowers@SharasdalFinale@@'
needs(followers, '?HasAura@Unit@@', '?HasItemCount@Player@@', '?GetQuestStatus@Player@@',
      '?InSamePhase@WorldObject@@', '?GetUnit@ObjectAccessor@@', '?DespawnOrUnsummon@Creature@@')
# Adelee's adjacent spawn pair compiles to a range subtraction rather than two immediates.
assert {1600, 1066, 6, 40341, 128911, 210045, 362063, 362066, 362092, 362093} <= values(followers)
for name in (damage, update, hit, followers):
    for forbidden in ('?AddItem@Player@@', '?CompleteQuest@Player@@', '?UpdateAchievementCriteria@Player@@', '?KilledMonsterCredit@Player@@'):
        assert not sites(ins(name), forbidden), (name, forbidden)
assert b'spell_sharasdal_follower_cleanup\0' in n['binary']
print('PASS selected living Lady scaled-lethal/owner/quest/wall gate; actual carry, full-GUID1906 publication, real209487 hit sole native Kill; final-line weapon unlock and native parent/child cleanup registration. Retained native victory/acquisition paths pass. Functional reconstruction; client timing/audio/visuals remain unverified.')
