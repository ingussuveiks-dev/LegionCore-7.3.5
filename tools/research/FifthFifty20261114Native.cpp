#include <unordered_set>
#include "Chat.h"
#include "Transport.h"
#include "Creature.h"
#include "ObjectMgr.h"
#include "PlayerDump.h"
#include "SpellAuras.h"
#include "WorldSession.h"
#include "SpellPackets.h"
#include "BattlegroundDefines.h"
#include "LFGList.h"
#include "Group.h"
#include <iostream>
#include <type_traits>
int main()
{
    static_assert(std::is_same<ObjectGuid::LowType,uint64>::value,"Native64-bit spawn keys");
    static_assert(std::is_same<decltype(&Transport::CreateNPCPassenger),Creature*(Transport::*)(ObjectGuid::LowType,CreatureData const*)>::value,"Native fallible passenger creation");
    static_assert(std::is_same<decltype(&ObjectMgr::DeleteCreatureData),void(ObjectMgr::*)(ObjectGuid::LowType const&)>::value,"Native owned record cleanup");
    static_assert(std::is_same<decltype(&Creature::LoadCreatureFromDB),bool(Creature::*)(ObjectGuid::LowType,Map*,bool)>::value,"Native DB reload counter");
    static_assert(std::is_same<decltype(&Aura::SetCharges),void(Aura::*)(uint8)>::value,"Native charge byte");
    static_assert(std::is_same<decltype(&WorldSession::SendPetBattleRequestFailed),void(WorldSession::*)(uint8)>::value,"Native pet response byte");
    static_assert(std::is_same<std::underlying_type<LFGListStatus>::type,uint8>::value,"Native LFG listing status byte");
    static_assert(MAX_RAID_SIZE==40&&SPELL_FAILED_UNKNOWN==289,"Matching7.3.5 bounds");
    if(ObjectGuid::GetMaxCounter(HighGuid::Player)!=0xffffffffffULL)return 1;
    uint64 counter=4294967303ULL;
    if(Trinity::StringFormat("GUID " UI64FMTD,counter)!="GUID 4294967303")return 2;
    ChatHandler handler(static_cast<WorldSession*>(nullptr));
    for(char const*text:{"7x","-1","4294967296","7 8"})
    {
        std::string input=text;
        CommandArgs args(&handler,input.data(),{CommandArgs::ARG_UINT});
        if(args.ValidArgs())return 3;
    }
    for(char const*text:{"nan","inf","1e100","2x"})
    {
        std::string input=text;
        CommandArgs args(&handler,input.data(),{CommandArgs::ARG_FLOAT});
        if(args.ValidArgs())return 4;
    }
    std::string text="4294967295 -2147483648 -12.5";
    CommandArgs args(&handler,text.data(),{CommandArgs::ARG_UINT,CommandArgs::ARG_INT,CommandArgs::ARG_FLOAT});
    if(!args.ValidArgs()||args.GetArg<uint32>(0)!=UINT32_MAX||args.GetArg<int32>(1)!=INT32_MIN||args.GetArg<float>(2)!=-12.5f)return 5;
    std::string quote="\"";
    CommandArgs quoted(&handler,quote.data(),{CommandArgs::ARG_QUOTE_ENCLOSED_STRING});
    if(quoted.ValidArgs())return 6;
    for(char const*type:{"creature","gameobject"})
        for(char const*bad:{"7x","-1","1099511627776","18446744073709551616"})
        {
            std::string link=std::string("|cffffffff|H")+type+":"+bad+"|h[Invalid]|h|r";
            if(handler.extractGuidFromLink(link.data()))return 7;
        }
    for(char const*bad:{"4294967296","18446744073709551616"})
    {
        std::string input=bad;
        if(handler.extractGameTeleFromLink(input.data()))return 8;
    }
    WorldPackets::Spells::CastFailed packet(SMSG_CAST_FAILED);
    packet.SpellID=133;packet.Reason=289;packet.FailedArg1=INT32_MIN;packet.FailedArg2=INT32_MAX;
    WorldPacket bytes(*packet.Write());ObjectGuid guid;int32 spell,visual,reason,first,second;
    bytes>>guid>>spell>>visual>>reason>>first>>second;
    if(guid||spell!=133||visual||reason!=289||first!=INT32_MIN||second!=INT32_MAX||bytes.rpos()!=bytes.size())return 9;
    if(MS::Battlegrounds::GetJoinTypeByBracketSlot(255)!=0xff||MS::Battlegrounds::GetJoinTypeByBracketSlot(1)!=MS::Battlegrounds::JoinType::Arena3v3)return 10;
    std::cout<<"PASS real linked typed-argument bounds/boost::any/native parsers,malformed world GUID/teleport links,7.3.5 CastFailed roundtrip,bracket mapping,40-bit counters,64-bit formatter and passenger/cleanup/aura/pet/LFG method contracts;no database/world/player mutation\n";
    return 0;
}
