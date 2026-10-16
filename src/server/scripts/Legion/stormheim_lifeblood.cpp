#include "stormheim_lifeblood.h"
#include "ScriptMgr.h"
#include "SmartAI.h"
#include "Player.h"
#include "GameObject.h"
#include "GossipDef.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "QuestData.h"
#include "EventObject.h"
#include "MotionMaster.h"
#include <map>
#include <mutex>

namespace StormheimLifeblood
{
constexpr uint32 QuestId = 39855, ExitCredit = 97487;
Position const Altar = {490.429f, 721.396f, 23.2744f, 3.80208f};
// Preserve the installed quest exit, beside the path to Havi (not EO333's
// unrelated shore destination). Core teleport arrival protection still applies.
Position const Landing = {3152.87f, 1536.12f, 181.33f, 6.09f};
Position const ReturnGate = {336.092f, 345.864f, 30.6906f, 0};
struct Run
{
    std::weak_ptr<uint8> lifetime;
    ObjectGuid spirit, altar;
    uint32 timer = 0, stage = 0;
    bool summoning = false, greeted = false;
};
struct OfferData { std::weak_ptr<uint8> lifetime; ObjectGuid altar; };
std::map<ObjectGuid, Run> runs;
std::map<ObjectGuid, OfferData> offers;
std::recursive_mutex mutex;

bool Scope(Player* p)
{
    return p && p->IsInWorld() && p->GetMapId() == 1463 && !p->GetInstanceId() &&
        p->GetZoneId() == 7588 && !p->HasPhaseId(8306);
}
bool Definition()
{
    auto q = sQuestDataStore->GetQuestTemplate(QuestId);
    if (!q || q->StartScript || q->HasSpecialFlag(QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT)) return false;
    uint32 count = 0;
    for (auto const& o : q->GetObjectives())
    {
        if (o.Type != QUEST_OBJECTIVE_MONSTER || o.ObjectID != ExitCredit || o.StorageIndex || o.Amount != 1 || o.Flags) return false;
        ++count;
    }
    return count == 1;
}
bool Accepted(Player* p)
{
    return p && Definition() && p->GetQuestStatus(QuestId) == QUEST_STATUS_INCOMPLETE &&
        !p->GetQuestObjectiveData(QuestId, ExitCredit);
}
bool Ready(Player* p)
{
    return Scope(p) && Accepted(p) && p->IsAlive() && !p->IsBeingTeleported() &&
        !p->GetVehicle() && !p->GetTransport() && !p->isInFlight() && !p->isInCombat();
}
bool Current(Player* p, Run const& r)
{
    auto s = p->getQuestStatus(QuestId);
    return Accepted(p) && s && !r.lifetime.expired() && s->ScriptLifetime == r.lifetime.lock();
}
bool Source(Player* p, GameObject* go, float distance)
{
    return Ready(p) && go && go->IsInWorld() && go->GetMapId() == 1463 && !go->GetInstanceId() &&
        go->GetEntry() == 244559 && go->GetDBTableGUIDLow() == 127713 &&
        go->GetGoType() == GAMEOBJECT_TYPE_QUESTGIVER && go->GetGOInfo()->questgiver.gossipID == 18790 &&
        go->GetExactDist(&Altar) < 1 && go->isSpawned() &&
        !go->HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_IN_USE | GO_FLAG_NOT_SELECTABLE) &&
        p->InSamePhase(go) && p->canSeeOrDetect(go) && p->GetExactDist(go) <= distance && p->IsWithinLOSInMap(go);
}
void RemoveSpirit(Player* p, Run& r)
{
    // Resolve only on this player's current map. Never retain a raw old Map
    // pointer across transfers; a stranded actor cleans itself on its map.
    if (auto c = ObjectAccessor::GetCreature(*p, r.spirit)) c->DespawnOrUnsummon();
    r.spirit.Clear();
}
void Clear(Player* p)
{
    offers.erase(p->GetGUID());
    auto it = runs.find(p->GetGUID());
    if (it != runs.end()) { RemoveSpirit(p, it->second); runs.erase(it); }
}
void RetireController(Player* p)
{
    // The native maintain aura also triggers missing spell81040 and summons a
    // slot6 guardian. Its own removal handler retires only192470 minions.
    p->RemoveAurasDueToSpell(192471, p->GetGUID());
}
Run& Start(Player* p)
{
    auto it = runs.find(p->GetGUID());
    if (it == runs.end())
    {
        RetireController(p);
        p->AddQuestDelayedEvent(QuestId, 0, [] {});
        Run r; r.lifetime = p->getQuestStatus(QuestId)->ScriptLifetime;
        it = runs.emplace(p->GetGUID(), r).first;
    }
    return it->second;
}
void Offer(Player* p, GameObject* go)
{
    std::lock_guard<std::recursive_mutex> lock(mutex);
    for (auto it = offers.begin(); it != offers.end();)
        if (it->second.lifetime.expired()) it = offers.erase(it); else ++it;
    offers.erase(p->GetGUID());
    if (!Source(p, go, go->GetInteractionDistance())) return;
    auto it = runs.find(p->GetGUID());
    if (it != runs.end() && !Current(p, it->second)) Clear(p);
    auto& r = Start(p);
    if (!r.stage) offers[p->GetGUID()] = {r.lifetime, go->GetGUID()};
}
bool Select(Player* p, GameObject* go, unsigned menu, unsigned option)
{
    if (menu != 18790 || option != 1) return false;
    std::lock_guard<std::recursive_mutex> lock(mutex);
    // GO AI dispatch precedes Player::OnGossipSelect validation. An incoming
    // packet alone is not evidence that this player was offered this choice.
    auto offer = offers.find(p->GetGUID());
    auto it = runs.find(p->GetGUID());
    auto& gossip = p->PlayerTalkClass->GetGossipMenu();
    if (!Source(p, go, go->GetInteractionDistance()) || offer == offers.end() || it == runs.end() ||
        !Current(p, it->second) || it->second.stage || offer->second.altar != go->GetGUID() ||
        offer->second.lifetime.expired() || offer->second.lifetime.lock() != it->second.lifetime.lock() ||
        gossip.GetMenuId() != menu || !gossip.GetItem(option)) return true;
    offers.erase(offer);
    p->PlayerTalkClass->ClearMenus(); p->PlayerTalkClass->SendCloseGossip();
    auto& r = it->second;
    r.altar = go->GetGUID(); r.stage = 1; r.timer = 8823;
    p->CastSpell(p, 212916, true); // native2257; optional, never a progression gate
    return true;
}
void CreditArrival(Player* p, Run const& r)
{
    auto q = sQuestDataStore->GetQuestTemplate(QuestId);
    for (auto const& o : q->GetObjectives())
    {
        p->SetQuestObjectiveData(q, &o, 1);
        p->SendQuestUpdateAddCredit(q, r.altar, o, 1);
        if (p->CanCompleteQuest(QuestId)) p->CompleteQuest(QuestId);
    }
}
void Update(Player* p, uint32 diff)
{
    auto it = runs.find(p->GetGUID());
    if (it != runs.end() && !Current(p, it->second)) { Clear(p); it = runs.end(); }
    if (it != runs.end() && it->second.stage == 4)
    {
        auto& r = it->second;
        auto& destination = p->GetTeleportDest();
        if (p->HasPhaseId(8306) || !p->IsAlive() || destination.GetMapId() != 1220 ||
            destination.GetExactDist(&Landing) > 1) { Clear(p); return; }
        // TeleportTo returning true only queues the transfer. Do not credit
        // until the actual map/coordinates are installed and its ACK completes.
        // Slow loading is not a failed quest attempt. The pending request is
        // bounded by this player's session/lifetime and cleared on logout.
        if (p->IsBeingTeleported()) return;
        if (p->GetMapId() == 1220 && !p->GetInstanceId() && p->GetExactDist(&Landing) < 5 &&
            !p->GetVehicle() && !p->GetTransport() && !p->isInFlight()) CreditArrival(p, r);
        Clear(p); return;
    }
    if (!Ready(p)) { Clear(p); return; }
    auto& r = Start(p);
    if (r.stage)
    {
        auto go = ObjectAccessor::GetGameObject(*p, r.altar);
        if (!Source(p, go, 10)) { Clear(p); return; }
        if (r.timer > diff) { r.timer -= diff; return; }
        // Keep each native line's complete duration, even after a long tick.
        if (r.stage == 1)
        {
            if (auto c = ObjectAccessor::GetCreature(*p, r.spirit))
            {
                c->GetMotionMaster()->MoveIdle();
                c->CastSpell(c, 216818, true); // Ashildir Trapped, private actor only
            }
            p->CastSpell(p, 212917, true); r.stage = 2; r.timer = 4978;
        }
        else if (r.stage == 2) { p->CastSpell(p, 212918, true); r.stage = 3; r.timer = 6396; }
        else
        {
            RemoveSpirit(p, r); r.stage = 4; r.timer = 0;
            // Do not cast192670: its linked scene/root/stun needs unverified
            // client callbacks. The existing direct exit uses core pet/arrival cleanup.
            if (!p->TeleportTo(1220, Landing.GetPositionX(), Landing.GetPositionY(), Landing.GetPositionZ(), Landing.GetOrientation())) Clear(p);
        }
        return;
    }
    if (r.timer > diff) { r.timer -= diff; return; }
    r.timer = 500;
    auto spirit = ObjectAccessor::GetCreature(*p, r.spirit);
    if (spirit && (!spirit->IsAlive() || !p->InSamePhase(spirit) || p->GetExactDist(spirit) > 70))
    {
        RemoveSpirit(p, r); spirit = nullptr;
    }
    if (!spirit)
    {
        r.summoning = true;
        auto c = p->SummonCreature(97558, p->GetPositionX(), p->GetPositionY(), p->GetPositionZ(), p->GetOrientation(), TEMPSUMMON_MANUAL_DESPAWN, 0, p->GetGUID());
        r.summoning = false;
        if (c && r.spirit != c->GetGUID()) c->DespawnOrUnsummon();
    }
}
}

