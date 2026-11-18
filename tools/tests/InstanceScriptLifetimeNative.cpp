// Actual WorldObject getter and ordinary native instance destruction.
#define CHALLENGE_LIFECYCLE_SUPPORT_ONLY
#include "LegionChallengeLifecycleNative.cpp"

#ifdef INSTANCE_SCRIPT_LIFETIME_BASELINE
static bool OriginalGetterFaults(WorldObject* object)
{
    __try { (void)object->GetInstanceScript(); }
    __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    { return true; }
    return false;
}
#endif

struct LifetimeMapCreature : Creature
{
    void AddToWorld() override
    {
        sObjectAccessor->AddObject(static_cast<Creature*>(this)); Unit::AddToWorld();
        Require(AIM_Initialize(new NullCreatureAI(this)), "Controlled unload AI missing");
    }
};

struct LifetimeWorldObject : WorldObject
{
    LifetimeWorldObject() : WorldObject(false) { }
};

int main(int argc, char** argv)
{
    try
    {
        InitializeDungeon(argc, argv, true);
        {
            MapInstanced parent(1492, 300);
            LifetimeWorldObject proxy; proxy.SetMap(&parent);
#ifdef INSTANCE_SCRIPT_LIFETIME_BASELINE
            Check(OriginalGetterFaults(&proxy), "Expected original native dungeon-parent getter access violation not reproduced");
#else
            Check(proxy.GetInstanceScript() == nullptr, "Dungeon parent map claimed an instance script");
#endif
            proxy.ResetMap();
            Check(proxy.GetInstanceScript() == nullptr, "Unbound world object claimed an instance script");
        }
#ifndef INSTANCE_SCRIPT_LIFETIME_BASELINE
        unsigned contexts = 0;
        for (Difficulty difficulty : {DIFFICULTY_NORMAL, DIFFICULTY_MYTHIC_KEYSTONE})
        for (uint32 entry : {97032u, 97043u, 105877u})
        {
            {
                MapInstanced parent(1492, 300);
                InstanceMap map(1492, 300, 9940800 + contexts, difficulty, &parent);
                map.CreateInstanceData(nullptr);
                LifetimeWorldObject proxy; proxy.SetMap(&map);
                Check(proxy.GetInstanceScript() == map.GetInstanceScript() && proxy.GetInstanceScript(),
                    "Live derived instance getter lost its actual script");
                proxy.ResetMap();
                auto creature = new LifetimeMapCreature();
                Require(creature->Create(177100 + contexts, &map, 1, entry, 0, ALLIANCE,
                    2934.49f, 715.757f, 554.286f, 1.55255f), "Native map-owned creature create failed");
                Require(map.AddToMap(static_cast<Creature*>(creature)), "Native map-owned creature publication failed");
                // Deliberately leave the heap creature owned by the map. The
                // real base Map destructor must clean it without a derived
                // InstanceMap dynamic type or a surviving script pointer.
            }
            ++contexts; Check(true, "Native map destructor did not return");
        }
        Check(contexts == 6, "Native map lifetime scope changed");
        std::cout << "PASS " << checked << " native instance-script lifetime checks:unbound/dungeon parent/live instance getters and6 map-owned NPC/vehicle/plume unloads across normal/Keystone. No actual client grid transition claim\n";
#else
        std::cout << "COUNTERFACTUAL " << checked << " native getter checks:expected access violation explicitly caught for a dungeon parent that is not an InstanceMap; original core required, not an ordinary successful run\n";
#endif
        CharacterDatabase.Close(); LoginDatabase.Close(); WorldDatabase.Close(); HotfixDatabase.Close(); MySQL::Library_End();
        return 0;
    }
    catch (std::exception const& error) { std::cerr << error.what() << '\n'; return 1; }
}
