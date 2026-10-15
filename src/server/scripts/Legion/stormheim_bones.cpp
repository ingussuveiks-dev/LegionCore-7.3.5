#include "ScriptMgr.h"
#include "SpellScript.h"
#include "Player.h"
#include "ObjectAccessor.h"
#include "QuestData.h"

namespace StormheimBones
{
constexpr uint32 QuestId = 39804;
bool Scope(Player* player)
{
    return player && player->IsInWorld() && player->GetMapId() == 1220 &&
        !player->GetInstanceId() && player->GetZoneId() == 7541;
}
bool Definition()
{
    auto quest = sQuestDataStore->GetQuestTemplate(QuestId);
    if (!quest || quest->StartScript) return false;
    uint32 mandatory = 0;
    for (auto const& objective : quest->GetObjectives())
        if (!(objective.Flags & QUEST_OBJECTIVE_FLAG_OPTIONAL))
        {
            if (objective.Type != QUEST_OBJECTIVE_MONSTER || objective.ObjectID != 97340 ||
                objective.StorageIndex != 0 || objective.Amount != 1) return false;
            ++mandatory;
        }
    return mandatory == 1;
}
bool Interact(Player* player, Creature* buck)
{
    if (!Scope(player) || !Definition() || !player->IsAlive() || player->IsBeingTeleported() ||
        player->GetVehicle() || player->GetTransport() || player->isInCombat() ||
        !buck || !buck->IsInWorld() || !buck->IsAlive() || buck->GetEntry() != 98230 ||
        buck->GetDBTableGUIDLow() != 338655 || buck->GetMapId() != 1220 || buck->GetInstanceId() ||
        buck->GetZoneId() != 7541 || buck->GetAreaId() != 7803 ||
        !buck->HasFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK) ||
        player->GetExactDist(buck) > 6.0f || !player->InSamePhase(buck) ||
        !player->canSeeOrDetect(buck) || !player->IsWithinLOSInMap(buck)) return false;
    // Do not synthesize a replacement actor or consume the interaction without Havi.
    auto havi = ObjectAccessor::GetCreature(*player, ObjectGuid::Create<HighGuid::Creature>(1220, 92539, 291280));
    return havi && havi->IsInWorld() && havi->IsAlive() && player->InSamePhase(havi) &&
        player->canSeeOrDetect(havi) && player->GetExactDist(havi) < 60.0f;
}
}

// 192171 is a fifteen-second self-targeted credit spell. Keep its native group
// credit and casting time; attach the missing native Havi conversation only
// after an actual successful hit on the current acceptance's clicked buck.
class spell_stormheim_bones_butchering : public SpellScript
{
    PrepareSpellScript(spell_stormheim_bones_butchering);
    ObjectGuid actor;
    std::weak_ptr<uint8> lifetime;
    bool scoped = false, captured = false, hit = false, completed = false;
    bool Valid(bool afterCredit = false)
    {
        using namespace StormheimBones;
        auto player = GetCaster()->ToPlayer();
        auto status = player ? player->getQuestStatus(QuestId) : nullptr;
        return captured && status && !lifetime.expired() && status->ScriptLifetime == lifetime.lock() &&
            (player->GetQuestStatus(QuestId) == QUEST_STATUS_INCOMPLETE ||
             (afterCredit && player->GetQuestStatus(QuestId) == QUEST_STATUS_COMPLETE)) &&
            Interact(player, ObjectAccessor::GetCreature(*player, actor));
    }
    SpellCastResult Check()
    {
        using namespace StormheimBones;
        auto player = GetCaster()->ToPlayer();
        if (!captured)
        {
            scoped = Scope(player);
            if (!scoped) return SPELL_CAST_OK;
            auto buck = GetOriginalTarget() ? GetOriginalTarget()->ToCreature() : nullptr;
            if (player->GetQuestStatus(QuestId) != QUEST_STATUS_INCOMPLETE || !Interact(player, buck))
                return SPELL_FAILED_BAD_TARGETS;
            actor = buck->GetGUID();
            player->AddQuestDelayedEvent(QuestId, 0, [] { });
            lifetime = player->getQuestStatus(QuestId)->ScriptLifetime;
            captured = true;
        }
        return Valid() ? SPELL_CAST_OK : SPELL_FAILED_BAD_TARGETS;
    }
    void Credit(SpellEffIndex index)
    {
        if (!scoped) return;
        if (GetHitPlayer() != GetCaster()->ToPlayer() || !Valid())
        {
            PreventHitDefaultEffect(index);
            return;
        }
        hit = true; // Default effect134 performs native capped player/group credit.
    }
    void Complete()
    {
        if (completed || !hit || !Valid(true)) return;
        auto player = GetCaster()->ToPlayer();
        if (player->GetQuestObjectiveData(StormheimBones::QuestId, 97340) < 1) return;
        completed = true;
        player->CastSpell(player, 192170, true); // Native conversation2247; optional playback cannot block credit.
    }
    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_stormheim_bones_butchering::Check);
        OnEffectHitTarget += SpellEffectFn(spell_stormheim_bones_butchering::Credit, EFFECT_0, SPELL_EFFECT_KILL_CREDIT2);
        AfterCast += SpellCastFn(spell_stormheim_bones_butchering::Complete);
    }
};

void AddSC_stormheim_bones()
{
    RegisterSpellScript(spell_stormheim_bones_butchering);
}
