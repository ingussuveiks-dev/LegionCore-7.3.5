#include "ScriptMgr.h"
#include "SmartAI.h"
#include "Player.h"
#include "GameObject.h"
#include "ObjectAccessor.h"
#include "SpellScript.h"
#include "Vehicle.h"
#include <map>
#include <memory>

struct npc_stormheim_havi_actor;
namespace StormheimTrials
{
constexpr uint32 QuestId = 38331, ReadyKey = 38331, ConsumedKey = 38332;
constexpr int32 OwnerKey = 1;
struct Ingredient { uint32 object, summonSpell, vehicle, impact, credit; };
Ingredient const Ingredients[] = {
    {252074,219922,110605,219924,110610},
    {252076,219933,110608,219936,110611},
    {252080,219945,110609,219947,110612}
};
Ingredient const* Kind(uint32 entry)
{
    for (auto const& kind : Ingredients)
        if (kind.object == entry || kind.vehicle == entry || kind.impact == entry) return &kind;
    return nullptr;
}
bool Scope(WorldObject* object)
{
    return object && object->IsInWorld() && object->GetMapId() == 1220 &&
        object->GetZoneId() == 7541 && object->GetAreaId() == 7808;
}
bool Active(Player* player)
{
    return Scope(player) && player->IsAlive() && !player->IsBeingTeleported() &&
        (player->GetTeam() == ALLIANCE || player->GetTeam() == HORDE) &&
        player->GetQuestStatus(QuestId) == QUEST_STATUS_INCOMPLETE;
}
bool Interact(Player* player, GameObject* object)
{
    return Active(player) && Scope(object) && object->isSpawned() &&
        !object->HasFlag(GAMEOBJECT_FIELD_FLAGS,GO_FLAG_IN_USE | GO_FLAG_NOT_SELECTABLE) &&
        !player->GetVehicle() && !player->GetTransport() && !player->isInCombat() &&
        player->GetExactDist(object) <= 6.0f && player->InSamePhase(object) &&
        player->canSeeOrDetect(object) && player->IsWithinLOSInMap(object);
}
struct Pending { ObjectGuid object; uint32 entry; std::weak_ptr<uint8> lifetime; };
std::map<ObjectGuid, Pending> pending;
std::map<ObjectGuid, ObjectGuid> vehicles;
// Controllable seats replace the current AI with PossessedAI. The original
// script stays alive in i_disabledAI, but must not be found through AI().
std::map<ObjectGuid, npc_stormheim_havi_actor*> proofs;
}

