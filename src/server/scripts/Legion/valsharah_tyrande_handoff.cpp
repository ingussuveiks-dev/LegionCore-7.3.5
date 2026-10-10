#include "ScriptMgr.h"
#include "Player.h"
#include "ObjectAccessor.h"
#include "TemporarySummon.h"
#include "QuestDef.h"
#include "SpellScript.h"

namespace ValsharahHandoff
{
// Existing destination of 206723, after the corruption movie (473).
Position const Arrival = {2891.17f,5894.22f,297.23f,6.02139f};

bool Needed(Player* player)
{
    return player->GetMapId() == 1220 &&
        (player->GetQuestStatus(38753) == QUEST_STATUS_COMPLETE ||
         (player->GetQuestRewardStatus(38753) && !player->GetQuestRewardStatus(41054) && !player->GetQuestRewardStatus(41056)));
}

void EnsureTyrande(Player* player)
{
    if (!Needed(player) || !player->IsAlive() || player->IsBeingTeleported() ||
        player->isWatchingMovie() || player->GetDistance(Arrival) > 80.0f)
        return;
    for (ObjectGuid const& guid : *player->GetSummonList(102938))
        if (Creature* npc = ObjectAccessor::GetCreature(*player, guid))
            if (npc->IsAlive()) return;
    // Recover the native quest ender at the existing arrival point. The NPC
    // is personal; another player's movie or quest cannot remove it.
    player->SummonCreature(102938, Arrival, TEMPSUMMON_MANUAL_DESPAWN, 0, 0, player->GetGUID());
}

void Clear(Player* player)
{
    auto guids = *player->GetSummonList(102938);
    for (ObjectGuid const& guid : guids)
        if (Creature* npc = ObjectAccessor::GetCreature(*player, guid))
            if (npc->ToTempSummon() && npc->ToTempSummon()->GetSummoner() == player)
                npc->DespawnOrUnsummon();
}

void Recover(Player* player)
{
    if (!Needed(player)) { Clear(player); return; }
    if (!player->IsAlive()) Clear(player);
    EnsureTyrande(player);
    // Player-owned events disappear on logout. Keep recovery available after
    // death, temporary absence or a grid unload until Love Lost is rewarded.
    player->AddDelayedEvent(5000, [player]() { Recover(player); });
}

void AfterMovie(Player* player, uint32 retries)
{
    if (!Needed(player)) return;
    if (player->isWatchingMovie() || player->IsBeingTeleported() || !player->IsAlive())
    {
        if (retries) player->AddDelayedEvent(500, [player,retries]() { AfterMovie(player,retries-1); });
        else Recover(player);
        return;
    }
    if (player->GetDistance(Arrival) > 80.0f)
    {
        Recover(player);
        return;
    }
    player->CastSpell(player, 218440, true); // native follow-up scene 1350
    // The native scene emits TYRANDE at 10s and ends at 12s. Completion or
    // cancellation must not strand a quest whose native search credit was
    // already earned. This fallback grants no objectives.
    player->AddDelayedEvent(15000, [player]() { Recover(player); });
}
}

class spell_valsharah_corruption_handoff : public SpellScript
{
    PrepareSpellScript(spell_valsharah_corruption_handoff);
    void Start()
    {
        if (Player* player = GetCaster()->ToPlayer())
            if (ValsharahHandoff::Needed(player))
                player->AddDelayedEvent(500, [player]() { ValsharahHandoff::AfterMovie(player,600); });
    }
    void Register() override { AfterCast += SpellCastFn(spell_valsharah_corruption_handoff::Start); }
};

class scene_valsharah_tyrande_handoff : public SceneTriggerScript
{
public:
    scene_valsharah_tyrande_handoff() : SceneTriggerScript("scene_valsharah_tyrande_handoff") { }
    bool OnTrigger(Player* player, SpellScene const* scene, std::string trigger) override
    {
        if (scene->MiscValue != 1350) return false;
        if (trigger == "TYRANDE") ValsharahHandoff::EnsureTyrande(player);
        return true;
    }
};

class player_valsharah_tyrande_handoff : public PlayerScript
{
public:
    player_valsharah_tyrande_handoff() : PlayerScript("player_valsharah_tyrande_handoff") { }
    void OnUpdate(Player* player, uint32) override
    {
        if (!ValsharahHandoff::Needed(player) || !player->IsAlive())
            ValsharahHandoff::Clear(player);
    }
    void OnLogin(Player* player) override
    {
        if (ValsharahHandoff::Needed(player))
            player->AddDelayedEvent(12000, [player]() { ValsharahHandoff::Recover(player); });
    }
    void OnMapChanged(Player* player) override
    {
        ValsharahHandoff::Clear(player);
        OnLogin(player);
    }
    void OnLogout(Player* player) override { ValsharahHandoff::Clear(player); }
    void OnQuestReward(Player* player, Quest const* quest) override
    {
        if (quest->GetQuestId() == 41054 || quest->GetQuestId() == 41056)
            ValsharahHandoff::Clear(player);
    }
};

void AddSC_valsharah_tyrande_handoff()
{
    RegisterSpellScript(spell_valsharah_corruption_handoff);
    new scene_valsharah_tyrande_handoff();
    new player_valsharah_tyrande_handoff();
}
