#include <unordered_set>
#include "Chat.h"
#include "Player.h"
#include "GameObject.h"
#include "CreatureGroups.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "Mail.h"
#include "LFGMgr.h"
#include "Scenario.h"
#include "World.h"
#include <iostream>
#include <type_traits>
int main()
{
    static_assert(std::is_same<ObjectGuid::LowType,uint64>::value,"Native GUID width");
    static_assert(std::is_same<decltype(FormationInfo{}.leaderGUID),uint64>::value,"Native formation leader width");
    static_assert(std::is_same<decltype(&Guild::ChangeMemberRank),bool(Guild::*)(ObjectGuid,uint8)>::value,"Native guild rank");
    static_assert(std::is_same<decltype(&GuildMgr::GetGuildById),Guild*(GuildMgr::*)(ObjectGuid::LowType) const>::value,"Native guild lookup key");
    static_assert(std::is_same<decltype(&GameObject::SetRespawnTime),void(GameObject::*)(int32)>::value,"Native signed respawn");
    static_assert(std::is_same<decltype(&Scenario::SetCurrentStep),void(Scenario::*)(uint8)>::value,"Native scenario step");
    static_assert(std::is_same<decltype(&MailDraft::AddMoney),MailDraft&(MailDraft::*)(uint64)>::value,"Native64-bit mail money");
    static_assert(std::is_same<lfg::LfgDungeonSet::value_type,uint32>::value,"Native LFG dungeon fields");
    static_assert(std::is_same<decltype(&Item::SetModifier),void(Item::*)(ItemModifier,uint32)>::value,"Native keystone fields");
    static_assert(MAX_SPELL_EFFECTS==32 && CRITERIA_TYPE_TOTAL==213 && MAX_MAIL_ITEMS==12 && SEC_CONSOLE==7,"Matching native bounds");
    if(ObjectGuid::GetMaxCounter(HighGuid::Player)!=UINT64_C(0xffffffffff)||ObjectGuid::GetMaxCounter(HighGuid::Creature)!=UINT64_C(0xffffffffff))return 1;
    uint64 wide=(uint64(1)<<32)+7;
    if(Trinity::StringFormat("GUID " UI64FMTD,wide)!="GUID 4294967303")return 2;
    ChatHandler handler(static_cast<WorldSession*>(nullptr));
    for(char const*text:{"123x","-1","4294967419","18446744073709551616"})
    {
        std::string input=text;
        if(handler.extractSpellIdFromLink(input.data())!=0)return 3;
    }
    for(auto const&entry:std::vector<std::pair<std::string,uint32>>{{"123",123},{"4294967295",UINT32_MAX},{"|Hspell:123|h[Spell Name]|h|r",123},{"|cffffffff|Henchant:123|h[Recipe]|h|r",123}})
    {
        auto input=entry.first;
        if(handler.extractSpellIdFromLink(input.data())!=entry.second)return 4;
    }
    // These catalog misses exercise the real core's return contract. No known
    // talent/glyph record, world/player instance, or database is fabricated.
    for(char const*text:{"|cffffffff|Htalent:4294967295|h[Missing]|h|r","|cffffffff|Hglyph:0:4294967304|h[Missing]|h|r"})
    {
        std::string input=text;
        if(handler.extractSpellIdFromLink(input.data())!=0)return 5;
    }
    std::cout<<"PASS real linked native spell-link numeric/rejected/talent-miss contracts,40-bit world counters,64-bit GUID formatter,rank/formation/respawn/step/mail/key/LFG widths and matching32/213/12/7 bounds;no live DB/world/character mutation\n";
    return 0;
}
