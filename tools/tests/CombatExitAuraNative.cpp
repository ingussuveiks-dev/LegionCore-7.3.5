// Actual Unit::ClearInCombat through CombatStop; native interrupt metadata.
#define HEALTH_OWNERSHIP_SUPPORT_ONLY
#include "HealthPercentOwnershipNative.cpp"

int main(int argc, char** argv)
{
    try
    {
        InitializeHealthOwnership(argc, argv);
        auto interrupt = sSpellMgr->GetSpellInfo(ChallengerBolstering);
        auto retained = sSpellMgr->GetSpellInfo(2479);
        Require(interrupt && (interrupt->AuraInterruptFlags[0] & AURA_INTERRUPT_FLAG_LEAVE_COMBAT)
            && retained && !(retained->AuraInterruptFlags[0] & AURA_INTERRUPT_FLAG_LEAVE_COMBAT),
            "Review changed native combat-exit aura metadata");
        unsigned cases = 0;
        for (bool encounter : {false, true})
        {
            MapInstanced parent(1492, 300);
            InstanceMap map(1492, 300, 9939500 + encounter, DIFFICULTY_MYTHIC_KEYSTONE, &parent);
            map.CreateInstanceData(nullptr);
            LifecyclePlayer owner(map, 721, Position{2934.49f, 711.757f, 554.286f, 1.55255f});
            HealthOwnershipActor npc(map, 97032, 174001), vehicle(map, 97043, 174002), enemy(map, 97032, 174003);
            DungeonPet pet(owner, map); DungeonGuardian guardian(owner, map);
            auto script = map.GetInstanceScript();
            if (encounter) Require(script->SetBossState(0, IN_PROGRESS), "Native encounter state not admitted");
            for (Unit* target : {static_cast<Unit*>(&npc), static_cast<Unit*>(&vehicle), static_cast<Unit*>(&pet),
                                static_cast<Unit*>(&guardian), static_cast<Unit*>(&owner)})
            {
                target->SetInCombatWith(&enemy);
                Require(target->isInCombat(), "Native combat state not set");
                // Self is authorized by the health handler for every type.
                Require(target->AddAura(ChallengerBolstering, target) && target->AddAura(2479, target),
                    "Native combat-exit test auras not applied");
                auto before = target->GetModifierValue(UNIT_MOD_HEALTH, TOTAL_PCT);
                target->CombatStop(true);
                bool keepEncounterPlayerAura = target->IsPlayer() && encounter;
                Check(!target->isInCombat(), "Native combat exit retained combat flag");
                Check(target->HasAura(ChallengerBolstering) == keepEncounterPlayerAura,
                    "Native combat-exit interrupt cleanup/encounter deferral differs");
                Check(target->HasAura(2479), "Native combat exit removed an unrelated aura");
                if (!keepEncounterPlayerAura)
                    Check(std::fabs(target->GetModifierValue(UNIT_MOD_HEALTH, TOTAL_PCT) * 1.2f - before) < .0001f,
                        "Native combat exit did not unwind the aura's health modifier");
                target->CombatStop(true);
                Check(target->HasAura(ChallengerBolstering) == keepEncounterPlayerAura && target->HasAura(2479),
                    "Repeated native combat exit changed interrupt/retained aura state");
                target->RemoveAllAuras();
                ++cases;
            }
            if (encounter) script->SetBossState(0, NOT_STARTED);
        }
        Check(cases == 10, "Native combat-exit case scope changed");
        std::cout << "PASS " << checked << " native combat-exit checks:ordinary NPC/NPC vehicle/Pet/Guardian/Player, native leave-combat interrupt flags, repeated exit, unrelated aura retention and player-only active-encounter deferral. Actual evade/charm/client combat remains separate\n";
        CharacterDatabase.Close(); LoginDatabase.Close(); WorldDatabase.Close(); HotfixDatabase.Close(); MySQL::Library_End();
        return 0;
    }
    catch (std::exception const& error) { std::cerr << error.what() << '\n'; return 1; }
}
