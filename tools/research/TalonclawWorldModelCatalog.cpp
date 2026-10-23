#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "DB2Meta.h"
#include <iostream>
#include <vector>

#pragma pack(push, 1)
struct KitEffectRecord
{
    SpellVisualKitEffectEntry effect;
    int32 kit;
};
struct KitAttachmentRecord
{
    SpellVisualKitModelAttachEntry attachment;
    int32 kit;
};
struct EffectNameRecord
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
    DB2Storage<CreatureDisplayInfoEntry> displays("CreatureDisplayInfo.db2", CreatureDisplayInfoLoadInfo::Instance());
    DB2Storage<CreatureModelDataEntry> models("CreatureModelData.db2", CreatureModelDataLoadInfo::Instance());
    DB2Storage<SpellVisualKitEntry> kits("SpellVisualKit.db2", SpellVisualKitLoadInfo::Instance());
    auto effectInfo = SpellVisualKitEffectLoadInfo::Instance();
    std::vector<DB2FieldMeta> effectFields(effectInfo->Fields, effectInfo->Fields + effectInfo->FieldCount);
    effectFields.push_back({true, FT_INT, "SpellVisualKitID"});
    DB2LoadInfo fullEffectInfo(effectFields.data(), effectFields.size(), SpellVisualKitEffectMeta::Instance(), HOTFIX_SEL_SPELL_VISUAL_KIT_EFFECT);
    auto attachmentInfo = SpellVisualKitModelAttachLoadInfo::Instance();
    std::vector<DB2FieldMeta> attachmentFields(attachmentInfo->Fields, attachmentInfo->Fields + attachmentInfo->FieldCount);
    attachmentFields.push_back({true, FT_INT, "SpellVisualKitID"});
    DB2LoadInfo fullAttachmentInfo(attachmentFields.data(), attachmentFields.size(), SpellVisualKitModelAttachMeta::Instance(), HOTFIX_SEL_SPELL_VISUAL_KIT_MODEL_ATTACH);
    DB2Storage<KitEffectRecord> effects("SpellVisualKitEffect.db2", &fullEffectInfo);
    DB2Storage<KitAttachmentRecord> attachments("SpellVisualKitModelAttach.db2", &fullAttachmentInfo);
    DB2FieldMeta nameFields[] = {
        {true, FT_INT, "ID"}, {false, FT_FLOAT, "EffectRadius"}, {false, FT_FLOAT, "BaseMissileSpeed"},
        {false, FT_FLOAT, "Scale"}, {false, FT_FLOAT, "MinAllowedScale"}, {false, FT_FLOAT, "MaxAllowedScale"},
        {false, FT_FLOAT, "Alpha"}, {false, FT_INT, "Flags"}, {true, FT_INT, "GenericID"},
        {true, FT_INT, "TextureFileDataID"}, {false, FT_BYTE, "Type"}, {true, FT_INT, "ModelFileDataID"},
        {false, FT_INT, "RibbonQualityID"}, {true, FT_INT, "DissolveEffectID"}
    };
    DB2LoadInfo nameInfo(nameFields, std::size(nameFields), SpellVisualEffectNameMeta::Instance(), HOTFIX_SEL_SPELL_VISUAL_EFFECT_NAME);
    DB2Storage<EffectNameRecord> names("SpellVisualEffectName.db2", &nameInfo);
    if (sizeof(KitEffectRecord) != fullEffectInfo.Meta->GetRecordSize() || sizeof(KitAttachmentRecord) != fullAttachmentInfo.Meta->GetRecordSize() ||
        sizeof(EffectNameRecord) != nameInfo.Meta->GetRecordSize())
        return 8;
    if (!displays.Load(argv[1], 0) || !models.Load(argv[1], 0))
        return 2;
    if (!kits.Load(argv[1], 0) || !effects.Load(argv[1], 0) || !attachments.Load(argv[1], 0) || !names.Load(argv[1], 0))
        return 4;
    for (uint32 id : {33225u, 66609u})
    {
        auto display = displays.LookupEntry(id);
        auto model = display ? models.LookupEntry(display->ModelID) : nullptr;
        if (!model)
            return 3;
        std::cout << "MODEL\t" << id << '\t' << display->ModelID << '\t' << model->FileDataID << '\t'
            << display->CreatureModelScale << '\t' << model->ModelScale << '\t' << uint32(display->CreatureModelAlpha) << '\t'
            << display->StateSpellVisualKitID << '\t' << display->ObjectEffectPackageID << '\n';
    }
    if (!kits.LookupEntry(61290))
        return 5;
    bool found = false;
    for (auto effect : effects)
        if (effect->kit == 61290)
        {
            std::cout << "KIT_EFFECT\t" << effect->kit << '\t' << effect->effect.EffectType << '\t' << effect->effect.Effect << '\n';
            if (effect->effect.EffectType == 2)
            {
                auto attachment = attachments.LookupEntry(effect->effect.Effect);
                auto name = attachment ? names.LookupEntry(attachment->attachment.SpellVisualEffectNameID) : nullptr;
                if (!name)
                    return 6;
                auto const& visual = attachment->attachment;
                std::cout << "ATTACHMENT\t" << visual.ID << '\t' << visual.SpellVisualEffectNameID << '\t'
                    << uint32(visual.AttachmentID) << '\t' << visual.Scale << '\t' << visual.AnimKitID << '\t'
                    << name->model << '\t' << name->alpha << '\t' << uint32(name->type) << '\t' << name->generic << '\n';
                found = true;
            }
        }
    return found ? 0 : 7;
}
