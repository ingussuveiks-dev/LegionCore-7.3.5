// Native health-aura handler, actual NPC vehicle/Pet/Guardian types and owners.
#define CHALLENGE_LIFECYCLE_SUPPORT_ONLY
#include "LegionChallengeLifecycleNative.cpp"

static void InitializeHealthOwnership(int argc, char** argv)
{
    InitializeDungeon(argc, argv, true);
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    for (auto table : {"spell_proc_event", "spell_proc_check", "spell_bonus_data"})
    {
        std::string name = table;
        WorldDatabase.DirectExecute(("CREATE TEMPORARY TABLE health_" + name + " AS SELECT * FROM " + name + " WHERE entry IN(209859,2479)").c_str());
        WorldDatabase.DirectExecute(("CREATE TEMPORARY TABLE " + name + " LIKE health_" + name).c_str());
        WorldDatabase.DirectExecute(("INSERT INTO " + name + " SELECT * FROM health_" + name).c_str());
    }
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sSpellMgr->LoadSpellProcEvents(); sSpellMgr->LoadSpellPrcoCheck(); sSpellMgr->LoadSpellBonusess();
}

struct HealthOwnershipActor : Creature
{
    HealthOwnershipActor(InstanceMap& map, uint32 entry, uint32 guid, Player* owner = nullptr)
    {
        Require(Create(guid, &map, 1, entry, 0, ALLIANCE, 2934.49f, 715.757f, 554.286f, 1.55255f), "Native health actor create failed");
        if (owner) { SetOwnerGUID(owner->GetGUID()); SetCreatorGUID(owner->GetGUID()); }
        SetHomePosition(*this);
        Require(map.AddToMap(static_cast<Creature*>(this)), "Native health actor publication failed");
    }
    void AddToWorld() override
    {
        sObjectAccessor->AddObject(static_cast<Creature*>(this)); Unit::AddToWorld();
        Require(AIM_Initialize(new NullCreatureAI(this)), "Controlled health actor AI failed");
        GetInstanceScript()->OnCreatureCreate(this); GetInstanceScript()->OnCreatureCreateForScript(this);
    }
    ~HealthOwnershipActor()
    {
        SetCharmerGUID(ObjectGuid::Empty);
        CombatStop(true); CastStop(); RemoveAllAuras(); m_Events.KillAllEvents(true);
        if (IsInWorld()) GetMap()->RemoveFromMap(static_cast<Creature*>(this), false);
        ResetMap();
    }
};

struct HealthOwnershipGuardianVehicle : Guardian
{
    HealthOwnershipGuardianVehicle(Player& owner, InstanceMap& map) : Guardian(nullptr, &owner, false)
    {
        Require(Create(173050, &map, 1, 97043, 0, ALLIANCE, 2934.49f, 715.757f, 554.286f, 1.55255f), "Native guardian vehicle create failed");
        SetOwnerGUID(owner.GetGUID()); SetCreatorGUID(owner.GetGUID()); setFaction(owner.getFaction());
        Require(map.AddToMap(static_cast<Creature*>(this)), "Native guardian vehicle publication failed");
    }
    void AddToWorld() override
    {
        sObjectAccessor->AddObject(static_cast<Creature*>(this)); Unit::AddToWorld();
        Require(AIM_Initialize(new NullCreatureAI(this)), "Controlled guardian vehicle AI failed");
    }
    ~HealthOwnershipGuardianVehicle()
    {
        CombatStop(true); CastStop(); RemoveAllAuras(); m_Events.KillAllEvents(true);
        if (IsInWorld()) GetMap()->RemoveFromMap(static_cast<Creature*>(this), false);
        ResetMap();
    }
};

static void CheckHealthAura(Unit& target, Unit& caster, bool allowed)
{
    auto modifier = target.GetModifierValue(UNIT_MOD_HEALTH, TOTAL_PCT);
    auto maxHealth = target.GetMaxHealth();
    target.SetHealth(maxHealth / 2);
    auto aura = caster.AddAura(ChallengerBolstering, &target);
    Check(aura && aura->GetEffect(EFFECT_0) && aura->GetEffect(EFFECT_0)->GetAmount() == 20.f,
        "Matching native 20-percent health effect missing");
    float ratio = target.GetModifierValue(UNIT_MOD_HEALTH, TOTAL_PCT) / modifier;
    std::cout << "HEALTH target=" << target.GetEntry() << " vehicle=" << target.IsVehicle()
              << " pet=" << target.isPet() << " guardian=" << target.isGuardian()
              << " caster=" << caster.GetGUID().GetGUIDLow() << " allowed=" << allowed
              << " ratio=" << ratio << '\n';
    Check(std::fabs(ratio - (allowed ? 1.2f : 1.f)) < .0001f, "Native health owner/type guard differs");
    // Pet/Guardian resource initialization is controlled; check their real
    // stat modifier above, not a synthetic complete pet-stat model.
    if (target.IsCreature() && !target.isPet() && !target.isGuardian())
    {
        double expected = double(maxHealth) * (allowed ? 1.2 : 1.0);
        Check(std::fabs(double(target.GetMaxHealth()) - expected) < 8, "Native NPC max health did not follow its percent modifier");
    }
    target.RemoveAurasDueToSpell(ChallengerBolstering);
    Check(std::fabs(target.GetModifierValue(UNIT_MOD_HEALTH, TOTAL_PCT) - modifier) < .0001f,
        "Native health aura removal retained a modifier");
}

