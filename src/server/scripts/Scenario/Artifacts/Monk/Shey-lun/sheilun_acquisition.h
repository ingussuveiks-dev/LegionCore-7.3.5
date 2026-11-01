#ifndef SHEILUN_ACQUISITION_H
#define SHEILUN_ACQUISITION_H

#include "InstanceScript.h"
#include "Scenario.h"
#include "ScenarioMgr.h"
#include "ObjectAccessor.h"
#include "GameObject.h"
#include "LootMgr.h"
#include "Item.h"
#include "Group.h"
#include "TemporarySummon.h"
#include "WorldSession.h"
#include "SpellScript.h"
#include "SmartAI.h"
#include "Vehicle.h"
#include "MoveSpline.h"
#include "InstanceSaveMgr.h"
#include "CreatureTextMgr.h"
#include "Log.h"
#include "QuestData.h"
#include "sheilun_return_route.h"
#include "sheilun_acquisition_api.h"
#include <set>

// Included once by Sheylun.cpp, after the original combat/rescue AIs. The
// instance owns every pending operation; callbacks retain GUIDs and weak quest
// attempts, never borrowed Player/Creature pointers or an accepted quest lease.
namespace SheilunAcquisition
{
void RestoreControl(Player* player, ObjectGuid kite);
constexpr uint32 QuestId = 41003;
constexpr uint32 ScenarioId = 1007;
constexpr uint32 MapId = 1541;
constexpr uint32 Artifact = 128937;
constexpr uint32 BossEntry = 101887;
constexpr uint32 StaffEntry = 102479;
constexpr uint32 ProxyEntry = 256913;
constexpr uint32 GuideEntry = 101880;
constexpr uint32 KiteEntry = 102648;

inline bool Active(Player* player)
{
    if (!player || player->getClass() != CLASS_MONK || player->isGameMaster() ||
        player->IsQuestRewarded(QuestId))
        return false;
    QuestStatus status = player->GetQuestStatus(QuestId);
    return status == QUEST_STATUS_INCOMPLETE || status == QUEST_STATUS_COMPLETE;
}

inline bool Receipt(Scenario* scenario, uint32 tree)
{
    auto const* progress = scenario ? scenario->GetAchievementMgr().GetCriteriaProgressMap() : nullptr;
    if (!progress) return false;
    auto found = progress->find(tree);
    return found != progress->end() && found->second.Counter >= 1;
}

inline Item* InventoryArtifact(Player* player)
{
    Item* item = player ? player->GetItemByEntry(Artifact, false) : nullptr;
    return item && item->GetOwnerGUID() == player->GetGUID() && item->GetCount() ? item : nullptr;
}

// Native accessors index creatures/GOs by low counter. Pending content work
// retains the full identity, so a replaced slot must never resolve as its old
// actor (in particular when retiring a source or a cross-map vehicle).
inline Creature* CreatureAt(Map* map, ObjectGuid guid)
{
    if (!map || guid.IsEmpty()) return nullptr;
    Creature* actor = map->GetCreature(guid);
    return actor && actor->GetGUID() == guid ? actor : nullptr;
}

inline GameObject* ObjectAt(Map* map, ObjectGuid guid)
{
    if (!map || guid.IsEmpty()) return nullptr;
    GameObject* object = map->GetGameObject(guid);
    return object && object->GetGUID() == guid ? object : nullptr;
}

enum class Travel : uint8
{
    None, Requested, Boarding, Credit, Departure, FirstFlight, ExitFirst, Transfer
};

struct Attempt
{
    ObjectGuid owner, boss, defeated, taran, staff, proxy, guide, kite, item, loot;
    std::weak_ptr<uint8> quest;
    std::set<ObjectGuid> allies;
    uint64 epoch = 0;
    uint64 claim = 0;
    uint64 request = 0;
    uint32 sourceTimer = 0;
    uint32 claimTimer = 0;
    uint32 dialogueTimer = 0;
    uint32 sceneWait = 0;
    uint32 sceneLaunchWait = 0;
    uint32 guideTimer = 0;
    uint32 travelTimer = 0, retryTimer = 0;
    uint32 travelElapsed = 0, farewellTimer = 0;
    uint8 claimEffects = 0, returnEffects = 0;
    uint8 sourceTries = 0;
    uint8 claimTries = 0;
    uint8 guideTries = 0;
    uint8 dialogue = 0;
    bool started = false;
    bool victory = false;
    bool manualOpen = false;
    bool manualReceipt = false;
    bool stored = false;
    bool existingInventory = false;
    bool claimed = false;
    bool claiming = false;
    bool logoutResume = false;
    bool aborting = false;
    bool spawningStaff = false;
    bool spawningKite = false;
    bool boarded = false;
    bool returnCredited = false;
    bool sceneLaunched = false;
    bool guideReady = false;
    bool farewellStarted = false;
    Travel travel = Travel::None;
};

class Instance : public InstanceScript
{
public:
    explicit Instance(InstanceMap* map) : InstanceScript(map) { }

