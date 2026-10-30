#include "ScriptMgr.h"
#include "Player.h"
#include "Item.h"
#include "GameObject.h"
#include "Scenario.h"
#include "ScenarioMgr.h"
#include "SpellScript.h"
#include "Spell.h"
#include "SpellMgr.h"
#include "MoveSpline.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "Creature.h"
#include "TemporarySummon.h"
#include "CreatureAI.h"
#include "Conversation.h"
#include "ConditionMgr.h"
#include "CreatureTextMgr.h"
#include "GameObjectAI.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "SmartAI.h"
#include "EventObject.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "ObjectVisitors.hpp"
#include <cmath>
#include <mutex>
#include <unordered_map>

namespace MarksmanshipInquisitor
{
bool Ready(Creature* actor)
{
    if (!actor || actor->GetEntry() != 101269 || actor->GetMapId() != 1489 ||
        actor->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC))
        return false;
    Scenario* scenario = sScenarioMgr->GetScenario(actor->GetInstanceId());
    return scenario && scenario->GetScenarioId() == 972 && scenario->GetCurrentStep() == 5;
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
        bool blocked = actor->GetEntry() == 101269 && actor->GetMapId() == 1489 && (!scenario ||
            (scenario->GetScenarioId() == 972 && !Ready(actor)));
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

struct npc_marksmanship_qormaladon : SmartAI
{
    explicit npc_marksmanship_qormaladon(Creature* creature) : SmartAI(creature) { }
    MarksmanshipInquisitor::Gate gate;

    void InitializeAI() override { SmartAI::InitializeAI(); gate.Update(me); }
    void Reset() override { SmartAI::Reset(); gate.Update(me); }
    void UpdateAI(uint32 diff) override { gate.Update(me); SmartAI::UpdateAI(diff); }
    void SetData(uint32 id, uint32 value) override { SmartAI::SetData(id, value); gate.Update(me); }
    void AttackStart(Unit* target) override { if (!gate.Update(me)) SmartAI::AttackStart(target); }
    void DamageTaken(Unit* attacker, uint32& damage, DamageEffectType type) override
    {
        if (gate.Update(me))
            damage = 0;
        else
            SmartAI::DamageTaken(attacker, damage, type);
    }
};

class condition_marksmanship_qormaladon_ready : public ConditionScript
{
public:
    condition_marksmanship_qormaladon_ready() : ConditionScript("condition_marksmanship_qormaladon_ready") { }
    bool OnConditionCheck(Condition*, ConditionSourceInfo& sourceInfo) override
    {
        WorldObject* object = sourceInfo.mConditionTargets[1];
        return MarksmanshipInquisitor::Ready(object ? object->ToCreature() : nullptr);
    }
};

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

namespace TalonclawAmbush
{
void Protect(Creature* actor, Unit* attacker, uint32& damage, uint32& encounterStep)
{
    if (!actor || !attacker || !damage || actor->GetEntry() != 100376 || actor->GetMapId() != 1511 || !actor->IsAlive())
        return;
    Scenario* scenario = sScenarioMgr->GetScenario(actor->GetInstanceId());
    if (!scenario || scenario->GetScenarioId() != 973)
        return;
    if (encounterStep && encounterStep != scenario->GetCurrentStep())
    {
        damage = 0;
        return;
    }
    if (scenario->GetCurrentStep() != 9 && scenario->GetCurrentStep() != 10)
        return;
    if (!encounterStep)
        encounterStep = scenario->GetCurrentStep();
    uint32 percent = encounterStep == 9 ? 70 : 60;
    uint64 health = actor->GetHealth(attacker);
    uint64 floor = actor->CountPctFromMaxHealth(percent, attacker);
    if (health > floor && damage < health - floor)
        return;
    damage = 0;
    uint64 baseFloor = std::max<uint64>(1, actor->CountPctFromMaxHealth(percent));
    if (actor->GetHealth() > baseFloor)
    {
        actor->SetHealth(baseFloor);
        TC_LOG_INFO("scripts.ai", "Talonclaw: protected Dakarr's ambush transition at scenario step %u", encounterStep);
    }
}
}

struct npc_talonclaw_ambush_dakarr : SmartAI
{
    explicit npc_talonclaw_ambush_dakarr(Creature* creature) : SmartAI(creature) { }
    uint32 encounterStep = 0;

    void DamageTaken(Unit* attacker, uint32& damage, DamageEffectType type) override
    {
        if (!encounterStep && me->GetEntry() == 100376 && me->GetMapId() == 1511)
        {
            Scenario* scenario = sScenarioMgr->GetScenario(me->GetInstanceId());
            if (scenario && scenario->GetScenarioId() == 973 && (scenario->GetCurrentStep() == 9 || scenario->GetCurrentStep() == 10))
                encounterStep = scenario->GetCurrentStep();
        }
        SmartAI::DamageTaken(attacker, damage, type);
    }

