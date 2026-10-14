#include "ScriptMgr.h"
#include "SmartAI.h"
#include "Player.h"
#include "ObjectAccessor.h"
#include "QuestData.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include <map>
#include <memory>

namespace StormheimBlessing
{
constexpr uint32 QuestId = 39597, MenuId = 18672;
struct Offer { ObjectGuid source; std::weak_ptr<uint8> token; };
std::map<ObjectGuid, Offer> offers;
std::map<ObjectGuid, ObjectGuid> ceremonies, activating;
bool Scope(WorldObject* object)
{
    return object && object->IsInWorld() && object->GetMapId() == 1220 && object->GetZoneId() == 7541;
}
bool Active(Player* player)
{
    return Scope(player) && player->IsAlive() && !player->IsBeingTeleported() &&
        player->GetQuestStatus(QuestId) == QUEST_STATUS_INCOMPLETE;
}
bool Interact(Player* player, Creature* creature)
{
    return Active(player) && Scope(creature) && creature->IsAlive() && !creature->IsQuestObjectExpired() &&
        creature->HasFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP) &&
        player->InSamePhase(creature) && player->canSeeOrDetect(creature) &&
        player->IsWithinDistInMap(creature, 5.0f) && player->IsWithinLOSInMap(creature);
}
void Credit(Player* player)
{
    if (!Active(player) || player->GetQuestObjectiveData(QuestId, 96285)) return;
    if (Quest const* quest = sQuestDataStore->GetQuestTemplate(QuestId))
        for (QuestObjective const& objective : quest->GetObjectives())
            if (objective.Type == QUEST_OBJECTIVE_MONSTER && objective.ObjectID == 96285)
            {
                player->SetQuestObjectiveData(quest, &objective, 1);
                player->SendQuestUpdateAddCredit(quest, ObjectGuid::Empty, objective, 1);
                if (player->CanCompleteQuest(QuestId)) player->CompleteQuest(QuestId);
                return;
            }
}
}