    Scenario* Progress() const
    {
        Scenario* scenario = sScenarioMgr->GetScenario(instance->GetInstanceId());
        return scenario && scenario->GetScenarioId() == ScenarioId && scenario->GetMap() == instance ? scenario : nullptr;
    }

    Player* Owner() const { return ObjectAccessor::FindPlayer(instance, state.owner); }

    bool Lease(Player* player) const
    {
        QuestStatusData* quest = player ? player->getQuestStatus(QuestId) : nullptr;
        return Active(player) && player->GetGUID() == state.owner && player->GetMap() == instance &&
            quest && !state.quest.expired() && quest->ScriptLifetime == state.quest.lock();
    }

    bool Present(Player* player, bool alive = true) const
    {
        return Lease(player) && player->IsInWorld() && player->CanContact() &&
            !player->HasPendingTeleport() && (!alive || player->IsAlive()) && !state.aborting && Progress();
    }

    bool Current(Player* player, uint64 epoch) const { return epoch == state.epoch && Present(player); }

    void OnPlayerEnter(Player* player) override;
    void OnPlayerLeave(Player* player) override;
    void OnCreatureCreate(Creature* creature) override;
    void CreatureDies(Creature* creature, Unit* killer) override;
    void Update(uint32 diff) override;

    void Started(Player* player);
    void BattleAlly(Creature* boss, Creature* ally);
    void Defeated(Creature* boss, Unit* killer);
    bool StaffSpawn(Creature* creature) const;
    bool Source(Player* player, WorldObject* object, uint32 entry) const;
    bool Open(Player* player, WorldObject* object);
    void Stored(Player* player, Item* item);
    bool BeginClaim(Player* player, Creature* staff, uint64& epoch, uint64& claim);
    bool ClaimEffect(Player* player, Creature* staff, uint64 epoch, uint64 claim, uint8 effect);
    void EndClaim(Player* player, uint64 epoch, uint64 claim, uint8 effects);
    bool SceneCast(Player* player) const;
    void SceneDone(Player* player);
    bool Guide(Player* player, Creature* guide) const;
    bool Request(Player* player, Creature* guide, uint32 menu, uint32 option);
    bool ReturnCast(Player* player, uint64& epoch, uint64& request) const;
    bool ReturnEffect(Player* player, uint64 epoch, uint64 request, uint8 effect);
    void EndReturnCast(Player* player, uint64 epoch, uint64 request, uint8 effects);
    void Kite(Creature* kite, Player* player);
    void Boarded(Creature* kite, Player* player, int8 seat, bool apply);
    void FlightEnded(Creature* kite);
    bool Transfer(Player* player) const;
    bool LogoutDeparture(Player* player) const
    {
        return Lease(player) && state.returnCredited && EarnedItem(player) &&
            (state.travel == Travel::Departure || state.travel == Travel::FirstFlight ||
                state.travel == Travel::ExitFirst || state.travel == Travel::Transfer);
    }
    Attempt const& State() const { return state; }

private:
    void Bind(Player* player, bool resume);
    void Retire(bool guide = true, bool kite = true);
    void Abort(Player* player);
    void SpawnSource(Player* player);
    void RetryClaim(Player* player);
    void Dialogue(Player* player, uint32 diff);
    void SpawnGuide(Player* player);
    void Speak(uint8 group);
    bool EarnedItem(Player* player) const;
    void TravelUpdate(Player* player, uint32 diff);

