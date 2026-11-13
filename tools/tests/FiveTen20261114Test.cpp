// Actual complete selected command handlers/summon callback. External services
// are deterministic in-memory endpoints; no real account/character writes.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>
#include "../../src/server/game/Chat/CommandIntegerArgument.h"
using uint8=uint8_t;using uint16=uint16_t;using uint32=uint32_t;using int32=int32_t;using uint64=uint64_t;
static void Require(bool b,char const* message){if(!b)throw std::runtime_error(message);}
#include "Constants.inc"
struct ObjectGuid{static ObjectGuid Empty;};ObjectGuid ObjectGuid::Empty;
struct Session;
struct Player {
 uint32 writes=0,maxPower=0,power=0,powerType=0,selectedTitle=2;uint64 titles=5,higherTitles=0x123456789abcdef;
 uint32 spells=0;float lastDistance=0;Session* session=nullptr;
 Session* GetSession(){return session;}
 void SetMaxPower(uint32 t,int32 v){++writes;powerType=t;maxPower=v;}
 void SetPower(uint32 t,int32 v){++writes;powerType=t;power=v;}
 void SetUInt64Value(uint32 field,uint64 v){Require(field==PLAYER_FIELD_KNOWN_TITLES,"Wrong title field");++writes;titles=v;}
 void SetUInt32Value(uint32 field,uint32 v){Require(field==PLAYER_FIELD_PLAYER_TITLE,"Wrong chosen-title field");++writes;selectedTitle=v;}
 int32 GetInt32Value(uint32 field){Require(field==PLAYER_FIELD_PLAYER_TITLE,"Wrong title lookup");return selectedTitle;}
 bool HasTitle(uint32 bit){return bit<64?bool(titles&(uint64(1)<<bit)):bit<128&&bool(higherTitles&(uint64(1)<<(bit-64)));}
 void GetClosePoint(float&x,float&y,float&z,float distance){lastDistance=distance;x=distance;y=1;z=2;}
 void CastSpell(float x,float y,float z,uint32 id,bool){Require(std::isfinite(x)&&std::isfinite(y)&&std::isfinite(z)&&id==123,"Invalid cast destination");++spells;}
};
struct Session{Player*player=nullptr;uint32 account=1;Player*GetPlayer(){return player;}uint32 GetAccountId(){return account;}};
struct Creature {uint32 entry=0,path=77;uint32 GetEntry(){return entry;}uint32 GetWaypointPath(){return path;}};
struct ChatHandler {
 Player*target=nullptr;Creature*creature=nullptr;Session*session=nullptr;bool lower=false,error=false,report=false;uint32 messages=0,databaseReached=0;
 ChatHandler()=default;explicit ChatHandler(Player*){}
 Session*GetSession(){return session;}Player*getSelectedPlayer(){return target;}Creature*getSelectedCreature(){return creature;}
 bool HasLowerSecurity(Player*,ObjectGuid){return lower;}bool needReportToTarget(Player*){return report;}
 std::string GetNameLink(Player* =nullptr){return "fixture";}
 template<class...T>void PSendSysMessage(T...){++messages;}template<class...T>void SendSysMessage(T...){++messages;}
 void SetSentErrorMessage(bool b){error=b;}
 uint32 extractSpellIdFromLink(char*args){char*id=strtok(args," ");return id?uint32(strtoul(id,nullptr,10)):0;}
};
struct CharTitlesEntry{int32 MaskID;};
struct TitleStore{std::vector<CharTitlesEntry> rows{{0},{0},{2},{63},{64},{130},{-1}};uint32 GetNumRows(){return uint32(rows.size());}CharTitlesEntry const*LookupEntry(uint32 i){return i<rows.size()?&rows[i]:nullptr;}}sCharTitlesStore;
struct SpellInfo{} info;
struct SpellManager{bool exists=true;SpellInfo const*GetSpellInfo(uint32 id){return exists&&id==123?&info:nullptr;}}spellMgr;
auto sSpellMgr=&spellMgr;
struct SpellMgr{static bool IsSpellValid(SpellInfo const*,Player*){return true;}};
static bool Utf8ToUpperOnlyLatin(std::string&s){if(s=="INVALID_UTF8")return false;for(char&c:s)if(c>='a'&&c<='z')c-=32;return true;}
struct LoginDatabasePreparedStatement{uint32 id=0;std::map<uint32,int32> fields;void setUInt32(uint32 i,uint32 v){fields[i]=int32(v);}void setUInt8(uint32 i,uint8 v){fields[i]=v;}void setInt32(uint32 i,int32 v){fields[i]=v;}};
using PreparedQueryResult=bool;
struct LoginService {
 LoginDatabasePreparedStatement pending;bool conflict=false;std::vector<LoginDatabasePreparedStatement>writes;
 LoginDatabasePreparedStatement*GetPreparedStatement(uint32 id){pending={id,{}};return &pending;}
 bool Query(LoginDatabasePreparedStatement*){return conflict;}
 void Execute(LoginDatabasePreparedStatement*p){writes.push_back(*p);}
}LoginDatabase;
namespace AccountMgr {
 static uint32 ownerSecurity=6,targetSecurity=0;static bool missing=false;
 uint32 GetId(std::string const&name){return missing||name=="MISSING"?0:2;}
 uint32 GetSecurity(uint32 account,int32){return account==1?ownerSecurity:targetSecurity;}
 bool IsConsoleAccount(uint32 value){return value==SEC_CONSOLE;}
}
struct Realm{struct{uint32 Realm=1;}Id;}realm;
struct EchoUnit {std::vector<uint32>casts,animations;void CastSpell(EchoUnit*,uint32 id,bool){casts.push_back(id);}void PlayOneShotAnimKit(uint32 id){animations.push_back(id);}};
using Unit=EchoUnit;
struct Events{uint32 event=0,time=0;void RescheduleEvent(uint32 e,uint32 t){event=e;time=t;}};
struct EchoBase{EchoUnit*me=nullptr;Events events;virtual void IsSummonedBy(Unit*){}};
static uint32 randomChoice=0;
struct BadRange:std::runtime_error{BadRange():std::runtime_error("Out-of-array RNG range") {}};
static uint32 urand(uint32 lo,uint32 hi){if(lo!=0||hi!=2)throw BadRange();Require(randomChoice<=hi,"Choice outside array");return randomChoice;}
#include "Selected.inc"
static uint32 cases=0;
template<class Fn>static bool Command(Fn fn,ChatHandler&h,char const*args){std::string text=args;return fn(&h,text.c_str());}
static void PowerTests(bool(*fn)(ChatHandler*,char const*),uint32 expected){
 for(char const*arg:{""," ","0","-1","1x","214748365","2147483648","4294967296","999999999999999999999"}){Player p;ChatHandler h;h.target=&p;Require(!Command(fn,h,arg)&&!p.writes,"Invalid scaled power changed player");++cases;}
 for(char const*arg:{"1","100","214748364"," +100 "}){Player p;ChatHandler h;h.target=&p;h.report=true;Require(Command(fn,h,arg)&&p.writes==2&&p.maxPower==p.power&&p.powerType==expected,"Valid scaled power changed");Require(p.power==uint32(std::strtol(arg,nullptr,10))*10,"Power scale no longer ten");++cases;}
 for(bool selected:{false,true}){Player p;ChatHandler h;h.target=selected?&p:nullptr;h.lower=true;Require(!Command(fn,h,"100")&&!p.writes,"Absent/unauthorized power target changed");++cases;}
}
int main(){try{
#ifdef REPAIR_WAYPOINT
 for(bool supplied:{false,true})for(uint32 entry:{0u,VISUAL_WAYPOINT,7u}){
  Creature c;c.entry=entry;ChatHandler h;h.creature=entry?&c:nullptr;
  Require(Command(CaseWaypoint::Selected::HandleWpShowCommand,h,supplied?"info 77":"info")==bool(entry==VISUAL_WAYPOINT),"Info actor guard differs");
  Require(h.databaseReached==uint32(entry==VISUAL_WAYPOINT),"Invalid info target reached DB boundary");++cases;
 }
#endif
#ifdef REPAIR_ACCOUNT
 Player target,caller;Session targetSession{&target,2},callerSession{&caller,1};target.session=&targetSession;caller.session=&callerSession;
 for(char const*arg:{""," ","1","ACCOUNT x 1","ACCOUNT 1x 1","ACCOUNT -1 1","ACCOUNT 1 realm","ACCOUNT 1 2147483648","ACCOUNT 4294967296 1","ACCOUNT 1 -2","MISSING 1 1","INVALID_UTF8 1 1","1x 1","1 realm"}){
  ChatHandler h;h.target=&target;h.session=&callerSession;LoginDatabase.writes.clear();Require(!Command(CaseAccount::Selected::HandleAccountSetGmLevelCommand,h,arg)&&LoginDatabase.writes.empty(),"Invalid account/GM/realm changed access");++cases;
 }
 for(bool console:{false,true})for(bool named:{false,true})for(int32 requested:{-1,1,25,2147483647})for(uint32 gm:{0u,2u}){
  ChatHandler h;h.target=&target;h.session=console?nullptr:&callerSession;LoginDatabase.writes.clear();LoginDatabase.conflict=false;
  std::string arg=(named?"ACCOUNT ":"")+std::to_string(gm)+" "+std::to_string(requested);
  Require(Command(CaseAccount::Selected::HandleAccountSetGmLevelCommand,h,arg.c_str()),"Valid GM request refused");Require(LoginDatabase.writes.size()==(gm?2u:1u),"Unexpected access write count");
  auto const&del=LoginDatabase.writes.front();Require(del.fields.at(0)==2,"Wrong account deletion");Require(del.id==(requested==-1?LOGIN_DEL_ACCOUNT_ACCESS:LOGIN_DEL_ACCOUNT_ACCESS_BY_REALM),"Wrong access delete route");
#ifdef REPAIR_ACCOUNT_REALM
  if(requested!=-1)Require(del.fields.at(1)==requested,"Access deleted from server's realm instead of requested realm");
#endif
  if(gm){auto const&ins=LoginDatabase.writes.back();Require(ins.fields.at(0)==2&&ins.fields.at(1)==int32(gm)&&ins.fields.at(2)==requested,"Access inserted with wrong account/level/realm");}++cases;
 }
 for(uint32 denied:{0u,1u,2u}){ChatHandler h;h.target=&target;h.session=&callerSession;LoginDatabase.writes.clear();LoginDatabase.conflict=denied==2;AccountMgr::targetSecurity=denied==0?6:0;
  Require(!Command(CaseAccount::Selected::HandleAccountSetGmLevelCommand,h,denied==1?"ACCOUNT 6 1":"ACCOUNT 1 -1")&&LoginDatabase.writes.empty(),"Existing privilege guard bypassed");++cases;}
 AccountMgr::targetSecurity=0;LoginDatabase.conflict=false;
#endif
#ifdef REPAIR_CAST
 for(char const*arg:{"123 nonsense","123 5x","123 nan","123 inf","123 -inf","123 1e1000"}){Player p;Session s{&p,1};ChatHandler h;h.session=&s;Require(!Command(CaseCast::Selected::HandleCastDistCommand,h,arg)&&!p.spells,"Invalid distance cast");++cases;}
 for(char const*arg:{"123","123 0","123 5.25","123 -5","123 2 triggered","123 3 t"}){Player p;Session s{&p,1};ChatHandler h;h.session=&s;Require(Command(CaseCast::Selected::HandleCastDistCommand,h,arg)&&p.spells==1,"Ordinary/triggered/directional cast changed");++cases;}
#endif
#ifdef REPAIR_TITLES
 for(char const*arg:{""," ","abc","5x","-1","18446744073709551616"}){Player p;ChatHandler h;h.target=&p;Require(!Command(CaseTitles::Selected::HandleTitlesSetMaskCommand,h,arg)&&!p.writes&&p.titles==5,"Invalid mask erased existing titles");++cases;}
 for(uint64 mask:{uint64(0),uint64(5),uint64(1)<<63,std::numeric_limits<uint64>::max(),uint64(1)<<3}){Player p;ChatHandler h;h.target=&p;Require(Command(CaseTitles::Selected::HandleTitlesSetMaskCommand,h,std::to_string(mask).c_str()),"Valid mask refused");Require(p.titles==(mask&(uint64(5)|(uint64(1)<<63)))&&p.higherTitles==0x123456789abcdef,"Title mask width aliases high native ID or rewrites higher blocks");++cases;}
 {Player p;p.selectedTitle=64;ChatHandler h;h.target=&p;Require(Command(CaseTitles::Selected::HandleTitlesSetMaskCommand,h,"0")&&p.selectedTitle==64&&p.higherTitles==0x123456789abcdef,"Unchanged higher-block selected title was reset");++cases;}
 for(bool selected:{false,true}){Player p;ChatHandler h;h.target=selected?&p:nullptr;h.lower=true;Require(!Command(CaseTitles::Selected::HandleTitlesSetMaskCommand,h,"5")&&!p.writes,"Title security/selection guard changed");++cases;}
#endif
#ifdef REPAIR_ENERGY
 PowerTests(CaseEnergy::Selected::HandleModifyEnergyCommand,POWER_ENERGY);
#endif
#ifdef REPAIR_RAGE
 PowerTests(CaseRage::Selected::HandleModifyRageCommand,POWER_RAGE);
#endif
#ifdef REPAIR_RUNIC
 PowerTests(CaseRunic::Selected::HandleModifyRunicPowerCommand,POWER_RUNIC_POWER);
#endif
#ifdef REPAIR_SOUL
 for(randomChoice=0;randomChoice<3;++randomChoice){EchoUnit source,actor;CaseSoul::Selected callback;callback.me=&actor;
#ifdef SOUL_RANGE_BASELINE
  bool bad=false;try{callback.IsSummonedBy(&source);}catch(BadRange const&){bad=true;}Require(bad,"Pinned invalid range not reproduced");
#else
  callback.IsSummonedBy(&source);Require(source.casts==std::vector<uint32>{194981}&&actor.animations==std::vector<uint32>{randomChoice==0?9038u:randomChoice==1?9039u:9176u}&&callback.events.time==4000,"Native clone/animation/damage schedule changed");
#endif
  ++cases;}
#endif
#ifdef SOUL_RANGE_BASELINE
 std::cout<<"COUNTERFACTUAL original summon callback requests9038..9176 for three-element array;range intercepted before undefined memory access;not repaired success\n";
#else
 std::cout<<"PASS "<<cases<<" selected production command/callback cases;native constants,controlled external services,no real account/character mutation. WP test ends before its first DB query;not whole waypoint editing/client traversal\n";
#endif
 return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