    void DamageTakenAfterScaling(Unit* attacker, uint32& damage, DamageEffectType) override
    {
        TalonclawAmbush::Protect(me, attacker, damage, encounterStep);
    }
};

namespace TalonclawAmbushEvent
{
struct State
{
    uint32 instance = 0;
    uint32 wait = 0;
    bool mist = false;
    bool scene = false;
};
std::mutex stateLock;
std::unordered_map<ObjectGuid, State> states;

void Save(Player* player, State const& state)
{
    std::lock_guard<std::mutex> guard(stateLock);
    states[player->GetGUID()] = state;
}

void Clear(Player* player)
{
    State state;
    {
        std::lock_guard<std::mutex> guard(stateLock);
        auto found = states.find(player->GetGUID());
        if (found == states.end())
            return;
        state = found->second;
        states.erase(found);
    }
    if (state.mist)
        player->RemoveAurasDueToSpell(198883, player->GetGUID());
    if (state.scene)
        player->RemoveAurasDueToSpell(197615, player->GetGUID());
}

State Load(Player* player)
{
    State state;
    {
        std::lock_guard<std::mutex> guard(stateLock);
        auto found = states.find(player->GetGUID());
        if (found != states.end())
            state = found->second;
    }
    if (state.instance != player->GetInstanceId())
    {
        Clear(player);
        state = State{};
        state.instance = player->GetInstanceId();
        Save(player, state);
    }
    return state;
}

Scenario* Current(Player* player)
{
    if (!player || !player->IsInWorld() || !player->CanContact() || !player->IsAlive() || player->IsBeingTeleported() ||
        player->GetMapId() != 1511 || player->getClass() != CLASS_HUNTER || player->GetQuestRewardStatus(40385) ||
        (player->GetQuestStatus(40385) != QUEST_STATUS_INCOMPLETE && player->GetQuestStatus(40385) != QUEST_STATUS_COMPLETE))
        return nullptr;
    Scenario* scenario = sScenarioMgr->GetScenario(player->GetInstanceId());
    return scenario && scenario->GetScenarioId() == 973 && (scenario->GetCurrentStep() == 10 || scenario->GetCurrentStep() == 11) ? scenario : nullptr;
}

bool Scoped(EventObject* event)
{
    return event && event->GetEntry() == 108 && event->GetMapId() == 1511;
}

bool Eligible(Player* player, EventObject* event)
{
    Scenario* scenario = Current(player);
    if (!scenario || !Scoped(event) || !event->eventTemplate || event->eventTemplate->radius != 24.0f ||
        player->GetMap() != event->GetMap() || !player->InSamePhase(event) || !player->IsWithinDistInMap(event, event->eventTemplate->radius))
        return false;
    uint32 root = scenario->GetCurrentStep() == 10 ? 0 : 2;
    if (sConditionMgr->GetConditionsForSmartEvent(108, root, SMART_SCRIPT_TYPE_EVENTOBJECT).empty() ||
        !sConditionMgr->IsObjectMeetingSmartEventConditions(108, root, SMART_SCRIPT_TYPE_EVENTOBJECT, player, event))
        return false;
    State state = Load(player);
    return root == 0 ? !player->HasAura(198883) : !state.scene && !player->HasAura(197615);
}

bool Dispatch(Player* player, EventObject* event, bool enter)
{
    if (!player || !player->IsAlive())
        return false;
    SmartScript script;
    script.OnInitialize(event, nullptr);
    script.ProcessEventsFor(enter ? SMART_EVENT_EVENTOBJECT_ONTRIGGER : SMART_EVENT_EVENTOBJECT_OFFTRIGGER, player, event->GetEntry());
    return true;
}

bool Enter(Player* player, EventObject* event)
{
    if (!Eligible(player, event))
        return false;
    uint32 step = Current(player)->GetCurrentStep();
    Dispatch(player, event, true);
    State state = Load(player);
    if (step == 10)
        state.mist = player->HasAura(198883, player->GetGUID());
    else
    {
        state.scene = player->HasAura(197615, player->GetGUID()) && player->HasSceneStatus(1145, SCENE_LAUNCH);
        if (!state.scene && player->HasAura(197615, player->GetGUID()))
        {
            player->RemoveAurasDueToSpell(197615, player->GetGUID());
            TC_LOG_ERROR("scripts", "Talonclaw: ambush scene aura applied without native scene launch for player %u", player->GetGUIDLow());
        }
    }
    Save(player, state);
    return step == 10 ? state.mist : state.scene;
}

void Update(Player* player, uint32 diff)
{
    Scenario* scenario = Current(player);
    if (!scenario)
    {
        if (player)
            Clear(player);
        return;
    }
    State state = Load(player);
    if (state.mist && scenario->GetCurrentStep() != 10)
    {
        player->RemoveAurasDueToSpell(198883, player->GetGUID());
        state.mist = false;
    }
    if (state.wait > diff)
    {
        state.wait -= diff;
        Save(player, state);
        return;
    }
    state.wait = 500;
    Save(player, state);
    if ((scenario->GetCurrentStep() == 10 && player->HasAura(198883)) || (scenario->GetCurrentStep() == 11 && state.scene))
        return;
    auto rearm = [player](WorldObject* object)
    {
        if (object->GetTypeId() != TYPEID_EVENTOBJECT)
            return;
        EventObject* event = static_cast<EventObject*>(object);
        if (Eligible(player, event))
            event->affectedPlayers.erase(player->GetGUID());
    };
    Trinity::WorldObjectWorker<decltype(rearm)> worker(player, rearm, GRID_MAP_TYPE_MASK_EVENTOBJECT);
    Trinity::VisitNearbyObject(player, 30.0f, worker);
}
}

class eventobject_talonclaw_ambush : public EventObjectScript
{
public:
    eventobject_talonclaw_ambush() : EventObjectScript("eventobject_talonclaw_ambush") { }
    bool IsTriggerMeets(Player* player, EventObject* event) override
    {
        return !TalonclawAmbushEvent::Scoped(event) || TalonclawAmbushEvent::Eligible(player, event);
    }
    bool OnTrigger(Player* player, EventObject* event, bool enter) override
    {
        return !enter || !TalonclawAmbushEvent::Scoped(event) ? TalonclawAmbushEvent::Dispatch(player, event, enter) : TalonclawAmbushEvent::Enter(player, event);
    }
};

class player_talonclaw_ambush_event : public PlayerScript
{
public:
    player_talonclaw_ambush_event() : PlayerScript("player_talonclaw_ambush_event") { }
    void OnUpdate(Player* player, uint32 diff) override { TalonclawAmbushEvent::Update(player, diff); }
    void OnLogout(Player* player) override { TalonclawAmbushEvent::Clear(player); }
};

namespace TitanstrikePrustaga
{
void Protect(Creature* actor, Unit* attacker, uint32& damage)
{
    if (!actor || !attacker || !damage || actor->GetEntry() != 106730 || actor->GetMapId() != 1609 || !actor->IsAlive())
        return;
    Scenario* scenario = sScenarioMgr->GetScenario(actor->GetInstanceId());
    if (!scenario || scenario->GetScenarioId() != 1099 || damage < actor->GetHealth(attacker))
        return;
    damage = 0;
    if (actor->GetHealth() > 1)
    {
        actor->SetHealth(1);
        TC_LOG_INFO("scripts.ai", "Titanstrike: protected Prustaga's surrender from a scaled killing blow");
    }
}
}

struct npc_titanstrike_final_prustaga : SmartAI
{
    explicit npc_titanstrike_final_prustaga(Creature* creature) : SmartAI(creature) { }
    void DamageTakenAfterScaling(Unit* attacker, uint32& damage, DamageEffectType) override
    {
        TitanstrikePrustaga::Protect(me, attacker, damage);
    }
};

namespace TitanstrikeAftermath
{
bool Current(Player* player, uint32 instance)
{
    if (!player || !player->IsInWorld() || !player->CanContact() ||
        player->GetMapId() != 1609 || player->GetInstanceId() != instance || player->getClass() != CLASS_HUNTER ||
        player->GetQuestRewardStatus(42185) ||
        (player->GetQuestStatus(42185) != QUEST_STATUS_INCOMPLETE && player->GetQuestStatus(42185) != QUEST_STATUS_COMPLETE))
        return false;
    Scenario* scenario = sScenarioMgr->GetScenario(instance);
    if (!scenario || scenario->GetScenarioId() != 1099 ||
        (scenario->GetCurrentStep() != 2 && scenario->GetCurrentStep() != 3))
        return false;
    auto progress = scenario->GetAchievementMgr().GetCriteriaProgressMap();
    auto defeated = progress->find(48020);
    return defeated != progress->end() && defeated->second.Counter;
}

bool Advance(Player* player, uint32 instance, bool& creditSent)
{
    if (!Current(player, instance) || !player->IsAlive() || player->IsBeingTeleported() ||
        player->FindNearestCreature(106730, 100.0f, true))
        return false;
    Creature* hati = player->FindNearestCreature(103154, 100.0f, true);
    if (!hati || !hati->IsInWorld() || !hati->IsAlive() || !hati->IsAIEnabled || !hati->AI() ||
        hati->GetMap() != player->GetMap() || !hati->InSamePhase(player))
        return false;
    Scenario* scenario = sScenarioMgr->GetScenario(instance);
    if (hati->HasAura(211967))
        return scenario->GetCurrentStep() == 3;
    if (!hati->HasAura(211970))
    {
        hati->AI()->SetData(1, 1);
        if (!hati->HasAura(211970) &&
            (hati->CastSpell(hati, 211970, true) != SPELL_CAST_OK || !hati->HasAura(211970)))
            return false;
    }
    if (scenario->GetCurrentStep() == 2)
    {
        auto progress = scenario->GetAchievementMgr().GetCriteriaProgressMap();
        auto handedOff = progress->find(48094);
        if (creditSent || (handedOff != progress->end() && handedOff->second.Counter))
            return false;
        creditSent = true;
        player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 50997, 0, 0, nullptr, true);
        if (scenario->GetCurrentStep() != 3)
            TC_LOG_ERROR("scripts.ai", "Titanstrike: aftermath credit did not publish the Hati step for player %u", player->GetGUIDLow());
    }
    if (scenario->GetCurrentStep() != 3 || !Current(player, instance) ||
        !hati->IsInWorld() || !hati->IsAlive() || hati->GetMap() != player->GetMap() || !hati->InSamePhase(player))
        return false;
    hati->SetFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
    return true;
}

struct Story : Conversation
{
    uint32 instance;
    uint32 retryRemaining = 0;
    bool creditSent = false;
    bool reportedWait = false;

