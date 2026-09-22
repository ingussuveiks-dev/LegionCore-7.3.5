#ifndef LEGION_BATTLE_PAY_BOOST_CHOICE_H
#define LEGION_BATTLE_PAY_BOOST_CHOICE_H

#include <cstdint>

namespace WorldPackets
{
namespace BattlePay
{
    struct BoostChoice
    {
        bool Valid;
        std::uint16_t SpecializationID;
        std::uint16_t FactionChoice;
    };

    // Legion packs the specialization into bits 0..23 and the zero-based
    // faction (Horde=0, Alliance=1) into bits 24..31. Our service uses 1/2.
    constexpr BoostChoice DecodeBoostChoice(std::uint32_t packed)
    {
        return (packed >> 24) <= 1 && (packed & 0x00FFFFFFu) > 0 &&
            (packed & 0x00FFFFFFu) <= 0xFFFFu
            ? BoostChoice{true, static_cast<std::uint16_t>(packed & 0x00FFFFFFu),
                static_cast<std::uint16_t>((packed >> 24) + 1)}
            : BoostChoice{false, 0, 0};
    }
}
}

#endif
