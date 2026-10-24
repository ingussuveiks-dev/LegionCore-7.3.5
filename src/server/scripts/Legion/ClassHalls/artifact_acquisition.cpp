#include "ScriptMgr.h"
#include "Player.h"
#include "Item.h"
#include "GameObject.h"
#include "Scenario.h"
#include "ScenarioMgr.h"
#include "SpellScript.h"
#include "Spell.h"
#include "SpellAuraEffects.h"
#include "Creature.h"
#include "TemporarySummon.h"
#include "CreatureAI.h"
#include "ConditionMgr.h"
#include "CreatureTextMgr.h"
#include "GameObjectAI.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "SmartAI.h"
#include <cmath>
#include <mutex>
#include <unordered_map>

namespace TalonclawDakarr
{
bool Eligible(Creature* actor, Player* player)
{
    return actor && player && player->IsInWorld() && player->CanContact() && !player->isGameMaster() && !player->IsBeingTeleported() &&
        player->getClass() == CLASS_HUNTER && !player->GetQuestRewardStatus(40385) &&
        (player->GetQuestStatus(40385) == QUEST_STATUS_INCOMPLETE || player->GetQuestStatus(40385) == QUEST_STATUS_COMPLETE) &&
        (player->IsAlive() || !player->GetCorpse()) && actor->GetMap() == player->GetMap() &&
        actor->InSamePhase(player) && actor->IsWithinDistInMap(player, 50.0f);
}

void RecoverDeathCredit(Creature* actor, Unit* killer)
{
    if (!actor || actor->GetMapId() != 1511)
        return;
    Scenario* scenario = sScenarioMgr->GetScenario(actor->GetInstanceId());
    if (!scenario || scenario->GetScenarioId() != 973 || scenario->GetCurrentStep() != 11 ||
        !sConditionMgr->IsObjectMeetingSmartEventConditions(99480, 1, SMART_SCRIPT_TYPE_CREATURE, killer, actor))
        return;
    Player* player = actor->GetLootRecipient();
    if (!Eligible(actor, player))
        player = killer ? killer->GetCharmerOrOwnerPlayerOrPlayerItself() : nullptr;
    if (!Eligible(actor, player))
        return;
    player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 47688, 0, 0, killer, true);
    if (scenario->GetCurrentStep() == 12)
        TC_LOG_INFO("entities.player", "Talonclaw: recovered final Dakarr death credit for player %u", player->GetGUIDLow());
    else
        TC_LOG_ERROR("scripts", "Talonclaw: final Dakarr death event did not advance scenario for player %u", player->GetGUIDLow());
}

struct Gate
{
    bool held = false;
    bool addedFlag = false;
    ReactStates reaction = REACT_AGGRESSIVE;

    bool Update(Creature* actor)
    {
        if (!actor)
            return true;
        Scenario* scenario = sScenarioMgr->GetScenario(actor->GetInstanceId());
        bool blocked = actor->GetMapId() == 1511 && (!scenario ||
            (scenario->GetScenarioId() == 973 && scenario->GetCurrentStep() != 11));
        if (blocked)
        {
            if (!held)
            {
                held = true;
                addedFlag = !actor->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                reaction = actor->GetReactState();
                if (actor->isInCombat())
                    actor->CombatStop(true);
            }
            actor->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            actor->SetReactState(REACT_PASSIVE);
        }
        else if (held)
        {
            if (addedFlag)
                actor->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            actor->SetReactState(reaction);
            held = false;
        }
        return blocked;
    }
};
}

struct npc_talonclaw_dakarr : SmartAI
{
    explicit npc_talonclaw_dakarr(Creature* creature) : SmartAI(creature) { }
    TalonclawDakarr::Gate gate;

    void InitializeAI() override { SmartAI::InitializeAI(); gate.Update(me); }
    void Reset() override { SmartAI::Reset(); gate.Update(me); }
    void UpdateAI(uint32 diff) override { gate.Update(me); SmartAI::UpdateAI(diff); }
    void AttackStart(Unit* target) override { if (!gate.Update(me)) SmartAI::AttackStart(target); }
    void DamageTaken(Unit* attacker, uint32& damage, DamageEffectType type) override
    {
        if (gate.Update(me))
            damage = 0;
        else
            SmartAI::DamageTaken(attacker, damage, type);
    }
    void JustDied(Unit* killer) override
    {
        SmartAI::JustDied(killer);
        TalonclawDakarr::RecoverDeathCredit(me, killer);
    }
};

