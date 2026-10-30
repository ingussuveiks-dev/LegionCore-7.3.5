#pragma once

#include "DB2Structure.h"

namespace StromkarGroundCrackData
{
constexpr uint32 Record = 10000;
constexpr uint32 Area = 900001;
constexpr uint32 Duration = 2000;
constexpr float Radius = 14.7181f;

// A custom projection of the original black/alpha797909 texture. These are
// guarded adaptations of native26972 DecalProperties24/TextureBlendSet21;
// they do not alter either original record or the authored burst/splash kit.
inline bool Supported(DecalPropertiesEntry const* d, TextureBlendSetEntry const* t)
{
    if (!d || !t || d->ID != Record || t->ID != Record || d->FileDataID ||
        d->InnerRadius != 0.5f || d->OuterRadius != 0.5f || d->Rim != 0.0f ||
        d->Gain != 4.0f || d->ModX != 4.0f || d->Scale != 1.0f || d->FadeIn != 0.5f || d->FadeOut != 0.5f ||
        d->Priority != 75 || d->BlendMode || d->TopTextureBlendSetID != Record || d->BotTextureBlendSetID != Record ||
        d->GameFlags || d->Flags || d->CasterDecalPropertiesID ||
        t->TextureFileDataID[0] != 797909 || t->TextureFileDataID[1] != 1002607 || t->TextureFileDataID[2] != 1002607 ||
        t->SwizzleRed != 0 || t->SwizzleGreen != 1 || t->SwizzleBlue != 2 || t->SwizzleAlpha != 3)
        return false;
    for (uint32 i = 0; i < 3; ++i)
        if (t->TextureScrollRateU[i] || t->TextureScrollRateV[i] || t->TextureScaleU[i] != 1.0f || t->TextureScaleV[i] != 1.0f)
            return false;
    for (float value : t->ModX)
        if (value != 4.0f)
            return false;
    return true;
}
}
