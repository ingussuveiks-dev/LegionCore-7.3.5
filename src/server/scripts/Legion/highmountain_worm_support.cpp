#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellScript.h"
#include "Player.h"
#include "TemporarySummon.h"
#include "ObjectAccessor.h"
#include <algorithm>
#include <map>
#include <mutex>

namespace WormSupport
{
constexpr uint32 Quest = 39776;
// Native spell area-trigger geometry was not present in the world database.
// These are bounded server reconstruction limits, not sniffed retail timings.
constexpr float Radius = 45.0f;
constexpr uint32 Duration = 45000;
struct Tribe { uint32 worm, objective, bar, call, ally, attack; };
Tribe const Tribes[] = {
    {97013,279939,195145,195144,99215,194019},
    {99216,280505,195148,195146,99219,195155},
    {99217,280504,195150,195149,99221,12548}
};
bool Active(Player* player)
{
    return player && player->IsInWorld() && player->IsAlive() && player->GetMapId() == 1220 &&
        player->GetQuestStatus(Quest) == QUEST_STATUS_INCOMPLETE && !player->IsBeingTeleported() &&
        !player->GetVehicle() && !player->GetTransport() && !player->isInFlight() && !player->IsFlying();
}
bool Eligible(Player* player, Creature* worm, Tribe const& tribe)
{
    return Active(player) && !player->GetQuestObjectiveData(Quest,tribe.worm) && worm &&
        worm->GetEntry() == tribe.worm && worm->IsAlive() && !worm->IsDespawn() &&
        player->InSamePhase(worm) && player->canSeeOrDetect(worm) &&
        player->IsWithinDistInMap(worm,Radius) && player->IsWithinLOSInMap(worm) &&
        player->IsValidAttackTarget(worm);
}
Creature* Nearest(Player* player, Tribe const*& chosen)
{
    chosen = nullptr;
    if (!Active(player)) return nullptr;
    Creature* result = nullptr;
    for (auto const& tribe : Tribes)
    {
        std::list<Creature*> worms;
        player->GetCreatureListWithEntryInGrid(worms,tribe.worm,Radius);
        for (Creature* worm : worms)
            if (Eligible(player,worm,tribe) && (!result || player->GetDistance(worm) < player->GetDistance(result)))
            { result = worm; chosen = &tribe; }
    }
    return result;
}
void Bars(Player* player, Tribe const* chosen = nullptr)
{
    for (auto const& tribe : Tribes)
        if (&tribe != chosen) player->RemoveAurasDueToSpell(tribe.bar);
    if (chosen && !player->HasAura(chosen->bar)) player->CastSpell(player,chosen->bar,true);
}
Player* Owner(Creature* ally)
{
    auto summon = ally->ToTempSummon();
    Unit* owner = summon ? summon->GetSummoner() : nullptr;
    return owner ? owner->ToPlayer() : nullptr;
}
bool HasAlly(Player* player, Tribe const& tribe)
{
    auto guids = *player->GetSummonList(tribe.ally);
    for (auto const& guid : guids)
        if (Creature* ally = ObjectAccessor::GetCreature(*player,guid))
            if (Owner(ally) == player && ally->IsAlive() && !ally->IsDespawn()) return true;
    return false;
}
void Clear(Player* player)
{
    Bars(player);
    for (auto const& tribe : Tribes)
    {
        auto guids = *player->GetSummonList(tribe.ally);
        for (auto const& guid : guids)
            if (Creature* ally = ObjectAccessor::GetCreature(*player,guid))
                if (Owner(ally) == player && ally->IsAIEnabled) ally->AI()->DoAction(1);
    }
}
}

struct npc_highmountain_worm_ally : ScriptedAI
{
    npc_highmountain_worm_ally(Creature* creature) : ScriptedAI(creature), summons(me) { }
    SummonList summons;
    ObjectGuid target;
    std::weak_ptr<uint8> lifetime;
    WormSupport::Tribe const* tribe = nullptr;
    uint32 elapsed = 0, attackTimer = 0;
    void IsSummonedBy(Unit* summoner) override
    {
        Player* player = summoner->ToPlayer();
        for (auto const& row : WormSupport::Tribes)
            if (row.ally == me->GetEntry()) tribe = &row;
        if (!tribe || !WormSupport::Active(player)) { me->DespawnOrUnsummon(); return; }
        player->AddQuestDelayedEvent(WormSupport::Quest,0,[] {});
        lifetime = player->getQuestStatus(WormSupport::Quest)->ScriptLifetime;
        // Direct SummonCreature has no SummonProperties: InitStats returns before
        // registering the summon. RemoveFromWorld still removes this GUID normally.
        auto list = player->GetSummonList(me->GetEntry());
        if (std::find(list->begin(),list->end(),me->GetGUID()) == list->end())
            list->push_back(me->GetGUID());
        // Damage attribution without registering a guardian or replacing the player's pet.
        me->SetOwnerGUID(player->GetGUID());
        me->setFaction(player->getFaction());
        me->RemoveFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC);
        me->SetReactState(REACT_PASSIVE);
        me->AddPlayerInPersonnalVisibilityList(player->GetGUID());
    }
    void SetGUID(ObjectGuid const& guid, int32) override { target = guid; }
    void JustSummoned(Creature* summon) override { summons.Summon(summon); }
    void Clean()
    {
        if (Creature* worm = ObjectAccessor::GetCreature(*me,target))
            worm->RemoveAurasDueToSpell(195155,me->GetGUID());
        summons.DespawnAll();
    }
    void DoAction(int32 action) override
    {
        if (action == 1) { Clean(); me->DespawnOrUnsummon(); }
    }
    void JustDied(Unit*) override { Clean(); }
    void UpdateAI(uint32 diff) override
    {
        elapsed += diff;
        Player* player = WormSupport::Owner(me);
        Creature* worm = ObjectAccessor::GetCreature(*me,target);
        if (!tribe || lifetime.expired() || elapsed >= WormSupport::Duration ||
            !WormSupport::Eligible(player,worm,*tribe))
        { DoAction(1); return; }
        if (!me->getVictim())
        {
            AttackStart(worm);
            if (tribe->ally == 99221) me->CastSpell(me,192412,true);
        }
        if (me->getVictim() != worm) { DoAction(1); return; }
        if (attackTimer <= diff)
        {
            if (!me->HasUnitState(UNIT_STATE_CASTING))
            {
                me->CastSpell(worm,tribe->attack,false);
                attackTimer = 5000;
            }
        }
        else attackTimer -= diff;
        DoMeleeAttackIfReady();
    }
};