namespace ArtifactAcquisition
{
struct Pickup {uint32 object,item,playerClass,quest,alternate;};
Pickup const Pickups[]={
    {249812,128861,CLASS_HUNTER,42185,0}, // Never Hunt Alone, after the workshop
    {248834,128808,CLASS_HUNTER,40385,0},
    {248419,128826,CLASS_HUNTER,40419,0},
    {248572,128868,CLASS_PRIEST,41631,0},
    {246699,128403,CLASS_DEATH_KNIGHT,40934,40986},
    {249814,127857,CLASS_MAGE,42011,0},
    {251049,128862,CLASS_MAGE,42479,0},
    {246021,128942,CLASS_WARLOCK,40623,41155},
    {249821,128943,CLASS_WARLOCK,42125,0},
    {246437,128858,CLASS_DRUID,40838,0}
};
bool Active(Player* p,uint32 quest)
{
    return quest && !p->GetQuestRewardStatus(quest) &&
        (p->GetQuestStatus(quest)==QUEST_STATUS_INCOMPLETE || p->GetQuestStatus(quest)==QUEST_STATUS_COMPLETE);
}
bool Use(Player* p,GameObject* go)
{
    for (Pickup const& row:Pickups)
        if (go->GetEntry()==row.object)
        {
            if (!p->IsAlive() || p->IsBeingTeleported() || p->getClass()!=row.playerClass ||
                (!Active(p,row.quest) && !Active(p,row.alternate)) || !go->isSpawned() ||
                go->HasFlag(GAMEOBJECT_FIELD_FLAGS,GO_FLAG_IN_USE|GO_FLAG_NOT_SELECTABLE) ||
                p->GetMap()!=go->GetMap() || !p->InSamePhase(go) || !p->canSeeOrDetect(go) || !p->IsWithinLOSInMap(go) ||
                p->GetExactDist(go)>5.0f || go->getLootState()==GO_JUST_DEACTIVATED) return true;
            if (row.object == 249812)
            {
                Scenario* scenario = sScenarioMgr->GetScenario(p->GetInstanceId());
                if (go->GetMapId() != 1609 || !scenario || scenario->GetScenarioId() != 1099 ||
                    scenario->GetCurrentStep() < 4 || scenario->GetCurrentStep() > 5)
                    return true;
            }
            if (row.object == 248834)
            {
                Scenario* scenario = sScenarioMgr->GetScenario(p->GetInstanceId());
                if (go->GetMapId() != 1511 || !scenario || scenario->GetScenarioId() != 973 ||
                    scenario->GetCurrentStep() < 12 || scenario->GetCurrentStep() > 13)
                    return true;
            }
            if (row.object == 248419)
            {
                Scenario* scenario = sScenarioMgr->GetScenario(p->GetInstanceId());
                if (go->GetMapId() != 1489 || !scenario || scenario->GetScenarioId() != 972 ||
                    scenario->GetCurrentStep() < 7 || scenario->GetCurrentStep() > 8)
                    return true;
                return false;
            }
            if (!p->HasItemCount(row.item,1,true)) return false;
            // A shop/bank copy satisfies ownership. Replay the same native
            // loot-completed AI event, without trying to loot another unique
            // weapon. Ordinary players still use real loot, including bag errors.
            go->SetLootState(GO_JUST_DEACTIVATED,p);
            // StoreNewItem normally emits this event. A previously bought
            // unique artifact emits no new item event at its quest chest.
            p->UpdateAchievementCriteria(CRITERIA_TYPE_OWN_ITEM,row.item,1);
            TC_LOG_INFO("entities.player","Artifact acquisition: player %u used owned artifact %u at object %u",p->GetGUIDLow(),row.item,row.object);
            return true;
        }
    return false;
}
}
namespace DarkRidersCompletion
{
void Recover(Player* player)
{
    // Shop ownership alone is not a quest completion. The persisted claim
    // objective is awarded by the final chest's native 204819 spell, after
    // Ariden has been defeated. Also repairs a claim saved before this fix.
    if (!player || !player->IsInWorld() || player->IsBeingTeleported() ||
        player->getClass() != CLASS_WARLOCK || player->GetQuestRewardStatus(40623) ||
        player->GetQuestStatus(40623) != QUEST_STATUS_INCOMPLETE ||
        player->GetQuestObjectiveData(40623, 103473) < 1 ||
        player->GetQuestObjectiveData(40623, 100813) >= 1 ||
        !player->HasItemCount(128942, 1, true))
        return;

    player->KilledMonsterCredit(100813);
    TC_LOG_INFO("entities.player", "Dark Riders: restored scenario completion after claimed Ulthalesh for player %u", player->GetGUIDLow());
}
}
class player_dark_riders_completion : public PlayerScript
{
public:
    player_dark_riders_completion() : PlayerScript("player_dark_riders_completion") { }
    void OnLogin(Player* player) override { DarkRidersCompletion::Recover(player); }
    void OnUpdate(Player* player, uint32) override { DarkRidersCompletion::Recover(player); }
};
namespace TalonclawWorldPickup
{
void Retire(GameObject* object, uint32 state, Unit* unit)
{
    if (!object || object->GetEntry() != 248834 || object->GetMapId() != 1511 || state != GO_JUST_DEACTIVATED ||
        object->getLootState() != GO_JUST_DEACTIVATED)
        return;
    Player* player = unit ? unit->ToPlayer() : nullptr;
    if (!player || !player->IsInWorld() || !player->IsAlive() || player->IsBeingTeleported() || player->getClass() != CLASS_HUNTER ||
        player->GetMap() != object->GetMap() || !player->InSamePhase(object) ||
        !ArtifactAcquisition::Active(player, 40385) || !player->HasItemCount(128808, 1, true))
        return;
    Scenario* scenario = sScenarioMgr->GetScenario(player->GetInstanceId());
    ObjectGuid source = object->GetSummonerGUID();
    if (!scenario || scenario->GetScenarioId() != 973 || scenario->GetCurrentStep() < 12 ||
        !source.IsCreatureOrVehicle() || source.GetEntry() != 99480)
        return;

    std::list<Creature*> actors;
    object->GetCreatureListWithEntryInGrid(actors, 100310, 3.0f);
    for (Creature* actor : actors)
        if (actor && actor->GetEntry() == 100310 && actor->IsInWorld() && !actor->IsDespawn() &&
            actor->GetMap() == object->GetMap() && actor->InSamePhase(object) && actor->GetExactDist(object) <= 3.0f)
            if (TempSummon* summon = actor->ToTempSummon())
                if (summon->GetSummonerGUID() == source)
                {
                    actor->DespawnOrUnsummon(1000);
                }
}
}
class go_artifact_acquisition : public GameObjectScript
{
public:
    go_artifact_acquisition():GameObjectScript("go_artifact_acquisition"){}
    bool OnGossipHello(Player* player,GameObject* go) override {return ArtifactAcquisition::Use(player,go);}
    void OnLootStateChanged(GameObject* object, uint32 state, Unit* unit) override { TalonclawWorldPickup::Retire(object, state, unit); }
};
class go_marksmanship_pickup : public GameObjectScript
{
    struct AI : SmartGameObjectAI
    {
        explicit AI(GameObject* object) : SmartGameObjectAI(object) { }

