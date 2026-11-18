#pragma once

namespace KeepingThePeace
{
constexpr uint32 QuestId = 42771;
constexpr uint32 SummonSpell = 228199;
constexpr uint32 BossEntry = 110635;

bool Pending(Player* player)
{
    return player && player->IsInWorld() && player->CanContact() && player->IsAlive() &&
        !player->HasPendingTeleport() && player->getClass() == CLASS_PALADIN &&
        player->GetMapId() == 0 && (player->GetAreaId() == 197 || player->GetAreaId() == 196) &&
        !player->GetQuestRewardStatus(QuestId) &&
        (player->GetQuestStatus(QuestId) == QUEST_STATUS_INCOMPLETE ||
         player->GetQuestStatus(QuestId) == QUEST_STATUS_COMPLETE) &&
        player->GetQuestObjectiveData(QuestId, 108815) >= 9 &&
        player->GetQuestObjectiveData(QuestId, BossEntry) < 1;
}

bool Current(Creature* actor, Player* player)
{
    if (!actor || !player || !actor->IsInWorld() || !actor->HasQuestObjectScope() || actor->IsQuestObjectExpired())
        return false;
    auto scope = actor->GetQuestObjectScope().lock();
    return scope && scope->Owner == player->GetGUID() &&
        scope->CancellationReceipt() == player->GetQuestCancellationReceipt(QuestId);
}

Creature* OwnedBoss(Player* player)
{
    if (!player)
        return nullptr;
    for (ObjectGuid const& guid : *player->GetSummonList(BossEntry))
        if (Creature* actor = player->GetMap()->GetCreature(guid))
            if (actor->GetEntry() == BossEntry && actor->IsInWorld() &&
                actor->GetUInt32Value(UNIT_FIELD_CREATED_BY_SPELL) == SummonSpell &&
                actor->HasQuestObjectScope() && Current(actor, player) &&
                actor->IsInPersonnalVisibilityList(player->GetGUID()) &&
                actor->ToTempSummon() && actor->ToTempSummon()->GetSummonerGUID() == player->GetGUID())
                return actor; // A corpse also owns its finite native-credit window.
    return nullptr;
}

struct State
{
    std::weak_ptr<QuestCancellationReceipt const> attempt;
    uint32 wait = 1000;
    uint8 requests = 0;
    bool pending = false;
};

void Tick(Player* player, uint32 diff, State& state)
{
    if (!Pending(player))
        return;
    auto receipt = player->GetQuestCancellationReceipt(QuestId);
    if (!receipt || receipt->load(std::memory_order_acquire))
        return;
    if (state.attempt.lock() != receipt)
    {
        state = State();
        state.attempt = receipt;
    }
    if (state.wait > diff)
    {
        state.wait -= diff;
        return;
    }
    state.wait = 1000;
    if (OwnedBoss(player))
    {
        state.pending = false;
        state.requests = 0;
        return;
    }
    if (state.pending)
    {
        state.pending = false;
        player->RemoveAurasDueToSpell(SummonSpell);
        TC_LOG_WARN("scripts.ai", "Keeping the Peace: native228199 did not publish an owned Dargal for player %u (attempt %u/3); no boss credit", player->GetGUIDLow(), uint32(state.requests));
    }
    if (state.requests >= 3)
        return;
    if (player->IsMounted() || player->GetVehicle() || player->isInFlight() ||
        player->HasUnitState(UNIT_STATE_CASTING))
        return;

    // Characters which reached COMPLETE with the old mandatory Amount0 must
    // still kill Dargal. Preserve the real nine-spirit counter and use the
    // ordinary native death-credit path, which only processes INCOMPLETE logs.
    if (player->GetQuestStatus(QuestId) == QUEST_STATUS_COMPLETE)
        player->SetQuestStatus(QuestId, QUEST_STATUS_INCOMPLETE);
    player->RemoveAurasDueToSpell(SummonSpell);
    ++state.requests;
    state.pending = true;
    state.wait = 5000;
    player->CastSpell(player, SummonSpell, true);
}

std::mutex stateLock;
std::unordered_map<ObjectGuid, State> states;
void Forget(Player* player)
{
    if (!player)
        return;
    std::lock_guard<std::mutex> guard(stateLock);
    states.erase(player->GetGUID());
}
void Update(Player* player, uint32 diff)
{
    if (!Pending(player))
    {
        if (player)
            player->RemoveAurasDueToSpell(SummonSpell);
        Forget(player);
        return;
    }
    State state;
    {
        std::lock_guard<std::mutex> guard(stateLock);
        auto found = states.find(player->GetGUID());
        if (found != states.end())
            state = found->second;
    }
    Tick(player, diff, state);
    if (!Pending(player))
    {
        Forget(player);
        return;
    }
    std::lock_guard<std::mutex> guard(stateLock);
    states[player->GetGUID()] = state;
}
}

