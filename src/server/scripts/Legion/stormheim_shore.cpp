#include "ScriptMgr.h"
#include "Player.h"
#include "GameObject.h"
#include "ObjectAccessor.h"
#include "QuestData.h"
#include "SpellScript.h"
#include <algorithm>
#include <map>
#include <memory>

namespace StormheimShore
{
constexpr uint32 QuestId = 38358;
constexpr uint32 Waste = 91693;
constexpr uint32 Filled = 91590;
constexpr uint32 Gauge = 181970;
constexpr uint32 FullContainer = 182151;

bool InShore(Player* player)
{
    return player && player->IsInWorld() && player->GetTeam() == HORDE &&
        player->GetMapId() == 1220 && player->GetAreaId() == 7563;
}

uint32 Progress(Player* player)
{
    int32 value = player->GetQuestObjectiveData(QuestId, Waste);
    return value < 0 ? 0 : value > 100 ? 100 : uint32(value);
}

bool Active(Player* player)
{
    return InShore(player) && player->IsAlive() && !player->IsBeingTeleported() &&
        player->GetQuestStatus(QuestId) == QUEST_STATUS_INCOMPLETE && Progress(player) < 100;
}

void Credit(Player* player, uint32 entry, uint32 amount)
{
    if (player->GetQuestStatus(QuestId) != QUEST_STATUS_INCOMPLETE)
        return;
    if (Quest const* quest = sQuestDataStore->GetQuestTemplate(QuestId))
        for (QuestObjective const& objective : quest->GetObjectives())
            if (objective.Type == QUEST_OBJECTIVE_MONSTER && objective.ObjectID == entry)
            {
                int32 old = player->GetQuestObjectiveData(QuestId, entry);
                uint32 before = old < 0 ? 0 : uint32(old);
                uint32 after = std::min<uint32>(objective.Amount, before + amount);
                if (after <= before)
                    return;
                player->SetQuestObjectiveData(quest, &objective, after);
                player->SendQuestUpdateAddCredit(quest, ObjectGuid::Empty, objective, after);
                if (player->CanCompleteQuest(QuestId))
                    player->CompleteQuest(QuestId);
                return;
            }
}

void Sync(Player* player)
{
    QuestStatus status = player->GetQuestStatus(QuestId);
    if (!InShore(player) || (status != QUEST_STATUS_INCOMPLETE && status != QUEST_STATUS_COMPLETE))
    {
        player->RemoveAurasDueToSpell(Gauge);
        player->RemoveAurasDueToSpell(FullContainer);
        return;
    }
    uint32 progress = Progress(player);
    if (progress == 100)
    {
        // Waste is a native optional/hidden counter. Only a genuinely full
        // container grants the required objective, including after login.
        Credit(player, Filled, 1);
        player->RemoveAurasDueToSpell(Gauge);
        if (player->IsAlive() && !player->HasAura(FullContainer))
            player->CastSpell(player, FullContainer, true);
        return;
    }
    player->RemoveAurasDueToSpell(FullContainer);
    if (!player->IsAlive() || player->IsBeingTeleported())
        return;
    // Do not overwrite a different encounter's alternate power bar.
    if (!player->HasAura(Gauge) && player->HasAuraType(SPELL_AURA_ENABLE_ALT_POWER))
        return;
    if (!player->HasAura(Gauge))
        player->CastSpell(player, Gauge, true);
    if (player->HasAura(Gauge))
        player->SetPower(POWER_ALTERNATE, progress);
}

bool Interact(Player* player, GameObject* object)
{
    return Active(player) && object && object->IsInWorld() && object->isSpawned() &&
        object->GetMapId() == 1220 && object->GetAreaId() == 7563 &&
        (object->GetEntry() == 240600 || object->GetEntry() == 249829) &&
        !player->GetVehicle() && !player->GetTransport() && !player->isInCombat() &&
        player->GetExactDist(object) <= 5.0f && player->InSamePhase(object) &&
        player->canSeeOrDetect(object) && player->IsWithinLOSInMap(object) &&
        !object->HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_NOT_SELECTABLE);
}

struct PumpAttempt
{
    ObjectGuid object;
    std::weak_ptr<uint8> lifetime;
    uint64 token = 0;
};
std::map<ObjectGuid, PumpAttempt> attempts;
uint64 nextToken = 0;
}

