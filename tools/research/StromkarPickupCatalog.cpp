#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "DB2FileLoader.h"
#include "DB2Meta.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <cstdlib>
#include <iostream>
#include <set>
#include <cstdio>
#include <filesystem>
#include <vector>
#pragma pack(push, 1)
struct StromkarKitEffectRecord
{
    SpellVisualKitEffectEntry effect;
    int32 kit;
};
struct StromkarVisualEventRecord
{
    SpellVisualEventEntry event;
    int32 visual;
};
struct StromkarKitAttachmentRecord
{
    SpellVisualKitModelAttachEntry attachment;
    int32 kit;
};
struct StromkarEffectNameRecord
{
    int32 ID;
    float radius, speed, scale, minimumScale, maximumScale, alpha;
    uint32 flags;
    int32 generic, texture;
    uint8 type;
    int32 model;
    uint32 ribbon;
    int32 dissolve;
};
#pragma pack(pop)
struct StromkarSceneFile : DB2FileSource
{
    std::string name;
    FILE* file;
    explicit StromkarSceneFile(std::string path) : name(std::move(path)), file(std::fopen(name.c_str(), "rb")) { }
    ~StromkarSceneFile() override { if (file) std::fclose(file); }
    bool IsOpen() const override { return file != nullptr; }
    bool Read(void* buffer, std::size_t size) override { return std::fread(buffer, size, 1, file) == 1; }
    std::size_t GetPosition() const override { return std::ftell(file); }
    std::size_t GetFileSize() const override { return std::filesystem::file_size(name); }
    char const* GetFileName() const override { return name.c_str(); }
};
int main(int argc, char** argv)
{
    if (argc != 2)
        return 1;
    std::string error;
    auto config = std::getenv("LEGION_DIALOGUE_CONFIG");
    if (!config || !sConfigMgr->LoadInitial(config, {}, error))
        return 2;
    MySQL::Library_Init();
    HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo", ""), 1, 1);
    if (HotfixDatabase.Open() || !HotfixDatabase.PrepareStatements())
        return 3;
    HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    auto check = HotfixDatabase.Query("SELECT @@session.transaction_read_only");
    if (!check || !check->Fetch()[0].GetBool())
        return 4;
    DB2Storage<SpellEntry> spells("Spell.db2", SpellLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2", SpellEffectLoadInfo::Instance());
    DB2Storage<ItemSparseEntry> items("ItemSparse.db2", ItemSparseLoadInfo::Instance());
    DB2Storage<ItemChildEquipmentEntry> children("ItemChildEquipment.db2", ItemChildEquipmentLoadInfo::Instance());
    DB2Storage<ScenarioStepEntry> steps("ScenarioStep.db2", ScenarioStepLoadInfo::Instance());
    DB2Storage<CriteriaTreeEntry> trees("CriteriaTree.db2", CriteriaTreeLoadInfo::Instance());
    DB2Storage<CriteriaEntry> criteria("Criteria.db2", CriteriaLoadInfo::Instance());
    if (!spells.Load(argv[1], 0) || !effects.Load(argv[1], 0) || !items.Load(argv[1], 0) || !children.Load(argv[1], 0) ||
        !steps.Load(argv[1], 0) || !trees.Load(argv[1], 0) || !criteria.Load(argv[1], 0))
        return 5;
    spells.LoadFromDB(); effects.LoadFromDB(); items.LoadFromDB(); children.LoadFromDB();
    steps.LoadFromDB(); trees.LoadFromDB(); criteria.LoadFromDB();
    DB2Storage<QuestObjectiveEntry> objectives("QuestObjective.db2", QuestObjectiveLoadInfo::Instance());
    if (!objectives.Load(argv[1], 0))
        return 25;
    objectives.LoadFromDB();
    std::cout << "NATIVE_OBJECTIVE_INDEX_SIZE\t" << objectives.GetNumRows() << '\n';
    for (auto objective : objectives)
        if (objective->QuestID == 41105)
            std::cout << "NATIVE_OBJECTIVE\t" << objective->ID << '\t' << unsigned(objective->Type) << '\t' << unsigned(objective->OrderIndex) << '\t'
                << unsigned(objective->StorageIndex) << '\t' << objective->ObjectID << '\t' << objective->Amount << '\t' << unsigned(objective->Flags) << '\t'
                << objective->Description->Str[0] << '\n';
    std::set<uint32> selected{206436, 206439, 206448, 225891, 201904, 201905, 207454, 206455, 206456, 204394, 202014, 63048, 204093,
        185674, 192084, 192085, 215997, 216016, 221509, 231348, 130966, 204641};
    DB2Storage<SpellCastingRequirementsEntry> castingRequirements("SpellCastingRequirements.db2", SpellCastingRequirementsLoadInfo::Instance());
    DB2Storage<SpellEquippedItemsEntry> equippedItems("SpellEquippedItems.db2", SpellEquippedItemsLoadInfo::Instance());
    if (!castingRequirements.Load(argv[1], 0) || !equippedItems.Load(argv[1], 0))
        return 30;
    castingRequirements.LoadFromDB(); equippedItems.LoadFromDB();
    for (auto row : castingRequirements)
        if (selected.count(row->SpellID))
            std::cout << "CASTING_REQUIREMENTS\t" << row->SpellID << '\t' << row->RequiredAreasID << '\t' << row->RequiresSpellFocus << '\t' << row->MinFactionID << '\t' << unsigned(row->MinReputation) << '\n';
    for (auto row : equippedItems)
        if (selected.count(row->SpellID))
            std::cout << "EQUIPMENT_REQUIREMENTS\t" << row->SpellID << '\t' << unsigned(row->EquippedItemClass) << '\t' << row->EquippedItemSubclass << '\t' << row->EquippedItemInvTypes << '\n';
    DB2Storage<SpellTargetRestrictionsEntry> targets("SpellTargetRestrictions.db2", SpellTargetRestrictionsLoadInfo::Instance());
    if (!targets.Load(argv[1], 0)) return 31;
    targets.LoadFromDB();
    for (auto row : targets)
        if (!row->DifficultyID && selected.count(row->SpellID))
            std::cout << "TARGET_REQUIREMENTS\t" << row->SpellID << '\t' << row->Targets << '\t' << unsigned(row->MaxTargets) << '\n';
    for (uint32 identity : selected)
    {
        auto spell = spells.LookupEntry(identity);
        if (!spell)
            return 6;
        std::cout << "SPELL\t" << identity << '\t' << spell->Name->Str[0] << '\n';
    }
    auto gazeVisualSpell = spells.LookupEntry(57404);
    std::cout << "GAZE_VISUAL_SPELL_COLLISION\t57404\t" << (gazeVisualSpell ? gazeVisualSpell->Name->Str[0] : "ABSENT") << '\n';
    DB2Storage<SpellXSpellVisualEntry> gazeVisuals("SpellXSpellVisual.db2", SpellXSpellVisualLoadInfo::Instance());
    if (!gazeVisuals.Load(argv[1], 0))
        return 28;
    gazeVisuals.LoadFromDB();
    std::set<uint32> poseVisuals;
    for (auto visual : gazeVisuals)
        if (visual->SpellID == 130966 || visual->SpellID == 204641)
        {
            poseVisuals.insert(visual->SpellVisualID);
            std::cout << "POSE_VISUAL\t" << visual->SpellID << '\t' << visual->SpellVisualID << '\n';
        }
    for (auto visual : gazeVisuals)
        if (visual->SpellVisualID == 57404 || visual->SpellID == 57404 || visual->SpellID == 230829)
        {
            auto owner = spells.LookupEntry(visual->SpellID);
            std::cout << "GAZE_VISUAL_REFERENCE\t" << visual->ID << '\t' << visual->SpellVisualID << '\t' << visual->SpellID << '\t'
                << unsigned(visual->DifficultyID) << '\t' << visual->CasterPlayerConditionID << '\t' << visual->ViewerPlayerConditionID << '\t'
                << (owner ? owner->Name->Str[0] : "ABSENT") << '\n';
            std::cout << "GAZE_VISUAL_ROW\t" << visual->ID << '\t' << visual->SpellVisualID << '\t' << visual->SpellID << '\t'
                << unsigned(visual->DifficultyID) << '\t' << visual->CasterPlayerConditionID << '\t' << visual->ViewerPlayerConditionID << '\t'
                << visual->CasterUnitConditionID << '\t' << visual->ViewerUnitConditionID << '\t' << visual->Probability << '\t'
                << unsigned(visual->Priority) << '\t' << unsigned(visual->Flags) << '\n';
        }
    for (auto spell : spells)
    {
        std::string name = spell->Name->Str[0];
        if (name.find("Danica") != std::string::npos || name.find("Thoradin") != std::string::npos || name.find("Skyhold") != std::string::npos || name == "Odyn's Gaze")
            std::cout << "FAREWELL_SPELL\t" << spell->ID << '\t' << name << '\n';
    }
    for (auto effect : effects)
        if (!effect->DifficultyID && effect->Effect == 28 && (effect->EffectMiscValue[0] == 93818 || effect->EffectMiscValue[0] == 93819 || effect->EffectMiscValue[0] == 93823 ||
            effect->EffectMiscValue[0] == 96549 || effect->EffectMiscValue[0] == 100622 || effect->EffectMiscValue[0] == 103608 ||
            effect->EffectMiscValue[0] == 105507 || effect->EffectMiscValue[0] == 112663 || effect->EffectMiscValue[0] == 116858))
            std::cout << "FAREWELL_SUMMON\t" << effect->SpellID << '\t' << effect->EffectIndex << '\t' << effect->EffectMiscValue[0] << '\t'
                << effect->ImplicitTarget[0] << '\t' << effect->ImplicitTarget[1] << '\n';
    for (auto effect : effects)
        if (!effect->DifficultyID && selected.count(effect->SpellID))
        {
            std::cout << "EFFECT\t" << effect->SpellID << '\t' << effect->EffectIndex << '\t' << effect->Effect << '\t' << effect->EffectAura << '\t'
                << effect->EffectMiscValue[0] << '\t' << effect->EffectTriggerSpell << '\t' << effect->ImplicitTarget[0] << '\t' << effect->ImplicitTarget[1] << '\n';
            if (effect->SpellID == 201904 || effect->SpellID == 201905)
                std::cout << "DRAIN_EFFECT\t" << effect->SpellID << '\t' << effect->EffectIndex << '\t' << effect->EffectAuraPeriod << '\t'
                    << effect->EffectBasePoints << '\t' << effect->EffectRadiusIndex[0] << '\t' << effect->EffectRadiusIndex[1] << '\n';
        }
    auto item = items.LookupEntry(128910);
    if (!item)
        return 7;
    std::cout << "ITEM\t" << item->ID << '\t' << item->MaxCount << '\t' << item->Stackable << '\t' << item->AllowableClass << '\t' << unsigned(item->ArtifactID) << '\n';
    for (auto child : children)
        if (child->ItemID == 128910)
            std::cout << "CHILD\t" << child->ItemID << '\t' << child->ChildItemID << '\t' << unsigned(child->ChildItemEquipSlot) << '\n';
    for (auto step : steps)
        if (step->ScenarioID == 1037)
        {
            std::cout << "STEP\t" << unsigned(step->OrderIndex) << '\t' << step->ID << '\t' << step->Title->Str[0] << '\t' << step->Description->Str[0] << '\n';
            std::set<uint32> pending{uint32(step->Criteriatreeid)}, seen;
            while (!pending.empty())
            {
                uint32 identity = *pending.begin();
                pending.erase(identity);
                if (!seen.insert(identity).second)
                    continue;
                auto tree = trees.LookupEntry(identity);
                if (!tree)
                    return 8;
                if (auto criterion = criteria.LookupEntry(tree->CriteriaID))
                    std::cout << "CRITERION\t" << unsigned(step->OrderIndex) << '\t' << criterion->ID << '\t' << unsigned(criterion->Type) << '\t' << criterion->Asset << '\t' << tree->Amount << '\n';
                for (auto descendant : trees)
                    if (uint32(descendant->Parent) == identity)
                        pending.insert(descendant->ID);
            }
        }
    DB2Storage<SceneScriptEntry> scripts("SceneScript.db2", SceneScriptLoadInfo::Instance());
    DB2Storage<SceneScriptPackageMemberEntry> members("SceneScriptPackageMember.db2", SceneScriptPackageMemberLoadInfo::Instance());
    DB2FileLoader texts;
    StromkarSceneFile textFile(std::string(argv[1]) + "SceneScriptText.db2");
    if (!scripts.Load(argv[1], 0) || !members.Load(argv[1], 0) || !texts.Load(&textFile, SceneScriptTextLoadInfo::Instance()))
        return 9;
    scripts.LoadFromDB(); members.LoadFromDB();
    DB2Storage<OverrideSpellDataEntry> overrides("OverrideSpellData.db2", OverrideSpellDataLoadInfo::Instance());
    if (!overrides.Load(argv[1], 0))
        return 12;
    overrides.LoadFromDB();
    auto execution = overrides.LookupEntry(684);
    if (!execution)
        return 13;
    for (uint32 slot = 0; slot < 10; ++slot)
        if (execution->Spells[slot])
            std::cout << "OVERRIDE\t684\t" << slot << '\t' << execution->Spells[slot] << '\n';
    auto skyhold = overrides.LookupEntry(730);
    if (!skyhold)
        return 26;
    std::cout << "OVERRIDE_META\t684\t" << unsigned(execution->Flags) << '\t' << execution->PlayerActionBarFileDataID << '\n';
    std::cout << "OVERRIDE_META\t730\t" << unsigned(skyhold->Flags) << '\t' << skyhold->PlayerActionBarFileDataID << '\n';
    for (uint32 slot = 0; slot < 10; ++slot)
        if (skyhold->Spells[slot])
            std::cout << "SKYHOLD_OVERRIDE\t730\t" << slot << '\t' << skyhold->Spells[slot] << '\n';
    DB2Storage<AreaTriggerEntry> staticTriggers("AreaTrigger.db2", AreaTriggerLoadInfo::Instance());
    if (!staticTriggers.Load(argv[1], 0))
        return 27;
    staticTriggers.LoadFromDB();
    auto staticGaze = staticTriggers.LookupEntry(11869);
    if (staticGaze)
        std::cout << "STATIC_GAZE\t" << staticGaze->ID << '\t' << staticGaze->ContinentID << '\t' << staticGaze->Pos.X << '\t'
            << staticGaze->Pos.Y << '\t' << staticGaze->Pos.Z << '\t' << staticGaze->Radius << '\n';
    else
        std::cout << "STATIC_GAZE_MISSING\t11869\n";
    for (auto entry : overrides)
        for (int32 spell : entry->Spells)
            if (spell == 206455)
                std::cout << "EXECUTE_OVERRIDE\t" << entry->ID << '\n';
    for (auto effect : effects)
        if (effect->EffectTriggerSpell == 206455 || (effect->Effect == 6 && effect->EffectAura == 293 && effect->EffectMiscValue[0] == 684))
            std::cout << "EXECUTE_REFERENCE\t" << effect->SpellID << '\t' << effect->EffectIndex << '\n';
    for (auto effect : effects)
        if (effect->EffectTriggerSpell == 206456)
            std::cout << "ARRIVAL_REFERENCE\t" << effect->SpellID << '\t' << effect->EffectIndex << '\t' << effect->DifficultyID << '\n';
    for (auto effect : effects)
        if (effect->EffectMiscValue[0] == 104307 || effect->EffectMiscValue[0] == 103151)
        {
            auto spell = spells.LookupEntry(effect->SpellID);
            std::cout << "ACTOR_REFERENCE\t" << effect->SpellID << '\t' << effect->EffectIndex << '\t' << effect->Effect << '\t'
                << effect->EffectMiscValue[0] << '\t' << effect->EffectMiscValue[1] << '\t' << effect->EffectTriggerSpell << '\t'
                << effect->ImplicitTarget[0] << '\t' << effect->ImplicitTarget[1] << '\t' << (spell ? spell->Name->Str[0] : "") << '\n';
        }
    DB2Storage<CreatureDisplayInfoEntry> actorDisplays("CreatureDisplayInfo.db2", CreatureDisplayInfoLoadInfo::Instance());
    DB2Storage<CreatureModelDataEntry> actorModels("CreatureModelData.db2", CreatureModelDataLoadInfo::Instance());
    if (!actorDisplays.Load(argv[1], 0) || !actorModels.Load(argv[1], 0))
        return 16;
    actorDisplays.LoadFromDB(); actorModels.LoadFromDB();
    for (uint32 identity : {67454u, 68556u})
    {
        auto display = actorDisplays.LookupEntry(identity);
        auto model = display ? actorModels.LookupEntry(display->ModelID) : nullptr;
        if (!model)
            return 29;
        std::cout << "ZAKAJZ_MODEL\t" << identity << '\t' << display->ModelID << '\t' << model->FileDataID << '\n';
    }
    auto thoradinDisplay = actorDisplays.LookupEntry(68038);
    auto thoradinModel = thoradinDisplay ? actorModels.LookupEntry(thoradinDisplay->ModelID) : nullptr;
    if (!thoradinModel || !thoradinModel->FileDataID)
        return 17;
    std::cout << "THORADIN_MODEL\t" << thoradinDisplay->ID << '\t' << thoradinDisplay->ModelID << '\t'
        << thoradinModel->FileDataID << '\t' << thoradinDisplay->CreatureModelScale << '\t' << unsigned(thoradinDisplay->CreatureModelAlpha) << '\n';
    std::cout << "THORADIN_VISUAL\t" << thoradinDisplay->ExtendedDisplayInfoID << '\t' << thoradinDisplay->StateSpellVisualKitID << '\t'
        << unsigned(thoradinDisplay->Flags) << '\t' << thoradinDisplay->TextureVariationFileDataID[0] << '\t'
        << thoradinDisplay->TextureVariationFileDataID[1] << '\t' << thoradinDisplay->TextureVariationFileDataID[2] << '\n';
    DB2Storage<SpellVisualKitEntry> actorKits("SpellVisualKit.db2", SpellVisualKitLoadInfo::Instance());
    auto kitEffectInfo = SpellVisualKitEffectLoadInfo::Instance();
    std::vector<DB2FieldMeta> kitFields(kitEffectInfo->Fields, kitEffectInfo->Fields + kitEffectInfo->FieldCount);
    kitFields.push_back({true, FT_INT, "SpellVisualKitID"});
    DB2LoadInfo fullKitInfo(kitFields.data(), kitFields.size(), SpellVisualKitEffectMeta::Instance(), HOTFIX_SEL_SPELL_VISUAL_KIT_EFFECT);
    DB2Storage<StromkarKitEffectRecord> actorKitEffects("SpellVisualKitEffect.db2", &fullKitInfo);
    if (sizeof(StromkarKitEffectRecord) != fullKitInfo.Meta->GetRecordSize() || !actorKits.Load(argv[1], 0) || !actorKitEffects.Load(argv[1], 0))
        return 18;
    actorKits.LoadFromDB();
    DB2Storage<SpellVisualAnimEntry> poseAnimations("SpellVisualAnim.db2", SpellVisualAnimLoadInfo::Instance());
    if (!poseAnimations.Load(argv[1], 0))
        return 30;
    poseAnimations.LoadFromDB();
    auto eventInfo = SpellVisualEventLoadInfo::Instance();
    std::vector<DB2FieldMeta> eventFields(eventInfo->Fields, eventInfo->Fields + eventInfo->FieldCount);
    eventFields.push_back({true, FT_INT, "SpellVisualID"});
    DB2LoadInfo fullEventInfo(eventFields.data(), eventFields.size(), SpellVisualEventMeta::Instance(), HOTFIX_SEL_SPELL_VISUAL_EVENT);
    DB2Storage<StromkarVisualEventRecord> gazeEvents("SpellVisualEvent.db2", &fullEventInfo);
    if (sizeof(StromkarVisualEventRecord) != fullEventInfo.Meta->GetRecordSize() || !gazeEvents.Load(argv[1], 0))
        return 29;
    std::set<uint32> gazeKits, poseKits;
    for (auto event : gazeEvents)
        if (poseVisuals.count(event->visual))
        {
            poseKits.insert(event->event.SpellVisualKitID);
            std::cout << "POSE_EVENT\t" << event->visual << '\t' << event->event.StartEvent << '\t'
                << event->event.EndEvent << '\t' << event->event.SpellVisualKitID << '\n';
        }
    for (auto event : gazeEvents)
        if (event->visual == 57404)
        {
            gazeKits.insert(event->event.SpellVisualKitID);
            std::cout << "GAZE_EVENT\t" << event->event.ID << '\t' << event->visual << '\t' << event->event.StartEvent << '\t'
                << event->event.EndEvent << '\t' << event->event.SpellVisualKitID << '\n';
        }
    auto thoradinKit = actorKits.LookupEntry(thoradinDisplay->StateSpellVisualKitID);
    if (!thoradinKit)
        return 19;
    std::cout << "THORADIN_KIT\t" << thoradinKit->ID << '\t' << thoradinKit->Flags << '\t' << thoradinKit->DelayMin << '\t' << thoradinKit->DelayMax << '\n';
    auto attachmentInfo = SpellVisualKitModelAttachLoadInfo::Instance();
    std::vector<DB2FieldMeta> attachmentFields(attachmentInfo->Fields, attachmentInfo->Fields + attachmentInfo->FieldCount);
    attachmentFields.push_back({true, FT_INT, "SpellVisualKitID"});
    DB2LoadInfo fullAttachmentInfo(attachmentFields.data(), attachmentFields.size(), SpellVisualKitModelAttachMeta::Instance(), HOTFIX_SEL_SPELL_VISUAL_KIT_MODEL_ATTACH);
    DB2Storage<StromkarKitAttachmentRecord> actorAttachments("SpellVisualKitModelAttach.db2", &fullAttachmentInfo);
    DB2FieldMeta nameFields[] = {
        {true, FT_INT, "ID"}, {false, FT_FLOAT, "EffectRadius"}, {false, FT_FLOAT, "BaseMissileSpeed"},
        {false, FT_FLOAT, "Scale"}, {false, FT_FLOAT, "MinAllowedScale"}, {false, FT_FLOAT, "MaxAllowedScale"},
        {false, FT_FLOAT, "Alpha"}, {false, FT_INT, "Flags"}, {true, FT_INT, "GenericID"},
        {true, FT_INT, "TextureFileDataID"}, {false, FT_BYTE, "Type"}, {true, FT_INT, "ModelFileDataID"},
        {false, FT_INT, "RibbonQualityID"}, {true, FT_INT, "DissolveEffectID"}
    };
    DB2LoadInfo nameInfo(nameFields, std::size(nameFields), SpellVisualEffectNameMeta::Instance(), HOTFIX_SEL_SPELL_VISUAL_EFFECT_NAME);
    DB2Storage<StromkarEffectNameRecord> actorEffectNames("SpellVisualEffectName.db2", &nameInfo);
    DB2Storage<SpellProceduralEffectEntry> actorProcedures("SpellProceduralEffect.db2", SpellProceduralEffectLoadInfo::Instance());
    if (sizeof(StromkarKitAttachmentRecord) != fullAttachmentInfo.Meta->GetRecordSize() || sizeof(StromkarEffectNameRecord) != nameInfo.Meta->GetRecordSize() ||
        !actorAttachments.Load(argv[1], 0) || !actorEffectNames.Load(argv[1], 0) || !actorProcedures.Load(argv[1], 0))
        return 20;
    actorProcedures.LoadFromDB();
    for (auto effect : actorKitEffects)
        if (effect->kit == thoradinKit->ID || gazeKits.count(effect->kit) || poseKits.count(effect->kit))
        {
            std::string tag = effect->kit == thoradinKit->ID ? "THORADIN" : poseKits.count(effect->kit) ? "POSE" : "GAZE";
            std::cout << tag << "_KIT_EFFECT\t" << effect->effect.ID << '\t' << effect->effect.EffectType << '\t' << effect->effect.Effect << '\n';
            if (poseKits.count(effect->kit) && effect->effect.EffectType == 6)
            {
                auto animation = poseAnimations.LookupEntry(effect->effect.Effect);
                if (!animation)
                    return 31;
                std::cout << "POSE_ANIMATION\t" << animation->ID << '\t' << animation->InitialAnimID << '\t'
                    << animation->LoopAnimID << '\t' << animation->AnimKitID << '\n';
            }
            if (effect->effect.EffectType == 2)
            {
                auto attachment = actorAttachments.LookupEntry(effect->effect.Effect);
                auto name = attachment ? actorEffectNames.LookupEntry(attachment->attachment.SpellVisualEffectNameID) : nullptr;
                if (!name)
                    return 21;
                auto const& visual = attachment->attachment;
                std::cout << tag << "_ATTACHMENT\t" << visual.ID << '\t' << visual.SpellVisualEffectNameID << '\t' << unsigned(visual.AttachmentID) << '\t'
                    << visual.Scale << '\t' << visual.AnimKitID << '\t' << name->model << '\t' << name->alpha << '\t' << unsigned(name->type) << '\n';
            }
            if (effect->effect.EffectType == 1)
            {
                auto procedure = actorProcedures.LookupEntry(effect->effect.Effect);
                if (!procedure)
                    return 22;
                std::cout << tag << "_PROCEDURE\t" << procedure->ID << '\t' << unsigned(procedure->Type);
                for (float value : procedure->Value)
                    std::cout << '\t' << value;
                std::cout << '\n';
            }
        }
    DB2Storage<SpellMiscEntry> miscellaneous("SpellMisc.db2", SpellMiscLoadInfo::Instance());
    DB2Storage<SpellRangeEntry> ranges("SpellRange.db2", SpellRangeLoadInfo::Instance());
    if (!miscellaneous.Load(argv[1], 0) || !ranges.Load(argv[1], 0))
        return 14;
    miscellaneous.LoadFromDB(); ranges.LoadFromDB();
    DB2Storage<SpellDurationEntry> durations("SpellDuration.db2", SpellDurationLoadInfo::Instance());
    if (!durations.Load(argv[1], 0))
        return 23;
    durations.LoadFromDB();
    for (auto entry : miscellaneous)
        if (!entry->DifficultyID && (entry->SpellID == 201904 || entry->SpellID == 201905 || entry->SpellID == 192084 || entry->SpellID == 192085))
        {
            auto duration = durations.LookupEntry(entry->DurationIndex);
            auto range = ranges.LookupEntry(entry->RangeIndex);
            if ((!duration && entry->DurationIndex) || !range)
                return 24;
            std::cout << "DRAIN_MISC\t" << entry->SpellID << '\t' << (duration ? duration->Duration : 0) << '\t' << (duration ? duration->MaxDuration : 0) << '\t'
                << entry->Speed << '\t' << range->RangeMax[0] << '\t' << range->RangeMax[1];
            for (int32 attribute : entry->Attributes)
                std::cout << '\t' << uint32(attribute);
            std::cout << '\n';
        }
    for (auto entry : miscellaneous)
        if (!entry->DifficultyID && entry->SpellID == 206455)
        {
            auto range = ranges.LookupEntry(entry->RangeIndex);
            if (!range)
                return 15;
            std::cout << "EXECUTE_RANGE\t" << range->RangeMin[0] << '\t' << range->RangeMax[0] << '\t' << range->RangeMin[1] << '\t' << range->RangeMax[1] << '\n';
        }
    std::set<uint32> pendingPackages{1590}, seenPackages, pendingScripts, seenScripts;
    while (!pendingPackages.empty())
    {
        uint32 package = *pendingPackages.begin();
        pendingPackages.erase(package);
        if (!seenPackages.insert(package).second)
            continue;
        for (auto member : members)
            if (member->SceneScriptPackageID == package)
            {
                if (member->ChildSceneScriptPackageID)
                    pendingPackages.insert(member->ChildSceneScriptPackageID);
                if (member->SceneScriptID)
                    pendingScripts.insert(member->SceneScriptID);
            }
    }
    while (!pendingScripts.empty())
    {
        uint32 identity = *pendingScripts.begin();
        pendingScripts.erase(identity);
        if (!seenScripts.insert(identity).second)
            continue;
        auto script = scripts.LookupEntry(identity);
        if (!script)
            return 10;
        bool found = false;
        for (uint32 recordIndex = 0; recordIndex < texts.GetRecordCount(); ++recordIndex)
        {
            auto record = texts.GetRecord(recordIndex);
            if (record && record.GetId() == identity)
            {
                std::cout << "SCENE_TEXT\t" << identity << '\t' << record.GetString("Name") << '\n' << record.GetString("Script") << '\n';
                found = true;
                break;
            }
        }
        if (!found)
            return 11;
        if (script->NextSceneScriptID)
            pendingScripts.insert(script->NextSceneScriptID);
    }
    HotfixDatabase.Close();
}