struct npc_stormheim_blessing_actor : SmartAI
{
    explicit npc_stormheim_blessing_actor(Creature* creature) : SmartAI(creature) { }
    ObjectGuid owner, original;
    std::shared_ptr<uint8> token;
    std::weak_ptr<QuestObjectScope> scope;
    uint32 stage = 0, timer = 6000, age = 0;
    bool scoped = false, stopped = false, applied = false, expired = false;
    ~npc_stormheim_blessing_actor()
    {
        Cleanup();
        auto found = StormheimBlessing::ceremonies.find(owner);
        if (found != StormheimBlessing::ceremonies.end() && found->second == me->GetGUID())
            StormheimBlessing::ceremonies.erase(found);
    }
    Player* Owner() const { return ObjectAccessor::GetPlayer(*me, owner); }
    bool Valid() const
    {
        Player* player = Owner();
        return !stopped && StormheimBlessing::Active(player) && token && me->IsAlive() &&
            player->getQuestStatus(StormheimBlessing::QuestId)->ScriptLifetime == token &&
            !me->IsQuestObjectExpired() && player->InSamePhase(me) && player->IsWithinDistInMap(me, 15.0f);
    }
    void Cleanup()
    {
        if (!scoped) return;
        // FindPlayer also reaches the owner after changing maps. Remove only
        // this actor's roots, never a newer ceremony or another caster's aura.
        if (Player* player = ObjectAccessor::FindPlayer(owner, false))
        {
            player->RemoveAurasDueToSpell(190904, me->GetGUID());
            player->RemoveAurasDueToSpell(190905, me->GetGUID());
        }
        auto found = StormheimBlessing::ceremonies.find(owner);
        if (found != StormheimBlessing::ceremonies.end() && found->second != me->GetGUID()) return;
        if (Creature* source = ObjectAccessor::GetCreature(*me, original))
        {
            source->RemoveFromHideList(owner);
            if (Player* player = Owner()) player->UpdateVisibilityForPlayer();
        }
        original = ObjectGuid::Empty;
    }
    void Stop()
    {
        if (stopped) return;
        stopped = true;
        Cleanup();
        me->RemoveAurasDueToSpell(190904, me->GetGUID());
        if (auto objects = scope.lock()) objects->Cancel();
        Player* player = Owner();
        if (player && player->getQuestStatus(StormheimBlessing::QuestId) &&
            player->getQuestStatus(StormheimBlessing::QuestId)->ScriptLifetime == token)
            player->CancelQuestObjects(StormheimBlessing::QuestId);
        me->DespawnOrUnsummon();
    }
    void IsSummonedBy(Unit* summoner) override
    {
        using namespace StormheimBlessing;
        Player* player = summoner ? summoner->ToPlayer() : nullptr;
        scoped = me->GetEntry() == 96257 && Scope(me) && Active(player) && activating.count(player->GetGUID());
        if (!scoped) { SmartAI::IsSummonedBy(summoner); return; }
        owner = player->GetGUID(); original = activating.at(owner);
        player->BindQuestObject(QuestId, me);
        token = player->getQuestStatus(QuestId)->ScriptLifetime;
        scope = player->getQuestStatus(QuestId)->ScriptObjects;
        me->RemoveFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP | UNIT_NPC_FLAG_QUESTGIVER);
        Talk(0, owner);
    }
    void sQuestAccept(Player* player, Quest const* quest) override
    {
        // Accepting the quest does not mean accepting the gift. Suppress the
        // old shared timed list; the native readiness option starts it.
        if (StormheimBlessing::Scope(me) && quest->GetQuestId() == StormheimBlessing::QuestId) return;
        SmartAI::sQuestAccept(player, quest);
    }
    void sGossipSelect(Player* player, uint32 menu, uint32 option) override
    {
        using namespace StormheimBlessing;
        if (!Scope(me) || menu != MenuId) { SmartAI::sGossipSelect(player, menu, option); return; }
        auto offered = offers.find(player->GetGUID());
        auto offeredToken = offered == offers.end() ? nullptr : offered->second.token.lock();
        if (scoped || option != 0 || !Interact(player, me) || !offeredToken ||
            offered->second.source != me->GetGUID() || offeredToken != player->getQuestStatus(QuestId)->ScriptLifetime ||
            player->PlayerTalkClass->GetGossipMenu().GetMenuId() != MenuId ||
            !player->PlayerTalkClass->GetGossipMenu().GetItem(option)) return;
        offers.erase(offered);
        player->PlayerTalkClass->ClearMenus();
        player->PlayerTalkClass->SendCloseGossip();
        auto found = ceremonies.find(player->GetGUID());
        if (found != ceremonies.end())
            if (Creature* previous = ObjectAccessor::GetCreature(*player, found->second))
                if (auto ai = dynamic_cast<npc_stormheim_blessing_actor*>(previous->AI()))
                {
                    if (ai->Valid()) return;
                    ai->Stop();
                }
        player->CancelQuestObjects(QuestId);
        activating[player->GetGUID()] = me->GetGUID();
        if (Creature* actor = player->SummonCreature(96257, me->GetPositionX(), me->GetPositionY(),
            me->GetPositionZ(), me->GetOrientation(), TEMPSUMMON_MANUAL_DESPAWN, 0))
        {
            ceremonies[player->GetGUID()] = actor->GetGUID();
            me->AddToHideList(player->GetGUID());
            me->DestroyForPlayer(player);
        }
        activating.erase(player->GetGUID());
    }
    void JustDied(Unit* killer) override
    {
        if (!scoped) { SmartAI::JustDied(killer); return; }
        Stop();
    }
    void UpdateAI(uint32 diff) override
    {
        if (!scoped) { SmartAI::UpdateAI(diff); return; }
        if (!Valid()) { Stop(); return; }
        age += diff;
        if (age >= 60000) { Stop(); return; }
        if (stage == 1 && (!me->GetAura(190904, me->GetGUID()) || !Owner()->GetAura(190904, me->GetGUID())))
        { Stop(); return; }
        if (timer > diff) { timer -= diff; return; }
        if (stage == 0)
        {
            Talk(1, owner);
            stage = 1;
            // Native target 92 uses the explicit object target. Both the
            // caster and exact player must actually receive aura190904.
            if (me->CastSpell(Owner(), 190904, true) != SPELL_CAST_OK ||
                !me->GetAura(190904, me->GetGUID()) || !Owner()->GetAura(190904, me->GetGUID()))
            { Stop(); return; }
            timer = 6000;
        }
        else if (stage == 1)
        {
            me->RemoveAurasDueToSpell(190904, me->GetGUID());
            Owner()->RemoveAurasDueToSpell(190904, me->GetGUID());
            stage = 2;
            if (me->CastSpell(Owner(), 190905, true) != SPELL_CAST_OK || !applied)
            { Stop(); return; }
            Talk(2, owner); timer = 6000;
        }
        else if (stage == 2)
        {
            // An accepted cast or a elapsed timer is not proof of its actual
            // two-second aura expiry. A dispel/cancel never grants credit.
            if (!expired)
            {
                if (!Owner()->GetAura(190905, me->GetGUID())) { Stop(); return; }
                timer = 100;
                return;
            }
            stage = 3;
            Talk(3, owner);
            StormheimBlessing::Credit(Owner());
            Stop();
        }
    }
};