struct npc_stormheim_lifeblood_ashildir : SmartAI
{
    explicit npc_stormheim_lifeblood_ashildir(Creature* c) : SmartAI(c) { }
    ObjectGuid owner;
    void IsSummonedBy(Unit* summoner) override
    {
        using namespace StormheimLifeblood;
        std::lock_guard<std::recursive_mutex> lock(mutex);
        auto p = summoner ? summoner->ToPlayer() : nullptr;
        auto it = p ? runs.find(p->GetGUID()) : runs.end();
        if (it == runs.end() || !Current(p, it->second) || !it->second.summoning || me->GetEntry() != 97558)
        { SmartAI::IsSummonedBy(summoner); return; }
        owner = p->GetGUID(); it->second.spirit = me->GetGUID();
        p->BindQuestObject(QuestId, me);
        me->AddPlayerInPersonnalVisibilityList(owner);
        me->SetReactState(REACT_PASSIVE);
        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC);
        me->setActive(true); me->GetMotionMaster()->MoveFollow(p, 1.5f, 3.14159f);
        if (!it->second.greeted)
        {
            // Native98494: "The time for planning is over ... we take action!"
            // Say this while setting out, not over Helya's later response.
            Talk(3, owner); it->second.greeted = true;
        }
    }
    void UpdateAI(uint32 diff) override
    {
        using namespace StormheimLifeblood;
        std::lock_guard<std::recursive_mutex> lock(mutex);
        if (owner)
        {
            auto p = ObjectAccessor::GetPlayer(*me, owner); auto it = runs.find(owner);
            if (!p || it == runs.end() || !Current(p, it->second) || it->second.spirit != me->GetGUID() || !Scope(p))
            { me->DespawnOrUnsummon(); return; }
        }
        SmartAI::UpdateAI(diff);
    }
};
class eventobject_stormheim_lifeblood_return : public EventObjectScript
{
public:
    eventobject_stormheim_lifeblood_return() : EventObjectScript("eventobject_stormheim_lifeblood_return") { }
    bool IsTriggerMeets(Player* p, EventObject* e) override
    {
        using namespace StormheimLifeblood;
        // This check executes BEFORE EventObject's template241140 cast. The
        // old source22 condition never gated that cast (no SmartAI rows).
        return Scope(p) && p->IsAlive() && p->GetQuestRewardStatus(QuestId) && !p->IsBeingTeleported() &&
            !p->GetVehicle() && !p->GetTransport() && !p->isInFlight() && !p->isInCombat() &&
            e && e->IsInWorld() && e->GetEntry() == 333 && e->GetDBTableGUIDLow() == 400325 &&
            e->GetMapId() == 1463 && e->GetZoneId() == 7588 && e->GetExactDist(&ReturnGate) < 0.5f &&
            p->InSamePhase(e) && p->GetExactDist(e) <= 2;
    }
};
class player_stormheim_lifeblood : public PlayerScript
{
public:
    player_stormheim_lifeblood() : PlayerScript("player_stormheim_lifeblood") { }
    void OnUpdate(Player* p, uint32 diff) override
    { std::lock_guard<std::recursive_mutex> lock(StormheimLifeblood::mutex); StormheimLifeblood::Update(p, diff); }
    void OnLogout(Player* p) override
    { std::lock_guard<std::recursive_mutex> lock(StormheimLifeblood::mutex); StormheimLifeblood::Clear(p); }
    void OnMapChanged(Player* p) override
    {
        using namespace StormheimLifeblood;
        std::lock_guard<std::recursive_mutex> lock(mutex);
        auto it = runs.find(p->GetGUID());
        if (it == runs.end() || it->second.stage != 4) Clear(p);
    }
    void OnLogin(Player* p) override
    {
        using namespace StormheimLifeblood;
        std::lock_guard<std::recursive_mutex> lock(mutex);
        Clear(p);
        if (!p->HasPhaseId(8306) && (Scope(p) || p->GetQuestStatus(QuestId) == QUEST_STATUS_INCOMPLETE ||
            p->GetQuestStatus(QuestId) == QUEST_STATUS_COMPLETE)) RetireController(p);
        // SaveToDB can save the queued destination before the client's ACK.
        // A disconnect in that window must not strand an incomplete quest in
        // Stormheim. Restart at the altar; a saved position is never credit.
        // Completed/rewarded quests, other locations and later phases stay put.
        if (Accepted(p) && p->IsInWorld() && p->IsAlive() && p->GetMapId() == 1220 && !p->GetInstanceId() &&
            !p->HasPhaseId(8306) && !p->IsBeingTeleported() && !p->GetVehicle() && !p->GetTransport() &&
            !p->isInFlight() && !p->isInCombat() && p->GetExactDist(&Landing) < 5)
            p->TeleportTo(1463, Altar.GetPositionX(), Altar.GetPositionY(), Altar.GetPositionZ(), Altar.GetOrientation());
    }
};
void AddSC_stormheim_lifeblood()
{
    RegisterCreatureAI(npc_stormheim_lifeblood_ashildir);
    new eventobject_stormheim_lifeblood_return(); new player_stormheim_lifeblood();
}