        bool GossipHello(Player* player, bool isUse) override
        {
            if (go->GetEntry() == 248419 && (!player || ArtifactAcquisition::Use(player, go)))
                return true;
            return SmartGameObjectAI::GossipHello(player, isUse);
        }
    };
public:
    go_marksmanship_pickup() : GameObjectScript("go_marksmanship_pickup") { }
    bool OnGossipHello(Player* player, GameObject* object) override { return !player || ArtifactAcquisition::Use(player, object); }
    GameObjectAI* GetAI(GameObject* object) const override { return new AI(object); }
};
namespace TalonclawPresentation
{
struct State
{
    uint32 instance = 0;
    uint32 elapsed = 0;
    bool admitted = false;
    bool finished = false;
    bool fallback = false;

    bool Current(Player* player) const
    {
        return player && player->IsInWorld() && player->IsAlive() && !player->IsBeingTeleported() &&
            player->getClass() == CLASS_HUNTER && player->GetMapId() == 1511 &&
            ArtifactAcquisition::Active(player, 40385) && player->HasItemCount(128808, 1, true);
    }

    void Begin(Player* player)
    {
        if (admitted || finished || !Current(player))
            return;
        Scenario* scenario = sScenarioMgr->GetScenario(player->GetInstanceId());
        if (!scenario || scenario->GetScenarioId() != 973 || scenario->GetCurrentStep() < 12 || scenario->GetCurrentStep() > 13)
            return;
        instance = player->GetInstanceId();
        admitted = true;
    }