    explicit Story(uint32 identity) : instance(identity) { }

    void Update(uint32 diff) override
    {
        if (!IsInWorld())
            return;
        Player* player = ObjectAccessor::GetPlayer(*this, GetCasterGUID());
        if (IsQuestObjectExpired() || !Current(player, instance) || !InSamePhase(player))
        {
            Remove();
            return;
        }
        auto started = playing.find(GetCasterGUID());
        if (started == playing.end() || getMSTime() - started->second <= GetUInt32Value(CONVERSATION_FIELD_LAST_LINE_END_TIME))
            return;
        if (diff < retryRemaining)
        {
            retryRemaining -= diff;
            return;
        }
        retryRemaining = 1000;
        if (Advance(player, instance, creditSent))
        {
            Remove();
            return;
        }
        if (!reportedWait)
        {
            reportedWait = true;
            TC_LOG_ERROR("scripts.ai", "Titanstrike: completed aftermath is waiting for eligible Hati/player context for player %u; no handoff bypass", player->GetGUIDLow());
        }
    }
};

void Request(Player* player, SpellInfo const* info, uint32 instance, uint32 attempt = 0)
{
    if (!Current(player, instance) || sScenarioMgr->GetScenario(instance)->GetCurrentStep() != 2)
        return;
    bool created = false;
    if (player->IsAlive() && !player->IsBeingTeleported() && !player->HasAura(211934) && !player->FindNearestCreature(106730, 100.0f, true))
    {
        auto story = new Story(instance);
        if (story->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(), 2167,
            player, info, player->GetPosition()))
        {
            if (story->GetUInt32Value(CONVERSATION_FIELD_LAST_LINE_END_TIME) >= 72398)
            {
                player->BindQuestObject(42185, story);
                created = true;
            }
            else
                story->Remove();
        }
        else
            delete story;
    }
    if (created)
        return;
    if (!attempt)
        TC_LOG_ERROR("scripts.ai", "Titanstrike: aftermath creation/context unavailable for player %u; retrying within this quest attempt", player->GetGUIDLow());
    if (attempt >= 30)
    {
        TC_LOG_ERROR("scripts.ai", "Titanstrike: aftermath retry exhausted for player %u; no Hati progression awarded", player->GetGUIDLow());
        return;
    }
    player->AddQuestDelayedEvent(42185, 1000, [player, info, instance, attempt]()
    {
        Request(player, info, instance, attempt + 1);
    });
}
}

class spell_titanstrike_aftermath : public SpellScript
{
    PrepareSpellScript(spell_titanstrike_aftermath);

    bool Validate(SpellInfo const* info) override
    {
        return info->Id == 211960 && info->Effects[EFFECT_0] &&
            info->Effects[EFFECT_0]->Effect == SPELL_EFFECT_SUMMON_CONVERSATION && info->Effects[EFFECT_0]->MiscValue == 2167;
    }

    SpellCastResult Check()
    {
        Player* player = GetCaster()->ToPlayer();
        return player && TitanstrikeAftermath::Current(player, player->GetInstanceId()) ? SPELL_CAST_OK : SPELL_FAILED_BAD_TARGETS;
    }

    void Create(SpellEffIndex effect)
    {
        PreventHitDefaultEffect(effect);
        if (Player* player = GetCaster()->ToPlayer())
            TitanstrikeAftermath::Request(player, GetSpellInfo(), player->GetInstanceId());
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_titanstrike_aftermath::Check);
        OnEffectHit += SpellEffectFn(spell_titanstrike_aftermath::Create, EFFECT_0, SPELL_EFFECT_SUMMON_CONVERSATION);
    }
};

