#pragma once

// Native scene1515 SmoothPhaseSpawnActor(92608, ...) explicitly waits for the
// original world unit to disappear. Hide only that unit for this scene's viewer;
// the ordinary Aura430 handler still starts/ends the matching client scene.
class spell_warrior_malgalor_arrival : public AuraScript
{
    PrepareAuraScript(spell_warrior_malgalor_arrival);
    ObjectGuid source, viewer;
    uint32 instance = 0;
    bool ownsHide = false;

    void Apply(AuraEffect const*, AuraEffectHandleModes)
    {
        if (ownsHide) return;
        Player* player = GetUnitOwner() ? GetUnitOwner()->ToPlayer() : nullptr;
        if (!player || player->GetMapId() != 1500 || player->getClass() != CLASS_WARRIOR) return;
        Scenario* scenario = sScenarioMgr->GetScenario(player->GetInstanceId());
        if (!scenario || scenario->GetScenarioId() != 933 || scenario->GetCurrentStep() != 4) return;
        Creature* actor = player->FindNearestCreature(92608, 300.0f, true);
        if (!actor || actor->GetDBTableGUIDLow() != 361167 || actor->GetMap() != player->GetMap())
        {
            TC_LOG_WARN("scripts", "Malgalor scene1144: native source361167 missing; replacement not hidden");
            return;
        }
        source = actor->GetGUID(); viewer = player->GetGUID(); instance = player->GetInstanceId();
        if (actor->ShouldHideFor(viewer)) return;
        ownsHide = true;
        actor->AddToHideList(viewer);
        actor->UpdateObjectVisibility();
    }

    void Removed(AuraEffect const*, AuraEffectHandleModes)
    {
        if (!ownsHide) return;
        ownsHide = false;
        Map* map = sMapMgr->FindMap(1500, instance);
        Creature* actor = map ? map->GetCreature(source) : nullptr;
        if (actor && actor->GetEntry() == 92608 && actor->GetDBTableGUIDLow() == 361167)
        {
            actor->RemoveFromHideList(viewer);
            if (!map->IsMapUnload()) actor->UpdateObjectVisibility();
        }
    }

    void Register() override
    {
        OnEffectApply += AuraEffectApplyFn(spell_warrior_malgalor_arrival::Apply, EFFECT_0, SPELL_AURA_ACTIVATE_SCENE, AURA_EFFECT_HANDLE_REAL);
        AfterEffectRemove += AuraEffectRemoveFn(spell_warrior_malgalor_arrival::Removed, EFFECT_0, SPELL_AURA_ACTIVATE_SCENE, AURA_EFFECT_HANDLE_REAL);
    }
};