    bool Update(Player* player, uint32 diff)
    {
        if (!admitted || finished)
            return false;
        if (!Current(player) || player->GetInstanceId() != instance)
        {
            admitted = false;
            return true;
        }
        if (diff < 20000 - elapsed)
        {
            elapsed += diff;
            return false;
        }
        elapsed = 20000;
        fallback = true;
        TC_LOG_WARN("scripts", "Talonclaw pickup: scene 1220 completion callback missing after 20000ms for player %u", player->GetGUIDLow());
        return true;
    }

    void Finish(Player* player)
    {
        if (!admitted || finished || !Current(player) || player->GetInstanceId() != instance ||
            (!fallback && !player->HasSceneStatus(1220, SCENE_COMPLETE)))
            return;
        finished = true;
        player->CastSpell(player, 197651, true);
    }
};
}

class spell_talonclaw_presentation : public AuraScript
{
    PrepareAuraScript(spell_talonclaw_presentation);
    TalonclawPresentation::State state;

    bool Validate(SpellInfo const*) override
    {
        return ValidateSpellInfo({197651});
    }

    Player* PlayerOwner()
    {
        Unit* owner = GetUnitOwner();
        return owner ? owner->ToPlayer() : nullptr;
    }

    void AfterApply(AuraEffect const*, AuraEffectHandleModes)
    {
        state.Begin(PlayerOwner());
    }

    void Update(uint32 diff, AuraEffect*)
    {
        if (state.Update(PlayerOwner(), diff))
            Remove();
    }

    void AfterRemove(AuraEffect const*, AuraEffectHandleModes)
    {
        state.Finish(PlayerOwner());
    }

    void Register() override
    {
        AfterEffectApply += AuraEffectApplyFn(spell_talonclaw_presentation::AfterApply, EFFECT_0, SPELL_AURA_ACTIVATE_SCENE, AURA_EFFECT_HANDLE_REAL);
        OnEffectUpdate += AuraEffectUpdateFn(spell_talonclaw_presentation::Update, EFFECT_0, SPELL_AURA_ACTIVATE_SCENE);
        AfterEffectRemove += AuraEffectRemoveFn(spell_talonclaw_presentation::AfterRemove, EFFECT_0, SPELL_AURA_ACTIVATE_SCENE, AURA_EFFECT_HANDLE_REAL);
    }
};

namespace MarksmanshipClaim { void Queue(Player* player); }

namespace MarksmanshipPresentation
{
struct State
{
    uint32 instance = 0;
    uint32 elapsed = 0;
    bool admitted = false;
    bool finished = false;
    bool fallback = false;

    bool Current(Player* player) const
    {
        return player && player->IsInWorld() && player->IsAlive() && !player->IsBeingTeleported() &&
            player->getClass() == CLASS_HUNTER && player->GetMapId() == 1489 &&
            ArtifactAcquisition::Active(player, 40419) && player->HasItemCount(128826, 1, true);
    }

    void Begin(Player* player)
    {
        if (admitted || finished || !Current(player))
            return;
        Scenario* scenario = sScenarioMgr->GetScenario(player->GetInstanceId());
        if (!scenario || scenario->GetScenarioId() != 972 || scenario->GetCurrentStep() < 7 || scenario->GetCurrentStep() > 8)
            return;
        instance = player->GetInstanceId();
        admitted = true;
    }

    bool Update(Player* player, uint32 diff)
    {
        if (!admitted || finished)
            return false;
        if (!Current(player) || player->GetInstanceId() != instance)
        {
            admitted = false;
            return true;
        }
        if (diff < 20000 - elapsed)
        {
            elapsed += diff;
            return false;
        }
        elapsed = 20000;
        fallback = true;
        TC_LOG_WARN("scripts", "Thas'dorah pickup: scene 1169 completion callback missing after 20000ms for player %u", player->GetGUIDLow());
        return true;
    }

