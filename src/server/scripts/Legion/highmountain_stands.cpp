#include "ScriptMgr.h"
#include "Player.h"
#include "SpellMgr.h"
#include <map>
#include <mutex>

namespace HighmountainStands
{
constexpr uint32 QuestId = 42512, Credit = 107704, Scene = 1287, SceneSpell = 213725, Teleport = 197461;
// Native quest POI 284367; Z checked against the intact 1532 navmesh.
Position const Confront = {3864.0f,3888.0f,901.463f,0.0f};
// Existing destination of 197461, beside quest ender Lasan 97407.
Position const Arrival = {3931.76f,3922.34f,905.17f,0.0f};
struct Run
{
    std::weak_ptr<uint8> lifetime;
    uint32 elapsed = 0, transferAt = 0;
    bool wantsTransfer = false, transferring = false, credited = false, finish = false;
};
std::map<ObjectGuid,Run> runs;
std::mutex mutex;
bool Present(Player* player)
{
    return player && player->IsInWorld() && player->IsAlive() && player->GetMapId() == 1220 &&
        !player->GetVehicle() && !player->GetTransport() && !player->isInFlight() && !player->IsFlying() &&
        player->GetExactDist(&Confront) < 180.0f;
}
void Clear(Player* player)
{
    { std::lock_guard<std::mutex> lock(mutex); runs.erase(player->GetGUID()); }
    // Aura removal cancels the client scene and releases its camera.
    player->RemoveAurasDueToSpell(SceneSpell);
}
void Event(Player* player, std::string const& event)
{
    if (!Present(player)) return;
    std::lock_guard<std::mutex> lock(mutex);
    auto it = runs.find(player->GetGUID());
    if (it == runs.end() || it->second.lifetime.expired()) return;
    Run& run = it->second;
    // Native package 1638 sends TELEPORT05 at 7s. Other events never award credit.
    if (event == "TELEPORT05" && run.elapsed >= 5000) run.wantsTransfer = true;
    // The core uses this event for both normal completion and an explicit skip.
    if (event == "complete") { run.wantsTransfer = true; run.finish = true; }
}
void Update(Player* player, uint32 diff)
{
    Run run;
    bool exists;
    {
        std::lock_guard<std::mutex> lock(mutex);
        auto it = runs.find(player->GetGUID());
        exists = it != runs.end();
        if (exists) { it->second.elapsed += diff; run = it->second; }
    }
    auto status = player->GetQuestStatus(QuestId);
    if (!exists)
    {
        if (status != QUEST_STATUS_INCOMPLETE || !Present(player) || player->IsBeingTeleported() ||
            player->GetQuestObjectiveData(QuestId,Credit) || player->GetExactDist(&Confront) > 15.0f ||
            !player->HasPhaseId(9180) || player->HasAuraType(SPELL_AURA_ACTIVATE_SCENE)) return;
        player->AddQuestDelayedEvent(QuestId,0,[] {});
        run.lifetime = player->getQuestStatus(QuestId)->ScriptLifetime;
        { std::lock_guard<std::mutex> lock(mutex); runs.emplace(player->GetGUID(),run); }
        // Preserve the native scene and its terrain-tracking quest 41246 effect.
        player->CastSpell(player,SceneSpell,true);
        if (!player->HasAura(SceneSpell)) Clear(player);
        return;
    }
    if (run.lifetime.expired() || !Present(player) ||
        (status != QUEST_STATUS_INCOMPLETE && !(run.credited && status == QUEST_STATUS_COMPLETE)) ||
        (player->IsBeingTeleported() && !run.transferring))
    { Clear(player); return; }

    // Native camera script lasts at least 19s. Allow 30s before recovering a lost
    // scene callback. This requires our own proximity-started live quest run.
    if (run.elapsed >= 30000) { run.wantsTransfer = true; run.finish = true; }
    if (run.transferring && !run.credited && run.elapsed-run.transferAt >= 10000)
    { Clear(player); return; }
    if (!run.transferring && run.wantsTransfer)
    {
        run.transferring = true; run.transferAt = run.elapsed;
        { std::lock_guard<std::mutex> lock(mutex); runs[player->GetGUID()] = run; }
        player->CastSpell(player,Teleport,true);
        return;
    }
    if (run.transferring && !run.credited && !player->IsBeingTeleported() &&
        player->GetExactDist(&Arrival) < 3.0f)
    {
        run.credited = true;
        { std::lock_guard<std::mutex> lock(mutex); runs[player->GetGUID()] = run; }
        // Change the terrain only after arrival is acknowledged on safe ground.
        if (!player->GetQuestObjectiveData(QuestId,Credit)) player->KilledMonsterCredit(Credit);
    }
    if (run.credited && (run.finish || !player->HasAura(SceneSpell))) Clear(player);
}
}

class scene_highmountain_stands : public SceneTriggerScript
{
public:
    scene_highmountain_stands() : SceneTriggerScript("scene_highmountain_stands") { }
    bool OnTrigger(Player* player, SpellScene const* scene, std::string event) override
    {
        if (scene->MiscValue != HighmountainStands::Scene) return false;
        HighmountainStands::Event(player,event);
        return true;
    }
};

class player_highmountain_stands : public PlayerScript
{
public:
    player_highmountain_stands() : PlayerScript("player_highmountain_stands") { }
    void OnUpdate(Player* player, uint32 diff) override { HighmountainStands::Update(player,diff); }
    void OnLogout(Player* player) override { HighmountainStands::Clear(player); }
    void OnMapChanged(Player* player) override { HighmountainStands::Clear(player); }
    void OnLogin(Player* player) override
    {
        HighmountainStands::Clear(player);
        // Repair only unrewarded auto-completion caused by the old Amount=0 row.
        if (player->GetQuestStatus(HighmountainStands::QuestId) == QUEST_STATUS_COMPLETE &&
            !player->GetQuestObjectiveData(HighmountainStands::QuestId,HighmountainStands::Credit))
            player->IncompleteQuest(HighmountainStands::QuestId);
    }
};

void AddSC_highmountain_stands()
{
    new scene_highmountain_stands();
    new player_highmountain_stands();
}