    Attempt state;
    ObjectGuid pendingEnter;
    bool pendingResume = false;
    uint64 serial = 0;
};

inline Instance* Controller(Player* player)
{
    return player && player->IsInWorld() && player->GetMapId() == MapId ? dynamic_cast<Instance*>(player->GetInstanceScript()) : nullptr;
}

inline Instance* Controller(Creature* actor)
{
    return actor && actor->GetMapId() == MapId ? dynamic_cast<Instance*>(actor->GetInstanceScript()) : nullptr;
}

inline bool Elapsed(uint32& remaining, uint32 diff)
{
    if (remaining > diff) { remaining -= diff; return false; }
    remaining = 0;
    return true;
}

inline bool At(Player* player, WorldObject* object)
{
    return player && object && object->IsInWorld() && !object->IsPreDelete() &&
        player->GetMap() == object->GetMap() && player->InSamePhase(object) &&
        player->CanTargetQuestObject(object) && player->canSeeOrDetect(object) &&
        player->IsWithinDistInMap(object, INTERACTION_DISTANCE) && player->IsWithinLOSInMap(object);
}

inline Position Landing()
{
    WorldSafeLocsEntry const* safe = sWorldSafeLocsStore.LookupEntry(SheilunReturnRoute::Landing);
    if (!safe || safe->MapID != SheilunReturnRoute::ReturnMap) return Position();
    return Position(safe->Loc.X, safe->Loc.Y, safe->Loc.Z, safe->Loc.O * float(M_PI) / 180.f);
}

inline void Instance::Bind(Player* player, bool resume)
{
    if (!Active(player)) return;
    player->AddQuestDelayedEvent(QuestId, 0, []() { });
    auto quest = player->getQuestStatus(QuestId);
    if (!quest || !quest->ScriptLifetime) return;
    if (!resume)
    {
        ObjectGuid boss = state.boss, taran = state.taran;
        state = Attempt();
        state.boss = boss;
        state.taran = taran;
        state.epoch = ++serial;
    }
    state.owner = player->GetGUID();
    state.quest = quest->ScriptLifetime;
    state.logoutResume = false;
    if (resume && state.victory && !state.claimed) state.sourceTimer = 1;
    // Relog cancelled the old optional scene; its actual earned pickup remains.
    // Replay the unfinished lore without fabricating a scene-complete receipt.
    if (resume && state.claimed && state.dialogue < 4)
    {
        state.dialogue = 1; state.dialogueTimer = 4000; state.sceneWait = 0;
    }
}

inline void Instance::OnPlayerEnter(Player* player)
{
    // Map::AddPlayerToMap has not inserted the player into the world yet.
    if (player && !player->isGameMaster())
    {
        pendingEnter = player->GetGUID();
        pendingResume = state.owner == pendingEnter && state.logoutResume && player->GetSession() &&
            player->GetSession()->PlayerLoading() && player->GetScenarioLoginInstance() == instance->GetInstanceId();
    }
}

inline void Instance::Retire(bool guide, bool kite)
{
    ObjectGuid staff = state.staff, proxy = state.proxy;
    state.staff.Clear(); state.proxy.Clear(); state.manualOpen = false; state.loot.Clear();
    if (Creature* actor = CreatureAt(instance, staff)) actor->DespawnOrUnsummon();
    if (GameObject* object = ObjectAt(instance, proxy)) object->Delete();
    if (guide)
    {
        ObjectGuid guid = state.guide; state.guide.Clear(); state.guideReady = false;
        if (Creature* actor = CreatureAt(instance, guid)) actor->DespawnOrUnsummon();
    }
    if (kite)
    {
        ObjectGuid guid = state.kite; state.kite.Clear(); state.boarded = false;
        if (Creature* actor = CreatureAt(instance, guid)) actor->DespawnOrUnsummon();
    }
}

inline void Instance::OnPlayerLeave(Player* player)
{
    if (!player || player->GetGUID() != state.owner) return;
    state.logoutResume = Lease(player) && player->GetSession() && player->GetSession()->PlayerLogout();
    RestoreControl(player, state.kite);
    // No continuation retains a raw pointer, and the source Loot is not removed
    // from inside StoreLootItem's borrowed-container callback.
    Retire();
    state.claiming = false;
    if (state.travel != Travel::Transfer) state.travel = Travel::None;
}

inline void Instance::Abort(Player* player)
{
    state.aborting = true;
    if (!player || !player->IsInWorld() || player->GetMap() != instance || player->HasPendingTeleport()) return;
    RestoreControl(player, state.kite);
    if (player->GetVehicle()) { player->ExitVehicle(); return; }
    if (!player->movespline->Finalized()) return;
    // Retaking a quest cannot reuse the abandoned shared Scenario criteria.
    // Remove only this exact nonpermanent in-memory scenario binding, after
    // AddPlayerToMap completed. Other group/player saves remain untouched.
    if (Group* group = player->GetGroup())
        if (InstanceGroupBind* bind = group->GetBoundInstance(instance))
            if (!bind->perm && bind->save && bind->save->GetInstanceId() == instance->GetInstanceId())
                group->UnbindInstance(MapId, uint8(instance->GetDifficultyID()), true);
    Retire();
    WorldSafeLocsEntry const* safe = sWorldSafeLocsStore.LookupEntry(SheilunReturnRoute::Landing);
    if (safe && safe->MapID == SheilunReturnRoute::ReturnMap)
    {
        Position destination = Landing();
        player->TeleportTo(safe->MapID, destination.GetPositionX(), destination.GetPositionY(),
            destination.GetPositionZ(), destination.GetOrientation());
    }
}

inline void Instance::OnCreatureCreate(Creature* creature)
{
    if (!creature) return;
    if (creature->GetEntry() == BossEntry && creature->GetDBTableGUIDLow() == 362866) state.boss = creature->GetGUID();
    if (creature->GetEntry() == 101881 && creature->GetDBTableGUIDLow() == 362837) state.taran = creature->GetGUID();
}

inline void Instance::Started(Player* player)
{
    if (state.owner.IsEmpty()) Bind(player, false);
    if (Present(player) && Receipt(Progress(), 46662)) state.started = true;
}

inline void Instance::BattleAlly(Creature* boss, Creature* ally)
{
    if (!boss || !ally || boss->GetGUID() != state.boss || ally->GetMap() != instance ||
        !ally->IsAlive() || !state.started || !ally->InSamePhase(boss)) return;
    switch (ally->GetEntry())
    {
        case 101881: case 101882: case 101883: case 101885: state.allies.insert(ally->GetGUID()); break;
        default: break;
    }
}

inline void Instance::CreatureDies(Creature* creature, Unit* killer) { Defeated(creature, killer); }

inline void Instance::Defeated(Creature* boss, Unit* killer)
{
    Player* player = Owner();
    if (!boss || boss->GetEntry() != BossEntry || boss->IsAlive() || boss->GetGUID() != state.boss ||
        boss->GetDBTableGUIDLow() != 362866 || !Present(player, false) || !state.started ||
        !Receipt(Progress(), 46662) || !state.defeated.IsEmpty() || !player->IsAtGroupRewardDistance(boss)) return;
    Player* credited = killer ? killer->GetCharmerOrOwnerPlayerOrPlayerItself() : boss->GetLootRecipient();
    bool eligible = credited && (credited == player || (player->GetGroup() && credited->GetGroup() == player->GetGroup()));
    if (!eligible && (!killer || !state.allies.count(killer->GetGUID()))) return;
    state.defeated = boss->GetGUID(); // Both AI and native instance death callbacks can arrive.
    uint64 epoch = state.epoch;
    player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 48495);
    player = Owner();
    if (epoch != state.epoch || !Present(player, false) || !Receipt(Progress(), 46663)) return;
    state.victory = true;
    Speak(13);
    state.sourceTimer = 5628;
}

