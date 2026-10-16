#include "ScriptMgr.h"
#include "SmartAI.h"
#include "SpellScript.h"
#include "Player.h"
#include "Item.h"
#include "GameObject.h"
#include "ObjectAccessor.h"
#include "QuestData.h"
#include "MotionMaster.h"
#include <map>
#include <mutex>
#include <list>
#include <set>

namespace StormheimStemTide
{
constexpr uint32 QuestId = 40003, Horn = 129161, Call = 193327;
struct Portal { uint32 credit, dragon; ObjectGuid::LowType marker, object; Position point; };
Portal const Portals[] = {
    {97983,98039,291595,115097,{2713.4f,970.118f,205.88f,0}},
    {97984,98040,291537,115080,{2601.57f,981.316f,216.77f,0}},
    {97985,98041,291523,115070,{2467.32f,1060.44f,216.77f,0}}
};
std::recursive_mutex mutex;
struct Run { ObjectGuid marker, object, dragon; uint32 slot = 0, age = 0; bool awarded = false; std::weak_ptr<uint8> lifetime; std::shared_ptr<uint8> ticket; };
std::map<ObjectGuid,Run> runs;
bool Scope(Player* p)
{
    return p && p->IsInWorld() && p->GetMapId() == 1220 && !p->GetInstanceId() && p->GetZoneId() == 7541 && !p->HasPhaseId(8306);
}
bool Definition()
{
    auto q = sQuestDataStore->GetQuestTemplate(QuestId);
    if (!q || q->StartScript || q->HasSpecialFlag(QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT)) return false;
    uint32 mask = 0;
    for (auto const& o : q->GetObjectives())
    {
        uint32 bit = 0;
        for (uint32 i = 0; i < 3; ++i)
            if (o.Type == QUEST_OBJECTIVE_MONSTER && o.ObjectID == Portals[i].credit && o.StorageIndex == i && o.Amount == 1 && !o.Flags) bit = 1u << i;
        if (o.Type == QUEST_OBJECTIVE_MONSTER && o.ObjectID == 98186 && o.StorageIndex == 3 && o.Amount == 3 && o.Flags == 28) bit = 8;
        if (!bit || (mask & bit)) return false;
        mask |= bit;
    }
    return mask == 15;
}
bool Ready(Player* p)
{
    return Scope(p) && p->IsAlive() && !p->IsBeingTeleported() && !p->GetVehicle() && !p->GetTransport() && !p->isInFlight() &&
        p->GetQuestStatus(QuestId) == QUEST_STATUS_INCOMPLETE && Definition() && p->GetItemCount(Horn, false);
}
int Identity(Creature* c)
{
    if (!c || !c->IsInWorld() || c->GetMapId() != 1220 || c->GetInstanceId() || c->GetZoneId() != 7541 || c->HasPhaseId(8306)) return -1;
    for (uint32 i = 0; i < 3; ++i)
        if (c->GetEntry() == Portals[i].credit && c->GetDBTableGUIDLow() == Portals[i].marker && c->GetExactDist(&Portals[i].point) < 1) return i;
    return -1;
}
int Identity(GameObject* go)
{
    if (!go || !go->IsInWorld() || go->GetEntry() != 244681 || go->GetMapId() != 1220 || go->GetInstanceId() || go->HasPhaseId(8306)) return -1;
    for (uint32 i = 0; i < 3; ++i)
        if (go->GetDBTableGUIDLow() == Portals[i].object && go->GetExactDist(&Portals[i].point) < 5) return i;
    return -1;
}
bool Source(Player* p, Creature* c, float range)
{
    return Ready(p) && Identity(c) >= 0 && c->IsAlive() && !p->GetQuestObjectiveData(QuestId, c->GetEntry()) &&
        p->InSamePhase(c) && p->canSeeOrDetect(c) && p->IsWithinLOSInMap(c) && p->GetExactDist(c) <= range;
}
GameObject* PortalObject(Player* p, uint32 slot)
{
    std::list<GameObject*> objects;
    GetGameObjectListWithEntryInGrid(objects, p, 244681, 100);
    for (auto go : objects) if (Identity(go) == int(slot) && go->isSpawned() && p->InSamePhase(go)) return go;
    return nullptr;
}
void Clear(Player* p)
{
    auto it = runs.find(p->GetGUID());
    if (it == runs.end()) return;
    if (auto c = ObjectAccessor::GetCreature(*p, it->second.dragon)) c->DespawnOrUnsummon();
    runs.erase(it);
}
bool Current(Player* p, Run const& r)
{
    auto s = p->getQuestStatus(QuestId);
    return Scope(p) && p->IsAlive() && !p->IsBeingTeleported() && s && !r.lifetime.expired() && s->ScriptLifetime == r.lifetime.lock() &&
        (r.awarded || Source(p, ObjectAccessor::GetCreature(*p, r.marker), 80));
}
bool Destroyed(Player* p, uint32 slot)
{
    return Scope(p) && Definition() && (p->GetQuestRewardStatus(QuestId) ||
        ((p->GetQuestStatus(QuestId) == QUEST_STATUS_INCOMPLETE || p->GetQuestStatus(QuestId) == QUEST_STATUS_COMPLETE) &&
            p->GetQuestObjectiveData(QuestId, Portals[slot].credit) == 1));
}
void Finish(Player* p, std::weak_ptr<uint8> ticket)
{
    std::lock_guard<std::recursive_mutex> lock(mutex);
    auto it = runs.find(p->GetGUID());
    if (it == runs.end() || ticket.expired() || it->second.ticket != ticket.lock()) return;
    auto& r = it->second;
    auto go = ObjectAccessor::GetGameObject(*p, r.object);
    if (r.awarded || !Current(p, r) || Identity(go) != int(r.slot) || !go->isSpawned() || !p->InSamePhase(go)) { Clear(p); return; }
    // Keep the installed five-second horn response. Optional flight/impact
    // visuals are independent of the personal, quest-lifetime-bound result.
    if (auto impact = p->SummonCreature(Portals[r.slot].credit, Portals[r.slot].point, TEMPSUMMON_TIMED_DESPAWN, 3000, 0, p->GetGUID()))
    {
        p->BindQuestObject(QuestId, impact);
        impact->AddPlayerInPersonnalVisibilityList(p->GetGUID());
        impact->CastSpell(impact, 196084, true); // actual native50884/60744 impact
    }
    auto q = sQuestDataStore->GetQuestTemplate(QuestId);
    r.awarded = true;
    for (auto const& o : q->GetObjectives()) if (o.ObjectID == Portals[r.slot].credit)
    {
        p->SetQuestObjectiveData(q, &o, 1);
        p->SendQuestUpdateAddCredit(q, r.marker, o, 1);
    }
    uint32 count = 0;
    for (auto const& portal : Portals) if (p->GetQuestObjectiveData(QuestId, portal.credit)) ++count;
    for (auto const& o : q->GetObjectives()) if (o.ObjectID == 98186)
    {
        p->SetQuestObjectiveData(q, &o, count);
        p->SendQuestUpdateAddCredit(q, r.marker, o, count);
    }
    if (p->CanCompleteQuest(QuestId)) p->CompleteQuest(QuestId);
    if (go->AI()) go->AI()->SetGUID(p->GetGUID(), Call);
    if (auto dragon = ObjectAccessor::GetCreature(*p, r.dragon))
    {
        auto const& point = Portals[r.slot].point;
        dragon->GetMotionMaster()->MoveSmoothFlyPath(2, Position(point.GetPositionX()+65, point.GetPositionY()+20, point.GetPositionZ()+45, 0), 20);
    }
}
void Begin(Player* p, Creature* c, std::weak_ptr<uint8> lifetime)
{
    if (!Source(p, c, 20) || lifetime.expired() || p->getQuestStatus(QuestId)->ScriptLifetime != lifetime.lock() || runs.count(p->GetGUID())) return;
    uint32 slot = uint32(Identity(c));
    auto go = PortalObject(p, slot);
    if (!go) return;
    Run r; r.slot = slot; r.marker = c->GetGUID(); r.object = go->GetGUID(); r.lifetime = lifetime; r.ticket = std::make_shared<uint8>();
    runs.emplace(p->GetGUID(), r);
    // Authored optional flyby using the native matching Vethir templates/model.
    // These are wild personal summons, never a guardian, vehicle or player mover.
    auto const& point = Portals[slot].point;
    if (auto dragon = p->SummonCreature(Portals[slot].dragon, Position(point.GetPositionX()-45, point.GetPositionY()-20, point.GetPositionZ()+35, 0), TEMPSUMMON_TIMED_DESPAWN, 10000, 0, p->GetGUID()))
    {
        p->BindQuestObject(QuestId, dragon);
        dragon->AddPlayerInPersonnalVisibilityList(p->GetGUID());
        dragon->SetReactState(REACT_PASSIVE);
        dragon->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_SELECTABLE);
        dragon->SetDisableGravity(true);
        dragon->SetCanFly(true);
        dragon->GetMotionMaster()->MoveSmoothFlyPath(1, Position(point.GetPositionX(), point.GetPositionY(), point.GetPositionZ()+25, 0), 12);
        runs[p->GetGUID()].dragon = dragon->GetGUID();
    }
    std::weak_ptr<uint8> ticket = r.ticket;
    p->AddQuestDelayedEvent(QuestId, 5000, [p,ticket] { Finish(p,ticket); });
}
}