class aura_stormheim_blessing_finish : public AuraScript
{
    PrepareAuraScript(aura_stormheim_blessing_finish);
    void Applied(AuraEffect const*, AuraEffectHandleModes)
    {
        if (auto ai = Actor()) ai->applied = true;
    }
    void Removed(AuraEffect const*, AuraEffectHandleModes)
    {
        if (GetTargetApplication()->GetRemoveMode() == AURA_REMOVE_BY_EXPIRE)
            if (auto ai = Actor()) ai->expired = ai->applied;
    }
    npc_stormheim_blessing_actor* Actor()
    {
        Creature* caster = GetCaster() ? GetCaster()->ToCreature() : nullptr;
        auto ai = caster ? dynamic_cast<npc_stormheim_blessing_actor*>(caster->AI()) : nullptr;
        return ai && ai->scoped && ai->Valid() && ai->stage == 2 && GetTarget()->GetGUID() == ai->owner ? ai : nullptr;
    }
    void Register() override
    {
        AfterEffectApply += AuraEffectApplyFn(aura_stormheim_blessing_finish::Applied, EFFECT_0, SPELL_AURA_MOD_ROOT, AURA_EFFECT_HANDLE_REAL);
        AfterEffectRemove += AuraEffectRemoveFn(aura_stormheim_blessing_finish::Removed, EFFECT_0, SPELL_AURA_MOD_ROOT, AURA_EFFECT_HANDLE_REAL);
    }
};

class npc_stormheim_blessing : public CreatureScript
{
public:
    npc_stormheim_blessing() : CreatureScript("npc_stormheim_blessing") { }
    bool OnGossipHello(Player* player, Creature* creature) override
    {
        using namespace StormheimBlessing;
        if (!Scope(creature)) return false;
        auto ai = dynamic_cast<npc_stormheim_blessing_actor*>(creature->AI());
        if (ai && ai->scoped) return true;
        offers.erase(player->GetGUID());
        if (Interact(player, creature))
        {
            player->AddQuestDelayedEvent(QuestId, 0, [] { });
            offers[player->GetGUID()] = {creature->GetGUID(), player->getQuestStatus(QuestId)->ScriptLifetime};
        }
        return false; // Native menu/quest list and localized ready option.
    }
    CreatureAI* GetAI(Creature* creature) const override { return new npc_stormheim_blessing_actor(creature); }
};

class player_stormheim_blessing : public PlayerScript
{
public:
    player_stormheim_blessing() : PlayerScript("player_stormheim_blessing") { }
    void OnLogout(Player* player) override
    {
        StormheimBlessing::offers.erase(player->GetGUID());
        auto found = StormheimBlessing::ceremonies.find(player->GetGUID());
        if (found != StormheimBlessing::ceremonies.end())
            if (Creature* actor = ObjectAccessor::GetCreature(*player, found->second))
                if (auto ai = dynamic_cast<npc_stormheim_blessing_actor*>(actor->AI())) ai->Stop();
    }
};

void AddSC_stormheim_blessing()
{
    new npc_stormheim_blessing();
    new player_stormheim_blessing();
    RegisterAuraScript(aura_stormheim_blessing_finish);
}
