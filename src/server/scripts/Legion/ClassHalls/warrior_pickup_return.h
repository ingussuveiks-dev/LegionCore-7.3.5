#pragma once

#include "AreaTrigger.h"
#include "QuestData.h"
#include "DB2Stores.h"
#include <atomic>
#include <map>
#include <mutex>

namespace WarriorPickupReturn
{
    uint32 const ReadyMarker = 0x575250;
    struct Mode { uint32 map, scenario, quest, item, child, slot, childSlot, scene, sceneSpell, equip, acquisition, conversation; };
    Mode const fury{1511,944,40043,128908,134553,15,16,1218,205443,194201,98033,849};
    Mode const protection{1495,909,39191,128289,128288,16,15,1215,205340,208562,94851,0};

    Mode const* Context(Player* player)
    {
        if (!player || !player->IsInWorld() || !player->GetMap() || player->GetMap()->IsMapUnload() ||
            player->getClass() != CLASS_WARRIOR || player->isGameMaster())
            return nullptr;
        Mode const* mode = player->GetMapId() == fury.map ? &fury : player->GetMapId() == protection.map ? &protection : nullptr;
        if (!mode || player->GetQuestRewardStatus(mode->quest) ||
            (player->GetQuestStatus(mode->quest) != QUEST_STATUS_INCOMPLETE && player->GetQuestStatus(mode->quest) != QUEST_STATUS_COMPLETE))
            return nullptr;
        Scenario* scenario = sScenarioMgr->GetScenario(player->GetInstanceId());
        return scenario && scenario->GetScenarioId() == mode->scenario ? mode : nullptr;
    }

    bool Earned(Player* player, Mode const& mode)
    {
        Item* owned = player ? player->GetItemByEntry(mode.item) : nullptr;
        if (Context(player) != &mode || !owned || !owned->GetTemplate())
            return false;
        Quest const* quest = sQuestDataStore->GetQuestTemplate(mode.quest);
        if (!quest)
            return false;
        bool acquisition = false, arrival = false;
        for (QuestObjective const& objective : quest->GetObjectives())
        {
            if (objective.ObjectID == 103739) { arrival = true; continue; }
            if (objective.Flags & QUEST_OBJECTIVE_FLAG_OPTIONAL)
                continue;
            if (uint32(objective.ObjectID) == mode.acquisition)
                acquisition = true;
            if (!player->HasQuestObjectiveComplete(quest, objective))
                return false;
        }
        return acquisition && arrival;
    }

    bool Equipped(Player* player, Mode const& mode)
    {
        Item* parent = player->GetItemByPos(INVENTORY_SLOT_BAG_0, mode.slot);
        Item* child = player->GetItemByPos(INVENTORY_SLOT_BAG_0, mode.childSlot);
        return parent && child && parent->GetEntry() == mode.item && child->GetEntry() == mode.child && parent->GetChildItem() == child->GetGUID();
    }

    bool StoredOffSpec(Player* player, Mode const& mode)
    {
        Item* parent = player->GetItemByEntry(mode.item);
        ItemTemplate const* proto = parent ? parent->GetTemplate() : nullptr;
        auto spec = sChrSpecializationStore.LookupEntry(player->GetSpecializationId());
        if (!proto || !spec || spec->ClassID != CLASS_WARRIOR || !proto->GetArtifactID() ||
            player->CanUseItem(proto) != EQUIP_ERR_CANT_USE_ITEM)
            return false;
        // Native CanUseItem returns this result only for an artifact/spec
        // mismatch. Keep that restriction and the owned pair in storage;
        // acquiring another spec's artifact must not require changing spec.
        Item* child = player->GetChildItemByGuid(parent->GetChildItem());
        return child && child->GetEntry() == mode.child && child->GetOwnerGUID() == player->GetGUID();
    }

    Player* RawOwner(Creature* actor)
    {
        TempSummon* summon = actor && actor->GetEntry() == 93819 ? actor->ToTempSummon() : nullptr;
        Unit* owner = summon ? summon->GetSummoner() : nullptr;
        return owner ? owner->ToPlayer() : nullptr;
    }