class player_keeping_the_peace : public PlayerScript
{
public:
    player_keeping_the_peace() : PlayerScript("player_keeping_the_peace") { }
    void OnUpdate(Player* player, uint32 diff) override { KeepingThePeace::Update(player, diff); }
    void OnBeforeLogout(Player* player) override { player->RemoveAurasDueToSpell(KeepingThePeace::SummonSpell); KeepingThePeace::Forget(player); }
    void OnLogout(Player* player) override { KeepingThePeace::Forget(player); }
    void OnMapChanged(Player* player) override { KeepingThePeace::Forget(player); }
};

// Combat stays on the native SmartAI data. This derived actor only owns the
// accepted quest lifetime and finite recovery/cleanup of the real summon.
struct npc_keeping_the_peace_dargal : SmartAI
{
    explicit npc_keeping_the_peace_dargal(Creature* actor) : SmartAI(actor) { }
    ObjectGuid owner;
    uint32 remaining = 300000;

    void CleanupMortars()
    {
        auto children = *me->GetSummonList(111212);
        for (ObjectGuid const& guid : children)
            if (Creature* child = me->GetMap()->GetCreature(guid))
                if (child->ToTempSummon() && child->ToTempSummon()->GetSummonerGUID() == me->GetGUID() &&
                    child->GetUInt32Value(UNIT_FIELD_CREATED_BY_SPELL) == 221411)
                    child->DespawnOrUnsummon();
    }

    void Reset() override
    {
        CleanupMortars();
        SmartAI::Reset();
    }

    void IsSummonedBy(Unit* summoner) override
    {
        Player* player = summoner ? summoner->ToPlayer() : nullptr;
        if (me->GetEntry() != KeepingThePeace::BossEntry || me->GetMapId() != 0 ||
            !KeepingThePeace::Pending(player) || !KeepingThePeace::Current(me, player) ||
            !me->ToTempSummon() || me->ToTempSummon()->GetSummonerGUID() != player->GetGUID() ||
            me->GetUInt32Value(UNIT_FIELD_CREATED_BY_SPELL) != KeepingThePeace::SummonSpell)
        {
            CleanupMortars();
            me->DespawnOrUnsummon();
            return;
        }
        owner = player->GetGUID();
        SmartAI::IsSummonedBy(summoner);
    }

    void UpdateAI(uint32 diff) override
    {
        Player* player = ObjectAccessor::GetPlayer(*me, owner);
        if (!KeepingThePeace::Pending(player) || !KeepingThePeace::Current(me, player) ||
            player->GetMap() != me->GetMap() || !me->IsWithinDistInMap(player, 120.0f) ||
            remaining <= diff)
        {
            CleanupMortars();
            me->DespawnOrUnsummon();
            return;
        }
        remaining -= diff;
        SmartAI::UpdateAI(diff);
    }

    void JustDied(Unit* killer) override
    {
        SmartAI::JustDied(killer);
        CleanupMortars();
        if (Player* player = ObjectAccessor::GetPlayer(*me, owner))
            player->RemoveAurasDueToSpell(KeepingThePeace::SummonSpell);
        // Native Unit/KillRewarder awards eligible player/owner/group credit.
        // Never award it to the pet or grant it from this cleanup timer.
        me->DespawnOrUnsummon(5000);
    }
};

struct npc_keeping_the_peace_mortar : SmartAI
{
    explicit npc_keeping_the_peace_mortar(Creature* actor) : SmartAI(actor) { }
    ObjectGuid parent, owner;

    void IsSummonedBy(Unit* summoner) override
    {
        Creature* boss = summoner ? summoner->ToCreature() : nullptr;
        if (me->GetEntry() != 111212 || me->GetMapId() != 0 ||
            !boss || boss->GetEntry() != KeepingThePeace::BossEntry || !boss->IsAlive() ||
            !boss->ToTempSummon() || !me->HasQuestObjectScope() || me->IsQuestObjectExpired() ||
            me->GetUInt32Value(UNIT_FIELD_CREATED_BY_SPELL) != 221411)
        {
            me->DespawnOrUnsummon();
            return;
        }
        Player* player = ObjectAccessor::GetPlayer(*me, boss->ToTempSummon()->GetSummonerGUID());
        if (!KeepingThePeace::Pending(player) || !KeepingThePeace::Current(me, player) ||
            boss->GetQuestObjectScope().lock() != me->GetQuestObjectScope().lock())
        {
            me->DespawnOrUnsummon();
            return;
        }
        parent = boss->GetGUID();
        owner = player->GetGUID();
        SetAutoAttack(false);
        SetCombatMove(false);
        SmartAI::IsSummonedBy(summoner);
        AttackStart(player);
    }

    void UpdateAI(uint32 diff) override
    {
        Creature* boss = me->GetMap()->GetCreature(parent);
        Player* player = ObjectAccessor::GetPlayer(*me, owner);
        if (!boss || !boss->IsAlive() || boss->IsQuestObjectExpired() || me->IsQuestObjectExpired() ||
            !KeepingThePeace::Pending(player) || !me->IsWithinDistInMap(player, 120.0f))
        {
            me->DespawnOrUnsummon();
            return;
        }
        if (!me->HasUnitState(UNIT_STATE_CASTING))
            me->SetFacingToObject(player);
        SmartAI::UpdateAI(diff);
    }
};
