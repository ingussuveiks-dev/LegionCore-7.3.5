#pragma once

// The real first Hight actor and native197120/134 own this report. Preserve
// group97778 and use the native personal194020/98514 marker, with no duplicate
// direct kill credit or unconditional OnSpellClick progression.
namespace MonkHaitClick
{
using Actor = npc_master_hait::npc_master_haitAI;
constexpr uint32 QuestId = 12103;

inline bool Source(Creature* source)
{
    return source && source->GetEntry() == 97778 && source->GetMapId() == 1014 && source->GetDBTableGUIDLow() == 362105;
}
inline Actor* Content(Creature* source)
{
    return Source(source) ? dynamic_cast<Actor*>(source->GetDisabledAI() ? source->GetDisabledAI() : source->GetAI()) : nullptr;
}
inline bool Active(Player* player)
{
    if (!player || !player->IsInWorld() || !player->CanContact() || !player->IsAlive() || player->HasPendingTeleport() ||
        player->getClass() != CLASS_MONK || player->GetMapId() != 1014 || player->GetQuestRewardStatus(QuestId) ||
        player->GetQuestStatus(QuestId) != QUEST_STATUS_INCOMPLETE || player->FindQuestSlot(QuestId) >= MAX_QUEST_LOG_SIZE)
        return false;
    auto scenario = sScenarioMgr->GetScenario(player->GetInstanceId());
    auto quest = sQuestDataStore->GetQuestTemplate(QuestId);
    return scenario && scenario->GetScenarioId() == 943 && scenario->GetCurrentStep() == 0 && quest &&
        quest->HasSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST) &&
        (!player->GetGroup() || !player->GetGroup()->isRaidGroup() || quest->IsAllowedInRaid(player->GetMap()->GetDifficultyID()));
}
inline Actor* Contact(Player* player, Creature* source)
{
    if (!(Active(player) && Source(source) && source->IsAlive() && source->IsInWorld() &&
        source->GetMap() == player->GetMap() && source->HasFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK) &&
        !source->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED) && player->InSamePhase(source) &&
        !source->ShouldHideFor(player->GetGUID()) && player->canSeeOrDetect(source) &&
        player->IsWithinDistInMap(source, INTERACTION_DISTANCE) && player->IsWithinLOSInMap(source))) return nullptr;
    auto chuan = source->FindNearestCreature(98001, 40.0f, true);
    auto demon = chuan ? chuan->FindNearestCreature(98011, 90.0f, true) : nullptr;
    return chuan && chuan->GetDBTableGUIDLow() == 362111 && demon && demon->GetDBTableGUIDLow() == 362135 ? Content(source) : nullptr;
}
struct Claim
{
    ObjectGuid source;
    Map* origin = nullptr; // identity only; never dereferenced after a callback
    uint32 instance = 0, waitStarted = 0;
    std::weak_ptr<uint8> quest, actor;
    std::shared_ptr<uint8> ticket;
    bool finished = false, waiting = false;
};
inline Actor* Current(Player* player, Claim const& claim)
{
    if (!player || player->GetMap() != claim.origin || player->GetInstanceId() != claim.instance)
        return nullptr;
    auto quest = player->getQuestStatus(QuestId);
    if (!quest || claim.quest.expired() || quest->ScriptLifetime != claim.quest.lock() || claim.actor.expired())
        return nullptr;
    auto source = player->GetMap()->GetCreature(claim.source);
    auto actor = Contact(player, source);
    return actor && actor->_clickLifetime == claim.actor.lock() && actor->_pendingClick.lock() == claim.ticket ? actor : nullptr;
}
inline void Complete(Player& player, std::shared_ptr<Claim> const& claim)
{
    if (claim->finished) return;
    Actor* actor = Current(&player, *claim);
    if (!actor) { claim->finished = true; return; }
    if (claim->waiting && GetMSTimeDiffToNow(claim->waitStarted) >= 2000)
    {
        claim->finished = true;
        TC_LOG_WARN("scripts", "Monk intro: native194020 produced no timely98514 receipt; retain Hight click for retry");
        return;
    }
    if (player.GetQuestObjectiveData(QuestId, 98514) <= 0)
    {
        // Player owns this queue and its callback; no foreign Player/AI is kept.
        player.AddQuestDelayedEvent(QuestId, 10, [owner = &player, claim]() { Complete(*owner, claim); });
        return;
    }
    claim->finished = true; // consume before the original script can re-enter
    player.GetMap()->GetCreature(claim->source)->RemoveFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
    actor->DoAction(1);
}
}