    void Finish(Player* player)
    {
        if (!admitted || finished || !Current(player) || player->GetInstanceId() != instance ||
            (!fallback && !player->HasSceneStatus(1169, SCENE_COMPLETE)))
            return;
        finished = true;
        SpellCastResult result = player->CastSpell(player, 200262, true);
        if (result != SPELL_CAST_OK)
        {
            TC_LOG_WARN("scripts", "Thas'dorah pickup: native equip 200262 failed with result %u for player %u; owned pickup replay remains available", uint32(result), player->GetGUIDLow());
            return;
        }
        Item* weapon = player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND);
        if (!weapon || weapon->GetEntry() != 128826)
        {
            TC_LOG_WARN("scripts", "Thas'dorah pickup: native equip 200262 was accepted but item 128826 is not equipped for player %u; owned pickup replay remains available", player->GetGUIDLow());
            return;
        }
        MarksmanshipClaim::Queue(player);
    }
};
}

class spell_marksmanship_presentation : public AuraScript
{
    PrepareAuraScript(spell_marksmanship_presentation);
    MarksmanshipPresentation::State state;

    bool Validate(SpellInfo const*) override
    {
        return ValidateSpellInfo({200262});
    }

    Player* PlayerOwner()
    {
        Unit* owner = GetUnitOwner();
        return owner ? owner->ToPlayer() : nullptr;
    }

    void AfterApply(AuraEffect const*, AuraEffectHandleModes)
    {
        state.Begin(PlayerOwner());
    }

    void Update(uint32 diff, AuraEffect*)
    {
        if (state.Update(PlayerOwner(), diff))
            Remove();
    }

    void AfterRemove(AuraEffect const*, AuraEffectHandleModes)
    {
        state.Finish(PlayerOwner());
    }

    void Register() override
    {
        AfterEffectApply += AuraEffectApplyFn(spell_marksmanship_presentation::AfterApply, EFFECT_1, SPELL_AURA_ACTIVATE_SCENE, AURA_EFFECT_HANDLE_REAL);
        OnEffectUpdate += AuraEffectUpdateFn(spell_marksmanship_presentation::Update, EFFECT_1, SPELL_AURA_ACTIVATE_SCENE);
        AfterEffectRemove += AuraEffectRemoveFn(spell_marksmanship_presentation::AfterRemove, EFFECT_1, SPELL_AURA_ACTIVATE_SCENE, AURA_EFFECT_HANDLE_REAL);
    }
};

namespace TalonclawReturn
{
void Recover(Player* player)
{
    if (!player || !player->IsInWorld() || !player->IsAlive() || player->IsBeingTeleported() ||
        player->getClass() != CLASS_HUNTER || player->GetMapId() != 1220 || player->GetZoneId() != 7502 ||
        player->GetQuestRewardStatus(40385) || player->GetQuestStatus(40385) != QUEST_STATUS_INCOMPLETE ||
        player->GetQuestObjectiveData(40385, 100111) < 1 || player->GetQuestObjectiveData(40385, 100112) < 1 ||
        player->GetQuestObjectiveData(40385, 103798) >= 1 || !player->HasItemCount(128808, 1, true))
        return;

    player->KilledMonsterCredit(103798);
    TC_LOG_INFO("entities.player", "Talonclaw: credited actual Dalaran return after claimed artifact for player %u", player->GetGUIDLow());
}
}

class player_talonclaw_return : public PlayerScript
{
public:
    player_talonclaw_return() : PlayerScript("player_talonclaw_return") { }
    void OnLogin(Player* player) override { TalonclawReturn::Recover(player); }
    void OnUpdate(Player* player, uint32) override { TalonclawReturn::Recover(player); }
};

namespace TalonclawDeparture
{
bool Ready(Player* player)
{
    if (!player || !player->IsInWorld() || !player->IsAlive() || player->IsBeingTeleported() ||
        player->getClass() != CLASS_HUNTER || player->GetMapId() != 1511 ||
        player->GetQuestRewardStatus(40385) || player->GetQuestStatus(40385) != QUEST_STATUS_INCOMPLETE ||
        player->GetQuestObjectiveData(40385, 100111) < 1 || player->GetQuestObjectiveData(40385, 100112) < 1 ||
        player->GetQuestObjectiveData(40385, 103798) >= 1 || !player->HasItemCount(128808, 1, true) ||
        player->HasAura(205589) || player->GetExactDist(3878.35f, 1636.92f, -0.0612079f) > 60.0f)
        return false;
    Scenario* scenario = sScenarioMgr->GetScenario(player->GetInstanceId());
    GameObjectTemplate const* portal = sObjectMgr->GetGameObjectTemplate(9000575);
    uint32 scriptId = sObjectMgr->GetScriptId("go_talonclaw_return_portal");
    CreatureTextGroup const* text = sCreatureTextMgr->GetTextGroup(100005, 1);
    return scenario && scenario->GetScenarioId() == 973 && scenario->GetCurrentStep() >= 12 &&
        portal && scriptId && portal->ScriptId == scriptId && text && text->size() == 1 && text->front().BroadcastTextID == 106397;
}

struct State
{
    ObjectGuid dorro, portal;
    uint32 instance = 0, wait = 0, failures = 0;
    bool spoken = false;

