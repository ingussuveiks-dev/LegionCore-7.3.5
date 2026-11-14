#include <unordered_set>
#include "Player.h"
#include "MiscPackets.h"
#include "MailPackets.h"
#include "TicketMgr.h"
#include "World.h"
#include "CommandIntegerArgument.h"
#include <iostream>
#include <type_traits>
int main()
{
    static_assert(std::is_same<decltype(std::declval<Unit&>().m_baseRatingValue[0]), int16&>::value, "Native rating cache is int16");
    static_assert(std::is_same<decltype(&Unit::SetEffectiveLevel), void(Unit::*)(uint8)>::value, "Effective level native byte");
    static_assert(std::is_same<decltype(&Unit::SetMaxItemLevel), void(Unit::*)(uint16)>::value, "Item level native word");
    static_assert(std::is_same<decltype(&Unit::SetMinItemLevel), void(Unit::*)(uint16)>::value, "Item level native word");
    static_assert(std::is_same<decltype(&Player::SetSkill), void(Player::*)(uint16,uint16,uint16,uint16)>::value, "Native skill widths");
    static_assert(std::is_same<decltype(&World::ShutdownServ), void(World::*)(uint32,uint32,uint8)>::value, "Native lifecycle arguments");
    static_assert(std::is_same<decltype(&TicketMgr::GetTicket), GmTicket*(TicketMgr::*)(uint32)>::value, "Native ticket id");
    static_assert(std::is_same<decltype(std::declval<WorldPackets::Misc::PhaseShiftDataPhase>().Id),uint16>::value, "Native phase word");
    static_assert(CRITERIA_TYPE_TOTAL==213 && MAX_DIFFICULTY==35 && INVENTORY_SLOT_BAG_0==255, "Matching Legion catalog bounds");
    WorldPackets::Mail::MailCommandResult result;
    result.Command=MAIL_MONEY_TAKEN;result.ErrorCode=MAIL_ERR_ITEM_HAS_EXPIRED;
    auto packet=result.Write();
    if(packet->size()!=24 || packet->read<uint32>(4)!=1 || packet->read<uint32>(8)!=21)return 1;
    WorldPackets::Misc::PhaseShiftDataPhase phase(65535);
    if(phase.Id!=65535)return 2;
    int16 rating=7;
    if(Trinity::ParseCommandInteger("32768",rating)||rating!=7)return 3;
    uint16 skill=7;
    if(Trinity::ParseCommandInteger("65536",skill)||skill!=7)return 4;
    std::cout<<"PASS matching native rating/level/item-level/skill/ticket/lifecycle/phase widths,criteria/difficulty bounds and real24-byte mail serializer;no live database or character mutation\n";
    return 0;
}
