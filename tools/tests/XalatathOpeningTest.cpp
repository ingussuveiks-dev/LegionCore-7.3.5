#include "Host.inc"
#define TC_LOG_ERROR(...) ((void)0)
enum { GAMEOBJECT_TYPE_GOOBER = 10 };
using ObjectGuid = uint32;
using LootType = uint32;
struct Spell {
    Unit* m_caster;
    GameObject* gameObjTarget;
    uint32 lootRequests = 0;
    void SendLoot(ObjectGuid const&, LootType);
};
#include "SendLoot.inc"
struct Pickup : Fixture {
    Pickup() {
        object.entry = 247690; object.mapId = 1539;
        player.cls = CLASS_PRIEST; player.quest = 40710; player.owned = false;
        scenarioManager.scenario.id = 991; scenarioManager.scenario.step = 8;
        object.AI()->InitializeAI();
    }
    uint32 Open() { Spell cast{&player, &object}; cast.SendLoot(10, 0); return cast.lootRequests; }
    bool OrdinaryUse() {
        if (binding.OnGossipHello(&player, &object)) return false;
        if (object.AI()->GossipUse(&player)) return false;
        return true;
    }
};
int main() {
    _set_abort_behavior(0, _CALL_REPORTFAULT);
    {
        Pickup pickup; scenarioManager.scenario.step = 7;
        assert(pickup.Open() == 0 && pickup.Script().gossip == 0 && pickup.object.lootState == 0);
    }
    for (uint32 invalid = 0; invalid < 25; ++invalid) {
        Pickup pickup;
        switch (invalid) {
            case 0: pickup.player.alive = false; break;
            case 1: pickup.player.teleport = true; break;
            case 2: pickup.player.cls = CLASS_HUNTER; break;
            case 3: pickup.player.quest = 41631; break;
            case 4: pickup.player.status = 0; break;
            case 5: pickup.player.status = 5; break;
            case 6: pickup.player.rewarded = true; break;
            case 7: pickup.player.phase = false; break;
            case 8: pickup.player.visible = false; break;
            case 9: pickup.player.los = false; break;
            case 10: pickup.player.distance = 5.01f; break;
            case 11: pickup.object.flags = GO_FLAG_IN_USE; break;
            case 12: pickup.object.flags = GO_FLAG_NOT_SELECTABLE; break;
            case 13: pickup.object.spawned = false; break;
            case 14: pickup.object.world = false; break;
            case 15: pickup.player.world = false; break;
            case 16: pickup.player.contact = false; break;
            case 17: pickup.player.map = &otherMap; break;
            case 18: pickup.object.mapId = 1; break;
            case 19: pickup.object.lootState = GO_JUST_DEACTIVATED; break;
            case 20: scenarioManager.scenario.id = 1092; break;
            case 21: scenarioManager.present = false; break;
            case 22: scenarioManager.scenario.step = 7; break;
            case 23: scenarioManager.scenario.step = 10; break;
            case 24: pickup.player.instance = 43; break;
        }
        uint32 previousState = pickup.object.lootState;
        pickup.Report(); assert(pickup.Script().gossip == 0);
        assert(!pickup.OrdinaryUse() && pickup.Open() == 0 && pickup.Script().gossip == 0);
        assert(pickup.object.lootState == previousState && pickup.player.owns == 0);
    }
    for (uint32 stage : {8u, 9u}) for (uint32 status : {1u, 3u}) for (bool owned : {false, true}) {
        Pickup pickup; scenarioManager.scenario.step = stage; pickup.player.status = status;
        pickup.player.owned = owned; pickup.player.distance = 5.0f;
        pickup.Report(); assert(pickup.Script().gossip == 1);
        assert(pickup.Open() == (owned ? 0u : 1u) && pickup.Script().gossip == 2);
        assert(pickup.OrdinaryUse() == !owned && pickup.Script().gossip == 3);
        assert(pickup.object.lootState == 0 && pickup.player.owns == 0);
        pickup.object.AI()->OnStateChanged(GO_JUST_DEACTIVATED, &pickup.player);
        assert(pickup.Script().state == 1 && pickup.Script().invoker == &pickup.player);
        pickup.object.AI()->Reset(); pickup.object.AI()->UpdateAI(200);
        assert(pickup.Script().resets == 1 && pickup.Script().elapsed == 200);
    }
    {
        Pickup pickup; pickup.player.bank = true;
        assert(pickup.Open() == 1 && pickup.object.lootState == 0);
        assert(pickup.object.AI()->GossipHello(nullptr, false));
        assert(binding.OnGossipHello(nullptr, &pickup.object) && binding.OnGossipHello(&pickup.player, nullptr));
    }
    {
        Pickup pickup; pickup.player.phase = false;
        assert(pickup.Open() == 0);
        pickup.player.phase = true; assert(pickup.Open() == 1);
        assert(pickup.Script().gossip == 1 && pickup.object.lootState == 0);
    }
    {
        Pickup pickup; pickup.object.entry = 246699; pickup.player.owned = true; pickup.player.phase = false;
        assert(!binding.OnGossipHello(&pickup.player, &pickup.object));
        assert(!pickup.object.AI()->GossipHello(&pickup.player, false) && pickup.Script().gossip == 1);
    }
    std::cout << "PASS: selected native SmartAI/report-use and open-lock prefix enforce Xalatath opening before gossip/loot;25 invalid contexts, valid8/9, actual-owner gossip without unique re-loot, preserved state/lifecycle and denied-then-eligible retry\n";
}