inline bool Instance::StaffSpawn(Creature* creature) const
{
    return creature && creature->GetEntry() == StaffEntry && state.spawningStaff && state.victory;
}

inline bool Instance::Source(Player* player, WorldObject* object, uint32 entry) const
{
    return Present(player) && state.victory && Receipt(Progress(), 46663) && object &&
        object->GetEntry() == entry && object->GetGUID() == (entry == StaffEntry ? state.staff : state.proxy) && At(player, object);
}

inline void Instance::SpawnSource(Player* player)
{
    if (!Present(player) || !state.victory || state.claimed || state.sourceTries >= 5) return;
    ++state.sourceTries;
    float z = instance->GetHeight(player->GetPhases(), -1017.29f, -2772.8f, 45.f, true, 15.f);
    if (!std::isfinite(z) || z < 35.f || z > 42.f)
    {
        TC_LOG_ERROR("scripts", "Sheilun: missing pool support; physical source remains pending");
        state.sourceTimer = 1000;
        return;
    }
    Position pose(-1017.29f, -2772.8f, z, 0.f);
    if (!CreatureAt(instance, state.staff))
    {
        state.spawningStaff = true;
        Creature* staff = player->SummonCreature(StaffEntry, pose, TEMPSUMMON_TIMED_DESPAWN, 600000, 0, player->GetGUID());
        state.spawningStaff = false;
        if (staff)
        {
            state.staff = staff->GetGUID();
            player->BindQuestObject(QuestId, staff);
            staff->CastSpell(staff, 202385, true);
        }
    }
    if (!ObjectAt(instance, state.proxy))
        if (GameObject* proxy = player->SummonGameObject(ProxyEntry, pose, 0.f, 0.f, 0.f, 0.f, 1.f, 600, player->GetGUID()))
        {
            state.proxy = proxy->GetGUID();
            player->BindQuestObject(QuestId, proxy);
            // Native GO_NOT_READY's default chest branch only arms GO_READY.
            // Do it before exposing this new interactive source; the generic
            // GO update throttle can otherwise leave its first loot empty.
            proxy->SetLootState(GO_READY);
        }
    if (!CreatureAt(instance, state.staff) || !ObjectAt(instance, state.proxy))
    {
        TC_LOG_ERROR("scripts", "Sheilun: physical source creation failed; bounded retry pending");
        state.sourceTimer = 1000;
    }
    else { state.sourceTries = 0; Speak(14); }
}