class spell_monk_hait_click : public SpellScript
{
    PrepareSpellScript(spell_monk_hait_click);
    std::shared_ptr<MonkHaitClick::Claim> claim;
    bool groupHit = false, after = false;

    bool Load() override { return GetCaster() && GetCaster()->GetMapId() == 1014; }
    bool Validate(SpellInfo const* info) override
    {
        auto arrival = sSpellMgr->GetSpellInfo(194020);
        return info && info->Id == 197120 && info->Effects[EFFECT_0] && info->Effects[EFFECT_1] &&
            info->Effects[EFFECT_0]->Effect == SPELL_EFFECT_SEND_EVENT && info->Effects[EFFECT_0]->MiscValue == 46736 &&
            info->Effects[EFFECT_1]->Effect == SPELL_EFFECT_KILL_CREDIT2 && info->Effects[EFFECT_1]->MiscValue == 97778 &&
            info->Effects[EFFECT_1]->TargetA.GetTarget() == TARGET_UNIT_CASTER && arrival && arrival->Effects[EFFECT_0] &&
            arrival->Effects[EFFECT_0]->Effect == SPELL_EFFECT_KILL_CREDIT && arrival->Effects[EFFECT_0]->MiscValue == 98514 &&
            arrival->Effects[EFFECT_0]->TargetA.GetTarget() == TARGET_UNIT_CASTER;
    }
    SpellCastResult Check()
    {
        auto player = GetCaster() ? GetCaster()->ToPlayer() : nullptr;
        if (claim) return MonkHaitClick::Current(player, *claim) ? SPELL_CAST_OK : SPELL_FAILED_BAD_TARGETS;
        auto source = player && player->GetMap() ? player->GetMap()->GetCreature(GetOriginalTargetGUID()) : nullptr;
        auto actor = MonkHaitClick::Contact(player, source);
        auto quest = player ? player->getQuestStatus(MonkHaitClick::QuestId) : nullptr;
        if (!actor || !quest) return SPELL_FAILED_BAD_TARGETS;
        if (!actor->_pendingClick.expired()) return SPELL_FAILED_NOT_READY;
        if (!quest->ScriptLifetime) quest->ScriptLifetime = std::make_shared<uint8>();
        claim = std::make_shared<MonkHaitClick::Claim>();
        claim->source = source->GetGUID(); claim->origin = player->GetMap(); claim->instance = player->GetInstanceId();
        claim->quest = quest->ScriptLifetime; claim->actor = actor->_clickLifetime;
        claim->ticket = std::make_shared<uint8>(); actor->_pendingClick = claim->ticket;
        return SPELL_CAST_OK;
    }
    void Event(SpellEffIndex effect)
    {
        auto player = GetCaster() ? GetCaster()->ToPlayer() : nullptr;
        if (!claim || !MonkHaitClick::Current(player, *claim)) PreventHitDefaultEffect(effect);
    }
    void Credit(SpellEffIndex effect)
    {
        auto player = GetCaster() ? GetCaster()->ToPlayer() : nullptr;
        if (!claim || GetHitUnit() != player || !MonkHaitClick::Current(player, *claim))
            PreventHitDefaultEffect(effect);
        else groupHit = true;
    }
    void After()
    {
        auto player = GetCaster() ? GetCaster()->ToPlayer() : nullptr;
        if (after || !groupHit || !claim || GetHitUnit() != player || !MonkHaitClick::Current(player, *claim)) return;
        after = true;
        if (player->GetQuestObjectiveData(MonkHaitClick::QuestId, 97778) <= 0) return;
        if (player->GetQuestObjectiveData(MonkHaitClick::QuestId, 98514) <= 0)
        {
            claim->waiting = true; claim->waitStarted = getMSTime();
            SpellCastResult result = player->CastSpell(player, 194020, true);
            if (result != SPELL_CAST_OK)
            {
                claim->finished = true;
                TC_LOG_WARN("scripts", "Monk intro: native194020 rejected (%u); retain Hight click for retry", uint32(result));
                return;
            }
        }
        MonkHaitClick::Complete(*player, claim);
    }
    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_monk_hait_click::Check);
        OnEffectHit += SpellEffectFn(spell_monk_hait_click::Event, EFFECT_0, SPELL_EFFECT_SEND_EVENT);
        OnEffectHitTarget += SpellEffectFn(spell_monk_hait_click::Credit, EFFECT_1, SPELL_EFFECT_KILL_CREDIT2);
        AfterHit += SpellHitFn(spell_monk_hait_click::After);
    }
};
