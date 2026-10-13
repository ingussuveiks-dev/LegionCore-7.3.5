#include "stormheim_rounds.h"
#include "stormheim_departure.h"
#include "ScriptMgr.h"
#include "SmartAI.h"
#include "SpellScript.h"
#include "Player.h"
#include "ObjectAccessor.h"
#include "TemporarySummon.h"
#include "QuestData.h"
#include "CreatureTextMgr.h"

namespace StormheimRounds
{
struct Route
{
    uint32 quest, team, map, guide, inspection, salute, finalNpc, bar, firstPath;
    Position start;
};
Route const AllianceRoute = {38206,ALLIANCE,0,96644,96662,96661,96663,220526,966440,{-8491.16f,1394.03f,196.457f,3.25092f}};
Route const HordeRoute = {39698,HORDE,1,96686,247491,96688,96689,220293,966860,{1497.14f,-5207.8f,11.2774f,1.81966f}};
Route const& For(Player* player) { return player->GetTeam() == ALLIANCE ? AllianceRoute : HordeRoute; }
bool Active(Player* player, Route const& route)
{
    return player && player->IsInWorld() && player->IsAlive() && player->GetTeam() == route.team &&
        player->GetMapId() == route.map && player->GetQuestStatus(route.quest) == QUEST_STATUS_INCOMPLETE &&
        !player->IsBeingTeleported() && !player->GetVehicle() && !player->GetTransport() && !player->isInFlight() && !player->IsFlying();
}
bool Interact(Player* player, WorldObject* object, float range)
{
    return player->InSamePhase(object) && player->canSeeOrDetect(object) &&
        player->GetExactDist(object) <= range && player->IsWithinLOSInMap(object);
}
Creature* Existing(Player* player, Route const& route)
{
    auto guids = *player->GetSummonList(route.guide);
    for (ObjectGuid const& guid : guids)
        if (Creature* guide = ObjectAccessor::GetCreature(*player,guid))
            if (guide->IsAlive() && !guide->IsDespawn() && guide->AI()->GetData(route.quest)) return guide;
    return nullptr;
}
void Ensure(Player* player)
{
    Route const& route = For(player);
    if (!Active(player,route) || player->isInCombat() || player->GetExactDist(&route.start) > 120.0f) return;
    // Old timed lists could award the final conversation before inspection or
    // salute. Reconcile only that invalid partial progress, preserving valid work.
    if (player->GetQuestObjectiveData(route.quest,route.finalNpc) &&
        (!player->GetQuestObjectiveData(route.quest,route.inspection) || !player->GetQuestObjectiveData(route.quest,route.salute)))
        if (Quest const* quest = sQuestDataStore->GetQuestTemplate(route.quest))
            for (QuestObjective const& objective : quest->GetObjectives())
                if (objective.ObjectID == route.finalNpc) player->SetQuestObjectiveData(quest,&objective,0);
    if (Existing(player,route)) return;
    player->RemoveAurasDueToSpell(route.bar);
    Position start = route.start;
    uint32 part = player->GetQuestObjectiveData(route.quest,route.salute) ? 2 : player->GetQuestObjectiveData(route.quest,route.inspection) ? 1 : 0;
    if (part)
        if (WPPath* path = sSmartWaypointMgr->GetPath(route.firstPath+part))
        {
            auto first = path->find(1);
            if (first != path->end()) start.Relocate(first->second->x,first->second->y,first->second->z);
        }
    player->SummonCreature(route.guide,start.GetPositionX(),start.GetPositionY(),start.GetPositionZ(),start.GetOrientation(),TEMPSUMMON_TIMED_DESPAWN,300000);
}

bool Controller::Start(Unit* summoner)
{
    Player* player = summoner ? summoner->ToPlayer() : nullptr;
    if (!player) return false;
    Route const& data = For(player);
    if (me->GetEntry() != data.guide || me->GetMapId() != data.map) return false;
    if (!Active(player,data) || Existing(player,data)) { me->DespawnOrUnsummon(); return true; }
    route = &data; owner = player->GetGUID();
    player->AddQuestDelayedEvent(route->quest,0,[] { });
    lifetime = player->getQuestStatus(route->quest)->ScriptLifetime;
    part = player->GetQuestObjectiveData(route->quest,route->salute) ? 2 : player->GetQuestObjectiveData(route->quest,route->inspection) ? 1 : 0;
    me->AddPlayerInPersonnalVisibilityList(owner);
    me->SetUInt32Value(UNIT_FIELD_NPC_FLAGS,0);
    me->SetReactState(REACT_PASSIVE); me->CombatStop(true); me->SetWalk(true); me->setActive(true);
    me->CastSpell(me,57764,true); // Keep the existing animation override.
    return true;
}
void Controller::Clear()
{
    if (Player* player = ObjectAccessor::GetPlayer(*me,owner))
        if (!lifetime.expired()) player->RemoveAurasDueToSpell(route->bar);
    me->StopMoving(); me->DespawnOrUnsummon();
}
uint32 Controller::GetData(uint32 id) const
{
    if (!route || lifetime.expired()) return 0;
    if (id == route->quest) return part == 3 ? (pending ? 5 : 4) : part+1;
    if (id == route->quest+100000) return part == 0 && arrived;
    if (id == route->quest+200000) return part == 1 && arrived;
    return 0;
}
void Controller::SetData(uint32 id, uint32 value)
{
    if (route && id == route->quest && part == 3) pending = value == 5;
}
void Controller::Movement(uint32 type, uint32 id)
{
    if (!route || !moving || type != POINT_MOTION_TYPE || id != token) return;
    WPPath* path = sSmartWaypointMgr->GetPath(route->firstPath+part);
    if (!path) return;
    auto point = path->find(node);
    if (point == path->end()) return;
    Position expected = {point->second->x,point->second->y,point->second->z,0};
    if (me->GetExactDist(&expected) > 1.0f) return;
    moving = false;
    if (++node > path->size()) arrived = true;
}
void Controller::Update(uint32 diff)
{
    Player* player = ObjectAccessor::GetPlayer(*me,owner);
    if (lifetime.expired() || !Active(player,*route) || !me->IsAlive() || !player->InSamePhase(me) ||
        !player->canSeeOrDetect(me) || player->GetExactDist(me) > 80.0f) { Clear(); return; }
    if (part == 3) return;
    // Do not consume movement or dialogue while the owner is in combat/away.
    if (player->isInCombat() || player->GetExactDist(me) > 40.0f)
    {
        if (moving) { me->StopMoving(); moving = false; ++token; }
        return;
    }
    me->CombatStop(true); me->SetReactState(REACT_PASSIVE);
    static uint32 const firstGroups[2][3] = {{0,4,7},{0,3,6}};
    static uint32 const counts[2][3] = {{4,3,4},{3,3,4}};
    static uint32 const gaps[2][3][4] = {{{4500,6000,8000,8000},{6000,8000,7000,0},{12000,6000,6000,0}},
                                       {{10000,8000,7000,0},{6000,12000,8000,0},{12000,6000,6000,6000}}};
    uint32 faction = route->team == ALLIANCE ? 0 : 1;
    if (talkTimer > diff) talkTimer -= diff;
    else if (speech < counts[faction][part])
    {
        sCreatureTextMgr->SendChat(me,firstGroups[faction][part]+speech,owner,CHAT_MSG_MONSTER_WHISPER);
        talkTimer = gaps[faction][part][speech++];
    }
    else talkTimer = 0;
    if (!arrived)
    {
        if (moving)
        {
            if (moveTimer > diff) { moveTimer -= diff; return; }
            me->StopMoving(); moving = false; // Retry a failed movement, never credit it.
        }
        WPPath* path = sSmartWaypointMgr->GetPath(route->firstPath+part);
        if (!path) return;
        auto point = path->find(node);
        if (point == path->end()) return;
        moving = true; moveTimer = 30000; ++token;
        // The capital ship decks are absent from the ground navmesh. Follow
        // their retained short deck waypoints without projecting them ashore.
        me->GetMotionMaster()->MovePoint(token,point->second->x,point->second->y,point->second->z,false);
        return;
    }
    if (speech < counts[faction][part] || talkTimer) return;
    if (part == 0 && !player->GetQuestObjectiveData(route->quest,route->inspection)) return;
    if (part == 1 && !player->GetQuestObjectiveData(route->quest,route->salute))
    {
        if (!player->HasAura(route->bar) && Interact(player,me,12.0f)) player->CastSpell(player,route->bar,true);
        return;
    }
    ++part; node = 1; speech = 0; talkTimer = 0; arrived = false;
}

bool CanInspect(Player* player, Creature* gun)
{
    if (!Active(player,AllianceRoute) || !gun || gun->GetEntry() != 110898 || !gun->IsAlive() || gun->GetMapId() != 0 ||
        player->isInCombat() || player->GetQuestObjectiveData(38206,96662) || !player->IsWithinDistInMap(gun,5.0f) ||
        !Interact(player,gun,8.0f)) return false; // The raised gun model's origin is above the deck; retain normal unit interaction reach.
    Creature* guide = Existing(player,AllianceRoute);
    return guide && guide->AI()->GetData(138206) && Interact(player,guide,40.0f);
}
bool CanSalute(Player* player)
{
    if (!player) return false;
    Route const& route = For(player);
    if (!Active(player,route) || player->isInCombat() || !player->HasAura(route.bar) || !player->GetQuestObjectiveData(route.quest,route.inspection) ||
        player->GetQuestObjectiveData(route.quest,route.salute)) return false;
    Creature* guide = Existing(player,route);
    return guide && guide->AI()->GetData(route.quest+200000) && Interact(player,guide,12.0f);
}
void Credit(Player* player, Route const& route, uint32 entry)
{
    if (Quest const* quest = sQuestDataStore->GetQuestTemplate(route.quest))
        for (QuestObjective const& objective : quest->GetObjectives())
            if (objective.Type == QUEST_OBJECTIVE_MONSTER && objective.ObjectID == entry && !player->GetQuestObjectiveData(route.quest,entry))
            {
                player->SetQuestObjectiveData(quest,&objective,1);
                player->SendQuestUpdateAddCredit(quest,ObjectGuid::Empty,objective,1);
                if (player->CanCompleteQuest(route.quest)) player->CompleteQuest(route.quest);
                else player->SetQuestStatus(route.quest,QUEST_STATUS_INCOMPLETE);
                return;
            }
}
void Finish(Player* player, ObjectGuid finalGuid, ObjectGuid guideGuid, uint32 step)
{
    Route const& route = For(player);
    if (!Active(player,route)) return;
    Creature* guide = Existing(player,route);
    if (!guide || guide->GetGUID() != guideGuid || guide->AI()->GetData(route.quest) != 5) return;
    Creature* target = ObjectAccessor::GetCreature(*player,finalGuid);
    if (player->isInCombat() || !target || !target->IsAlive() || target->GetMapId() != route.map ||
        !Interact(player,target,8.0f)) { guide->AI()->SetData(route.quest,4); return; }
    if (step == 0 && route.team == ALLIANCE)
    {
        sCreatureTextMgr->SendChat(target,1,player->GetGUID(),CHAT_MSG_MONSTER_WHISPER);
        player->AddQuestDelayedEvent(route.quest,5000,[player,finalGuid,guideGuid]() { Finish(player,finalGuid,guideGuid,1); });
        return;
    }
    Credit(player,route,route.finalNpc);
}
void FinalHello(Player* player, Creature* target)
{
    Route const& route = For(player);
    if (!Active(player,route) || player->isInCombat() || target->GetEntry() != route.finalNpc || target->GetMapId() != route.map || !target->IsAlive() || !Interact(player,target,6.0f) ||
        !player->GetQuestObjectiveData(route.quest,route.inspection) || !player->GetQuestObjectiveData(route.quest,route.salute)) return;
    Creature* guide = Existing(player,route);
    if (!guide || guide->AI()->GetData(route.quest) != 4 || !Interact(player,guide,20.0f)) return;
    guide->AI()->SetData(route.quest,5);
    sCreatureTextMgr->SendChat(target,route.team == ALLIANCE ? 0 : 3,player->GetGUID(),CHAT_MSG_MONSTER_WHISPER);
    ObjectGuid guid = target->GetGUID();
    ObjectGuid guideGuid = guide->GetGUID();
    player->AddQuestDelayedEvent(route.quest,route.team == ALLIANCE ? 4000 : 6000,[player,guid,guideGuid]() { Finish(player,guid,guideGuid,0); });
}
bool CanInspectCatapult(Player* player, GameObject* catapult)
{
    if (!Active(player,HordeRoute) || player->isInCombat() || catapult->GetEntry() != 247491 || catapult->GetMapId() != 1 ||
        !catapult->isSpawned() || player->GetQuestObjectiveData(39698,247491) || !Interact(player,catapult,5.0f)) return false;
    Creature* guide = Existing(player,HordeRoute);
    return guide && guide->AI()->GetData(139698) && Interact(player,guide,40.0f);
}
bool CanStart(Player* player, uint32 spell)
{
    return player && spell == (player->GetTeam() == ALLIANCE ? 69333u : 69334u) && Active(player,For(player)) && !player->isInCombat();
}
struct Inspection
{
    std::weak_ptr<uint8> lifetime;
    bool captured = false;
    bool Check(Player* player, Creature* gun)
    {
        if (!CanInspect(player,gun)) return false;
        if (!captured)
        {
            player->AddQuestDelayedEvent(38206,0,[] { });
            lifetime = player->getQuestStatus(38206)->ScriptLifetime;
            captured = true;
        }
        return !lifetime.expired();
    }
    void Hit(Player* player, Creature* gun)
    {
        if (!lifetime.expired() && CanInspect(player,gun)) Credit(player,AllianceRoute,96662);
    }
};
}