struct npc_stormheim_havi_actor : SmartAI
{
    explicit npc_stormheim_havi_actor(Creature* creature) : SmartAI(creature) { }
    ~npc_stormheim_havi_actor() override
    {
        auto proof = StormheimTrials::proofs.find(me->GetGUID());
        if (proof != StormheimTrials::proofs.end() && proof->second == this)
            StormheimTrials::proofs.erase(proof);
        auto found = StormheimTrials::vehicles.find(owner);
        if (found != StormheimTrials::vehicles.end() && found->second == me->GetGUID())
            StormheimTrials::vehicles.erase(found);
    }
    ObjectGuid owner;
    std::weak_ptr<uint8> lifetime;
    bool consumed = false, hasBoarded = false, stopped = false;
    uint32 age = 0;
    Player* Owner() const
    {
        using namespace StormheimTrials;
        Player* player = ObjectAccessor::GetPlayer(*me,owner);
        auto token = lifetime.lock();
        if (stopped || !Active(player) || !token || !player->getQuestStatus(QuestId) ||
            player->getQuestStatus(QuestId)->ScriptLifetime != token || !me->IsAlive() ||
            !Scope(me) || !player->InSamePhase(me) || player->GetExactDist(me) > 8.0f) return nullptr;
        return player;
    }
    bool Boarded() const
    {
        Player* player = Owner();
        Vehicle* kit = me->GetVehicleKit();
        return player && kit && player->GetVehicleBase() == me && kit->GetPassenger(0) == player;
    }
    ObjectGuid GetGUID(int32 id) override { return id == StormheimTrials::OwnerKey && !owner.IsEmpty() ? owner : SmartAI::GetGUID(id); }
    uint32 GetData(uint32 id) const override
    {
        if (!owner.IsEmpty() && id == StormheimTrials::ReadyKey) return Boarded();
        if (!owner.IsEmpty() && id == StormheimTrials::ConsumedKey) return consumed;
        return SmartAI::GetData(id);
    }
    void SetData(uint32 id, uint32 value) override
    {
        if (!owner.IsEmpty() && id == StormheimTrials::ConsumedKey) consumed = value != 0;
        else SmartAI::SetData(id,value);
    }
    void IsSummonedBy(Unit* summoner) override
    {
        using namespace StormheimTrials;
        Player* player = summoner ? summoner->ToPlayer() : nullptr;
        auto kind = Kind(me->GetEntry());
        auto found = player ? pending.find(player->GetGUID()) : pending.end();
        if (Scope(me) && Active(player) && kind && kind->vehicle == me->GetEntry() && found != pending.end())
        {
            auto token = found->second.lifetime.lock();
            GameObject* object = ObjectAccessor::GetGameObject(*me,found->second.object);
            if (token && token == player->getQuestStatus(QuestId)->ScriptLifetime &&
                found->second.entry == kind->object && Interact(player,object))
            {
                owner = player->GetGUID(); lifetime = token;
                vehicles[owner] = me->GetGUID(); pending.erase(found);
                proofs[me->GetGUID()] = this;
                me->AddPlayerInPersonnalVisibilityList(owner);
                // Native timed list boards the owner through 52391 after two
                // seconds. This summoned invisible seat needs no public click.
                me->RemoveFlag(UNIT_FIELD_NPC_FLAGS,UNIT_NPC_FLAG_SPELLCLICK);
                me->ForceValuesUpdateAtIndex(UNIT_FIELD_NPC_FLAGS);
                me->setActive(true);
                Monitor();
            }
        }
        SmartAI::IsSummonedBy(summoner);
    }
    void Stop()
    {
        if (stopped) return;
        stopped = true;
        Player* player = ObjectAccessor::GetPlayer(*me,owner);
        if (player && player->GetVehicleBase() == me) player->ExitVehicle();
        auto found = StormheimTrials::vehicles.find(owner);
        if (found != StormheimTrials::vehicles.end() && found->second == me->GetGUID())
            StormheimTrials::vehicles.erase(found);
        me->DespawnOrUnsummon();
    }
    void UpdateAI(uint32 diff) override
    {
        if (!owner.IsEmpty() && !CheckSession()) return;
        SmartAI::UpdateAI(diff);
    }
    bool CheckSession()
    {
        if (stopped) return false;
        if (Boarded()) hasBoarded = true;
        if (!Owner() || (hasBoarded && !Boarded()) || (!hasBoarded && age >= 10000))
        { Stop(); return false; }
        return true;
    }
    void Monitor()
    {
        // Unit-owned events continue while PossessedAI is active and are
        // cancelled with the creature. Never rerun the native boarding list.
        me->AddDelayedEvent(500,[this]
        {
            age += 500;
            if (CheckSession()) Monitor();
        });
    }
    void PassengerBoarded(Unit* passenger, int8 seat, bool apply) override
    {
        if (apply && !owner.IsEmpty() && seat == 0 && passenger->GetGUID() == owner) hasBoarded = true;
        SmartAI::PassengerBoarded(passenger,seat,apply);
    }
    void SpellHit(Unit* caster, SpellInfo const* spell) override;
};

namespace StormheimTrials
{
npc_stormheim_havi_actor* Proof(Creature* creature)
{
    auto found = creature ? proofs.find(creature->GetGUID()) : proofs.end();
    return found == proofs.end() ? nullptr : found->second;
}
Creature* Source(Unit* caster)
{
    if (!caster) return nullptr;
    Creature* creature = caster->ToCreature();
    if (!Scope(creature)) return nullptr;
    auto kind = Kind(creature->GetEntry());
    if (!kind || kind->vehicle != creature->GetEntry()) return nullptr;
    auto ai = Proof(creature);
    return ai && !ai->owner.IsEmpty() && ai->Boarded() ? creature : nullptr;
}
}

