#ifndef SHEILUN_TACTICS_H
#define SHEILUN_TACTICS_H

// Included inside SheilunPlan, after its actor/player/receipt admission helpers.
inline bool ChoiceLive(Player*& player, Creature*& actor, Role const& role, Map* map,
    ObjectGuid const& recipient, ObjectGuid const& source, std::weak_ptr<uint8> const& attempt)
{
    player = ObjectAccessor::FindPlayer(map, recipient);
    actor = map->GetCreature(source);
    Scenario* scenario = World(actor);
    auto status = player ? player->getQuestStatus(41003) : nullptr;
    return !attempt.expired() && status && status->ScriptLifetime == attempt.lock() &&
        PlayerContext(player, actor) && scenario && scenario->GetCurrentStep() == 2 &&
        actor->GetEntry() == role.entry && actor->GetDBTableGUIDLow() == role.spawn &&
        actor->HasFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP) && !Receipt(scenario, role.receipt);
}

inline bool ApplyTactic(Player*& player, Creature*& actor, Role const& role, uint32 option, Map* map,
    ObjectGuid const& recipient, ObjectGuid const& source, std::weak_ptr<uint8> const& attempt)
{
    uint32 chosen = option ? 204653 : 204654;
    uint32 opposite = option ? 204654 : 204653;
    SpellCastResult result = actor->CastSpell(actor, chosen, true);
    if (!ChoiceLive(player, actor, role, map, recipient, source, attempt)) return false;
    if (result != SPELL_CAST_OK || !actor->HasAura(chosen))
    {
        TC_LOG_WARN("scripts", "Sheilun plan: native tactic %u not applied (cast result %u); role selection remains available", chosen, uint32(result));
        return false;
    }
    // Keep the previous tactic until the requested native aura is confirmed.
    // Utility buffs and their native area triggers are independent of tactics.
    actor->RemoveAurasDueToSpell(opposite);
    if (!ChoiceLive(player, actor, role, map, recipient, source, attempt)) return false;
    if (!actor->HasAura(chosen) || actor->HasAura(opposite))
    {
        TC_LOG_WARN("scripts", "Sheilun plan: native tactic %u did not remain exclusive; role selection remains available", chosen);
        return false;
    }
    return true;
}
#endif