inline bool Instance::Open(Player* player, WorldObject* object)
{
    if (!object || (!Source(player, object, StaffEntry) && !Source(player, object, ProxyEntry)) || state.claimed) return false;
    if (Item* item = InventoryArtifact(player))
    {
        state.manualOpen = true;
        state.item = item->GetGUID();
        state.existingInventory = true;
        state.manualReceipt = true;
        state.stored = true;
        state.claimTimer = 1;
        return true;
    }
    GameObject* proxy = ObjectAt(instance, state.proxy);
    if (!proxy || !Source(player, proxy, ProxyEntry)) return false;
    if (proxy->getLootState() != GO_READY && proxy->getLootState() != GO_ACTIVATED) return false;
    uint64 epoch = state.epoch;
    player->SendLoot(proxy->GetGUID(), LOOT_CORPSE);
    player = Owner(); proxy = ObjectAt(instance, state.proxy);
    if (!Current(player, epoch) || !proxy || player->GetLootGUID() != proxy->GetGUID()) return false;
    Loot* loot = proxy->IsPersonal() ? player->GetPersonalLoot(proxy->GetGUID()) : &proxy->loot;
    if (!loot || loot->GetGUID().IsEmpty()) return false;
    state.loot = loot->GetGUID(); state.manualOpen = true;
    return true;
}

inline void Instance::Stored(Player* player, Item* item)
{
    if (!Present(player) || !state.victory || !state.manualOpen || state.stored || !item ||
        item->GetEntry() != Artifact || item->GetOwnerGUID() != state.owner || !item->GetCount()) return;
    Item* owned = player->GetItemByGuid(item->GetGUID());
    GameObject* proxy = ObjectAt(instance, state.proxy);
    if (owned != item || !Source(player, proxy, ProxyEntry)) return;
    Loot* loot = proxy->IsPersonal() ? player->GetPersonalLoot(proxy->GetGUID()) : &proxy->loot;
    if (!loot || loot->GetGUID() != state.loot || state.loot.IsEmpty()) return;
    bool consumed = false;
    for (auto const& slot : loot->items) consumed |= slot.item.ItemID == Artifact && slot.is_looted;
    for (auto const& slot : loot->quest_items) consumed |= slot.item.ItemID == Artifact && slot.is_looted;
    if (!consumed) return;
    state.item = item->GetGUID(); state.stored = true; state.manualReceipt = true; state.claimTimer = 1;
}