class spell_highmountain_call_tribe : public SpellScript
{
    PrepareSpellScript(spell_highmountain_call_tribe);
    ObjectGuid target;
    WormSupport::Tribe const* tribe = nullptr;
    std::weak_ptr<uint8> lifetime;
    bool checked = false;
    SpellCastResult Check()
    {
        Player* player = GetCaster()->ToPlayer();
        if (checked)
        {
            // The core checks again after the cast time. Keep the original GUID,
            // including when a second spawn of the same entry is now nearer.
            Creature* worm = player ? ObjectAccessor::GetCreature(*player,target) : nullptr;
            return !lifetime.expired() && tribe && WormSupport::Eligible(player,worm,*tribe) &&
                player->HasAura(tribe->bar) && !WormSupport::HasAlly(player,*tribe)
                ? SPELL_CAST_OK : SPELL_FAILED_DONT_REPORT;
        }
        Creature* worm = WormSupport::Nearest(player,tribe);
        if (!worm || tribe->call != GetSpellInfo()->Id || !player->HasAura(tribe->bar) ||
            WormSupport::HasAlly(player,*tribe)) return SPELL_FAILED_DONT_REPORT;
        target = worm->GetGUID();
        if (!checked)
        {
            player->AddQuestDelayedEvent(WormSupport::Quest,0,[] {});
            lifetime = player->getQuestStatus(WormSupport::Quest)->ScriptLifetime;
            checked = true;
        }
        return SPELL_CAST_OK;
    }
    void Handle(SpellEffIndex index)
    {
        PreventHitDefaultEffect(index);
        Player* player = GetCaster()->ToPlayer();
        if (!player || !tribe || lifetime.expired()) return;
        Creature* worm = ObjectAccessor::GetCreature(*player,target);
        // Recheck after the native cast time: a stale button must not retarget a different worm.
        if (!WormSupport::Eligible(player,worm,*tribe) || WormSupport::HasAlly(player,*tribe)) return;
        if (Creature* ally = player->SummonCreature(tribe->ally,player->GetNearPosition(2.0f,0.0f),
            TEMPSUMMON_TIMED_DESPAWN,WormSupport::Duration+1000,0,player->GetGUID()))
        {
            if (ally->IsAIEnabled) ally->AI()->SetGUID(target,0);
            else ally->DespawnOrUnsummon();
        }
    }
    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_highmountain_call_tribe::Check);
        OnEffectHit += SpellEffectFn(spell_highmountain_call_tribe::Handle,EFFECT_0,SPELL_EFFECT_SEND_EVENT);
    }
};

class player_highmountain_worm_support : public PlayerScript
{
public:
    player_highmountain_worm_support() : PlayerScript("player_highmountain_worm_support") { }
    std::map<ObjectGuid,uint32> timers;
    std::mutex mutex;
    void OnUpdate(Player* player, uint32 diff) override
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (!WormSupport::Active(player))
        {
            // Also remove persisted bars after login with an abandoned/completed quest.
            WormSupport::Bars(player);
            if (timers.erase(player->GetGUID())) WormSupport::Clear(player);
            return;
        }
        uint32& timer = timers[player->GetGUID()];
        if (timer > diff) { timer -= diff; return; }
        timer = 500;
        WormSupport::Tribe const* tribe;
        WormSupport::Nearest(player,tribe);
        WormSupport::Bars(player,tribe);
    }
    void Reset(Player* player)
    {
        std::lock_guard<std::mutex> lock(mutex);
        timers.erase(player->GetGUID());
        WormSupport::Clear(player);
    }
    void OnLogin(Player* player) override { Reset(player); }
    void OnLogout(Player* player) override { Reset(player); }
    void OnMapChanged(Player* player) override { Reset(player); }
};

void AddSC_highmountain_worm_support()
{
    RegisterCreatureAI(npc_highmountain_worm_ally);
    RegisterSpellScript(spell_highmountain_call_tribe);
    new player_highmountain_worm_support();
}