namespace TitanstrikeCombatDialogue
{
bool Scoped(Creature* actor)
{
    return actor && actor->GetEntry() == 106714 && actor->GetMapId() == 1609;
}

bool Allowed(Creature* actor)
{
    if (!Scoped(actor))
        return true;
    if (!actor->IsInWorld() || !actor->IsAlive())
        return false;
    Scenario* scenario = sScenarioMgr->GetScenario(actor->GetInstanceId());
    if (!scenario || scenario->GetScenarioId() != 1099 || scenario->GetCurrentStep() != 2)
        return false;
    auto progress = scenario->GetAchievementMgr().GetCriteriaProgressMap();
    auto defeated = progress->find(48020);
    if (defeated != progress->end() && defeated->second.Counter)
        return false;
    Creature* boss = actor->FindNearestCreature(106730, 100.0f, true);
    return boss && boss->IsInWorld() && boss->IsAlive() && boss->GetHealth() > 1 &&
        boss->GetMap() == actor->GetMap() && boss->InSamePhase(actor);
}

struct Story : Conversation
{
    ObjectGuid opponent;

    explicit Story(ObjectGuid identity) : opponent(identity) { }

    void Update(uint32 diff) override
    {
        if (!IsInWorld())
            return;
        Creature* actor = ObjectAccessor::GetCreature(*this, GetCasterGUID());
        if (!Scoped(actor) || !Allowed(actor) || !InSamePhase(actor))
        {
            Remove();
            return;
        }
        Creature* boss = actor->FindNearestCreature(106730, 100.0f, true);
        if (!boss || boss->GetGUID() != opponent)
        {
            Remove();
            return;
        }
        Conversation::Update(diff);
    }
};

void Create(Creature* actor, SpellInfo const* info, Position const& position)
{
    if (!Scoped(actor) || !Allowed(actor))
        return;
    Creature* boss = actor->FindNearestCreature(106730, 100.0f, true);
    if (!boss)
        return;
    auto story = new Story(boss->GetGUID());
    if (!story->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(),
        info->Effects[EFFECT_0]->MiscValue, actor, info, position))
    {
        delete story;
        TC_LOG_ERROR("scripts.ai", "Titanstrike: combat conversation %u creation failed for Thorim; no progression changed", info->Id);
    }
}
}

class spell_titanstrike_combat_dialogue : public SpellScript
{
    PrepareSpellScript(spell_titanstrike_combat_dialogue);

    bool Validate(SpellInfo const* info) override
    {
        return info->Effects[EFFECT_0] && info->Effects[EFFECT_0]->Effect == SPELL_EFFECT_SUMMON_CONVERSATION &&
            ((info->Id == 211824 && info->Effects[EFFECT_0]->MiscValue == 2152) ||
             (info->Id == 211826 && info->Effects[EFFECT_0]->MiscValue == 2153) ||
             (info->Id == 211827 && info->Effects[EFFECT_0]->MiscValue == 2154));
    }

    SpellCastResult Check()
    {
        return TitanstrikeCombatDialogue::Allowed(GetCaster()->ToCreature()) ? SPELL_CAST_OK : SPELL_FAILED_BAD_TARGETS;
    }