inline bool Instance::EarnedItem(Player* player) const
{
    Item* item = player ? player->GetItemByGuid(state.item) : nullptr;
    return state.stored && state.manualReceipt && item && item->GetEntry() == Artifact &&
        item->GetOwnerGUID() == state.owner && item->GetCount() && InventoryArtifact(player);
}

inline bool Instance::BeginClaim(Player* player, Creature* staff, uint64& epoch, uint64& claim)
{
    if (!Source(player, staff, StaffEntry) || state.claimed) return false;
    epoch = state.epoch; claim = state.claim;
    return true;
}

inline bool Instance::ClaimEffect(Player* player, Creature* staff, uint64 epoch, uint64 claim, uint8 effect)
{
    if (!Current(player, epoch) || claim != state.claim || !state.claiming || !EarnedItem(player) ||
        !Source(player, staff, StaffEntry) || effect > 2 || (state.claimEffects & (1 << effect))) return false;
    state.claimEffects |= 1 << effect; // Reserve before native handler re-entry.
    return true;
}

inline void Instance::EndClaim(Player* player, uint64 epoch, uint64 claim, uint8 effects)
{
    if (!Current(player, epoch) || claim != state.claim) return;
    if (!state.claiming)
    {
        if (Creature* staff = CreatureAt(instance, state.staff)) Open(player, staff);
        return;
    }
    state.claiming = false;
    // Effect160 queues202437 through the native delayed trigger. AfterCast is
    // too early to claim its scene was launched; the guarded child confirms it.
    if (effects & 4) { state.sceneLaunchWait = 2000; state.claimTimer = 0; }
    else state.claimTimer = 1000;
}

inline bool Instance::SceneCast(Player* player) const
{
    return Present(player) && EarnedItem(player) && !state.claimed && (state.claimEffects & 4) &&
        Receipt(Progress(), 46717) && (state.claiming || state.sceneLaunchWait);
}

inline void Instance::SceneDone(Player* player)
{
    if (!SceneCast(player) || !player->HasSceneStatus(1192, SCENE_LAUNCH) || player->HasSceneStatus(1192, SCENE_COMPLETE)) return;
    state.sceneLaunched = true; state.sceneLaunchWait = 0; state.claimTimer = 0;
    state.claimed = true; state.sceneWait = 0; state.dialogue = 0; state.dialogueTimer = 0;
}

inline void Instance::RetryClaim(Player* player)
{
    if (!Present(player) || !EarnedItem(player) || state.claimed || state.claiming || state.claimTries >= 5) return;
    Creature* staff = CreatureAt(instance, state.staff);
    if (!Source(player, staff, StaffEntry)) { state.claimTimer = 500; return; }
    ++state.claimTries; ++state.claim; state.claiming = true;
    uint64 epoch = state.epoch, claim = state.claim;
    player->CastSpell(staff, 202435, true);
    if (Current(Owner(), epoch) && state.claim == claim && state.claiming)
    {
        state.claiming = false; state.claimTimer = 1000;
        TC_LOG_ERROR("scripts", "Sheilun: native claim did not finish; stored artifact retained for retry");
    }
}

inline void Instance::Speak(uint8 group)
{
    if (Creature* actor = CreatureAt(instance, state.taran))
        // Native map cap is one non-GM player; map range keeps the final voice
        // audible after takeoff instead of losing it outside SAY distance.
        sCreatureTextMgr->SendChat(actor, group, state.owner, CHAT_MSG_ADDON, LANG_ADDON, TEXT_RANGE_MAP);
}

inline void Instance::Dialogue(Player* player, uint32 diff)
{
    if (!state.claimed || state.guideReady || state.guideTimer) return;
    if (!state.dialogue)
    {
        if (!player->HasSceneStatus(1192, SCENE_COMPLETE))
        {
            state.sceneWait += diff;
            if (state.sceneWait < 90000) return;
            TC_LOG_ERROR("scripts", "Sheilun: equipment scene completion timed out; artifact retained, return available");
        }
        state.dialogue = 1; state.dialogueTimer = 4000;
    }
    if (!Elapsed(state.dialogueTimer, diff)) return;
    switch (state.dialogue++)
    {
        case 1: Speak(15); state.dialogueTimer = 13534; break;
        case 2: Speak(16); state.dialogueTimer = 7361; break;
        case 3: Speak(17); state.dialogueTimer = 6173; break;
        case 4: state.guideTimer = 3000; break;
        default: break;
    }
}