    Player* Owner(Creature* actor)
    {
        Player* player = RawOwner(actor);
        auto scope = actor ? actor->GetQuestObjectScope().lock() : nullptr;
        Mode const* mode = Context(player);
        return scope && !scope->Cancelled && player && mode && scope->Owner == player->GetGUID() &&
            actor->GetMap() == player->GetMap() && actor->InSamePhase(player) &&
            scope->CancellationReceipt() == player->GetQuestCancellationReceipt(mode->quest) && Earned(player, *mode) ? player : nullptr;
    }

    struct ValkyrAI : ScriptedAI
    {
        explicit ValkyrAI(Creature* actor) : ScriptedAI(actor) { }
        ObjectGuid area;
        uint32 retry = 0;
        bool warned = false;
        void ClearArea()
        {
            if (AreaTrigger* trigger = me->GetMap()->GetAreaTrigger(area))
                if (trigger->GetCasterGUID() == me->GetGUID())
                    trigger->Remove();
            area.Clear();
        }
        void Reset() override
        {
            ClearArea(); retry = 0;
            me->SetReactState(REACT_PASSIVE);
            me->RemoveFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
        }
        void JustDied(Unit*) override { ClearArea(); }
        void UpdateAI(uint32 diff) override
        {
            Player* player = Owner(me);
            if (!player)
            {
                ClearArea(); me->DespawnOrUnsummon(); return;
            }
            if (me->GetCustomData() != ReadyMarker || !player->CanContact() || !player->IsAlive() || player->IsBeingTeleported())
            {
                ClearArea(); return;
            }
            if (retry > diff) { retry -= diff; return; }
            retry = 1000;
            if (AreaTrigger* trigger = me->GetMap()->GetAreaTrigger(area))
                if (trigger->IsInWorld() && trigger->GetCasterGUID() == me->GetGUID())
                {
                    if (trigger->IsUnitAffected(player->GetGUID()) && trigger->IsInArea(player) && !player->HasAura(215997))
                        trigger->AffectUnit(player, AT_ACTION_MOMENT_ENTER);
                    return;
                }
            Position pos = me->GetPosition();
            AreaTrigger* trigger = new AreaTrigger;
            if (!trigger->CreateAreaTrigger(sObjectMgr->GetGenerator<HighGuid::AreaTrigger>()->Generate(),
                0, me, nullptr, pos, pos, nullptr, ObjectGuid::Empty, 11869))
            {
                delete trigger;
                if (!warned) { TC_LOG_WARN("scripts.ai", "Warrior pickup return: custom11869 creation failed; retrying without transfer or credit"); warned = true; }
                return;
            }
            if (!trigger->InheritQuestObjectScope(me) || Owner(me) != player)
            {
                trigger->Remove();
                return;
            }
            trigger->SetDuration(-1);
            area = trigger->GetGUID();
            warned = false;
        }
    };

    CreatureAI* CreateAI(Creature* actor)
    {
        if (actor->HasQuestObjectScope() && (actor->GetMapId() == 1511 || actor->GetMapId() == 1495) && RawOwner(actor))
            return new ValkyrAI(actor);
        return nullptr;
    }

    struct Publication { std::atomic<bool> finished{false}; };
    struct PickupConversation : Conversation
    {
        std::weak_ptr<Publication> publication;
        ObjectGuid owner;
        void Update(uint32 diff) override
        {
            // Observe the native per-viewer publication clock. Creation or a
            // guessed pickup timer is not evidence the full clip was admitted.
            auto viewer = playing.find(owner);
            uint32 duration = GetUInt32Value(CONVERSATION_FIELD_LAST_LINE_END_TIME);
            if (IsInWorld() && duration && viewer != playing.end() && getMSTime() - viewer->second >= duration)
                if (auto receipt = publication.lock())
                    receipt->finished.store(true, std::memory_order_release);
            Conversation::Update(diff);
        }
    };

    struct State
    {
        Mode const* mode;
        uint32 instance, retry = 0, sceneTimeout = 60000;
        bool sceneStarted = false, equipAck = false, storageAck = false;
        std::atomic<bool> sceneAck{false};
        bool warnedScene = false, warnedConversation = false, warnedSpawn = false;
        std::shared_ptr<QuestCancellationReceipt const> attempt;
        ObjectGuid conversation, valkyr;
        std::shared_ptr<Publication> publication;
        State(Player* player, Mode const* value) : mode(value), instance(player->GetInstanceId()), attempt(player->GetQuestCancellationReceipt(value->quest)) { }
        bool Valid(Player* player) const
        {
            return Context(player) == mode && player->GetInstanceId() == instance && attempt && !attempt->load() &&
                attempt == player->GetQuestCancellationReceipt(mode->quest);
        }