class spell_stormheim_rounds_start : public SpellScript
{
    PrepareSpellScript(spell_stormheim_rounds_start);
    SpellCastResult Check() { return StormheimRounds::CanStart(GetCaster()->ToPlayer(),GetSpellInfo()->Id) ? SPELL_CAST_OK : SPELL_FAILED_BAD_TARGETS; }
    void Start(SpellEffIndex index)
    {
        PreventHitDefaultEffect(index);
        if (Player* player = GetCaster()->ToPlayer()) StormheimRounds::Ensure(player);
    }
    void Register() override { OnCheckCast += SpellCheckCastFn(spell_stormheim_rounds_start::Check); OnEffectHitTarget += SpellEffectFn(spell_stormheim_rounds_start::Start,EFFECT_0,SPELL_EFFECT_SEND_EVENT); }
};
class spell_stormheim_rounds_salute : public SpellScript
{
    PrepareSpellScript(spell_stormheim_rounds_salute);
    SpellCastResult Check()
    {
        Player* player = GetCaster()->ToPlayer();
        if (!player || GetSpellInfo()->Id != (player->GetTeam() == ALLIANCE ? 220525u : 220291u) || !StormheimRounds::CanSalute(player)) return SPELL_FAILED_BAD_TARGETS;
        return SPELL_CAST_OK;
    }
    void Register() override { OnCheckCast += SpellCheckCastFn(spell_stormheim_rounds_salute::Check); }
};
class spell_stormheim_inspect_gun : public SpellScript
{
    PrepareSpellScript(spell_stormheim_inspect_gun);
    StormheimRounds::Inspection inspection;
    SpellCastResult Check()
    {
        return inspection.Check(GetCaster()->ToPlayer(),GetExplTargetUnit() ? GetExplTargetUnit()->ToCreature() : nullptr) ? SPELL_CAST_OK : SPELL_FAILED_BAD_TARGETS;
    }
    void Hit(SpellEffIndex) { inspection.Hit(GetCaster()->ToPlayer(),GetHitCreature()); }
    void Register() override { OnCheckCast += SpellCheckCastFn(spell_stormheim_inspect_gun::Check); OnEffectHitTarget += SpellEffectFn(spell_stormheim_inspect_gun::Hit,EFFECT_0,SPELL_EFFECT_DUMMY); }
};
struct npc_stormheim_rounds_final : SmartAI
{
    explicit npc_stormheim_rounds_final(Creature* creature) : SmartAI(creature) { }
    void sGossipHello(Player* player) override { StormheimRounds::FinalHello(player,me); SmartAI::sGossipHello(player); }
    void sQuestAccept(Player* player, Quest const* quest) override { if (!StormheimDeparture::Accept(player,me,quest)) SmartAI::sQuestAccept(player,quest); }
};
class go_stormheim_inspect_catapult : public GameObjectScript
{
public:
    go_stormheim_inspect_catapult() : GameObjectScript("go_stormheim_inspect_catapult") { }
    bool OnGossipHello(Player* player, GameObject* go) override { return !StormheimRounds::CanInspectCatapult(player,go); }
};
class player_stormheim_rounds : public PlayerScript
{
public:
    player_stormheim_rounds() : PlayerScript("player_stormheim_rounds") { }
    std::unordered_map<ObjectGuid,uint32> timers;
    void OnUpdate(Player* player, uint32 diff) override
    {
        uint32& timer = timers[player->GetGUID()];
        if (timer > diff) { timer -= diff; return; }
        timer = 1000;
        StormheimRounds::Route const& route = StormheimRounds::For(player);
        if (!StormheimRounds::Active(player,route)) { if (player->HasAura(route.bar)) player->RemoveAurasDueToSpell(route.bar); return; }
        StormheimRounds::Ensure(player);
    }
    void OnLogout(Player* player) override { timers.erase(player->GetGUID()); player->RemoveAurasDueToSpell(StormheimRounds::For(player).bar); }
};
void AddSC_stormheim_rounds()
{
    RegisterSpellScript(spell_stormheim_rounds_start);
    RegisterSpellScript(spell_stormheim_rounds_salute);
    RegisterSpellScript(spell_stormheim_inspect_gun);
    RegisterCreatureAI(npc_stormheim_rounds_final);
    new go_stormheim_inspect_catapult();
    new player_stormheim_rounds();
}