    void Create(SpellEffIndex effect)
    {
        Creature* actor = GetCaster()->ToCreature();
        if (!TitanstrikeCombatDialogue::Scoped(actor))
            return;
        PreventHitDefaultEffect(effect);
        Position position = actor->GetPosition();
        if (GetExplTargetDest() && GetHitDest())
            position = *GetHitDest();
        TitanstrikeCombatDialogue::Create(actor, GetSpellInfo(), position);
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_titanstrike_combat_dialogue::Check);
        OnEffectHit += SpellEffectFn(spell_titanstrike_combat_dialogue::Create, EFFECT_0, SPELL_EFFECT_SUMMON_CONVERSATION);
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
            if (row.object == 248572)
            {
                Scenario* scenario = sScenarioMgr->GetScenario(p->GetInstanceId());
                if (go->GetMapId() != 1583 || !scenario || scenario->GetScenarioId() != 1065 ||
                    scenario->GetCurrentStep() < 6 || scenario->GetCurrentStep() > 7)
                    return true;
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
namespace TitanstrikeWorldPickup
{
void Retire(GameObject* object, uint32 state, Unit* unit)
{
    if (!object || object->GetEntry() != 249812 || object->GetMapId() != 1609 || state != GO_JUST_DEACTIVATED ||
        object->getLootState() != GO_JUST_DEACTIVATED)
        return;
    Player* player = unit ? unit->ToPlayer() : nullptr;
    if (!player || !player->IsInWorld() || !player->IsAlive() || player->IsBeingTeleported() || player->getClass() != CLASS_HUNTER ||
        player->GetMap() != object->GetMap() || !player->InSamePhase(object) ||
        !ArtifactAcquisition::Active(player, 42185) || !player->HasItemCount(128861, 1, true))
        return;
    Scenario* scenario = sScenarioMgr->GetScenario(player->GetInstanceId());
    ObjectGuid source = object->GetSummonerGUID();
    if (!scenario || scenario->GetScenarioId() != 1099 || scenario->GetCurrentStep() < 4 || scenario->GetCurrentStep() > 5 ||
        !source.IsCreatureOrVehicle() || source.GetEntry() != 103154)
        return;

    std::list<Creature*> actors;
    object->GetCreatureListWithEntryInGrid(actors, 106745, 6.0f);
    for (Creature* actor : actors)
        if (actor && actor->GetEntry() == 106745 && actor->IsInWorld() && !actor->IsDespawn() &&
            actor->GetMap() == object->GetMap() && actor->InSamePhase(object) && actor->GetExactDist(object) <= 6.0f)
            if (TempSummon* summon = actor->ToTempSummon())
                if (summon->GetSummonerGUID() == source)
                    actor->DespawnOrUnsummon(1000);
}
}
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
    void OnLootStateChanged(GameObject* object, uint32 state, Unit* unit) override
    {
        TalonclawWorldPickup::Retire(object, state, unit);
        TitanstrikeWorldPickup::Retire(object, state, unit);
    }
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

namespace StromkarZakajz
{
void Protect(Creature* actor, Unit* attacker, uint32& damage)
{
    if (!actor || !attacker || !damage || !actor->IsAlive() || actor->GetEntry() != 104276 || actor->GetMapId() != 1539)
        return;
    Scenario* scenario = sScenarioMgr->GetScenario(actor->GetInstanceId());
    if (!scenario || scenario->GetScenarioId() != 1037 || scenario->GetCurrentStep() < 8 || scenario->GetCurrentStep() > 9 ||
        damage < actor->GetHealth(attacker))
        return;
    damage = 0;
    if (actor->GetHealth() > 1)
    {
        actor->SetHealth(1);
        TC_LOG_INFO("scripts.ai", "Stromkar: protected Zakajz's native finale from a scaled killing blow");
    }
}
}

struct npc_stromkar_zakajz : SmartAI
{
    explicit npc_stromkar_zakajz(Creature* actor) : SmartAI(actor) { }
    ObjectGuid thoradin = {};
    bool defeatPoseApplied = false;
    bool executionStarted = false;
    bool regenerationHeld = false;
    bool previousRegeneration = false;
    uint32 executionFlags = 0;
    uint32 defeatPoseRetry = 0;
    void ClearDefeatPose()
    {
        if (defeatPoseApplied)
            me->RemoveAurasDueToSpell(130966, me->GetGUID());
        defeatPoseApplied = false;
        if (regenerationHeld)
            me->setRegeneratingHealth(previousRegeneration);
        regenerationHeld = false;
    }
    void Reset() override
    {
        ClearDefeatPose();
        me->RemoveFlag(UNIT_FIELD_FLAGS, executionFlags);
        executionFlags = 0;
        executionStarted = false;
        defeatPoseRetry = 0;
        SmartAI::Reset();
    }
    void UpdateAI(uint32 diff) override
    {
        SmartAI::UpdateAI(diff);
        Scenario* scenario = me->IsInWorld() && me->GetMapId() == 1539 && me->GetEntry() == 104276
            ? sScenarioMgr->GetScenario(me->GetInstanceId()) : nullptr;
        if (!scenario || scenario->GetScenarioId() != 1037 ||
            (scenario->GetCurrentStep() != 9 && !(executionStarted && scenario->GetCurrentStep() == 10)) ||
            !me->IsAlive() || (!executionStarted && me->GetHealthPct() > 1.0f))
        {
            ClearDefeatPose();
            defeatPoseRetry = 0;
            return;
        }
        // The initial Zakajz103398 uses this same native permanent dead pose.
        // It has stun/dummy effects, not a real death or a progression event.
        if (!me->HasAura(130966))
        {
            if (defeatPoseRetry > diff)
                defeatPoseRetry -= diff;
            else
            {
                defeatPoseRetry = 2000;
                if (me->AddAura(130966, me))
                    defeatPoseApplied = true;
            }
        }
    }
    void SpellHit(Unit* caster, SpellInfo const* spell) override
    {
        // The actual post-arrival Dark Drain starts the native terminal visual.
        // Keep the downed body throughout it; removing130966 made Zakajz stand.
        if (spell && spell->Id == 201904 && me->GetEntry() == 104276 && me->GetMapId() == 1539)
        {
            Scenario* scenario = me->IsInWorld() ? sScenarioMgr->GetScenario(me->GetInstanceId()) : nullptr;
            if (executionStarted || !caster || !caster->ToPlayer() || !me->IsAlive() || me->GetHealthPct() > 1.0f ||
                !scenario || scenario->GetScenarioId() != 1037 ||
                (scenario->GetCurrentStep() != 9 && scenario->GetCurrentStep() != 10))
                return;
            executionStarted = true;
            // This remains a living, scripted corpse. Ordinary out-of-combat
            // regeneration otherwise heals it on the next native tick, which
            // used to clear the downed pose. Restore the original setting on
            // a real reset rather than changing the shared creature template.
            previousRegeneration = me->isRegeneratingHealth();
            regenerationHeld = true;
            me->setRegeneratingHealth(false);
            uint32 flags = UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_SELECTABLE;
            executionFlags = flags & ~me->GetUInt32Value(UNIT_FIELD_FLAGS);
            me->SetFlag(UNIT_FIELD_FLAGS, flags);
            me->SetReactState(REACT_PASSIVE);
            me->CombatStop(true);
            // Match the native death path's bidirectional threat cleanup.
            // CombatStop alone leaves hostile references on participants;
            // do not clear a player's unrelated combat or manufacture death.
            me->DeleteThreatList();
            me->getHostileRefManager().deleteReferences();
        }
        SmartAI::SpellHit(caster, spell);
    }
    void JustSummoned(Creature* actor) override
    {
        SmartAI::JustSummoned(actor);
        if (actor && actor->GetEntry() == 104307 && me->GetEntry() == 104276 && me->GetMapId() == 1539)
            thoradin = actor->GetGUID();
    }
    bool HasThoradin() const
    {
        Creature* actor = ObjectAccessor::GetCreature(*me, thoradin);
        return actor && actor->IsInWorld() && actor->IsAlive() && actor->GetEntry() == 104307 && actor->InSamePhase(me);
    }
    void DamageTakenAfterScaling(Unit* attacker, uint32& damage, DamageEffectType type) override
    {
        SmartAI::DamageTakenAfterScaling(attacker, damage, type);
        if (executionStarted)
        {
            damage = 0;
            return;
        }
        StromkarZakajz::Protect(me, attacker, damage);
    }
};

namespace StromkarThoradinPresence
{
bool Missing(Condition* condition, ConditionSourceInfo& sourceInfo)
{
    if (!condition || condition->SourceType != CONDITION_SOURCE_TYPE_SMART_EVENT || condition->SourceEntry != 104276 ||
        condition->SourceGroup != 10 || condition->SourceId || condition->ConditionType != CONDITION_NEAR_CREATURE ||
        condition->ConditionTarget != 1 || condition->ConditionValue1 != 104307 || condition->ConditionValue2 != 50 ||
        condition->ConditionValue3 || !condition->NegativeCondition)
        return true;
    Creature* owner = sourceInfo.mConditionTargets[1] ? sourceInfo.mConditionTargets[1]->ToCreature() : nullptr;
    auto* content = owner ? dynamic_cast<npc_stromkar_zakajz*>(owner->AI()) : nullptr;
    return content && !content->HasThoradin();
}
}

class condition_stromkar_thoradin_absence : public ConditionScript
{
public:
    condition_stromkar_thoradin_absence() : ConditionScript("condition_stromkar_thoradin_absence") { }
    bool OnConditionCheck(Condition* condition, ConditionSourceInfo& sourceInfo) override
    {
        return StromkarThoradinPresence::Missing(condition, sourceInfo);
    }
};

namespace XalatathFarthing
{
void Protect(Creature* actor, Unit* attacker, uint32& damage)
{
    if (!actor || !attacker || !damage || !actor->IsAlive() || actor->GetEntry() != 101148 || actor->GetMapId() != 1539)
        return;
    Scenario* scenario = sScenarioMgr->GetScenario(actor->GetInstanceId());
    if (!scenario || scenario->GetScenarioId() != 991 || scenario->GetCurrentStep() < 7 || scenario->GetCurrentStep() > 8 ||
        damage < actor->GetHealth(attacker))
        return;
    damage = 0;
    if (actor->GetHealth() > 1)
    {
        actor->SetHealth(1);
        TC_LOG_INFO("scripts.ai", "Xalatath: protected Farthing's native finale from a scaled killing blow");
    }
}
}

struct npc_xalatath_farthing : SmartAI
{
    explicit npc_xalatath_farthing(Creature* actor) : SmartAI(actor) { }
    void Reset() override
    {
        ++finaleGeneration;
        SmartAI::Reset();
    }
    uint32 GetFinaleGeneration() const { return finaleGeneration; }
    void DamageTakenAfterScaling(Unit* attacker, uint32& damage, DamageEffectType type) override
    {
        SmartAI::DamageTakenAfterScaling(attacker, damage, type);
        XalatathFarthing::Protect(me, attacker, damage);
    }

private:
    uint32 finaleGeneration = 0;
};

namespace XalatathSoulAttack
{
Creature* Target(Creature* actor)
{
    if (!actor || actor->GetEntry() != 102247 || !actor->IsInWorld() || !actor->IsAlive() || actor->GetMapId() != 1539)
        return nullptr;
    TempSummon* summon = actor->ToTempSummon();
    Unit* summoner = summon ? summon->GetSummoner() : nullptr;
    Creature* owner = summoner ? summoner->ToCreature() : nullptr;
    if (!owner || owner->GetEntry() != 101148 || !owner->IsInWorld() || !owner->IsAlive() ||
        owner->GetMap() != actor->GetMap() || !owner->InSamePhase(actor) || !actor->IsWithinDistInMap(owner, 30.0f) ||
        !owner->HasAura(61204) || owner->GetHealthPct() > 1.0f || !owner->IsAIEnabled ||
        !dynamic_cast<npc_xalatath_farthing*>(owner->AI()))
        return nullptr;
    Scenario* scenario = sScenarioMgr->GetScenario(actor->GetInstanceId());
    return scenario && scenario->GetScenarioId() == 991 && scenario->GetCurrentStep() == 7 ? owner : nullptr;
}
}

struct npc_xalatath_soul_attack : SmartAI
{
    explicit npc_xalatath_soul_attack(Creature* actor) : SmartAI(actor) { }

    void IsSummonedBy(Unit* summoner) override
    {
        if (summoner && summoner == XalatathSoulAttack::Target(me))
        {
            generation = static_cast<npc_xalatath_farthing*>(summoner->ToCreature()->AI())->GetFinaleGeneration();
            activated = true;
            SmartAI::IsSummonedBy(summoner);
        }
    }

    void SpellHitTarget(Unit* target, SpellInfo const* spell) override
    {
        SmartAI::SpellHitTarget(target, spell);
        if (activated && spell && spell->Id == 201899 && target && target == XalatathSoulAttack::Target(me) &&
            generation == static_cast<npc_xalatath_farthing*>(target->ToCreature()->AI())->GetFinaleGeneration())
            acknowledged = true;
    }

    void UpdateAI(uint32 diff) override
    {
        SmartAI::UpdateAI(diff);
        if (!activated || acknowledged)
            return;
        if (retryTimer > diff)
        {
            retryTimer -= diff;
            return;
        }
        retryTimer = 5000;
        Creature* owner = XalatathSoulAttack::Target(me);
        if (!owner)
            return;
        if (generation != static_cast<npc_xalatath_farthing*>(owner->AI())->GetFinaleGeneration())
        {
            activated = false;
            return;
        }
        if (me->IsNonMeleeSpellCast(false))
            return;
        SpellCastResult result = me->CastSpell(owner, 201899, false);
        if (result != SPELL_CAST_OK)
            TC_LOG_WARN("scripts.ai", "Xalatath: soul attack 201899 failed with result %u; retain Farthing and retry the native cast", uint32(result));
    }

private:
    bool acknowledged = false;
    bool activated = false;
    uint32 generation = 0;
    uint32 retryTimer = 5000;
};

namespace XalatathPresentation
{
bool Complete(Player* player, SpellScene const* scene, std::string const& trigger)
{
    if (!scene || scene->MiscValue != 1187 || scene->SceneScriptPackageID != 1553)
        return false;
    if (trigger != "complete" || !player || !player->HasSceneStatus(1187, SCENE_COMPLETE) ||
        !player->IsInWorld() || !player->CanContact() || !player->IsAlive() || player->IsBeingTeleported() ||
        player->getClass() != CLASS_PRIEST || player->GetMapId() != 1539 ||
        !ArtifactAcquisition::Active(player, 40710) || !player->GetItemByEntry(128827))
        return true;
    Scenario* scenario = sScenarioMgr->GetScenario(player->GetInstanceId());
    if (!scenario || scenario->GetScenarioId() != 991 || scenario->GetCurrentStep() < 8 || scenario->GetCurrentStep() > 9)
        return true;
    SpellCastResult result = player->CastSpell(player, 201778, true);
    if (result != SPELL_CAST_OK)
    {
        TC_LOG_WARN("scripts", "Xalatath pickup: native equip 201778 failed with result %u for player %u; no item or progression granted", uint32(result), player->GetGUIDLow());
        return true;
    }
    Item* weapon = player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND);
    Item* child = player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND);
    if (!weapon || weapon->GetEntry() != 128827 || !child || child->GetEntry() != 133958 || weapon->GetChildItem() != child->GetGUID())
        TC_LOG_WARN("scripts", "Xalatath pickup: native equip 201778 accepted but blade/paired item is not equipped for player %u; retain native inventory errors", player->GetGUIDLow());
    return true;
}
}