class go_stormheim_volatile_barrel : public GameObjectScript
{
public:
    go_stormheim_volatile_barrel() : GameObjectScript("go_stormheim_volatile_barrel") { }
    bool OnGossipHello(Player* player, GameObject* object) override
    {
        using namespace StormheimShore;
        if (object->GetMapId() != 1220 || object->GetAreaId() != 7563)
            return false;
        if (!Interact(player, object) || object->getLootState() != GO_READY ||
            object->HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_IN_USE))
            return true;

        // Open-lock spell 182021 has already finished before GameObject::Use
        // reaches this hook. Pump 181981 is native instant/triggered; keep its
        // proof only within this synchronous cast, even when the cast fails.
        player->AddQuestDelayedEvent(QuestId, 0, [] { });
        uint64 token = ++nextToken;
        attempts[player->GetGUID()] = {object->GetGUID(), player->getQuestStatus(QuestId)->ScriptLifetime, token};
        uint32 before = Progress(player);
        SpellCastResult result = player->CastSpell(player, 181981, true);
        auto found = attempts.find(player->GetGUID());
        if (found != attempts.end() && found->second.token == token)
            attempts.erase(found);
        // On success retain the ordinary goober opening/closing animation and
        // native cooldown. Data10 is zero only on the script-bound barrels,
        // so the core cannot cast a second, unguarded Pump afterward.
        return result != SPELL_CAST_OK || Progress(player) == before;
    }
};

class spell_stormheim_shore_pump : public SpellScript
{
    PrepareSpellScript(spell_stormheim_shore_pump);
    StormheimShore::PumpAttempt attempt;
    bool captured = false;
    bool completed = false;
    uint32 amount = 0;

    bool Valid(Player* player)
    {
        if (!captured || attempt.lifetime.expired() || !player)
            return false;
        auto found = StormheimShore::attempts.find(player->GetGUID());
        return found != StormheimShore::attempts.end() && found->second.token == attempt.token &&
            StormheimShore::Interact(player, ObjectAccessor::GetGameObject(*player, attempt.object));
    }
    SpellCastResult Check()
    {
        Player* player = GetCaster()->ToPlayer();
        if (!player)
            return SPELL_FAILED_BAD_TARGETS;
        auto found = StormheimShore::attempts.find(player->GetGUID());
        if (!captured && found != StormheimShore::attempts.end())
        {
            attempt = found->second;
            captured = true;
        }
        return Valid(player) ? SPELL_CAST_OK : SPELL_FAILED_BAD_TARGETS;
    }
    void Pump(SpellEffIndex index)
    {
        PreventHitDefaultEffect(index);
        // Effect 90 otherwise discards the native 15..20 amount and grants 1.
        int32 value = GetEffectValue();
        if (Valid(GetCaster()->ToPlayer()) && value >= 15 && value <= 20)
            amount = uint32(value);
    }
    void Complete()
    {
        Player* player = GetCaster()->ToPlayer();
        if (completed || !amount || !Valid(player))
            return;
        completed = true;
        StormheimShore::attempts.erase(player->GetGUID());
        StormheimShore::Credit(player, StormheimShore::Waste, amount);
        StormheimShore::Sync(player);
    }
    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_stormheim_shore_pump::Check);
        OnEffectHitTarget += SpellEffectFn(spell_stormheim_shore_pump::Pump, EFFECT_0, SPELL_EFFECT_KILL_CREDIT);
        AfterCast += SpellCastFn(spell_stormheim_shore_pump::Complete);
    }
};

class player_stormheim_shore : public PlayerScript
{
public:
    player_stormheim_shore() : PlayerScript("player_stormheim_shore") { }
    std::map<ObjectGuid, uint32> polls;
    void OnLogin(Player* player) override { StormheimShore::Sync(player); }
    void OnUpdate(Player* player, uint32 diff) override
    {
        uint32& timer = polls[player->GetGUID()];
        if (timer > diff) { timer -= diff; return; }
        timer = 1000;
        if (player->HasAura(StormheimShore::Gauge) || player->HasAura(StormheimShore::FullContainer) ||
            player->GetQuestStatus(StormheimShore::QuestId) != QUEST_STATUS_NONE)
            StormheimShore::Sync(player);
    }
    void OnLogout(Player* player) override
    {
        polls.erase(player->GetGUID());
        StormheimShore::attempts.erase(player->GetGUID());
    }
};

void AddSC_stormheim_shore()
{
    new go_stormheim_volatile_barrel();
    RegisterSpellScript(spell_stormheim_shore_pump);
    new player_stormheim_shore();
}
