#pragma once

#include "DB2HotfixGenerator.h"
#include "DB2LoadInfo.h"
#include "DB2Structure.h"
#include <cmath>
#include <vector>

namespace StromkarBladeVisual
{
// The old disabled storage omits the native relationship column. Keep its
// prefix type compatible with existing research readers, but register and
// serialize the complete26972 record for the actual client hotfix.
#pragma pack(push, 1)
struct Attachment
{
    SpellVisualKitModelAttachEntry value;
    int32 kit;
};
#pragma pack(pop)

inline DB2LoadInfo const* LoadInfo()
{
    static std::vector<DB2FieldMeta> fields = []
    {
        auto original = SpellVisualKitModelAttachLoadInfo::Instance();
        std::vector<DB2FieldMeta> result(original->Fields, original->Fields + original->FieldCount);
        result.push_back({true, FT_INT, "SpellVisualKitID"});
        return result;
    }();
    static DB2LoadInfo const info(fields.data(), fields.size(), SpellVisualKitModelAttachMeta::Instance(), HOTFIX_SEL_SPELL_VISUAL_KIT_MODEL_ATTACH);
    return &info;
}

inline bool Supported(Attachment const* row)
{
    if (!row)
        return false;
    auto const& v = row->value;
    return row->kit == 64756 && v.ID == 318611 && v.SpellVisualEffectNameID == 26999 &&
        v.AttachmentID == 255 && v.Flags == 0 && v.PositionerID == 19 &&
        std::fabs(v.Offset[0] - 0.1f) < 0.000001f && std::fabs(v.Offset[1] - 0.25f) < 0.000001f &&
        std::fabs(v.Offset[2] - 0.45f) < 0.000001f &&
        std::fabs(v.Yaw - 4.18879f) < 0.00001f && std::fabs(v.Pitch - 0.872665f) < 0.00001f &&
        v.Roll == 0.0f && (v.Scale == 0.025f || v.Scale == 1.0f);
}

inline void Fix(Attachment* row)
{
    // Full item scale is authored by the native Strom'kar item kits65550/65551.
    // Preserve this embedded kit's original position, tilt, model and lifetime.
    row->value.Scale = 1.0f;
}

inline bool Apply(DB2Storage<Attachment>& store)
{
    if (!Supported(store.LookupEntry(318611)))
        return false;
    DB2HotfixGenerator<Attachment> hotfix(store);
    hotfix.ApplyHotfix(318611, Fix, true);
    return true;
}
}