    void Failed(Player* player, char const* actor)
    {
        ++failures;
        wait = 1000;
        if (failures == 1 || failures == 30)
            TC_LOG_WARN("scripts", "Talonclaw departure: missing %s for player %u, attempt %u/30", actor, player->GetGUIDLow(), failures);
    }

    void Update(Player* player, uint32 diff)
    {
        if (failures >= 30)
            return;
        if (diff < wait)
        {
            wait -= diff;
            return;
        }
        wait = 0;
        Creature* actor = player->GetMap()->GetCreature(dorro);
        if (actor && (!actor->IsAlive() || actor->IsQuestObjectExpired() || !player->InSamePhase(actor)))
        {
            actor->DespawnOrUnsummon();
            actor = nullptr;
        }
        if (!actor)
        {
            actor = player->SummonCreature(100005, 3878.35f, 1636.92f, -0.0612079f, 1.89629f,
                TEMPSUMMON_MANUAL_DESPAWN, 0, player->GetGUID());
            if (!actor)
            {
                Failed(player, "Dorro");
                return;
            }
            player->BindQuestObject(40385, actor);
            dorro = actor->GetGUID();
            spoken = false;
            wait = 10000;
            return;
        }
        if (!spoken)
        {
            if (!actor->IsAIEnabled)
            {
                Failed(player, "Dorro AI");
                return;
            }
            actor->AI()->Talk(1, player->GetGUID());
            spoken = true;
            wait = 6000;
            return;
        }
        if (player->GetMap()->GetGameObject(portal))
            return;
        Position pos = actor->GetNearPosition(2.0f, 1.5707963f);
        GameObject* object = player->SummonGameObject(9000575, pos, actor->GetOrientation(), 0.0f, 0.0f,
            std::sin(actor->GetOrientation() * 0.5f), std::cos(actor->GetOrientation() * 0.5f), 300, player->GetGUID());
        if (!object)
        {
            Failed(player, "return portal");
            return;
        }
        player->BindQuestObject(40385, object);
        portal = object->GetGUID();
        failures = 0;
    }
};

bool Use(Player* player, GameObject* object)
{
    if (!object || object->GetEntry() != 9000575 || !Ready(player) || object->GetOwnerGUID() != player->GetGUID() ||
        object->IsQuestObjectExpired() || !object->isSpawned() || player->GetMap() != object->GetMap() ||
        object->HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_IN_USE | GO_FLAG_NOT_SELECTABLE) ||
        !player->InSamePhase(object) || !player->canSeeOrDetect(object) || !player->IsWithinLOSInMap(object) ||
        player->GetExactDist(object) > 5.0f)
        return true;
    SpellCastResult result = player->CastSpell(player, 190598, true);
    if (result != SPELL_CAST_OK)
        TC_LOG_WARN("scripts", "Talonclaw departure: return cast failed for player %u, result %u", player->GetGUIDLow(), uint32(result));
    return true;
}
}

class player_talonclaw_departure : public PlayerScript
{
    std::mutex mutex;
    std::unordered_map<ObjectGuid, TalonclawDeparture::State> states;

    void Clear(Player* player)
    {
        if (states.erase(player->GetGUID()))
            player->CancelQuestObjects(40385);
    }

public:
    player_talonclaw_departure() : PlayerScript("player_talonclaw_departure") { }
    void OnUpdate(Player* player, uint32 diff) override
    {
        if (!player)
            return;
        std::lock_guard<std::mutex> lock(mutex);
        if (!TalonclawDeparture::Ready(player))
        {
            Clear(player);
            return;
        }
        auto found = states.find(player->GetGUID());
        if (found != states.end() && found->second.instance != player->GetInstanceId())
            Clear(player);
        auto& state = states[player->GetGUID()];
        state.instance = player->GetInstanceId();
        state.Update(player, diff);
    }
    void OnLogout(Player* player) override
    {
        if (!player)
            return;
        std::lock_guard<std::mutex> lock(mutex);
        Clear(player);
    }
    void OnMapChanged(Player* player) override { OnLogout(player); }
};