struct npc_stormheim_stem_marker : SmartAI
{
    explicit npc_stormheim_stem_marker(Creature* c) : SmartAI(c) { }
    void SpellHit(Unit* unit, SpellInfo const* spell) override
    {
        // Replace the shared timed action list only for the three original
        // markers. The spell handler owns the exact player/acceptance ticket.
        if (StormheimStemTide::Identity(me) >= 0 && spell && spell->Id == StormheimStemTide::Call) return;
        SmartAI::SpellHit(unit, spell);
    }
};
struct go_stormheim_stem_portal : SmartGameObjectAI
{
    explicit go_stormheim_stem_portal(GameObject* g) : SmartGameObjectAI(g) { }
    std::set<ObjectGuid> hidden;
    uint32 timer = 0;
    void SetGUID(ObjectGuid const& guid, int32 id) override
    {
        auto slot = StormheimStemTide::Identity(go);
        auto p = ObjectAccessor::GetPlayer(*go, guid);
        if (id != int32(StormheimStemTide::Call) || slot < 0 || !StormheimStemTide::Destroyed(p, uint32(slot))) return;
        if (hidden.insert(guid).second) { go->AddToHideList(guid); p->UpdateVisibilityOf(go); }
    }
    void UpdateAI(uint32 diff) override
    {
        SmartGameObjectAI::UpdateAI(diff);
        auto slot = StormheimStemTide::Identity(go);
        if (slot < 0) return;
        if (timer > diff) { timer -= diff; return; } timer = 500;
        for (auto it = hidden.begin(); it != hidden.end();)
        {
            auto p = ObjectAccessor::GetPlayer(*go, *it);
            if (StormheimStemTide::Destroyed(p, uint32(slot))) { ++it; continue; }
            go->RemoveFromHideList(*it); if (p) p->UpdateVisibilityOf(go); it = hidden.erase(it);
        }
        std::list<Player*> nearby; go->GetPlayerListInGrid(nearby, 100);
        for (auto p : nearby) SetGUID(p->GetGUID(), StormheimStemTide::Call);
    }
};
class spell_stormheim_stem_horn : public SpellScript
{
    PrepareSpellScript(spell_stormheim_stem_horn);
    bool checked = false;
    ObjectGuid source;
    std::weak_ptr<uint8> lifetime;
    bool Capture()
    {
        using namespace StormheimStemTide;
        auto p = GetCaster() ? GetCaster()->ToPlayer() : nullptr;
        auto item = GetCastItem();
        if (!p || !item || item->GetEntry() != Horn || item->GetOwnerGUID() != p->GetGUID() || !Ready(p)) return false;
        auto it = runs.find(p->GetGUID());
        if (it != runs.end() && !Current(p, it->second)) Clear(p);
        if (runs.count(p->GetGUID())) return false;
        if (checked) return !lifetime.expired() && p->getQuestStatus(QuestId)->ScriptLifetime == lifetime.lock() && Source(p, ObjectAccessor::GetCreature(*p, source), 20);
        checked = true;
        Creature* selected = nullptr; float distance = 20.01f;
        for (auto const& portal : Portals)
        {
            std::list<Creature*> nearby;
            GetCreatureListWithEntryInGrid(nearby, p, portal.credit, 20);
            for (auto c : nearby)
                if (Source(p, c, 20) && PortalObject(p, uint32(Identity(c))) && p->GetExactDist(c) < distance) { selected = c; distance = p->GetExactDist(c); }
        }
        if (!selected) return false;
        p->AddQuestDelayedEvent(QuestId, 0, [] {});
        source = selected->GetGUID(); lifetime = p->getQuestStatus(QuestId)->ScriptLifetime;
        return true;
    }
    SpellCastResult Check() { std::lock_guard<std::recursive_mutex> lock(StormheimStemTide::mutex); return Capture() ? SPELL_CAST_OK : SPELL_FAILED_BAD_TARGETS; }
    void Select(WorldObject*& target)
    {
        std::lock_guard<std::recursive_mutex> lock(StormheimStemTide::mutex);
        target = Capture() ? ObjectAccessor::GetCreature(*GetCaster(), source) : nullptr;
    }
    void Hit(SpellEffIndex)
    {
        std::lock_guard<std::recursive_mutex> lock(StormheimStemTide::mutex);
        if (!Capture() || !GetHitUnit() || GetHitUnit()->GetGUID() != source) return;
        StormheimStemTide::Begin(GetCaster()->ToPlayer(), GetHitUnit()->ToCreature(), lifetime);
    }
    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_stormheim_stem_horn::Check);
        OnObjectTargetSelect += SpellObjectTargetSelectFn(spell_stormheim_stem_horn::Select, EFFECT_0, TARGET_UNIT_NEARBY_ENTRY);
        OnEffectHitTarget += SpellEffectFn(spell_stormheim_stem_horn::Hit, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};
class player_stormheim_stem_tide : public PlayerScript
{
public:
    player_stormheim_stem_tide() : PlayerScript("player_stormheim_stem_tide") { }
    void OnUpdate(Player* p, uint32 diff) override
    {
        std::lock_guard<std::recursive_mutex> lock(StormheimStemTide::mutex);
        auto it = StormheimStemTide::runs.find(p->GetGUID());
        if (it == StormheimStemTide::runs.end()) return;
        it->second.age += diff;
        if (!StormheimStemTide::Current(p, it->second) || it->second.age >= 10000) StormheimStemTide::Clear(p);
    }
    void OnLogout(Player* p) override { std::lock_guard<std::recursive_mutex> lock(StormheimStemTide::mutex); StormheimStemTide::Clear(p); }
    void OnMapChanged(Player* p) override { OnLogout(p); }
};
void AddSC_stormheim_stem_tide()
{
    RegisterCreatureAI(npc_stormheim_stem_marker);
    RegisterGameObjectAI(go_stormheim_stem_portal);
    RegisterSpellScript(spell_stormheim_stem_horn);
    new player_stormheim_stem_tide();
}
