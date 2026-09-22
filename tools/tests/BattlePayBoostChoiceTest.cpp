#include "../../src/server/game/Server/Packets/BattlePayBoostChoice.h"

using WorldPackets::BattlePay::DecodeBoostChoice;

static_assert(DecodeBoostChoice(0x0000010Du).Valid, "Horde Monk choice");
static_assert(DecodeBoostChoice(0x0000010Du).SpecializationID == 269, "Monk spec");
static_assert(DecodeBoostChoice(0x0000010Du).FactionChoice == 1, "Horde mapping");
static_assert(DecodeBoostChoice(0x0100010Du).Valid, "Alliance Monk choice");
static_assert(DecodeBoostChoice(0x0100010Du).SpecializationID == 269, "Keep spec");
static_assert(DecodeBoostChoice(0x0100010Du).FactionChoice == 2, "Alliance mapping");
static_assert(DecodeBoostChoice(0x00000109u).SpecializationID == 265, "Warlock spec");
static_assert(DecodeBoostChoice(0x0100FFFFu).Valid, "Representable upper boundary");
static_assert(!DecodeBoostChoice(0x0200010Du).Valid, "Reject faction 2");
static_assert(!DecodeBoostChoice(0xFF00010Du).Valid, "Reject faction 255");
static_assert(!DecodeBoostChoice(0x0001010Du).Valid, "Never truncate spec to 269");
static_assert(!DecodeBoostChoice(0x01010000u).Valid, "Reject 24-bit spec overflow");
static_assert(!DecodeBoostChoice(0).Valid, "Reject zero spec");
static_assert(!DecodeBoostChoice(0x01000000u).Valid, "Reject Alliance zero spec");

int main() { return 0; }