inline void Instance::SpawnGuide(Player* player)
{
    if (!Present(player) || !state.claimed || state.guideTries >= 5 || CreatureAt(instance, state.guide)) return;
    ++state.guideTries;
    Creature* taran = CreatureAt(instance, state.taran);
    // A missing optional speaker must not strand an actual artifact owner.
    // This fallback is the same declared functional pool anchor, not an
    // invented retail Tak-Tak pose or a replacement progression event.
    Position pose = taran ? taran->GetHomePosition() : Position(-1017.29f,-2772.8f,38.57111f,0.f);
    if (!taran) TC_LOG_WARN("scripts", "Sheilun: outro speaker unavailable; return guide uses validated pool fallback");
    float z = instance->GetHeight(player->GetPhases(), pose.GetPositionX(), pose.GetPositionY(), pose.GetPositionZ()+5.f, true, 10.f);
    if (!std::isfinite(z) || std::fabs(z-pose.GetPositionZ()) > 3.f) { state.guideTimer = 1000; return; }
    pose.m_positionZ = z;
    if (Creature* guide = player->SummonCreature(GuideEntry, pose, TEMPSUMMON_TIMED_DESPAWN, 600000, 0, player->GetGUID()))
    {
        state.guide = guide->GetGUID(); player->BindQuestObject(QuestId, guide);
        guide->setActive(true);
        guide->SetWalk(true);
        guide->GetMotionMaster()->MoveFollow(player, 2.f, float(M_PI));
        state.guideReady = true;
        state.guideTries = 0;
    }
    else { state.guideTimer = 1000; TC_LOG_ERROR("scripts", "Sheilun: return guide summon failed; retry pending"); }
}

inline void Instance::Update(uint32 diff)
{
    if (!pendingEnter.IsEmpty())
    {
        Player* player = ObjectAccessor::FindPlayer(instance, pendingEnter);
        if (player && player->IsInWorld())
        {
            pendingEnter.Clear();
            if (state.owner.IsEmpty()) Bind(player, false);
            else if (!Lease(player))
            {
                bool resume = pendingResume && state.owner == player->GetGUID() && state.logoutResume &&
                    Active(player) && (!state.victory || Receipt(Progress(), 46663));
                if (resume) Bind(player, true);
                else { Abort(player); return; }
            }
            if (Lease(player) && state.victory && !state.claimed) state.sourceTimer = 1;
            if (Lease(player) && state.claimed && state.dialogue >= 4 && !CreatureAt(instance, state.guide)) state.guideTimer = 1;
        }
    }
    Player* player = Owner();
    if (!player) return;
    if (!Lease(player) || state.aborting) { Abort(player); return; }
    if (!Present(player)) return;
    // Native opcode storage has finished before this instance update. Remove
    // the spent world visual/source without invalidating its earned receipt.
    if (state.claimed && (!state.staff.IsEmpty() || !state.proxy.IsEmpty())) Retire(false, false);
    if (state.victory && !state.claimed && !state.sourceTimer && state.sourceTries < 5 &&
        (!CreatureAt(instance, state.staff) || !ObjectAt(instance, state.proxy))) state.sourceTimer = 1000;
    if (state.claimed && state.guideReady && state.travel == Travel::None && !CreatureAt(instance, state.guide))
    {
        state.guideReady = false; state.guideTimer = 1000;
    }
    if (state.sceneLaunchWait && Elapsed(state.sceneLaunchWait, diff))
    {
        state.claimEffects &= ~uint8(4); state.claimTimer = 1000;
        TC_LOG_ERROR("scripts", "Sheilun: native equipment scene launch failed; stored artifact retained for retry");
    }
    if (state.sourceTimer && Elapsed(state.sourceTimer, diff)) SpawnSource(player);
    if (state.claimTimer && Elapsed(state.claimTimer, diff)) RetryClaim(player);
    Dialogue(player, diff);
    if (state.guideTimer && Elapsed(state.guideTimer, diff)) SpawnGuide(player);
    TravelUpdate(player, diff);
}
}
#endif