class go_talonclaw_return_portal : public GameObjectScript
{
    struct AI : GameObjectAI
    {
        explicit AI(GameObject* object) : GameObjectAI(object) { }
        void UpdateAI(uint32) override
        {
            Player* owner = ObjectAccessor::GetPlayer(*go, go->GetOwnerGUID());
            if (go->IsQuestObjectExpired() || !TalonclawDeparture::Ready(owner) || !owner->InSamePhase(go))
                go->Delete();
        }
    };
public:
    go_talonclaw_return_portal() : GameObjectScript("go_talonclaw_return_portal") { }
    bool OnGossipHello(Player* player, GameObject* object) override { return TalonclawDeparture::Use(player, object); }
    GameObjectAI* GetAI(GameObject* object) const override { return new AI(object); }
};

namespace MarksmanshipReturn
{
bool Scoped(Unit* caster)
{
    return caster && caster->IsCreature() && caster->GetEntry() == 100397 &&
        caster->GetMapId() == 1489 && !caster->ToCreature()->GetScriptId();
}

bool Current(Player* player, Unit* caster)
{
    if (!player || !Scoped(caster) || !caster->IsInWorld() || !caster->IsAlive() || !player->IsInWorld() || !player->IsAlive() ||
        player->getClass() != CLASS_HUNTER || player->GetMapId() != 1489 ||
        player->GetInstanceId() != caster->GetInstanceId() || !player->InSamePhase(caster) ||
        !ArtifactAcquisition::Active(player, 40419) || !player->HasItemCount(128826, 1, true))
        return false;
    Scenario* scenario = sScenarioMgr->GetScenario(player->GetInstanceId());
    return scenario && scenario->GetScenarioId() == 972 && scenario->GetCurrentStep() == 8;
}

struct State
{
    bool accepted = false;

    void Teleport(Player* player, Unit* caster, Spell* spell, SpellEffIndex effect)
    {
        accepted = false;
        if (!Current(player, caster) || player->IsBeingTeleported())
            return;
        Player::ScopedTeleportDelay guard(*player);
        spell->EffectTeleportUnits(effect);
        accepted = guard.IsPending();
    }

    bool CreditAllowed(Player* player, Unit* caster) const
    {
        return accepted && Current(player, caster) && player->IsBeingTeleportedFar();
    }
};
}

class spell_marksmanship_return : public SpellScript
{
    PrepareSpellScript(spell_marksmanship_return);
    MarksmanshipReturn::State state;
    bool scoped = false;

    bool Load() override
    {
        scoped = MarksmanshipReturn::Scoped(GetCaster());
        return true;
    }

    SpellCastResult Check()
    {
        Unit* target = GetExplTargetUnit();
        Player* player = target ? target->ToPlayer() : nullptr;
        if (scoped &&
            (!MarksmanshipReturn::Current(player, GetCaster()) || player->IsBeingTeleported()))
            return SPELL_FAILED_BAD_TARGETS;
        return SPELL_CAST_OK;
    }

    void Hit(SpellEffIndex effect)
    {
        if (!scoped)
            return;
        if (effect == EFFECT_0)
        {
            PreventHitDefaultEffect(effect);
            state.Teleport(GetHitPlayer(), GetCaster(), GetSpell(), effect);
        }
        else if (!state.CreditAllowed(GetHitPlayer(), GetCaster()))
            PreventHitDefaultEffect(effect);
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_marksmanship_return::Check);
        OnEffectHitTarget += SpellEffectFn(spell_marksmanship_return::Hit, EFFECT_0, SPELL_EFFECT_TELEPORT_L);
        OnEffectHitTarget += SpellEffectFn(spell_marksmanship_return::Hit, EFFECT_1, SPELL_EFFECT_KILL_CREDIT);
    }
};