#ifndef HEALTH_OWNERSHIP_SUPPORT_ONLY
int main(int argc, char** argv)
{
    try
    {
        InitializeHealthOwnership(argc, argv);
        auto normal = sObjectMgr->GetCreatureTemplate(97032);
        auto vehicle = sObjectMgr->GetCreatureTemplate(97043);
        Require(normal && !normal->VehicleId && vehicle && vehicle->VehicleId == 4383, "Review changed native Maw vehicle metadata");
        unsigned cases = 0;
        for (uint8 race : {uint8(RACE_HUMAN), uint8(RACE_ORC)})
        {
            MapInstanced parent(1492, 300);
            InstanceMap map(1492, 300, 9939000 + race, DIFFICULTY_MYTHIC_KEYSTONE, &parent);
            map.CreateInstanceData(nullptr);
            LifecyclePlayer owner(map, 711, Position{2934.49f, 711.757f, 554.286f, 1.55255f});
            LifecyclePlayer foreign(map, 712, owner.GetPosition());
            for (auto player : {&owner, &foreign})
            {
                player->SetByteValue(UNIT_FIELD_BYTES_0, 0, race); player->setFactionForRace(race);
                auto display = sChrRacesStore.LookupEntry(race)->MaleDisplayID;
                player->SetDisplayId(display); player->SetNativeDisplayId(display);
            }
            HealthOwnershipActor npc(map, 97032, 173001), npcVehicle(map, 97043, 173002), ownedVehicle(map, 97043, 173003, &owner);
            HealthOwnershipActor controlledVehicle(map, 97043, 173004), missingOwnerVehicle(map, 97043, 173005);
            DungeonPet pet(owner, map); DungeonGuardian guardian(owner, map);
            HealthOwnershipGuardianVehicle guardianVehicle(owner, map);
            Require(!npc.isAnySummons() && npcVehicle.IsVehicle() && npcVehicle.isAnySummons()
                && guardianVehicle.IsVehicle() && guardianVehicle.isGuardian(), "Native test actor type scope changed");
            auto run = [&](Unit& target, Unit& caster, bool allowed) { CheckHealthAura(target, caster, allowed); ++cases; };
            for (Unit* target : {static_cast<Unit*>(&npc), static_cast<Unit*>(&npcVehicle)})
                for (Unit* caster : {static_cast<Unit*>(&owner), static_cast<Unit*>(&foreign), target}) run(*target, *caster, true);
            for (Unit* target : {static_cast<Unit*>(&pet), static_cast<Unit*>(&guardian), static_cast<Unit*>(&ownedVehicle), static_cast<Unit*>(&guardianVehicle)})
            {
                run(*target, foreign, false); run(*target, owner, true); run(*target, *target, true);
            }
            controlledVehicle.m_ControlledByPlayer = true;
            run(controlledVehicle, foreign, false); run(controlledVehicle, controlledVehicle, true);
            controlledVehicle.m_ControlledByPlayer = false;
            // Controlled GUID receipt only; full charm/AI transition is not modeled.
            controlledVehicle.SetCharmerGUID(owner.GetGUID());
            run(controlledVehicle, foreign, false); run(controlledVehicle, owner, true);
            controlledVehicle.SetCharmerGUID(ObjectGuid::Empty);
            missingOwnerVehicle.SetOwnerGUID(ObjectGuid::Create<HighGuid::Player>(799999));
            run(missingOwnerVehicle, foreign, false); run(missingOwnerVehicle, missingOwnerVehicle, true);
            missingOwnerVehicle.SetOwnerGUID(ObjectGuid::Empty);
            missingOwnerVehicle.SetCreatorGUID(ObjectGuid::Create<HighGuid::Player>(799999));
            run(missingOwnerVehicle, foreign, false); run(missingOwnerVehicle, missingOwnerVehicle, true);
            missingOwnerVehicle.SetCreatorGUID(ObjectGuid::Empty);
            missingOwnerVehicle.SetGuidValue(UNIT_FIELD_DEMON_CREATOR, ObjectGuid::Create<HighGuid::Player>(799999));
            run(missingOwnerVehicle, foreign, false); run(missingOwnerVehicle, missingOwnerVehicle, true);
            missingOwnerVehicle.SetGuidValue(UNIT_FIELD_DEMON_CREATOR, ObjectGuid::Empty);
        }
        Check(cases == 56, "Native health ownership case count changed");
        std::cout << "PASS " << checked << " native health-owner checks:56 real aura cases, NPC/vehicle/Pet/Guardian/guardian-vehicle, owner/self/foreign, control/charm/missing-owner receipts and two races. Actual charm traversal/pet resource presentation remain separate\n";
        CharacterDatabase.Close(); LoginDatabase.Close(); WorldDatabase.Close(); HotfixDatabase.Close(); MySQL::Library_End();
        return 0;
    }
    catch (std::exception const& error) { std::cerr << error.what() << '\n'; return 1; }
}
#endif
