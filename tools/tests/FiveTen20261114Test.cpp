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
#include <set>
#include <functional>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>
#include "../../src/server/game/Chat/CommandIntegerArgument.h"
#ifdef REPAIR_FLOAT_ARGUMENT
#include "../../src/server/game/Chat/CommandFloatArgument.h"
#endif
using uint8=uint8_t;using uint16=uint16_t;using uint32=uint32_t;using int32=int32_t;using uint64=uint64_t;using int64=int64_t;
static void Require(bool b,char const* message){if(!b)throw std::runtime_error(message);}
#include "Constants.inc"
struct ObjectGuid{using LowType=uint64;uint64 GetGUIDLow(){return 1;}std::string ToString(){return "controlled:1";}static ObjectGuid Empty;};ObjectGuid ObjectGuid::Empty;
#define TC_LOG_DEBUG(...) ((void)0)
#include "NativeTokenizer.inc"
struct Session;struct Player;struct Creature;
struct Item{uint64 xp=100;uint32 writes=0,states=0;Player*owner=nullptr;uint64 GetUInt64Value(uint32 field){Require(field==ITEM_FIELD_ARTIFACT_XP,"Wrong artifact field read");return xp;}void SetUInt64Value(uint32 field,uint64 value){Require(field==ITEM_FIELD_ARTIFACT_XP,"Wrong artifact field write");++writes;xp=value;}void SetState(uint32 state,Player*player){Require(state==ITEM_CHANGED,"Wrong item state");++states;owner=player;}};
struct PhaseManager{uint32 writes=0,mask=0;void SetCustomPhase(uint32 value){++writes;mask=value;}};
struct Unit {
 uint32 writes=0,powerType=0,selectedTitle=2,spells=0,stats=0,ratings=0,nativeMoneyErrors=0;int32 maxPower=0,power=0;
 uint64 titles=5,higherTitles=0x123456789abcdef,money=100;float lastDistance=0,scale=1,average=0;uint64 health=100,maxHealth=100;uint32 display=0,phase=1,drunk=0;std::set<uint32>phaseIds;PhaseManager phaseMgr;Item*artifact=nullptr;bool flying=false,pvp=false;Session*session=nullptr;
 std::map<uint32,float>speeds;std::map<uint16,uint32>flags;std::vector<uint32>casts,animations;
 virtual ~Unit()=default;virtual Player*ToPlayer(){return nullptr;}virtual Creature*ToCreature(){return nullptr;}
 uint32 GetTypeId(){return ToPlayer()?TYPEID_PLAYER:3;}uint32 GetValuesCount(){return ToPlayer()?PLAYER_FIELD_END:UNIT_END;}
 Session* GetSession(){return session;}bool isInFlight(){return flying;}
 void SetMaxPower(uint32 t,int32 v){++writes;powerType=t;maxPower=v;}
 void SetPower(uint32 t,int32 v){++writes;powerType=t;power=v;}
 void SetUInt64Value(uint32 field,uint64 v){Require(field==PLAYER_FIELD_KNOWN_TITLES,"Wrong title field");++writes;titles=v;}
 void SetUInt32Value(uint32 field,uint32 v){++writes;if(field==PLAYER_FIELD_PLAYER_TITLE)selectedTitle=v;else flags[uint16(field)]=v;}
 uint32 GetUInt32Value(uint32 field){return flags[uint16(field)];}
 void SetHealth(uint64 value){++writes;health=value;}void SetMaxHealth(uint64 value){++writes;maxHealth=value;}
 void SetDrunkValue(uint8 value){++writes;drunk=value;}
 void SetDisplayId(uint32 value){++writes;display=value;}
 void SetPhaseMask(uint32 mask,bool){++writes;phase=mask;}
 PhaseManager&GetPhaseMgr(){return phaseMgr;}void SetPhaseId(std::set<uint32>const&ids,bool){++writes;phaseIds=ids;}
 Item*GetArtifactWeapon(){return artifact;}
 void ModifyCurrency(uint32 id,int32 amount,bool chat,bool ignore,bool week,bool season){Require(chat&&ignore&&!week&&!season,"Currency flags changed");++writes;powerType=id;power=amount;}
 int32 GetInt32Value(uint32 field){Require(field==PLAYER_FIELD_PLAYER_TITLE,"Wrong title lookup");return selectedTitle;}
 bool HasTitle(uint32 bit){return bit<64?bool(titles&(uint64(1)<<bit)):bit<128&&bool(higherTitles&(uint64(1)<<(bit-64)));}
 void GetClosePoint(float&x,float&y,float&z,float distance){lastDistance=distance;x=distance;y=1;z=2;}
 void CastSpell(float x,float y,float z,uint32 id,bool){Require(std::isfinite(x)&&std::isfinite(y)&&std::isfinite(z)&&id==123,"Invalid cast destination");++spells;}
 void CastSpell(Unit*,uint32 id,bool){casts.push_back(id);}void PlayOneShotAnimKit(uint32 id){animations.push_back(id);}
 void SetSpeed(uint32 mode,float value,bool){++writes;speeds[mode]=value;}
 void SetObjectScale(float value){++writes;scale=value;}
 void SetFloatValue(uint32 field,float value){Require(field==PLAYER_FIELD_AVG_ITEM_LEVEL+EquippedAvgItemLevel,"Wrong item-level field");++writes;average=value;}
 bool HasPvpStatsScalingEnabled(){return pvp;}void SendUpdateStat(uint32 mask){Require(mask==USM_ALL,"Wrong stats mask");++stats;}void SendUpdateCR(uint32 mask){Require(mask==CR_PVP_UPDATE_MASK,"Wrong rating mask");++ratings;}
 bool HasFlag(uint16 field,uint32 mask){return (flags[field]&mask)!=0;}void RemoveFlag(uint16 field,uint32 mask){++writes;flags[field]&=~mask;}void SetFlag(uint16 field,uint32 mask){++writes;flags[field]|=mask;}
 uint64 GetMoney(){return money;}void SetMoney(uint64 value){++writes;money=value;}
 bool ModifyMoney(int64 amount,bool sendError=true);
 void SendEquipError(uint32 code){Require(code==EQUIP_ERR_TOO_MUCH_GOLD,"Wrong native money error");++nativeMoneyErrors;}

};
struct MoneyScripts{std::function<void(int64&)>hook;template<class T>void OnPlayerMoneyChanged(T*,int64&value) {if(hook)hook(value);}} moneyScripts;auto sScriptMgr=&moneyScripts;
#include "NativeModifyMoney.inc"
struct Player:Unit{Player*ToPlayer()override{return this;}};
struct Session{Player*player=nullptr;uint32 account=1;Player*GetPlayer(){return player;}uint32 GetAccountId(){return account;}};
struct CreatureTemplate{uint32 faction=1;float scale=1.5f;};
struct CreatureData{float personalSize=3;};
struct CreatureAI{uint32 writes=0,key=0,value=0;void SetData(uint32 a,uint32 b){++writes;key=a;value=b;}};
struct MotionMaster{uint32 initializes=0;void Initialize(){++initializes;}};
struct Creature:Unit{uint32 entry=0,path=77,faction=1,level=1,saves=0,nativeDisplay=0,outfit=0,moveType=0,deathCalls=0,respawns=0,delay=0;bool pet=false,alive=true,hasAI=true;float radius=0;CreatureTemplate templateData;CreatureAI ai;MotionMaster motion;
 Creature*ToCreature()override{return this;}uint32 GetEntry(){return entry;}uint32 GetWaypointPath(){return path;}uint32 getFaction(){return faction;}void setFaction(uint32 value){++writes;faction=value;}ObjectGuid GetGUID(){return {};}
 bool isPet(){return pet;}void SetLevel(uint8 value){++writes;level=value;}void SaveToDB(){++saves;}CreatureTemplate const*GetCreatureTemplate(){return &templateData;}
 CreatureAI*AI(){return hasAI?&ai:nullptr;}std::string GetNPCAIName(){return "ControlledAI";}std::string GetScriptName(){return "";}char const*GetName(){return "Controlled";}
 void SetNativeDisplayId(uint32 value){++writes;nativeDisplay=value;}void SetOutfit(int32 value){++writes;outfit=uint32(value);}
 uint64 dbGuid=7;uint64 GetDBTableGUIDLow(){return dbGuid;}void SetRespawnRadius(float value){++writes;radius=value;}void SetDefaultMovementType(MovementGeneratorType value){++writes;moveType=value;}
 MotionMaster*GetMotionMaster(){return &motion;}bool IsAlive(){return alive;}void setDeathState(DeathState state){Require(state==JUST_DIED,"Unexpected death mode");++deathCalls;alive=false;}void Respawn(){++respawns;alive=true;}
 void SetRespawnDelay(uint32 value){++writes;delay=value;}
};
struct Pet:Creature{Pet(){pet=true;}PetType type=HUNTER_PET;PetType getPetType(){return type;}void GivePetLevel(uint8 value){++writes;level=value;}};
struct WorldConfig{uint32 getIntConfig(WorldIntConfigs key){Require(key==CONFIG_MAX_PLAYER_LEVEL,"Wrong level config");return 120;}}worldConfig;auto sWorld=&worldConfig;
struct ObjectManager{CreatureTemplate data;CreatureData spawn;uint32 GetXPForLevel(uint32 level){return level*100;}CreatureTemplate const*GetCreatureTemplate(uint32){return &data;}CreatureData&NewOrExistCreatureData(uint64 id){Require(id==7,"Wrong spawn");return spawn;}}objectManager;auto sObjectMgr=&objectManager;
struct WorldDatabasePreparedStatement{uint32 id=0;std::map<uint32,uint64>values;std::map<uint32,float>floats;void setUInt16(uint32 i,uint16 v){values[i]=v;}void setUInt32(uint32 i,uint32 v){values[i]=v;}void setUInt64(uint32 i,uint64 v){values[i]=v;}void setUInt8(uint32 i,uint8 v){values[i]=v;}void setFloat(uint32 i,float v){Require(std::isfinite(v),"Nonfinite DB write");floats[i]=v;}};
struct WorldService{WorldDatabasePreparedStatement pending;std::vector<WorldDatabasePreparedStatement>writes;WorldDatabasePreparedStatement*GetPreparedStatement(uint32 id){pending={id,{},{}};return &pending;}void Execute(WorldDatabasePreparedStatement*row){writes.push_back(*row);}}WorldDatabase;
struct ChatHandler {
 Player*target=nullptr;Unit*unit=nullptr;Creature*creature=nullptr;Session*session=nullptr;bool lower=false,error=false,report=false;uint32 messages=0,databaseReached=0;
 ChatHandler()=default;explicit ChatHandler(Player*){}
 Session*GetSession(){return session;}Player*getSelectedPlayer(){return target;}Unit*getSelectedUnit(){return unit?unit:target;}Creature*getSelectedCreature(){return creature;}
 bool HasLowerSecurity(Player*,ObjectGuid){return lower;}bool needReportToTarget(Player*){return report;}
 std::string GetNameLink(Player* =nullptr){return "fixture";}
 template<class...T>void PSendSysMessage(T...){++messages;}template<class...T>void SendSysMessage(T...){++messages;}
 void SetSentErrorMessage(bool b){error=b;}
 char*extractKeyFromLink(char*args,char const*){return strtok(args," ");}
 uint32 extractSpellIdFromLink(char*args){char*id=strtok(args," ");return id?uint32(strtoul(id,nullptr,10)):0;}
};
struct NativeRow{uint32 ID;};using CurrencyTypesEntry=NativeRow;using PhaseEntry=NativeRow;
struct NativeStore{std::map<uint32,NativeRow>rows;NativeRow const*LookupEntry(uint32 id){auto it=rows.find(id);return it==rows.end()?nullptr:&it->second;}}sFactionTemplateStore{{{1,{1}},{2,{2}}}},sCurrencyTypesStore{{{1,{1}},{2,{2}}}},sCreatureDisplayInfoStore{{{123,{123}}}},sPhaseStore{{{1,{1}},{2,{2}}}},sEmotesStore{{{1,{1}},{2,{2}}}};
struct PrecisionManager{float GetCurrencyPrecision(uint32 id){return id==2?100.f:1.f;}}sDB2Manager;
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
struct EchoUnit:Unit{};
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
#ifdef REPAIR_FLOAT_ARGUMENT
static void FloatTests(bool(*fn)(ChatHandler*,char const*),std::vector<uint32>const&modes,bool scale=false,bool flightAllowed=false){
 for(char const*arg:{""," ","nan","NaN(4)","inf","-inf","1e1000","1e-1000","0","-1","1x","1 2"}){Player p;ChatHandler h;h.target=&p;Require(!Command(fn,h,arg)&&!p.writes,"Invalid float reached unit mutation");++cases;}
 {Player p;ChatHandler h;h.target=&p;Require(!Command(fn,h,scale?"10.01":"50.01")&&!p.writes,"Existing upper range bypassed");++cases;}
 for(char const*arg:{"0.1","1","1.25","  +2.5 \t","0x1p1"}){Player p;ChatHandler h;h.target=&p;h.report=true;Require(Command(fn,h,arg),"Valid finite/native C float refused");float expected=std::strtof(arg,nullptr);
  if(scale)Require(p.writes==1&&p.scale==expected,"Native object scale write changed");
  else {Require(p.writes==modes.size()&&p.speeds.size()==modes.size(),"Speed command touched wrong modes");for(auto mode:modes)Require(p.speeds.at(mode)==expected,"Speed magnitude changed");}++cases;}
 {Player p;ChatHandler h;h.target=&p;h.lower=true;Require(!Command(fn,h,"2")&&!p.writes,"Float command bypassed security");++cases;}
 {ChatHandler h;Require(!Command(fn,h,"2"),"Float command accepted missing target");++cases;}
 {Player p;p.flying=true;ChatHandler h;h.target=&p;bool accepted=Command(fn,h,"2");Require(accepted==(flightAllowed||scale),"Existing in-flight policy changed");if(!accepted)Require(!p.writes,"Rejected flight changed speeds");++cases;}
 if(scale){Unit npc;ChatHandler h;h.unit=&npc;h.lower=true;Require(Command(fn,h,"2.5")&&npc.scale==2.5f&&npc.writes==1,"NPC scale incorrectly inherited Player security");++cases;}
}
#endif