        void Clear(Player* player)
        {
            if (Conversation* current = ObjectAccessor::GetConversation(*player, conversation))
                if (current->GetCasterGUID() == player->GetGUID())
                    current->Remove();
            if (Creature* actor = ObjectAccessor::GetCreature(*player, valkyr))
                if (RawOwner(actor) == player)
                {
                    if (auto* ai = dynamic_cast<ValkyrAI*>(actor->AI()))
                        ai->ClearArea();
                    actor->DespawnOrUnsummon();
                }
            if (!valkyr.IsEmpty())
                player->RemoveAurasDueToSpell(215997);
        }

        void Tick(Player* player, uint32 diff)
        {
            if (!Valid(player))
                return;
            if (retry > diff) { retry -= diff; return; }
            uint32 elapsed = diff + (1000 - std::min(uint32(1000), retry));
            retry = 1000;
            if (!Earned(player, *mode) || !player->CanContact() || !player->IsAlive() || player->IsBeingTeleported())
                return;
            if (mode->conversation && (!publication || !publication->finished.load(std::memory_order_acquire)))
            {
                if (!ObjectAccessor::GetConversation(*player, conversation))
                {
                    auto clip = new PickupConversation;
                    publication = std::make_shared<Publication>();
                    clip->publication = publication; clip->owner = player->GetGUID();
                    if (!clip->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(), mode->conversation, player, nullptr, *player))
                    {
                        delete clip;
                        if (!warnedConversation) { TC_LOG_WARN("scripts.ai", "Warrior pickup return: native conversation creation failed; retrying without transfer"); warnedConversation = true; }
                        return;
                    }
                    if (!Valid(player) || !Earned(player, *mode))
                    {
                        clip->Remove();
                        return;
                    }
                    player->BindQuestObject(mode->quest, clip, false);
                    conversation = clip->GetGUID();
                    warnedConversation = false;
                }
            }
            if (!sceneAck)
            {
                // The real aura430 launch plus native non-complete status is
                // the observation receipt. The scene bridge acknowledges even
                // a fast completion before this periodic recovery tick.
                if (!sceneStarted && player->HasAura(mode->sceneSpell) &&
                    player->HasSceneStatus(mode->scene, SCENE_LAUNCH) && !player->HasSceneStatus(mode->scene, SCENE_COMPLETE))
                    sceneStarted = true;
                if (sceneStarted && player->HasSceneStatus(mode->scene, SCENE_COMPLETE))
                    sceneAck = true;
                else
                {
                    sceneTimeout -= std::min(sceneTimeout, elapsed);
                    if (!sceneStarted || !sceneTimeout || !player->HasAura(mode->sceneSpell))
                    {
                        if (!sceneTimeout)
                            player->RemoveAurasDueToSpell(mode->sceneSpell);
                        SpellCastResult result = player->CastSpell(player, mode->sceneSpell, true);
                        sceneStarted = player->HasAura(mode->sceneSpell) && player->HasSceneStatus(mode->scene, SCENE_LAUNCH) &&
                            !player->HasSceneStatus(mode->scene, SCENE_COMPLETE);
                        sceneTimeout = 60000;
                        if (result != SPELL_CAST_OK && !warnedScene) { TC_LOG_WARN("scripts.ai", "Warrior pickup return: native scene cast failed; retrying without completion"); warnedScene = true; }
                        if (result == SPELL_CAST_OK)
                            warnedScene = false;
                    }
                    return;
                }
            }

