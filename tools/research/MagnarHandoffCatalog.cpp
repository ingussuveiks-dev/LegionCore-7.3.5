#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "DB2Meta.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <cstdlib>
#include <iostream>
#include <set>
#include <string>
#include <vector>

#pragma pack(push, 1)
struct HandoffKitEffect
{
    SpellVisualKitEffectEntry effect;
    int32 kit;
};
struct HandoffKitAttachment
{
    SpellVisualKitModelAttachEntry attachment;
    int32 kit;
};
struct HandoffEffectName
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
    auto readOnly = HotfixDatabase.Query("SELECT @@session.transaction_read_only");
    if (!readOnly || !readOnly->Fetch()[0].GetBool())
        return 4;
    DB2Storage<SpellEntry> spells("Spell.db2", SpellLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2", SpellEffectLoadInfo::Instance());
    DB2Storage<SpellMiscEntry> misc("SpellMisc.db2", SpellMiscLoadInfo::Instance());
    DB2Storage<SpellDurationEntry> durations("SpellDuration.db2", SpellDurationLoadInfo::Instance());
    DB2Storage<SpellCastTimesEntry> casts("SpellCastTimes.db2", SpellCastTimesLoadInfo::Instance());
    DB2Storage<SpellXSpellVisualEntry> visuals("SpellXSpellVisual.db2", SpellXSpellVisualLoadInfo::Instance());
    DB2Storage<CreatureDisplayInfoEntry> displays("CreatureDisplayInfo.db2", CreatureDisplayInfoLoadInfo::Instance());
    DB2Storage<CreatureModelDataEntry> models("CreatureModelData.db2", CreatureModelDataLoadInfo::Instance());
    DB2Storage<SpellVisualKitEntry> kits("SpellVisualKit.db2", SpellVisualKitLoadInfo::Instance());
    auto effectInfo = SpellVisualKitEffectLoadInfo::Instance();
    std::vector<DB2FieldMeta> effectFields(effectInfo->Fields, effectInfo->Fields + effectInfo->FieldCount);
    effectFields.push_back({true, FT_INT, "SpellVisualKitID"});
    DB2LoadInfo kitEffectInfo(effectFields.data(), effectFields.size(), SpellVisualKitEffectMeta::Instance(), HOTFIX_SEL_SPELL_VISUAL_KIT_EFFECT);
    auto attachmentInfo = SpellVisualKitModelAttachLoadInfo::Instance();
    std::vector<DB2FieldMeta> attachmentFields(attachmentInfo->Fields, attachmentInfo->Fields + attachmentInfo->FieldCount);
    attachmentFields.push_back({true, FT_INT, "SpellVisualKitID"});
    DB2LoadInfo kitAttachmentInfo(attachmentFields.data(), attachmentFields.size(), SpellVisualKitModelAttachMeta::Instance(), HOTFIX_SEL_SPELL_VISUAL_KIT_MODEL_ATTACH);
    DB2FieldMeta nameFields[] = {
        {true, FT_INT, "ID"}, {false, FT_FLOAT, "EffectRadius"}, {false, FT_FLOAT, "BaseMissileSpeed"},
        {false, FT_FLOAT, "Scale"}, {false, FT_FLOAT, "MinAllowedScale"}, {false, FT_FLOAT, "MaxAllowedScale"},
        {false, FT_FLOAT, "Alpha"}, {false, FT_INT, "Flags"}, {true, FT_INT, "GenericID"},
        {true, FT_INT, "TextureFileDataID"}, {false, FT_BYTE, "Type"}, {true, FT_INT, "ModelFileDataID"},
        {false, FT_INT, "RibbonQualityID"}, {true, FT_INT, "DissolveEffectID"}
    };
    DB2LoadInfo nameInfo(nameFields, std::size(nameFields), SpellVisualEffectNameMeta::Instance(), HOTFIX_SEL_SPELL_VISUAL_EFFECT_NAME);
    DB2Storage<HandoffKitEffect> kitEffects("SpellVisualKitEffect.db2", &kitEffectInfo);
    DB2Storage<HandoffKitAttachment> attachments("SpellVisualKitModelAttach.db2", &kitAttachmentInfo);
    DB2Storage<HandoffEffectName> names("SpellVisualEffectName.db2", &nameInfo);
    if (sizeof(HandoffKitEffect) != kitEffectInfo.Meta->GetRecordSize() ||
        sizeof(HandoffKitAttachment) != kitAttachmentInfo.Meta->GetRecordSize() ||
        sizeof(HandoffEffectName) != nameInfo.Meta->GetRecordSize())
        return 8;
    if (!spells.Load(argv[1], 0) || !effects.Load(argv[1], 0) || !misc.Load(argv[1], 0) ||
        !durations.Load(argv[1], 0) || !casts.Load(argv[1], 0) || !visuals.Load(argv[1], 0) ||
        !displays.Load(argv[1], 0) || !models.Load(argv[1], 0) || !kits.Load(argv[1], 0) ||
        !kitEffects.Load(argv[1], 0) || !attachments.Load(argv[1], 0) || !names.Load(argv[1], 0))
        return 5;
    spells.LoadFromDB(); effects.LoadFromDB(); misc.LoadFromDB();
    durations.LoadFromDB(); casts.LoadFromDB(); visuals.LoadFromDB();
    displays.LoadFromDB(); models.LoadFromDB();
    kits.LoadFromDB();
    for (uint32 identity : {33225u, 64600u, 64124u, 68279u})
    {
        auto display = displays.LookupEntry(identity);
        auto model = display ? models.LookupEntry(display->ModelID) : nullptr;
        if (!model)
            return 7;
        std::cout << "DISPLAY\t" << identity << '\t' << display->ModelID << '\t' << model->FileDataID << '\t'
            << display->StateSpellVisualKitID << '\t' << unsigned(display->CreatureModelAlpha) << '\n';
    }
    if (!kits.LookupEntry(58143))
        return 9;
    bool foundBeam = false;
    for (auto effect : kitEffects)
        if (effect->kit == 58143)
        {
            std::cout << "KIT_EFFECT\t" << effect->kit << '\t' << effect->effect.EffectType << '\t' << effect->effect.Effect << '\n';
            if (effect->effect.EffectType == 2)
            {
                auto attachment = attachments.LookupEntry(effect->effect.Effect);
                auto name = attachment ? names.LookupEntry(attachment->attachment.SpellVisualEffectNameID) : nullptr;
                if (!name)
                    return 10;
                auto const& visual = attachment->attachment;
                std::cout << "ATTACHMENT\t" << visual.ID << '\t' << visual.SpellVisualEffectNameID << '\t'
                    << unsigned(visual.AttachmentID) << '\t' << visual.Scale << '\t' << visual.AnimKitID << '\t'
                    << name->model << '\t' << name->alpha << '\t' << unsigned(name->type) << '\t' << name->generic << '\n';
                foundBeam = true;
            }
        }
    if (!foundBeam)
        return 11;
    std::set<uint32> wanted{188067, 188068, 188074, 189718, 189721, 190588, 192065};
    for (auto spell : spells)
    {
        std::string name = spell->Name->Str[0];
        if (name.find("Magnar") != std::string::npos || name.find("Hruthnir") != std::string::npos ||
            name.find("Valarjar") != std::string::npos || name.find("Blackwyrm") != std::string::npos)
            std::cout << "CANDIDATE\t" << spell->ID << '\t' << name << '\n';
    }
    for (uint32 identity : wanted)
    {
        auto spell = spells.LookupEntry(identity);
        if (!spell)
        {
            std::cerr << "Missing native handoff candidate Spell " << identity << '\n';
            for (auto effect : effects)
                if (effect->SpellID == identity || (wanted.count(effect->SpellID) && effect->EffectTriggerSpell == identity))
                    std::cerr << "Candidate edge " << effect->SpellID << ' ' << effect->Effect << ' '
                        << effect->EffectMiscValue[0] << ' ' << effect->EffectTriggerSpell << '\n';
            return 6;
        }
        std::cout << "SPELL\t" << identity << '\t' << spell->Name->Str[0] << '\n';
        for (auto entry : misc)
            if (entry->SpellID == identity && !entry->DifficultyID)
            {
                auto duration = durations.LookupEntry(entry->DurationIndex);
                auto cast = casts.LookupEntry(entry->CastingTimeIndex);
                std::cout << "TIME\t" << identity << '\t' << (cast ? cast->Base : 0) << '\t'
                    << (duration ? duration->Duration : 0) << '\t' << (duration ? duration->MaxDuration : 0) << '\n';
            }
        for (auto effect : effects)
            if (effect->SpellID == identity && !effect->DifficultyID)
            {
                std::cout << "EFFECT\t" << identity << '\t' << effect->EffectIndex << '\t' << effect->Effect << '\t'
                    << effect->EffectAura << '\t' << effect->EffectMiscValue[0] << '\t' << effect->EffectMiscValue[1]
                    << '\t' << effect->EffectBasePoints << '\t' << effect->EffectTriggerSpell << '\t'
                    << effect->ImplicitTarget[0] << '\t' << effect->ImplicitTarget[1] << '\n';
                if (effect->EffectTriggerSpell)
                    std::cout << "TRIGGER_REFERENCE\t" << identity << '\t' << effect->EffectTriggerSpell << '\t'
                        << bool(spells.LookupEntry(effect->EffectTriggerSpell)) << '\n';
            }
        for (auto visual : visuals)
            if (visual->SpellID == identity)
                std::cout << "VISUAL\t" << identity << '\t' << visual->SpellVisualID << '\t'
                    << unsigned(visual->DifficultyID) << '\n';
    }
    HotfixDatabase.Close();
    MySQL::Library_End();
}
