#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
#include <stdexcept>

static void Require(bool value, char const* message)
{
    if (!value)
        throw std::runtime_error(message);
}

// Content evidence only: this reader does not run the outdoor encounter.
int Run(int argc, char** argv)
{
    if (argc != 2)
        return 1;
#define STORE(T, N, F, I) DB2Storage<T> N(F ".db2", I::Instance()); if (!N.Load(argv[1], 0)) return 2
    STORE(SpellEffectEntry, effects, "SpellEffect", SpellEffectLoadInfo);
    STORE(SpellRadiusEntry, radii, "SpellRadius", SpellRadiusLoadInfo);
    STORE(SpellEntry, spells, "Spell", SpellLoadInfo);
    STORE(SpellMiscEntry, misc, "SpellMisc", SpellMiscLoadInfo);
    STORE(SpellDurationEntry, durations, "SpellDuration", SpellDurationLoadInfo);
    STORE(SpellRangeEntry, ranges, "SpellRange", SpellRangeLoadInfo);
    STORE(SpellTargetRestrictionsEntry, targetRestrictions, "SpellTargetRestrictions", SpellTargetRestrictionsLoadInfo);
    STORE(SpellAuraRestrictionsEntry, auraRestrictions, "SpellAuraRestrictions", SpellAuraRestrictionsLoadInfo);
    STORE(QuestV2CliTaskEntry, tasks, "QuestV2CliTask", QuestV2CliTaskLoadInfo);
    STORE(QuestObjectiveEntry, objectives, "QuestObjective", QuestObjectiveLoadInfo);
    STORE(PlayerConditionEntry, conditions, "PlayerCondition", PlayerConditionLoadInfo);
    STORE(AreaTableEntry, areas, "AreaTable", AreaTableLoadInfo);
    STORE(CreatureDisplayInfoEntry, displays, "CreatureDisplayInfo", CreatureDisplayInfoLoadInfo);
    STORE(CreatureModelDataEntry, models, "CreatureModelData", CreatureModelDataLoadInfo);
    STORE(FactionTemplateEntry, factions, "FactionTemplate", FactionTemplateLoadInfo);
#undef STORE

    auto effect = [&](uint32 spell, int32 index) -> SpellEffectEntry const*
    {
        SpellEffectEntry const* result = nullptr;
        for (auto row : effects)
            if (row->SpellID == spell && row->DifficultyID == 0 && row->EffectIndex == index)
            {
                Require(!result, "Ambiguous native effect");
                result = row;
            }
        Require(result != nullptr, "Missing native effect");
        return result;
    };

    auto credit = effect(229113, 0);
    Require(credit->Effect == 90 && credit->EffectMiscValue[0] == 115168 &&
        credit->ImplicitTarget[0] == 18 && credit->ImplicitTarget[1] == 16,
        "Rockaway personal credit changed");
    auto loot = effect(229113, 1);
    Require(loot->Effect == 189 && loot->EffectMiscValue[0] == 70762 && loot->EffectMiscValue[1] == 420,
        "Rockaway bonus-loot effect changed");
    auto radius = radii.LookupEntry(41);
    Require(credit->EffectRadiusIndex[1] == 41 && loot->EffectRadiusIndex[1] == 41 &&
        radius && radius->Radius == 150.0f && radius->RadiusMax == 150.0f,
        "Native Rockaway completion radius changed");
    std::cout << "credit229113: effect90 marker115168; effect189 misc70762/420 (not an item count)\n";

    auto outro = effect(230020, 0);
    Require(outro->Effect == 219 && outro->EffectMiscValue[0] == 3963,
        "Rockaway outro conversation changed");
    for (auto row : effects)
        if (row->SpellID == 230020)
            Require(row->EffectItemType != 142487 && row->Effect != 189 && row->Effect != 24,
                "Reinspect outro reward acquisition");

    auto task = tasks.LookupEntry(44746);
    auto coast = areas.LookupEntry(7833);
    Require(coast && coast->ContinentID == 1220 && coast->ParentAreaID == 7503,
        "Native Rockaway Coast map/zone changed");
    auto condition = conditions.LookupEntry(47902);
    Require(task, "Missing native Rockaway task");
    std::cout << "task filters: min " << int(task->FiltMinLevel) << " max " << int(task->FiltMaxLevel)
        << " skill " << task->FiltMinSkillID << " races " << task->FiltRaces << " classes " << task->FiltClasses << std::endl;
    Require(task && task->ConditionID == 47902 && task->WorldStateExpressionID == 20875 && condition &&
        task->FiltMinLevel == 110 && task->FiltMaxLevel == -1 && !task->FiltMinSkillID &&
        task->FiltRaces == -1 && task->FiltClasses == 65535 &&
        condition->PrevQuestID[0] == 41694 && condition->PrevQuestID[1] == 44721 &&
        condition->PrevQuestLogic == 1, "Native landing unlock changed");
    for (auto id : task->FiltCompletedQuest)
        Require(!id, "Reinspect additional native arrival prerequisites");
    auto objective = objectives.LookupEntry(286897);
    Require(objective && objective->QuestID == 44746 && objective->Type == 0 &&
        objective->ObjectID == 115168 && objective->Amount == 1,
        "Native Rockaway objective marker changed");
    std::cout << "arrival44746: area7833/map1220/zone7503; level110, both factions; condition47902 requires41694 AND44721; marker115168 x1\n";
    for (uint32 id : {2817u, 16u, 14u})
        Require(factions.LookupEntry(id) && factions.LookupEntry(id)->IsHostileToPlayers(),
            "Encounter faction is not hostile to players");

    for (uint32 id : {70526u, 66238u, 73956u, 33108u})
    {
        auto display = displays.LookupEntry(id);
        auto model = display ? models.LookupEntry(display->ModelID) : nullptr;
        Require(model && model->FileDataID, "Missing native encounter model");
        std::cout << "display " << id << " model " << model->ID << " file " << model->FileDataID << '\n';
    }

    Require(effect(231296, 0)->Effect == 28 && effect(231296, 0)->EffectMiscValue[0] == 116187,
        "Tentacle summon changed");
    Require(effect(231359, 0)->Effect == 28 && effect(231359, 0)->EffectMiscValue[0] == 116195,
        "Slime summon changed");
    Require(effect(231298, 1)->EffectAura == 12, "Tentacle stun changed");
    Require(effect(231354, 1)->EffectTriggerSpell == 231359 &&
        effect(231356, 1)->EffectTriggerSpell == 231359, "Breath missile summon changed");
    Require(effect(231459, 0)->EffectBasePoints == 231329, "Taint dummy changed");
    Require(effect(232681, 0)->EffectTriggerSpell == 232679 &&
        effect(232679, 0)->Effect == 2, "Outdoor tainted essence missile changed");
    for (uint32 id : {231328u, 231333u})
    {
        SpellMiscEntry const* ritual = nullptr;
        for (auto row : misc)
            if (row->SpellID == id && row->DifficultyID == 0)
            {
                Require(!ritual, "Ambiguous ritual miscellaneous record");
                ritual = row;
            }
        Require(ritual && (ritual->Attributes[1] & 0x44) &&
            durations.LookupEntry(ritual->DurationIndex) &&
            durations.LookupEntry(ritual->DurationIndex)->Duration == -1 &&
            effect(id, 0)->Effect == 6 && effect(id, 0)->EffectAura == 4 &&
            effect(id, 0)->ImplicitTarget[0] == 1,
            "Ritual channel/self-dummy data changed");
        std::cout << "ritual " << id << ": channeled, self dummy aura; actor/beam binding unproven\n";
    }
    SpellMiscEntry const* channel = nullptr;
    for (auto row : misc)
        if (row->SpellID == 231433 && row->DifficultyID == 0)
        {
            Require(!channel, "Ambiguous reconstructed channel record");
            channel = row;
        }
    Require(channel && (channel->Attributes[1] & 0x44) &&
        durations.LookupEntry(channel->DurationIndex) && durations.LookupEntry(channel->DurationIndex)->Duration == -1 &&
        ranges.LookupEntry(channel->RangeIndex) && ranges.LookupEntry(channel->RangeIndex)->RangeMax[0] == 100.0f &&
        ranges.LookupEntry(channel->RangeIndex)->RangeMax[1] == 100.0f &&
        effect(231433, 0)->Effect == 6 && effect(231433, 0)->EffectAura == 4 &&
        effect(231433, 0)->ImplicitTarget[0] == 38 && effect(231433, 0)->ImplicitTarget[1] == 0,
        "Reconstructed231433 requires infinite100-yard nearby-entry channel");
    std::cout << "channel231433: infinite100-yard native target38; exact owner enforced by scoped hook; retail binding unproven\n";
    for (auto row : targetRestrictions)
        if (row->SpellID == 231433 && row->DifficultyID == 0)
            Require(!row->TargetCreatureType && !row->Targets, "Reinspect native channel explicit/creature target restriction");
    for (auto row : auraRestrictions)
        if (row->SpellID == 231433 && row->DifficultyID == 0)
            Require(!row->TargetAuraSpell && !row->ExcludeTargetAuraSpell && !row->TargetAuraState &&
                !row->ExcludeTargetAuraState && !row->CasterAuraSpell && !row->ExcludeCasterAuraSpell &&
                !row->CasterAuraState && !row->ExcludeCasterAuraState, "Reinspect native channel aura restrictions");
    for (uint32 id : {229113u, 230020u, 231295u, 231296u, 231298u, 231328u, 231329u,
        231333u, 231340u, 231351u, 231353u, 231354u, 231356u, 231359u, 231386u, 231459u, 232681u, 232679u})
    {
        Require(spells.LookupEntry(id) != nullptr, "Missing native encounter spell");
        for (auto row : effects)
            if (row->SpellID == id && row->DifficultyID == 0)
                std::cout << "spell " << id << " effect " << row->EffectIndex << " type " << row->Effect
                    << " aura " << row->EffectAura << " misc " << row->EffectMiscValue[0] << ','
                    << row->EffectMiscValue[1] << " trigger " << row->EffectTriggerSpell
                    << " base " << row->EffectBasePoints << " target " << row->ImplicitTarget[0]
                    << ',' << row->ImplicitTarget[1] << '\n';
    }
    std::cout << "PASS: typed native catalog. Encounter implementation and reward count are NOT verified.\n";
    return 0;
}

int main(int argc, char** argv)
{
    try { return Run(argc, argv); }
    catch (std::exception const& e) { std::cerr << e.what() << '\n'; return 1; }
}