namespace BloodArtifactScene
{
struct Pickup
{
    uint32 scene, package, map, scenario, firstStep, quest, item;
};

Pickup const Pickups[] = {
    {1168, 1532, 1489, 940, 9, 40740, 128402}
};

// Scene-specific admission is independent of ordinary native equip-spell use.
// In particular, a late completion after map change must not fall through to
// the source17 branch that preserves those spells outside their quest maps.
bool Denied(Player* player, SpellScene const* scene, std::string const& trigger)
{
    if (!scene)
        return false;
    Pickup const* pickup = nullptr;
    for (Pickup const& row : Pickups)
        if (uint32(scene->MiscValue) == row.scene)
        {
            pickup = &row;
            break;
        }
    if (!pickup)
        return false;
    if (uint32(scene->SceneScriptPackageID) != pickup->package || trigger != "complete" || !player ||
        !player->HasSceneStatus(pickup->scene, SCENE_COMPLETE) || !player->IsInWorld() ||
        !player->CanContact() || !player->IsAlive() || player->IsBeingTeleported() ||
        player->getClass() != CLASS_DEATH_KNIGHT || player->GetMapId() != pickup->map ||
        !ArtifactAcquisition::Active(player, pickup->quest) || !player->HasItemCount(pickup->item, 1, false))
        return true;
    Scenario* scenario = sScenarioMgr->GetScenario(player->GetInstanceId());
    return !scenario || scenario->GetScenarioId() != pickup->scenario ||
        scenario->GetCurrentStep() < pickup->firstStep || scenario->GetCurrentStep() > pickup->firstStep + 1;
}
}