static void NpcRejects(bool(*fn)(ChatHandler*,char const*),std::vector<char const*>args){
 for(auto arg:args){Creature c;ChatHandler h;h.creature=&c;WorldDatabase.writes.clear();objectManager.spawn.personalSize=3;Require(!Command(fn,h,arg)&&!c.writes&&!c.saves&&!c.ai.writes&&WorldDatabase.writes.empty()&&objectManager.spawn.personalSize==3,"Invalid NPC input mutated world/AI/persistence");++cases;}
 {ChatHandler h;WorldDatabase.writes.clear();Require(!Command(fn,h,"1")&&WorldDatabase.writes.empty(),"Absent NPC accepted");++cases;}
}
int main(){try{
#ifdef NPC_LEVEL_BASELINE
 Creature original;ChatHandler originalHandler;originalHandler.creature=&original;Require(Command(CaseNpcLevel::Selected::HandleNpcSetLevelCommand,originalHandler,"257")&&original.level==1&&original.saves==1,"Original narrowed level alias not reproduced");std::cout<<"COUNTERFACTUAL original NPC level257 wraps to1 and reaches controlled persistence;no real world DB writes\n";return 0;
#endif
#ifdef CORE_MONEY_BASELINE
 Player original;original.money=100;Require(original.ModifyMoney(int64(MAX_MONEY_AMOUNT)+1)&&original.money>MAX_MONEY_AMOUNT,"Original native unsigned cap underflow not reproduced");std::cout<<"COUNTERFACTUAL actual original core money accepts above-cap positive amount;controlled field only\n";return 0;
#endif
#ifdef ARTIFACT_XP_BASELINE
 Player original;Item item;item.xp=0;original.artifact=&item;ChatHandler originalHandler;originalHandler.target=&original;Require(Command(CaseArtifactXP::Selected::HandleModifyArtifactXPCommand,originalHandler,"-1")&&item.xp==UINT64_MAX,"Original negative raw artifact XP not reproduced");std::cout<<"COUNTERFACTUAL actual original artifact command converts-1 toUINT64_MAX;controlled item only\n";return 0;
#endif
#ifdef SPEED_FLOAT_BASELINE
 Player p;ChatHandler h;h.target=&p;Require(Command(CaseSpeed::Selected::HandleModifySpeedCommand,h,"nan")&&p.writes==1&&std::isnan(p.speeds[MOVE_RUN]),"Original nonfinite speed mutation not reproduced");
 std::cout<<"COUNTERFACTUAL original speed handler accepts NaN through ordered bounds and reaches native speed setter;controlled setter,no real movement/client mutation\n";return 0;
#endif
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
#ifdef REPAIR_FLOAT_ARGUMENT
 for(char const*arg:std::vector<char const*>{nullptr,""," ","nan","-inf","1e1000","1x","1 2"}){float value=7;Require(!Trinity::ParseCommandFloat(arg,value)&&value==7,"Rejected shared float changed receiver");++cases;}
 for(char const*arg:{"0","-5","+2.5"," 1e1\t","0x1p1","1e-1000"}){float value=7;Require(Trinity::ParseCommandFloat(arg,value)&&std::isfinite(value),"Native finite C syntax lost");++cases;}
#endif
#ifdef REPAIR_ASPEED
 FloatTests(CaseASpeed::Selected::HandleModifyASpeedCommand,{MOVE_WALK,MOVE_RUN,MOVE_SWIM,MOVE_FLIGHT});
#endif
#ifdef REPAIR_SPEED
 FloatTests(CaseSpeed::Selected::HandleModifySpeedCommand,{MOVE_RUN});
#endif
#ifdef REPAIR_SWIM
 FloatTests(CaseSwim::Selected::HandleModifySwimCommand,{MOVE_SWIM});
#endif
#ifdef REPAIR_BWALK
 FloatTests(CaseBWalk::Selected::HandleModifyBWalkCommand,{MOVE_RUN_BACK});
#endif
#ifdef REPAIR_FLY
 FloatTests(CaseFly::Selected::HandleModifyFlyCommand,{MOVE_FLIGHT},false,true);
#endif
#ifdef REPAIR_SCALE
 FloatTests(CaseScale::Selected::HandleModifyScaleCommand,{},true);
#endif
#ifdef REPAIR_ITEMLEVEL
 for(char const*arg:{""," ","nan","-inf","1e1000","0","-1","1000.01","1x"}){Player p;ChatHandler h;h.target=&p;Require(!Command(CaseItemLevel::Selected::HandleModifyItemsLevel,h,arg)&&!p.writes&&!p.stats&&!p.ratings,"Invalid item-level affected stats");++cases;}
 for(bool pvp:{false,true})for(char const*arg:{"0.5","1000","500.25 extra"}){Player p;p.pvp=pvp;ChatHandler h;h.target=&p;Require(Command(CaseItemLevel::Selected::HandleModifyItemsLevel,h,arg)&&p.writes==1&&p.average==std::strtof(arg,nullptr)&&p.stats==uint32(pvp)&&p.ratings==uint32(pvp),"Native item-level field/PvP refresh changed");++cases;}
 {Player p;ChatHandler h;h.target=&p;h.lower=true;Require(!Command(CaseItemLevel::Selected::HandleModifyItemsLevel,h,"50")&&!p.writes,"Item-level bypassed higher security");++cases;}
 {ChatHandler h;Require(!Command(CaseItemLevel::Selected::HandleModifyItemsLevel,h,"50"),"Item-level accepted absent Player");++cases;}
#endif
#ifdef REPAIR_BIT
 for(char const*arg:{""," ","1","-1 1","65536 1","65552 1","16x 1","16 1x","16 0","16 33","99999999999999999999 1","16 4294967328"}){Player p;ChatHandler h;h.target=&p;Require(!Command(CaseBit::Selected::HandleModifyBitCommand,h,arg)&&!p.writes,"Malformed/narrowed field or bit mutated unit");++cases;}
 for(uint32 bit:{1u,31u,32u}){Player p;ChatHandler h;h.target=&p;auto field=uint16(OBJECT_END+4);std::string arg=std::to_string(field)+" "+std::to_string(bit);Require(Command(CaseBit::Selected::HandleModifyBitCommand,h,arg.c_str())&&p.flags[field]==(uint32(1)<<(bit-1)),"Native flag toggle set wrong bit");Require(Command(CaseBit::Selected::HandleModifyBitCommand,h,arg.c_str())&&!p.flags[field]&&p.writes==2,"Native flag toggle failed removal");++cases;}
 {Player p;ChatHandler h;h.target=&p;h.lower=true;std::string arg=std::to_string(OBJECT_END+4)+" 1";Require(!Command(CaseBit::Selected::HandleModifyBitCommand,h,arg.c_str())&&!p.writes,"Field toggle bypassed Player security");++cases;}
 {Creature c;ChatHandler h;h.unit=&c;h.lower=true;std::string arg=std::to_string(OBJECT_END+4)+" 1";Require(Command(CaseBit::Selected::HandleModifyBitCommand,h,arg.c_str())&&c.writes==1,"Creature field path changed");++cases;}
#endif
#ifdef REPAIR_POWER
 for(char const*arg:{""," ","1","-1 1","19 1","1x 1","1 1x","1 2147483648","1 -2147483649","4294967296 1"}){Player p;ChatHandler h;h.target=&p;Require(!Command(CasePower::Selected::HandleModifyPowerCommand,h,arg)&&!p.writes,"Invalid native power/width reached setter");++cases;}
 for(uint32 type:{0u,1u,3u,6u,18u})for(int32 value:{0,1,-1,std::numeric_limits<int32>::min(),std::numeric_limits<int32>::max()}){Player p;ChatHandler h;h.target=&p;std::string arg=std::to_string(type)+" "+std::to_string(value);Require(Command(CasePower::Selected::HandleModifyPowerCommand,h,arg.c_str())&&p.writes==1&&p.powerType==type&&p.power==value,"Native signed power argument changed");++cases;}
 {Player p;ChatHandler h;h.target=&p;h.lower=true;Require(!Command(CasePower::Selected::HandleModifyPowerCommand,h,"1 1")&&!p.writes,"Generic power bypassed higher security");++cases;}
#endif
#ifdef REPAIR_MONEY
 for(char const*arg:{""," ","abc","500x","9223372036854775808","-9223372036854775809"}){Player p;ChatHandler h;h.target=&p;Require(!Command(CaseMoney::Selected::HandleModifyMoneyCommand,h,arg)&&!p.writes&&p.money==100,"Malformed64-bit money affected balance");++cases;}
 for(auto const&row:std::vector<std::tuple<char const*,uint64,uint64>>{{"0",100,100},{"5000000000",100,5000000100},{"-5000000000",6000000000,1000000000},{"-5000000000",100,0},{"9223372036854775807",100,MAX_MONEY_AMOUNT},{"-9223372036854775808",MAX_MONEY_AMOUNT,0},{"99999999999",0,MAX_MONEY_AMOUNT},{"-50",100,50}}){Player p;p.money=std::get<1>(row);ChatHandler h;h.target=&p;h.report=true;Require(Command(CaseMoney::Selected::HandleModifyMoneyCommand,h,std::get<0>(row))&&p.money==std::get<2>(row),"Native64-bit money/clamp/subtraction changed");++cases;}
 {Player p;p.money=MAX_MONEY_AMOUNT-50;ChatHandler h;h.target=&p;Require(!Command(CaseMoney::Selected::HandleModifyMoneyCommand,h,"50")&&p.money==MAX_MONEY_AMOUNT-50&&!p.writes&&p.nativeMoneyErrors==1&&!h.messages,"Rejected native gold-cap change was reported successful");++cases;}
 for(bool selected:{false,true}){Player p;ChatHandler h;h.target=selected?&p:nullptr;h.lower=true;Require(!Command(CaseMoney::Selected::HandleModifyMoneyCommand,h,"5000000000")&&!p.writes,"Money security/selection path changed");++cases;}
#endif

#ifdef REPAIR_COREMONEY
 for(auto const&row:std::vector<std::tuple<uint64,int64,uint64,bool>>{{100,0,100,true},{100,-1,99,true},{100,-100,0,true},{100,INT64_MIN,0,true},{MAX_MONEY_AMOUNT,INT64_MIN,0,true},{0,INT64_MAX,0,false},{100,int64(MAX_MONEY_AMOUNT)+1,100,false},{0,int64(MAX_MONEY_AMOUNT),0,false},{100,50,150,true},{MAX_MONEY_AMOUNT-50,50,MAX_MONEY_AMOUNT-50,false},{MAX_MONEY_AMOUNT-51,50,MAX_MONEY_AMOUNT-1,true}}){Player p;p.money=std::get<0>(row);Require(p.ModifyMoney(std::get<1>(row))==std::get<3>(row)&&p.money==std::get<2>(row),"Core money boundary changed");++cases;}
 for(int64 changed:{int64(0),INT64_MIN,INT64_MAX}){Player p;p.money=MAX_MONEY_AMOUNT;moneyScripts.hook=[changed](int64&value){value=changed;};bool ok=p.ModifyMoney(1,false);Require(ok==(changed<=0)&&p.money==(changed==INT64_MIN?0:MAX_MONEY_AMOUNT)&&!p.nativeMoneyErrors,"Post-hook native money amount not validated");if(!changed)Require(!p.writes,"Zero hook wrote money");++cases;}moneyScripts.hook={};
#endif
#ifdef REPAIR_DRUNK
 for(char const*arg:{""," ","-1","1x","4294967296"}){Player p;ChatHandler h;h.target=&p;Require(!Command(CaseDrunk::Selected::HandleModifyDrunkCommand,h,arg)&&!p.writes,"Invalid drunk input mutated player");++cases;}
 for(uint32 value:{0u,1u,100u,101u,256u,UINT32_MAX}){Player p;ChatHandler h;h.target=&p;Require(Command(CaseDrunk::Selected::HandleModifyDrunkCommand,h,std::to_string(value).c_str())&&p.drunk==std::min(value,100u)&&p.writes==1,"Native drunk clamp narrowed too early");++cases;}
 for(bool selected:{false,true}){Player p;ChatHandler h;h.target=selected?&p:nullptr;h.lower=true;Require(!Command(CaseDrunk::Selected::HandleModifyDrunkCommand,h,"50")&&!p.writes,"Drunk selection/security bypass");++cases;}
#endif
#ifdef REPAIR_HP
 for(char const*arg:{""," ","0","-1","500x","2147483648","4294967297"}){Player p;ChatHandler h;h.target=&p;Require(!Command(CaseHP::Selected::HandleModifyHPCommand,h,arg)&&!p.writes,"Invalid health changed player");++cases;}
 for(int32 value:{1,500,INT32_MAX}){Player p;ChatHandler h;h.target=&p;h.report=true;Require(Command(CaseHP::Selected::HandleModifyHPCommand,h,std::to_string(value).c_str())&&p.writes==2&&p.health==uint64(value)&&p.maxHealth==uint64(value),"Valid health native endpoints changed");++cases;}
 for(bool selected:{false,true}){Player p;ChatHandler h;h.target=selected?&p:nullptr;h.lower=true;Require(!Command(CaseHP::Selected::HandleModifyHPCommand,h,"50")&&!p.writes,"Health selection/security bypass");++cases;}
#endif
#ifdef REPAIR_MANA
 for(char const*arg:{""," ","0","-1","500x","2147483648","4294967297"}){Player p;ChatHandler h;h.target=&p;Require(!Command(CaseMana::Selected::HandleModifyManaCommand,h,arg)&&!p.writes,"Invalid mana changed player");++cases;}
 for(int32 value:{1,500,INT32_MAX}){Player p;ChatHandler h;h.target=&p;h.report=true;Require(Command(CaseMana::Selected::HandleModifyManaCommand,h,std::to_string(value).c_str())&&p.writes==2&&p.power==value&&p.maxPower==value&&p.powerType==POWER_MANA,"Valid mana raw units changed");++cases;}
 for(bool selected:{false,true}){Player p;ChatHandler h;h.target=selected?&p:nullptr;h.lower=true;Require(!Command(CaseMana::Selected::HandleModifyManaCommand,h,"50")&&!p.writes,"Mana selection/security bypass");++cases;}
#endif
#ifdef REPAIR_FACTION
 for(char const*arg:{"","2x","-1","4294967298","2 1x","2 1 -1","2 1 2 4294967296","3 1 2 3"}){Creature c;ChatHandler h;h.creature=&c;Require(!Command(CaseFaction::Selected::HandleModifyFactionCommand,h,arg)&&!c.writes&&c.faction==1,"Faction partially mutated on invalid operand");++cases;}
 for(char const*arg:{"2","2 0","2 4294967295 3 4"}){Creature c;c.flags[UNIT_FIELD_FLAGS]=5;c.flags[UNIT_FIELD_NPC_FLAGS]=6;c.flags[OBJECT_FIELD_DYNAMIC_FLAGS]=7;ChatHandler h;h.creature=&c;Require(Command(CaseFaction::Selected::HandleModifyFactionCommand,h,arg)&&c.writes==4&&c.faction==2,"Faction native writes changed");if(std::string(arg)=="2")Require(c.flags[UNIT_FIELD_FLAGS]==5&&c.flags[UNIT_FIELD_NPC_FLAGS]==6&&c.flags[OBJECT_FIELD_DYNAMIC_FLAGS]==7,"Faction defaults lost");++cases;}
 {Creature c;ChatHandler h;h.creature=&c;Require(Command(CaseFaction::Selected::HandleModifyFactionCommand,h," ")&&!c.writes&&h.messages==1,"Faction read-only inspection lost");++cases;}
#endif
#ifdef REPAIR_CURRENCY
 for(char const*arg:{""," ","1","1x 1","4294967297 1","-1 1","3 1","1 1x","1 0","1 2147483648","2 21474837","2 -21474837"}){Player p;ChatHandler h;h.target=&p;Require(!Command(CaseCurrency::Selected::HandleModifyCurrencyCommand,h,arg)&&!p.writes,"Invalid/overflow currency mutated balance");++cases;}
 for(uint32 id:{1u,2u})for(int32 value:{-21474836,-1,1,21474836}){Player p;ChatHandler h;h.target=&p;std::string arg=std::to_string(id)+" "+std::to_string(value);Require(Command(CaseCurrency::Selected::HandleModifyCurrencyCommand,h,arg.c_str())&&p.writes==1&&p.power==value*(id==2?100:1)&&p.powerType==id,"Currency precision/native sign changed");++cases;}
 for(bool selected:{false,true}){Player p;ChatHandler h;h.target=selected?&p:nullptr;h.lower=true;Require(!Command(CaseCurrency::Selected::HandleModifyCurrencyCommand,h,"1 1")&&!p.writes,"Currency selection/security bypass");++cases;}
#endif
#ifdef REPAIR_ARTIFACTXP
 for(char const*arg:{""," ","0","-1","1x","18446744073709551616","18446744073709551615"}){Player p;Item item;p.artifact=&item;ChatHandler h;h.target=&p;Require(!Command(CaseArtifactXP::Selected::HandleModifyArtifactXPCommand,h,arg)&&!item.writes&&!item.states&&item.xp==100,"Invalid/overflow artifact XP mutated item");++cases;}
 for(uint64 value:{uint64(1),uint64(5000000000),UINT64_MAX-100}){Player p;Item item;p.artifact=&item;ChatHandler h;h.target=&p;Require(Command(CaseArtifactXP::Selected::HandleModifyArtifactXPCommand,h,std::to_string(value).c_str())&&item.writes==1&&item.states==1&&item.owner==&p&&item.xp==100+value,"Native raw artifact XP/state changed");++cases;}
 for(uint32 mode:{0u,1u,2u}){Player p;Item item;p.artifact=mode==1?nullptr:&item;ChatHandler h;h.target=mode?&p:nullptr;h.lower=mode==2;Require(!Command(CaseArtifactXP::Selected::HandleModifyArtifactXPCommand,h,"1")&&!item.writes,"Artifact absent target/item/security bypass");++cases;}
#endif
#ifdef REPAIR_MORPH
 for(char const*arg:{""," ","0","-1","123x","4294967419","124"}){Player p;ChatHandler h;h.target=&p;Require(!Command(CaseMorph::Selected::HandleModifyMorphCommand,h,arg)&&!p.writes,"Invalid native display mutated unit");++cases;}
 for(uint32 mode:{0u,1u,2u,3u}){Player p;Creature c;Session session{&p,1};ChatHandler h;h.session=&session;if(mode==1||mode==3)h.target=&p;if(mode==2)h.unit=&c;h.lower=mode==3;bool ok=Command(CaseMorph::Selected::HandleModifyMorphCommand,h,"123");Require(ok==(mode!=3),"Morph target admission changed");Unit&u=mode==2?static_cast<Unit&>(c):static_cast<Unit&>(p);Require(u.writes==(ok?2u:0u)&&(!ok||u.display==123),"Morph setter/mirror cleanup changed");++cases;}
#endif
#ifdef REPAIR_PHASE
 for(char const*arg:{""," ","-1","1x","4294967296"}){Player p;ChatHandler h;h.target=&p;Require(!Command(CasePhase::Selected::HandleModifyPhaseCommand,h,arg)&&!p.phaseMgr.writes,"Invalid phase reset visibility");++cases;}
 for(uint32 value:{0u,1u,UINT32_MAX})for(uint32 mode:{0u,1u,2u,3u}){Player p;Creature c;Session session{&p,1};ChatHandler h;h.session=&session;if(mode==1||mode==3)h.target=&p;if(mode==2)h.unit=&c;h.lower=mode==3;bool ok=Command(CasePhase::Selected::HandleModifyPhaseCommand,h,std::to_string(value).c_str());Require(ok==(mode!=3),"Phase target security changed");if(mode==2)Require(c.writes==1&&c.phase==value,"Creature phase mask changed");else Require(p.phaseMgr.writes==(ok?1u:0u)&&(!ok||p.phaseMgr.mask==value),"Native player custom phase changed");++cases;}
#endif
#ifdef REPAIR_PHASEIDS
 for(char const*arg:{""," ","-1","1x","4294967297","1 3","1 -1","1 2x"}){Player p;p.phaseIds={2};ChatHandler h;h.target=&p;Require(!Command(CasePhaseIDs::Selected::HandleModifyPhaseIDsCommand,h,arg)&&!p.writes&&p.phaseIds==std::set<uint32>{2},"Invalid phase list partially applied/cleared");++cases;}
 for(char const*arg:{"0","1","1 2 1"," 0  1 2 "})for(uint32 mode:{0u,1u,2u,3u}){Player p;Creature c;Session session{&p,1};ChatHandler h;h.session=&session;if(mode==1||mode==3)h.target=&p;if(mode==2)h.unit=&c;h.lower=mode==3;bool ok=Command(CasePhaseIDs::Selected::HandleModifyPhaseIDsCommand,h,arg);Require(ok==(mode<2)&&!c.writes&&p.writes==(ok?1u:0u),"Phase IDs target admission changed");if(ok)Require(p.phaseIds==(std::string(arg)=="0"?std::set<uint32>{}:std::string(arg)=="1"?std::set<uint32>{1}:std::set<uint32>{1,2}),"Native phase set/zero reset changed");++cases;}
#endif

#ifdef REPAIR_NPCLEVEL
 NpcRejects(CaseNpcLevel::Selected::HandleNpcSetLevelCommand,{""," ","0","-1","1x","256","257","124","4294967297"});
 for(uint32 level:{1u,120u,123u})for(uint32 mode:{0u,1u,2u}){Creature c;Pet pet;pet.type=mode==1?HUNTER_PET:SUMMON_PET;Creature&target=mode?static_cast<Creature&>(pet):c;ChatHandler h;h.creature=&target;Require(Command(CaseNpcLevel::Selected::HandleNpcSetLevelCommand,h,std::to_string(level).c_str())&&target.level==level&&target.saves==uint32(mode==0),"NPC/Pet native level/save route changed");if(mode==1)Require(pet.flags[UNIT_FIELD_PET_NEXT_LEVEL_EXPERIENCE]==level*25&&pet.flags[UNIT_FIELD_PET_EXPERIENCE]==0,"Hunter pet native experience route lost");++cases;}
#endif
#ifdef REPAIR_NPCFACTION
 NpcRejects(CaseNpcFaction::Selected::HandleNpcSetFactionIdCommand,{""," ","-1","1x","65537","4294967297","3"});
 {Creature c;c.entry=99;ChatHandler h;h.creature=&c;WorldDatabase.writes.clear();Require(Command(CaseNpcFaction::Selected::HandleNpcSetFactionIdCommand,h,"2")&&c.faction==2&&c.templateData.faction==2&&WorldDatabase.writes.size()==1&&WorldDatabase.writes[0].id==WORLD_UPD_CREATURE_FACTION&&WorldDatabase.writes[0].values.at(0)==2&&WorldDatabase.writes[0].values.at(1)==99,"Faction live/template/prepared DB operand divergence");++cases;}
#endif
#ifdef REPAIR_NPCFLAGS
 NpcRejects(CaseNpcFlags::Selected::HandleNpcSetFlagCommand,{""," ","-1","1x","4294967296"});
 for(uint32 value:{0u,1u,UINT32_MAX}){Creature c;c.entry=99;ChatHandler h;h.creature=&c;WorldDatabase.writes.clear();Require(Command(CaseNpcFlags::Selected::HandleNpcSetFlagCommand,h,std::to_string(value).c_str())&&c.flags[UNIT_FIELD_NPC_FLAGS]==value&&WorldDatabase.writes.size()==1&&WorldDatabase.writes[0].id==WORLD_UPD_CREATURE_NPCFLAG&&WorldDatabase.writes[0].values.at(0)==value&&WorldDatabase.writes[0].values.at(1)==99,"NPC flags native field/persistence changed");++cases;}
#endif
#ifdef REPAIR_NPCDATA
 NpcRejects(CaseNpcData::Selected::HandleNpcSetDataCommand,{""," ","1","0 1","1 0","-1 1","1 -1","1x 1","1 1x","4294967297 1","1 4294967297"});
 {Creature c;ChatHandler h;h.creature=&c;c.hasAI=false;Require(!Command(CaseNpcData::Selected::HandleNpcSetDataCommand,h,"1 1")&&!c.ai.writes,"NPC data dereferenced absent AI");++cases;}
 for(uint32 value:{1u,UINT32_MAX}){Creature c;ChatHandler h;h.creature=&c;std::string arg="1 "+std::to_string(value);Require(Command(CaseNpcData::Selected::HandleNpcSetDataCommand,h,arg.c_str())&&c.ai.writes==1&&c.ai.key==1&&c.ai.value==value,"Native AI SetData operands changed");++cases;}
#endif
#ifdef REPAIR_NPCEMOTE
 NpcRejects(CaseNpcEmote::Selected::HandleNpcPlayEmoteCommand,{""," ","-1","1x","4294967297","3"});
 for(uint32 value:{0u,1u,2u}){Creature c;ChatHandler h;h.creature=&c;Require(Command(CaseNpcEmote::Selected::HandleNpcPlayEmoteCommand,h,std::to_string(value).c_str())&&c.writes==1&&c.flags[UNIT_FIELD_EMOTE_STATE]==value,"Native emote state/reset changed");++cases;}
#endif
#ifdef REPAIR_NPCMODEL
 NpcRejects(CaseNpcModel::Selected::HandleNpcSetModelCommand,{""," ","0","-1","123x","4294967419","124"});
 {Creature c;ChatHandler h;h.creature=&c;Require(Command(CaseNpcModel::Selected::HandleNpcSetModelCommand,h,"123")&&c.writes==4&&c.saves==1&&c.display==123&&c.nativeDisplay==123&&c.outfit==123,"Native display/outfit/persistence changed");++cases;}
 {Pet pet;ChatHandler h;h.creature=&pet;Require(!Command(CaseNpcModel::Selected::HandleNpcSetModelCommand,h,"123")&&!pet.writes&&!pet.saves,"Pet model refusal lost");++cases;}
#endif
#ifdef REPAIR_NPCPHASE
 NpcRejects(CaseNpcPhase::Selected::HandleNpcSetPhaseCommand,{""," ","0","-1","1x","4294967297"});
 for(uint32 value:{1u,UINT32_MAX})for(bool pet:{false,true}){Creature c;c.pet=pet;ChatHandler h;h.creature=&c;Require(Command(CaseNpcPhase::Selected::HandleNpcSetPhaseCommand,h,std::to_string(value).c_str())&&c.writes==1&&c.phase==value&&c.saves==uint32(!pet),"NPC/pet native phase/save policy changed");++cases;}
#endif
#ifdef REPAIR_NPCSPAWNDIST
 NpcRejects(CaseNpcSpawnDist::Selected::HandleNpcSetSpawnDistCommand,{""," ","-1","1x","nan","inf","1e1000"});
 for(char const*arg:{"0","1.5","0x1p1"})for(bool alive:{false,true}){Creature c;c.dbGuid=uint64(1)<<40;c.alive=alive;ChatHandler h;h.creature=&c;WorldDatabase.writes.clear();Require(Command(CaseNpcSpawnDist::Selected::HandleNpcSetSpawnDistCommand,h,arg)&&c.radius==std::strtof(arg,nullptr)&&c.motion.initializes==1&&c.deathCalls==uint32(alive)&&c.respawns==uint32(alive)&&WorldDatabase.writes.size()==1&&WorldDatabase.writes[0].floats.at(0)==c.radius&&WorldDatabase.writes[0].values.at(2)==c.dbGuid&&WorldDatabase.writes[0].id==WORLD_UPD_CREATURE_SPAWN_DISTANCE,"Native spawn radius/live-dead reset/DB route changed");Require(c.moveType==uint32(c.radius>0?RANDOM_MOTION_TYPE:IDLE_MOTION_TYPE),"Radius zero movement policy changed");++cases;}
#endif
#ifdef REPAIR_NPCSPAWNTIME
 NpcRejects(CaseNpcSpawnTime::Selected::HandleNpcSetSpawnTimeCommand,{""," ","-1","1x","2147483648","4294967297"});
 for(int32 value:{0,1,INT32_MAX}){Creature c;c.dbGuid=uint64(1)<<40;ChatHandler h;h.creature=&c;WorldDatabase.writes.clear();Require(Command(CaseNpcSpawnTime::Selected::HandleNpcSetSpawnTimeCommand,h,std::to_string(value).c_str())&&c.writes==1&&c.delay==uint32(value)&&WorldDatabase.writes.size()==1&&WorldDatabase.writes[0].id==WORLD_UPD_CREATURE_SPAWN_TIME_SECS&&WorldDatabase.writes[0].values.at(0)==uint32(value)&&WorldDatabase.writes[0].values.at(1)==c.dbGuid,"Native respawn delay/DB seconds changed");++cases;}
#endif
#ifdef REPAIR_NPCSIZE
 NpcRejects(CaseNpcSize::Selected::HandleNpcSetSizeCommand,{""," ","-1","1x","nan","inf","1e1000"});
 for(char const*arg:{"0","1.5","0x1p1"})for(bool pet:{false,true}){Creature c;c.pet=pet;ChatHandler h;h.creature=&c;objectManager.spawn.personalSize=3;Require(Command(CaseNpcSize::Selected::HandleNpcSetSizeCommand,h,arg)&&c.writes==1&&c.scale==(std::strtof(arg,nullptr)==0?objectManager.data.scale:std::strtof(arg,nullptr))&&c.saves==uint32(!pet),"Native size/reset/pet save policy changed");Require(objectManager.spawn.personalSize==(pet?3:std::strtof(arg,nullptr)),"NPC size persisted override wrong");++cases;}
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