            if (!equipAck && !storageAck)
            {
                storageAck = StoredOffSpec(player, *mode);
                if (!storageAck && !Equipped(player, *mode))
                    player->CastSpell(player, mode->equip, true);
                equipAck = Equipped(player, *mode);
                if (!equipAck && !storageAck)
                {
                    retry = 5000;
                    return; // Native bag/equip errors remain actionable; never grant replacements.
                }
            }
            if (mode->conversation && (!publication || !publication->finished.load(std::memory_order_acquire)))
                return;
            if (!Valid(player) || !Earned(player, *mode))
                return;
            if (Creature* current = ObjectAccessor::GetCreature(*player, valkyr))
                if (current->IsInWorld() && current->IsAlive() && Owner(current) == player)
                {
                    current->SetCustomData(ReadyMarker);
                    return;
                }
            // Conservative collision-supported recovery pose beside the real
            // owner; exact retail exit coordinates are not established here.
            Position pos = player->GetPosition();
            player->MovePositionToFirstCollision(pos, 2.0f, 0.0f);
            if (Creature* actor = player->SummonCreature(93819, pos, TEMPSUMMON_MANUAL_DESPAWN, 0))
            {
                if (Owner(actor) == player)
                {
                    actor->SetCustomData(ReadyMarker);
                    valkyr = actor->GetGUID();
                    warnedSpawn = false;
                    return;
                }
                actor->DespawnOrUnsummon();
            }
            if (!warnedSpawn) { TC_LOG_WARN("scripts.ai", "Warrior pickup return: owned Val'kyr unavailable; retrying without transfer or credit"); warnedSpawn = true; }
        }
    };
    std::mutex statesMutex;
    std::map<ObjectGuid, std::shared_ptr<State>> states;
    std::shared_ptr<State> Current(Player* player, Mode const* mode)
    {
        std::lock_guard<std::mutex> guard(statesMutex);
        auto& state = states[player->GetGUID()];
        if (!state || state->mode != mode || state->instance != player->GetInstanceId() || !state->attempt || state->attempt->load() ||
            state->attempt != player->GetQuestCancellationReceipt(mode->quest))
            state = std::make_shared<State>(player, mode);
        return state;
    }
    bool Complete(Player* player, unsigned int scene)
    {
        Mode const* mode = Context(player);
        if (mode && scene == mode->scene && player->HasSceneStatus(scene, SCENE_COMPLETE) && Earned(player, *mode))
        {
            auto state = Current(player, mode);
            state->sceneAck = true;
            bool stored = StoredOffSpec(player, *mode);
            if (stored)
                state->storageAck = true;
            return stored;
        }
        return false;
    }
}

class player_warrior_pickup_return : public PlayerScript
{
public:
    player_warrior_pickup_return() : PlayerScript("player_warrior_pickup_return") { }
    void Forget(Player* player)
    {
        std::shared_ptr<WarriorPickupReturn::State> old;
        { std::lock_guard<std::mutex> guard(WarriorPickupReturn::statesMutex); auto& states = WarriorPickupReturn::states; auto found = states.find(player->GetGUID()); if (found != states.end()) { old = found->second; states.erase(found); } }
        if (old) old->Clear(player);
    }
    void OnBeforeLogout(Player* player) override { Forget(player); }
    void OnLogout(Player* player) override { Forget(player); }
    void OnMapChanged(Player* player) override { Forget(player); }
    void OnUpdate(Player* player, uint32 diff) override
    {
        auto mode = WarriorPickupReturn::Context(player);
        if (!mode) { Forget(player); return; }
        auto state = WarriorPickupReturn::Current(player, mode);
        state->Tick(player, diff);
    }
};

class condition_warrior_pickup_return : public ConditionScript
{
public:
    condition_warrior_pickup_return() : ConditionScript("condition_warrior_pickup_return") { }
    bool OnConditionCheck(Condition* condition, ConditionSourceInfo& source) override
    {
        if (!condition || condition->SourceType != CONDITION_SOURCE_TYPE_AREATRIGGER_ACTION || condition->SourceGroup != 11869)
            return true;
        WorldObject* base = source.mConditionTargets[0];
        if (!base || (base->GetMapId() != 1511 && base->GetMapId() != 1495))
            return true; // Retain ordinary Dalaran and Stromkar gaze behavior.
        Creature* actor = base->ToCreature();
        if (!actor)
            return false;
        Player* player = source.mConditionTargets[1] ? source.mConditionTargets[1]->ToPlayer() : nullptr;
        if (condition->SourceEntry == 1)
            return player && WarriorPickupReturn::RawOwner(actor) == player;
        return player && actor->GetCustomData() == WarriorPickupReturn::ReadyMarker && WarriorPickupReturn::Owner(actor) == player && player->CanContact() && player->IsAlive() && !player->IsBeingTeleported();
    }
};