class scene_blood_artifact_equipment : public SceneTriggerScript
{
public:
    scene_blood_artifact_equipment() : SceneTriggerScript("scene_blood_artifact_equipment") { }
    bool OnTrigger(Player* player, SpellScene const* scene, std::string trigger) override
    {
        return BloodArtifactScene::Denied(player, scene, trigger);
    }
};

namespace MonkArtifactScene
{
struct Pickup
{
    uint32 scene, package, map, scenario, firstStep, quest, item;
};

Pickup const Pickups[] = {
    {1160, 1524, 1528, 983, 6, 40570, 128940},
    {1342, 1668, 1625, 1137, 7, 42765, 128938}
};

// Scene-specific admission is independent of ordinary native equip-spell use.
// In particular, a late completion after map change must not fall through to
// the source17 branch that preserves those spells outside their quest maps.
bool Denied(Player* player, SpellScene const* scene, std::string const& trigger)
{
    if (!scene)
        return false;
    Pickup const* pickup = nullptr;
    for (Pickup const& row : Pickups)
        if (uint32(scene->MiscValue) == row.scene)
        {
            pickup = &row;
            break;
        }
    if (!pickup)
        return false;
    if (uint32(scene->SceneScriptPackageID) != pickup->package || trigger != "complete" || !player ||
        !player->HasSceneStatus(pickup->scene, SCENE_COMPLETE) || !player->IsInWorld() ||
        !player->CanContact() || !player->IsAlive() || player->IsBeingTeleported() ||
        player->getClass() != CLASS_MONK || player->GetMapId() != pickup->map ||
        !ArtifactAcquisition::Active(player, pickup->quest) || !player->HasItemCount(pickup->item, 1, false))
        return true;
    Scenario* scenario = sScenarioMgr->GetScenario(player->GetInstanceId());
    return !scenario || scenario->GetScenarioId() != pickup->scenario ||
        scenario->GetCurrentStep() < pickup->firstStep || scenario->GetCurrentStep() > pickup->firstStep + 1;
}
}

class scene_monk_artifact_equipment : public SceneTriggerScript
{
public:
    scene_monk_artifact_equipment() : SceneTriggerScript("scene_monk_artifact_equipment") { }
    bool OnTrigger(Player* player, SpellScene const* scene, std::string trigger) override
    {
        return MonkArtifactScene::Denied(player, scene, trigger);
    }
};

class scene_xalatath_pickup : public SceneTriggerScript
{
public:
    scene_xalatath_pickup() : SceneTriggerScript("scene_xalatath_pickup") { }
    bool OnTrigger(Player* player, SpellScene const* scene, std::string trigger) override
    {
        return XalatathPresentation::Complete(player, scene, trigger);
    }
};

namespace XalatathPickup
{
bool Denied(Player* player, GameObject* object)
{
    if (!object)
        return true;
    if (object->GetEntry() != 247690)
        return false;
    if (!player || !player->IsInWorld() || !player->CanContact() || !player->IsAlive() || player->IsBeingTeleported() ||
        player->getClass() != CLASS_PRIEST || !ArtifactAcquisition::Active(player, 40710) ||
        !object->IsInWorld() || !object->isSpawned() || object->GetMapId() != 1539 ||
        object->HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_IN_USE | GO_FLAG_NOT_SELECTABLE) ||
        object->getLootState() == GO_JUST_DEACTIVATED || player->GetMap() != object->GetMap() ||
        !player->InSamePhase(object) || !player->canSeeOrDetect(object) || !player->IsWithinLOSInMap(object) ||
        player->GetExactDist(object) > 5.0f)
        return true;
    Scenario* scenario = sScenarioMgr->GetScenario(player->GetInstanceId());
    return !scenario || scenario->GetScenarioId() != 991 || scenario->GetCurrentStep() < 8 || scenario->GetCurrentStep() > 9;
}
}

class go_xalatath_pickup : public GameObjectScript
{
    struct AI : SmartGameObjectAI
    {
        explicit AI(GameObject* object) : SmartGameObjectAI(object) { }
        bool GossipHello(Player* player, bool isUse) override
        {
            if (XalatathPickup::Denied(player, go))
                return true;
            bool owned = go->GetEntry() == 247690 && player && player->HasItemCount(128827, 1, false);
            bool handled = SmartGameObjectAI::GossipHello(player, isUse);
            return owned || handled;
        }
    };
public:
    go_xalatath_pickup() : GameObjectScript("go_xalatath_pickup") { }
    bool OnGossipHello(Player* player, GameObject* object) override { return XalatathPickup::Denied(player, object); }
    GameObjectAI* GetAI(GameObject* object) const override { return new AI(object); }
};

namespace StromkarPickup
{
bool Denied(Player* player, GameObject* object)
{
    if (!object)
        return true;
    if (object->GetEntry() != 247877)
        return false;
    if (!player || !player->IsInWorld() || !player->CanContact() || !player->IsAlive() || player->IsBeingTeleported() ||
        player->getClass() != CLASS_WARRIOR || !ArtifactAcquisition::Active(player, 41105) ||
        !object->IsInWorld() || !object->isSpawned() || object->GetMapId() != 1539 ||
        object->HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_IN_USE | GO_FLAG_NOT_SELECTABLE) ||
        object->getLootState() == GO_JUST_DEACTIVATED || player->GetMap() != object->GetMap() ||
        !player->InSamePhase(object) || !player->canSeeOrDetect(object) || !player->IsWithinLOSInMap(object) ||
        player->GetExactDist(object) > 5.0f)
        return true;
    Scenario* scenario = sScenarioMgr->GetScenario(player->GetInstanceId());
    return !scenario || scenario->GetScenarioId() != 1037 || scenario->GetCurrentStep() != 9;
}
}

class go_stromkar_pickup : public GameObjectScript
{
    struct AI : SmartGameObjectAI
    {
        explicit AI(GameObject* object) : SmartGameObjectAI(object) { }
        bool GossipHello(Player* player, bool isUse) override
        {
            if (StromkarPickup::Denied(player, go))
                return true;
            return SmartGameObjectAI::GossipHello(player, isUse);
        }
    };
public:
    go_stromkar_pickup() : GameObjectScript("go_stromkar_pickup") { }
    bool OnGossipHello(Player* player, GameObject* object) override { return StromkarPickup::Denied(player, object); }
    GameObjectAI* GetAI(GameObject* object) const override { return new AI(object); }
};