namespace MarksmanshipClaim
{
bool Ready(Player* player, uint32 instance)
{
    if (!player || !player->IsInWorld() || !player->IsAlive() || player->IsBeingTeleported() ||
        player->getClass() != CLASS_HUNTER || player->GetMapId() != 1489 || player->GetInstanceId() != instance ||
        !ArtifactAcquisition::Active(player, 40419) || !player->HasItemCount(128826, 1, true))
        return false;
    Scenario* scenario = sScenarioMgr->GetScenario(instance);
    return scenario && scenario->GetScenarioId() == 972 && scenario->GetCurrentStep() == 8;
}

bool Native(Creature* actor)
{
    if (actor->GetScriptId() || actor->GetCreatureTemplate()->AIName != "SmartAI")
        return false;
    if (actor->GetDBTableGUIDLow() && !sSmartScriptMgr->GetScript(-int32(actor->GetDBTableGUIDLow()), SMART_SCRIPT_TYPE_CREATURE).empty())
        return false;
    auto events = sSmartScriptMgr->GetScript(100397, SMART_SCRIPT_TYPE_CREATURE);
    uint32 claim = 0, conversation = 0, matchingIds = 0;
    for (auto const& entry : events)
    {
        if (entry.event_id == 52 || entry.event_id == 53)
            ++matchingIds;
        if (entry.event_id == 52 && entry.link == 53 && entry.GetEventType() == SMART_EVENT_TEXT_OVER &&
            entry.event.event_chance == 100 && !entry.event.event_phase_mask && entry.event.event_flags == SMART_EVENT_FLAG_NOT_REPEATABLE &&
            entry.event.textOver.textGroupID == 22 && entry.event.textOver.creatureEntry == 100397 &&
            entry.GetActionType() == SMART_ACTION_TALK && entry.action.talk.textGroupID == 23 &&
            entry.action.talk.duration == 5000 && entry.GetTargetType() == SMART_TARGET_ACTION_INVOKER &&
            (entry.event.event_flags & SMART_EVENT_FLAG_NOT_REPEATABLE))
            ++claim;
        if (entry.event_id == 53 && !entry.link && entry.GetEventType() == SMART_EVENT_LINK &&
            entry.event.event_chance == 100 && !entry.event.event_phase_mask && entry.event.event_flags == SMART_EVENT_FLAG_NOT_REPEATABLE &&
            entry.GetActionType() == SMART_ACTION_SUMMON_CONVERSATION && entry.action.conversation.id == 1654 &&
            !entry.action.conversation.targetX && !entry.action.conversation.targetY && !entry.action.conversation.targetZ &&
            entry.GetTargetType() == SMART_TARGET_ACTION_INVOKER)
            ++conversation;
    }
    return matchingIds == 2 && claim == 1 && conversation == 1;
}

bool Attempt(Player* player)
{
    Creature* actor = player->FindNearestCreature(100397, 100.0f, true);
    if (!actor || !actor->IsInWorld() || !actor->IsAlive() || !actor->IsAIEnabled || !actor->AI() ||
        actor->GetMapId() != player->GetMapId() || actor->GetInstanceId() != player->GetInstanceId() || !player->InSamePhase(actor))
        return false;
    if (!Native(actor))
        return true;
    SmartScript* script = static_cast<SmartAI*>(actor->AI())->GetScript();
    if (script->HasRunEvent(52, SMART_EVENT_TEXT_OVER))
        return true;
    if (script->IsTextPending(22, 100397))
        return false;
    script->ProcessEventsFor(SMART_EVENT_TEXT_OVER, player, 22, 100397);
    return script->HasRunEvent(52, SMART_EVENT_TEXT_OVER);
}

void Retry(Player* player, uint32 instance, uint32 elapsed)
{
    if (!Ready(player, instance) || Attempt(player))
        return;
    if (elapsed >= 60000)
    {
        TC_LOG_WARN("scripts", "Thas'dorah pickup: native Vereesa claim dialogue unavailable after 60000ms for player %u; owned pickup replay remains available", player->GetGUIDLow());
        return;
    }
    player->AddDelayedEvent(500, [player, instance, elapsed]() { Retry(player, instance, elapsed + 500); });
}

void Queue(Player* player)
{
    if (player)
        Retry(player, player->GetInstanceId(), 0);
}
}

void AddSC_artifact_acquisition()
{
    RegisterCreatureAI(npc_talonclaw_dakarr);
    new go_artifact_acquisition();
    new go_marksmanship_pickup();
    new player_dark_riders_completion();
    RegisterAuraScript(spell_talonclaw_presentation);
    RegisterAuraScript(spell_marksmanship_presentation);
    RegisterSpellScript(spell_marksmanship_return);
    new player_talonclaw_return();
    new player_talonclaw_departure();
    new go_talonclaw_return_portal();
}