void npc_stormheim_havi_actor::SpellHit(Unit* caster, SpellInfo const* spell)
{
    using namespace StormheimTrials;
    auto kind = Kind(spell->Id);
    if (!Scope(me) || me->GetEntry() != 110604 || !kind || kind->impact != spell->Id)
    { SmartAI::SpellHit(caster,spell); return; }
    Creature* source = Source(caster);
    auto proof = Proof(source);
    if (!source || !proof || source->GetEntry() != kind->vehicle || !source->InSamePhase(me) || proof->consumed) return;
    proof->consumed = true;
    // Preserve native passenger/group credit and independent stew splash.
    // Progress comes from this real pot hit, never from grabbing or boarding.
    SmartAI::SpellHit(source,spell);
}

class spell_stormheim_havi_impact : public SpellScript
{
    PrepareSpellScript(spell_stormheim_havi_impact);
    void Targets(std::list<WorldObject*>& targets)
    {
        using namespace StormheimTrials;
        if (!Scope(GetCaster())) return;
        Creature* source = Source(GetOriginalCaster());
        if (!source) source = Source(GetCaster());
        auto kind = Kind(m_scriptSpellId);
        if (!source || !kind || source->GetEntry() != kind->vehicle) { targets.clear(); return; }
        targets.remove_if([source](WorldObject* target)
        {
            Creature* pot = target ? target->ToCreature() : nullptr;
            return !StormheimTrials::Scope(pot) || !pot->IsAlive() || pot->GetEntry() != 110604 || !source->InSamePhase(pot);
        });
    }
    void Register() override
    {
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_stormheim_havi_impact::Targets,EFFECT_0,TARGET_UNIT_DEST_AREA_ENTRY);
    }
};

class go_stormheim_havi_ingredient : public GameObjectScript
{
public:
    go_stormheim_havi_ingredient() : GameObjectScript("go_stormheim_havi_ingredient") { }
    static bool Use(Player* player, GameObject* object)
    {
        using namespace StormheimTrials;
        if (!Scope(object)) return false;
        auto kind = Kind(object->GetEntry());
        if (!kind || kind->object != object->GetEntry() || !Interact(player,object) ||
            player->GetQuestObjectiveData(QuestId,kind->credit) > 0) return true;
        auto found = vehicles.find(player->GetGUID());
        if (found != vehicles.end())
            if (Creature* previous = ObjectAccessor::GetCreature(*player,found->second))
            {
                auto ai = Proof(previous);
                if (!ai || (ai->Owner() && (!ai->hasBoarded || ai->Boarded()))) return true;
                ai->Stop();
            }
        player->AddQuestDelayedEvent(QuestId,0,[] { });
        pending[player->GetGUID()] = {object->GetGUID(),object->GetEntry(),player->getQuestStatus(QuestId)->ScriptLifetime};
        player->CastSpell(player,kind->summonSpell,true);
        pending.erase(player->GetGUID());
        return true; // Suppress both SmartAI and the duplicate native GO cast.
    }
    bool OnGossipHello(Player* player, GameObject* object) override { return Use(player,object); }
    struct IngredientAI : SmartGameObjectAI
    {
        explicit IngredientAI(GameObject* object) : SmartGameObjectAI(object), object(object) { }
        GameObject* object;
        bool GossipHello(Player* player, bool isUse) override
        {
            // ReportUse goes directly to GO AI, bypassing GameObjectScript.
            if (StormheimTrials::Scope(object)) return Use(player,object);
            return SmartGameObjectAI::GossipHello(player,isUse);
        }
        bool GossipUse(Player* player) override
        {
            if (StormheimTrials::Scope(object)) return Use(player,object);
            return SmartGameObjectAI::GossipUse(player);
        }
    };
    GameObjectAI* GetAI(GameObject* object) const override { return new IngredientAI(object); }
};

void AddSC_stormheim_trials()
{
    RegisterCreatureAI(npc_stormheim_havi_actor);
    RegisterSpellScript(spell_stormheim_havi_impact);
    new go_stormheim_havi_ingredient();
}