namespace StromkarPresentation
{
bool Complete(Player* player, SpellScene const* scene, std::string const& trigger)
{
    if (!scene || scene->MiscValue != 1225 || scene->SceneScriptPackageID != 1590)
        return false;
    if (trigger != "complete" || !player || !player->HasSceneStatus(1225, SCENE_COMPLETE) ||
        !player->IsInWorld() || !player->CanContact() || !player->IsAlive() || player->IsBeingTeleported() ||
        player->getClass() != CLASS_WARRIOR || player->GetMapId() != 1539 ||
        !ArtifactAcquisition::Active(player, 41105) || !player->GetItemByEntry(128910))
        return true;
    Scenario* scenario = sScenarioMgr->GetScenario(player->GetInstanceId());
    if (!scenario || scenario->GetScenarioId() != 1037 || scenario->GetCurrentStep() != 9)
        return true;
    SpellCastResult result = player->CastSpell(player, 206439, true);
    if (result != SPELL_CAST_OK)
    {
        TC_LOG_WARN("scripts", "Stromkar pickup: native equip 206439 failed with result %u for player %u; existing item retained", uint32(result), player->GetGUIDLow());
        return true;
    }
    Item* weapon = player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND);
    if (!weapon || weapon->GetEntry() != 128910)
        TC_LOG_WARN("scripts", "Stromkar pickup: native equip 206439 accepted but blade is not equipped for player %u; retain native inventory errors", player->GetGUIDLow());
    return true;
}
}

class scene_stromkar_pickup : public SceneTriggerScript
{
public:
    scene_stromkar_pickup() : SceneTriggerScript("scene_stromkar_pickup") { }
    bool OnTrigger(Player* player, SpellScene const* scene, std::string trigger) override
    {
        return StromkarPresentation::Complete(player, scene, trigger);
    }
};

#include "stromkar_return.h"

struct npc_stromkar_thoradin : SmartAI
{
    explicit npc_stromkar_thoradin(Creature* actor) : SmartAI(actor) { }
    bool initialized = false;
    bool preserveIntro = false;
    bool introFinished = false;
    bool finalePending = false;
    bool finaleDispatched = false;
    StromkarReturn::State returnState;
    void IsSummonedBy(Unit* summoner) override
    {
        if (!initialized)
        {
            initialized = true;
            Scenario* scenario = sScenarioMgr->GetScenario(me->GetInstanceId());
            preserveIntro = me->GetEntry() == 104307 && me->GetMapId() == 1539 && scenario &&
                scenario->GetScenarioId() == 1037 && scenario->GetCurrentStep() == 9;
        }
        SmartAI::IsSummonedBy(summoner);
    }
    void SetData(uint32 id, uint32 value) override
    {
        StromkarReturn::Signal(me, id, value, returnState);
        if (!preserveIntro || value != 1 || (id != 1 && id != 2))
        {
            SmartAI::SetData(id, value);
            return;
        }
        if (id == 2)
            introFinished = true;
        else
            finalePending = true;
        if (introFinished && finalePending && !finaleDispatched)
        {
            finaleDispatched = true;
            SmartAI::SetData(1, 1);
        }
    }
    void UpdateAI(uint32 diff) override
    {
        SmartAI::UpdateAI(diff);
        StromkarReturn::Update(me, diff, returnState);
    }
};

#include "lights_wrath_takeover.h"
#include "lights_wrath_pickup.h"

#include "stromkar_awakening.h"
#include "stromkar_ritual_beam.h"
#include "monk_kite_travel.h"
#include "valzuun_intel.h"
#include "stromkar_split.h"

void AddSC_artifact_acquisition()
{
    RegisterCreatureAI(npc_stromkar_illusionist);
    RegisterSpellScript(spell_stromkar_split_transport);
    new npc_stromkar_valkyr();
    new condition_stromkar_return();
    RegisterSpellScript(spell_monk_kite_travel);
    RegisterCreatureAI(npc_valzuun_intel);
    RegisterGameObjectAI(go_lights_wrath_pickup);
    RegisterSpellScript(spell_lights_wrath_pickup_cast);
    RegisterAuraScript(spell_lights_wrath_pickup_scene);
    RegisterGameObjectAI(go_lights_wrath_return);
    RegisterSpellScript(spell_lights_wrath_return);
    RegisterCreatureAI(npc_lights_wrath_takeover);
    RegisterSpellScript(spell_lights_wrath_control_request);
    RegisterAuraScript(spell_lights_wrath_control_channel);
    RegisterSpellScript(spell_lights_wrath_control_result);
    RegisterCreatureAI(npc_marksmanship_qormaladon);
    new condition_marksmanship_qormaladon_ready();
    RegisterCreatureAI(npc_talonclaw_dakarr);
    RegisterCreatureAI(npc_talonclaw_ambush_dakarr);
    new eventobject_talonclaw_ambush();
    new player_talonclaw_ambush_event();
    RegisterCreatureAI(npc_titanstrike_final_prustaga);
    RegisterCreatureAI(npc_xalatath_farthing);
    RegisterCreatureAI(npc_stromkar_zakajz);
    RegisterCreatureAI(npc_stromkar_awakening);
    RegisterSpellScript(spell_stromkar_opening_recoil);
    RegisterSpellScript(spell_stromkar_ritual_beam);
    RegisterCreatureAI(npc_stromkar_thoradin);
    new condition_stromkar_thoradin_absence();
    RegisterCreatureAI(npc_xalatath_soul_attack);
    RegisterSpellScript(spell_titanstrike_aftermath);
    RegisterSpellScript(spell_titanstrike_combat_dialogue);
    new go_artifact_acquisition();
    new go_marksmanship_pickup();
    new player_dark_riders_completion();
    RegisterAuraScript(spell_talonclaw_presentation);
    RegisterAuraScript(spell_marksmanship_presentation);
    RegisterSpellScript(spell_marksmanship_return);
    new player_talonclaw_return();
    new player_talonclaw_departure();
    new go_talonclaw_return_portal();
    new scene_blood_artifact_equipment();
    new scene_monk_artifact_equipment();
    new scene_xalatath_pickup();
    new scene_stromkar_pickup();
    new go_stromkar_pickup();
    new go_xalatath_pickup();
}
