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
#include <memory>
#include <set>
#include <functional>
#include <ctime>
#include <inttypes.h>
#define UI64FMTD "%" PRIu64
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>
#include "../../src/server/game/Chat/CommandIntegerArgument.h"
#include "../../src/server/game/Chat/CommandFloatArgument.h"
using int16=int16_t;using uint8=uint8_t;using uint16=uint16_t;using uint32=uint32_t;using int32=int32_t;using uint64=uint64_t;using int64=int64_t;
static void Require(bool b,char const* message){if(!b)throw std::runtime_error(message);}
#include "Constants.inc"
#include "NativeCoordinates.inc"
#include "NativeFall.inc"
enum class HighGuid{Item,Player,Creature};
struct ObjectGuid{using LowType=uint64;uint64 low=1;HighGuid kind=HighGuid::Creature;uint64 GetGUIDLow(){return low;}uint64 GetCounter()const{return low;}static LowType GetMaxCounter(HighGuid);template<HighGuid Kind>static ObjectGuid Create(LowType counter){return {counter&GetMaxCounter(Kind),Kind};}std::string ToString(){return "controlled:1";}static ObjectGuid Empty;};ObjectGuid ObjectGuid::Empty;
#include "NativeGuidLimit.inc"

#define TC_LOG_DEBUG(...) ((void)0)
#include "NativeTokenizer.inc"
struct Position{float x=1,y=2,z=3,o=.5f;};
struct FactionName{char const*Str[1]={"Controlled faction"};};
struct FactionEntry{uint32 ID=1;int32 ReputationIndex=0;uint32 ParagonFactionID=0;FactionName*Name=nullptr;};
struct ReputationManager{uint32 writes=0;int32 delta=0;bool paragon=false;void ModifyReputation(FactionEntry const*,int32 value){++writes;delta=value;}void ModifyParagonReputation(FactionEntry const*,int32 value){++writes;delta=value;paragon=true;}int32 GetReputation(FactionEntry const*){return delta;}};
struct Session;struct Player;struct Creature;struct GameObject;
struct ItemTemplate{uint32 entry=7;};
struct Item{uint64 xp=100;uint32 writes=0,states=0,reads=0;Player*owner=nullptr;std::map<uint16,uint32>fields;ItemTemplate proto;uint8 bag=0,slot=1;bool protectedRemove=false,removed=false;uint64 GetUInt64Value(uint32 field){Require(field==ITEM_FIELD_ARTIFACT_XP,"Wrong artifact field read");return xp;}void SetUInt64Value(uint32 field,uint64 value){Require(field==ITEM_FIELD_ARTIFACT_XP,"Wrong artifact field write");++writes;xp=value;}void SetState(uint32 state,Player*player){Require(state==ITEM_CHANGED,"Wrong item state");++states;owner=player;}uint16 GetValuesCount(){return ITEM_END;}uint32 GetUInt32Value(uint16 field){Require(field<ITEM_END,"Bad item read index");++reads;return fields[field];}void SetUInt32Value(uint16 field,uint32 value){Require(field<ITEM_END,"Bad item write index");++writes;fields[field]=value;}uint8 GetBagSlot(){return bag;}uint8 GetSlot(){return slot;}ItemTemplate const*GetTemplate(){return &proto;}};
struct PhaseManager{uint32 writes=0,mask=0;void SetCustomPhase(uint32 value){++writes;mask=value;}};
struct WorldPacket{std::vector<int16>ids;};
namespace WorldPackets{namespace Misc{struct StreamingMovie{std::vector<int16>MovieIDs;WorldPacket packet;WorldPacket const*Write(){packet.ids=MovieIDs;return &packet;}};}}
struct MotionMaster{uint32 initializes=0,jumps=0;std::vector<float>jumpArgs;void Initialize(){++initializes;}void MoveJump(float x,float y,float z,float xy,float vz){++jumps;jumpArgs={x,y,z,xy,vz};}};
struct Unit {
 uint16 countOverride=0,dynamicCount=3;uint32 fieldReads=0,badFieldWrites=0,dynamicReads=0,dynamicWrites=0,itemQueries=0,destroyCalls=0,m_killPoints=17;uint64 lastItem=0;std::map<uint16,float>floatFields;std::map<uint16,std::vector<uint32>>dynamicFields;std::map<uint64,Item*>ownedItems;
 MotionMaster motion;bool unitEligible=true,selection=false;uint32 lastError=0,errorChannel=0,lastMovie=0,lastCinematic=0,lastEmote=0,lastSound=0,partyCode=0,packetCalls=0;bool distanceSound=false;std::vector<int16>streamIDs;
 Position position;ReputationManager reputation;bool honorAccepted=true;int32 honorAmount=0,lastVisual=0;std::vector<float>basePoints;uint32 lastTarget=7,mount=0;bool honorToken=false;uint32 honorGroup=0;
 uint32 writes=0,powerType=0,selectedTitle=2,spells=0,stats=0,ratings=0,nativeMoneyErrors=0;int32 maxPower=0,power=0;
 uint64 titles=5,higherTitles=0x123456789abcdef,money=100;float lastDistance=0,scale=1,average=0;uint64 health=100,maxHealth=100;uint32 display=0,phase=1,drunk=0;std::set<uint32>phaseIds;PhaseManager phaseMgr;Item*artifact=nullptr;bool flying=false,pvp=false;Session*session=nullptr;
 std::map<uint32,float>speeds;std::map<uint16,uint32>flags;std::vector<uint32>casts,animations;
 virtual ~Unit()=default;virtual Player*ToPlayer(){return nullptr;}virtual Creature*ToCreature(){return nullptr;}
 Unit*ToUnit(){return unitEligible?this:nullptr;}MotionMaster*GetMotionMaster(){return &motion;}bool GetSelection(){return selection;}
 void SendCinematicStart(uint32 id){++writes;lastCinematic=id;}void SendMovieStart(uint32 id){++writes;lastMovie=id;}void HandleEmoteCommand(uint32 id){++writes;lastEmote=id;}
 void PlayDistanceSound(uint32 id,Player*){++writes;lastSound=id;distanceSound=true;}void PlayDirectSound(uint32 id,Player*){++writes;lastSound=id;distanceSound=false;}
 void SendEquipError(InventoryResult code,Item*,Item*){++writes;lastError=uint32(code);errorChannel=1;}void SendSellError(SellResult code,uint32,ObjectGuid){++writes;lastError=uint32(code);errorChannel=2;}void SendBuyError(BuyResult code){++writes;lastError=uint32(code);errorChannel=3;}
 void SendPushToPartyResponse(Player*player,uint8 code){Require(player==ToPlayer(),"Party response actor changed");++writes;partyCode=code;}
 void SendDirectMessage(WorldPacket const*packet){++packetCalls;streamIDs=packet->ids;}
 ObjectGuid GetGUID(){return {}; }Position GetPosition(){return position;}uint32 GetMapId(){return 1;}float GetPositionX(){return position.x;}float GetPositionY(){return position.y;}float GetPositionZ(){return position.z;}float GetOrientation(){return position.o;}uint32 GetLastTargetedGO(){return lastTarget;}
 void CastCustomSpell(Unit*target,uint32 id,float*a,float*b,float*c,bool triggered){Require(target==this&&id==123&&triggered,"Custom native target/trigger changed");++spells;basePoints={*a,*b,*c};}
 void PlaySpellVisual(Position,int32 id,float,ObjectGuid,bool){++writes;lastVisual=id;}void PlayOrphanSpellVisual(Position,Position,Position,int32 id,float,ObjectGuid,bool){++writes;lastVisual=id;}
 void Mount(uint32 id){++writes;mount=id;}
 bool RewardHonor(Unit*target,uint32 group,int32 value,bool token=true){Require(!target&&group<=1,"Honor native arguments changed");honorToken=token;honorGroup=group;honorAmount=value;if(honorAccepted)++writes;return honorAccepted;}uint32 GetCurrency(uint32 type){Require(type==CURRENCY_TYPE_HONOR_POINTS,"Wrong honor currency");return writes;}
 ReputationManager&GetReputationMgr(){return reputation;}
 uint32 GetTypeId(){return ToPlayer()?TYPEID_PLAYER:3;}uint16 GetValuesCount(){return countOverride?countOverride:uint16(ToPlayer()?PLAYER_FIELD_END:UNIT_END);}
 Session* GetSession(){return session;}bool isInFlight(){return flying;}
 void SetMaxPower(uint32 t,int32 v){++writes;powerType=t;maxPower=v;}
 void SetPower(uint32 t,int32 v){++writes;powerType=t;power=v;}
 void SetUInt64Value(uint32 field,uint64 v){Require(field==PLAYER_FIELD_KNOWN_TITLES,"Wrong title field");++writes;titles=v;}
 void SetUInt32Value(uint16 field,uint32 v){if(field>=GetValuesCount()){++badFieldWrites;return;}++writes;if(field==PLAYER_FIELD_PLAYER_TITLE)selectedTitle=v;else flags[uint16(field)]=v;}
 uint32 GetUInt32Value(uint16 field){Require(field<GetValuesCount(),"Native field read bound");++fieldReads;return flags[field];}
 float GetFloatValue(uint16 field){Require(field<GetValuesCount(),"Native float read bound");++fieldReads;return floatFields[field];}
 uint16 GetDynamicValuesCount(){return dynamicCount;}uint32 GetDynamicValue(uint16 field,uint16 offset){Require(field<dynamicCount,"Native dynamic field bound");++dynamicReads;auto&values=dynamicFields[field];return offset<values.size()?values[offset]:0;}void SetDynamicValue(uint16 field,uint16 offset,uint32 value){Require(field<dynamicCount,"Native dynamic field bound");++dynamicWrites;auto&values=dynamicFields[field];if(values.size()<=offset)values.resize(offset+1);values[offset]=value;}
 Item*GetItemByGuid(ObjectGuid guid){Require(guid.kind==HighGuid::Item,"Wrong inventory GUID type");++itemQueries;lastItem=guid.GetCounter();auto it=ownedItems.find(lastItem);return it==ownedItems.end()?nullptr:it->second;}
 void DestroyItem(uint8 bag,uint8 slot,bool update){Require(update,"Native item update flag changed");++destroyCalls;for(auto it=ownedItems.begin();it!=ownedItems.end();++it){auto item=it->second;if(item->bag!=bag||item->slot!=slot)continue;if(item->protectedRemove)return;item->removed=true;item->slot=255;ownedItems.erase(it);return;}}

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
 void SetSpeed(uint32 mode,float value,bool=false){++writes;speeds[mode]=value;}
 void SetObjectScale(float value){++writes;scale=value;}
 void SetFloatValue(uint16 field,float value){Require(field<GetValuesCount()&&std::isfinite(value),"Invalid native float field write");++writes;floatFields[field]=value;if(field==PLAYER_FIELD_AVG_ITEM_LEVEL+EquippedAvgItemLevel)average=value;}
 bool HasPvpStatsScalingEnabled(){return pvp;}void SendUpdateStat(uint32 mask){Require(mask==USM_ALL,"Wrong stats mask");++stats;}void SendUpdateCR(uint32 mask){Require(mask==CR_PVP_UPDATE_MASK,"Wrong rating mask");++ratings;}
 bool HasFlag(uint16 field,uint32 mask){return (flags[field]&mask)!=0;}void RemoveFlag(uint16 field,uint32 mask){++writes;flags[field]&=~mask;}void SetFlag(uint16 field,uint32 mask){++writes;flags[field]|=mask;}
 uint64 GetMoney(){return money;}void SetMoney(uint64 value){++writes;money=value;}
 bool ModifyMoney(int64 amount,bool sendError=true);
 void SendEquipError(uint32 code){Require(code==EQUIP_ERR_TOO_MUCH_GOLD,"Wrong native money error");++nativeMoneyErrors;}

};
struct MoneyScripts{uint32 expired=0;Player*expirePlayer=nullptr;ItemTemplate const*expireTemplate=nullptr;void OnItemExpire(Player*player,ItemTemplate const*info){++expired;expirePlayer=player;expireTemplate=info;}std::function<void(int64&)>hook;template<class T>void OnPlayerMoneyChanged(T*,int64&value) {if(hook)hook(value);}} moneyScripts;auto sScriptMgr=&moneyScripts;
#include "NativeModifyMoney.inc"
using WorldObject=Unit;
struct Player:Unit{Player*ToPlayer()override{return this;}};
struct Session{Player*player=nullptr;uint32 account=1;Player*GetPlayer(){return player;}uint32 GetAccountId(){return account;}};
struct CreatureTemplate{uint32 faction=1;float scale=1.5f;std::string Name[1]={"Controlled NPC"};};
struct CreatureData{float personalSize=3;};
struct CreatureAI{uint32 writes=0,key=0,value=0;void SetData(uint32 a,uint32 b){++writes;key=a;value=b;}};

struct Creature:Unit{uint32 entry=0,path=77,faction=1,level=1,saves=0,nativeDisplay=0,outfit=0,moveType=0,deathCalls=0,respawns=0,delay=0;bool pet=false,alive=true,hasAI=true;float radius=0;CreatureTemplate templateData;CreatureAI ai;MotionMaster motion;
 Creature*ToCreature()override{return this;}uint32 GetEntry(){return entry;}uint32 GetWaypointPath(){return path;}uint32 getFaction(){return faction;}void setFaction(uint32 value){++writes;faction=value;}ObjectGuid GetGUID(){return {};}
 uint32 groupCalls=0,lastGroup=0,entryCalls=0;bool groupDelete=false,entryAccepted=true;void SummonCreatureGroup(uint32 id){++groupCalls;lastGroup=id;groupDelete=false;}void SummonCreatureGroupDespawn(uint32 id){++groupCalls;lastGroup=id;groupDelete=true;}bool UpdateEntry(uint32 id){++entryCalls;if(entryAccepted){++writes;entry=id;}return entryAccepted;}
 bool isPet(){return pet;}void SetLevel(uint8 value){++writes;level=value;}void SaveToDB(){++saves;}CreatureTemplate const*GetCreatureTemplate(){return &templateData;}
 CreatureAI*AI(){return hasAI?&ai:nullptr;}std::string GetNPCAIName(){return "ControlledAI";}std::string GetScriptName(){return "";}char const*GetName(){return "Controlled";}
 void SetNativeDisplayId(uint32 value){++writes;nativeDisplay=value;}void SetOutfit(int32 value){++writes;outfit=uint32(value);}
 uint64 dbGuid=7;uint64 GetDBTableGUIDLow(){return dbGuid;}void SetRespawnRadius(float value){++writes;radius=value;}void SetDefaultMovementType(MovementGeneratorType value){++writes;moveType=value;}
 MotionMaster*GetMotionMaster(){return &motion;}bool IsAlive(){return alive;}void setDeathState(DeathState state){Require(state==JUST_DIED,"Unexpected death mode");++deathCalls;alive=false;}void Respawn(){++respawns;alive=true;}
 void SetRespawnDelay(uint32 value){++writes;delay=value;}
};
struct Pet:Creature{Pet(){pet=true;}PetType type=HUNTER_PET;PetType getPetType(){return type;}void GivePetLevel(uint8 value){++writes;level=value;}};
struct WorldConfig{uint32 getIntConfig(WorldIntConfigs key){Require(key==CONFIG_MAX_PLAYER_LEVEL,"Wrong level config");return 120;}}worldConfig;auto sWorld=&worldConfig;
struct GameObjectTemplate{std::string name="Controlled GO";float size=1.5f;};
struct GameObjectData{uint32 id=99;float personalSize=3;};
struct GameObject:Unit{uint32 despawns=0,customCalls=0,customAnim=0,byteOffset=0,byteValue=0;int32 healthDelta=0;Unit*healthActor=nullptr;void SetByteValue(uint32 field,uint8 offset,uint8 value){Require(field==GAMEOBJECT_FIELD_BYTES_1&&offset<4,"Wrong GO byte field");++writes;byteOffset=offset;byteValue=value;}void SendCustomAnim(uint32 id){++customCalls;customAnim=id;}void SendObjectDeSpawnAnim(ObjectGuid){++despawns;}void ModifyHealth(int32 delta,Unit*actor){++writes;healthDelta=delta;healthActor=actor;}GameObjectTemplate info;uint32 saves=0,refreshes=0,destroys=0,visibility=0;float rz=0,ry=0,rx=0;ObjectGuid GetGUID(){return {7};}uint32 GetEntry(){return 99;}GameObjectTemplate const*GetGOInfo(){return &info;}void Relocate(float x,float y,float z){++writes;position.x=x;position.y=y;position.z=z;}void Relocate(float x,float y,float z,float o){Relocate(x,y,z);position.o=o;}void RelocateStationaryPosition(float,float,float,float){++writes;}void SetWorldRotationAngles(float z,float y,float x){++writes;rz=z;ry=y;rx=x;}void DestroyForNearbyPlayers(){++destroys;}void UpdateObjectVisibility(){++visibility;}void SaveToDB(){++saves;}void Refresh(){++refreshes;}};
struct MapManager{static bool IsValidMapCoord(uint32 id,float x,float y,float z){return id==1&&Trinity::IsValidMapCoord(x,y,z);}};
struct ObjectManager{CreatureTemplate data;CreatureData spawn;GameObjectTemplate goInfo;GameObjectData goData;uint32 GetXPForLevel(uint32 level){return level*100;}CreatureTemplate const*GetCreatureTemplate(uint32){return &data;}CreatureData&NewOrExistCreatureData(uint64 id){Require(id==7,"Wrong spawn");return spawn;}GameObjectData const*GetGOData(uint32 id){return id==7?&goData:nullptr;}GameObjectTemplate const*GetGameObjectTemplate(uint32 id){return id==99?&goInfo:nullptr;}GameObjectData&NewGOData(uint64 id){Require(id==7,"Wrong GO spawn");return goData;}}objectManager;auto sObjectMgr=&objectManager;
struct WorldDatabasePreparedStatement{uint32 id=0;std::map<uint32,uint64>values;std::map<uint32,float>floats;void setUInt16(uint32 i,uint16 v){values[i]=v;}void setUInt32(uint32 i,uint32 v){values[i]=v;}void setUInt64(uint32 i,uint64 v){values[i]=v;}void setUInt8(uint32 i,uint8 v){values[i]=v;}void setFloat(uint32 i,float v){Require(std::isfinite(v),"Nonfinite DB write");floats[i]=v;}};
struct Field{uint64 integer=0;float number=0;uint64 GetUInt64(){return integer;}uint32 GetUInt32(){return uint32(integer);}uint16 GetUInt16(){return uint16(integer);}float GetFloat(){return number;}};
struct QueryRow{Field fields[6]{{(uint64(1)<<32)+7,0},{99,0},{0,1},{0,2},{0,3},{1,0}};Field*Fetch(){return fields;}bool NextRow(){return false;}};
using PreparedQueryResult=std::shared_ptr<QueryRow>;
struct WorldService{WorldDatabasePreparedStatement pending,lastQuery;uint32 queries=0;bool hasRow=false;std::vector<WorldDatabasePreparedStatement>writes;WorldDatabasePreparedStatement*GetPreparedStatement(uint32 id){pending={id,{},{}};return &pending;}void Execute(WorldDatabasePreparedStatement*row){writes.push_back(*row);}PreparedQueryResult Query(WorldDatabasePreparedStatement*row){++queries;lastQuery=*row;return hasRow?std::make_shared<QueryRow>():nullptr;}}WorldDatabase;
struct ChatHandler {
 GameObject*go=nullptr;GameObject*GetObjectGlobalyWithGuidOrNearWithDbGuid(uint32 id,uint32 entry){Require(id==7&&entry==99,"Wrong GO lookup");return go;}
 Player*target=nullptr;Unit*unit=nullptr;Creature*creature=nullptr;Session*session=nullptr;bool lower=false,error=false,report=false;uint32 messages=0,databaseReached=0;
 ChatHandler()=default;explicit ChatHandler(Player*){}
 WorldObject*getSelectedObject(){return unit?unit:target;}
 Session*GetSession(){return session;}Player*getSelectedPlayer(){return target;}Unit*getSelectedUnit(){return unit?unit:target;}Creature*getSelectedCreature(){return creature;}
 bool HasLowerSecurityAccount(Player*,uint32,bool strong){Require(strong,"Account addon strong authority gate lost");return lower;}
 template<class...T>void SendGlobalGMSysMessage(T...){++messages;}char const*GetTrinityString(uint32){return "active";}
 bool HasLowerSecurity(Player*,ObjectGuid){return lower;}bool needReportToTarget(Player*){return report;}
 std::string GetNameLink(Player* =nullptr){return "fixture";}
 template<class...T>void PSendSysMessage(T...){++messages;}template<class...T>void SendSysMessage(T...){++messages;}
 void SetSentErrorMessage(bool b){error=b;}
 char*extractKeyFromLink(char*args,char const*){return strtok(args," ");}
 uint32 extractSpellIdFromLink(char*args){char*id=strtok(args," ");return id?uint32(strtoul(id,nullptr,10)):0;}
};
struct NativeRow{uint32 ID;};using CurrencyTypesEntry=NativeRow;using PhaseEntry=NativeRow;
struct NativeStore{std::map<uint32,NativeRow>rows;NativeRow const*LookupEntry(uint32 id){auto it=rows.find(id);return it==rows.end()?nullptr:&it->second;}}sFactionTemplateStore{{{1,{1}},{2,{2}}}},sCurrencyTypesStore{{{1,{1}},{2,{2}}}},sCreatureDisplayInfoStore{{{123,{123}}}},sPhaseStore{{{1,{1}},{2,{2}}}},sEmotesStore{{{1,{1}},{2,{2}}}},sMovieStore{{{1,{1}},{2,{2}}}};
FactionName factionName;struct FactionStore{std::map<uint32,FactionEntry>rows{{1,{1,0,0,&factionName}},{2,{2,0,2,&factionName}},{3,{3,-1,0,&factionName}}};FactionEntry const*LookupEntry(uint32 id){auto it=rows.find(id);return it==rows.end()?nullptr:&it->second;}}sFactionStore;
struct PrecisionManager{bool GetFactionParagon(uint32 id){return id==2;}float GetCurrencyPrecision(uint32 id){return id==2?100.f:1.f;}}sDB2Manager;
struct Battlefield{uint32 writes=0,starts=0,ends=0,toggles=0,timer=0,packets=0;bool enabled=true,winner=false;void StartBattle(){++writes;++starts;}void EndBattle(bool force){++writes;++ends;winner=force;}bool IsEnabled(){return enabled;}void ToggleBattlefield(bool value){++writes;++toggles;enabled=value;}void SetTimer(uint32 value){++writes;timer=value;}void SendInitWorldStatesToAll(){++packets;}};
struct BattlefieldManager{Battlefield first,second;uint32 lookups=0;Battlefield*GetBattlefieldByBattleId(uint32 id){++lookups;return id==1?&first:id==2?&second:nullptr;}}battlefieldManager;auto sBattlefieldMgr=&battlefieldManager;
struct GameEventData{bool valid=false;std::string description="Controlled event";time_t start=0,end=100000;uint32 occurence=5,length=1;bool isValid()const{return valid;}};
struct GameEventMgr{using GameEventDataMap=std::vector<GameEventData>;using ActiveEvents=std::set<uint16>;GameEventDataMap events{{false},{true},{true},{false}};ActiveEvents active{1};uint32 writes=0;GameEventDataMap const&GetEventMap(){return events;}ActiveEvents const&GetActiveEventList(){return active;}uint32 NextCheck(uint16){return 10;}void StartEvent(uint16 id,bool force){Require(force,"Native event force lost");++writes;active.insert(id);}void StopEvent(uint16 id,bool force){Require(force,"Native event force lost");++writes;active.erase(id);}}gameEventManager;auto sGameEventMgr=&gameEventManager;
namespace GameTime{time_t GetGameTime(){return 100;}}
static std::string TimeToTimestampStr(time_t){return "controlled time";}static std::string secsToTimeString(uint64){return "controlled duration";}
struct CharTitlesEntry{int32 MaskID;};
struct TitleStore{std::vector<CharTitlesEntry> rows{{0},{0},{2},{63},{64},{130},{-1}};uint32 GetNumRows(){return uint32(rows.size());}CharTitlesEntry const*LookupEntry(uint32 i){return i<rows.size()?&rows[i]:nullptr;}}sCharTitlesStore;
struct SpellInfo{} info;
struct SpellManager{bool exists=true;SpellInfo const*GetSpellInfo(uint32 id){return exists&&id==123?&info:nullptr;}}spellMgr;
auto sSpellMgr=&spellMgr;
struct SpellMgr{static bool IsSpellValid(SpellInfo const*,Player*){return true;}};
static bool Utf8ToUpperOnlyLatin(std::string&s){if(s=="INVALID_UTF8")return false;for(char&c:s)if(c>='a'&&c<='z')c-=32;return true;}
struct LoginDatabasePreparedStatement{uint32 id=0;std::map<uint32,int32> fields;void setUInt32(uint32 i,uint32 v){fields[i]=int32(v);}void setUInt8(uint32 i,uint8 v){fields[i]=v;}void setInt32(uint32 i,int32 v){fields[i]=v;}};

struct LoginService {
 LoginDatabasePreparedStatement pending;bool conflict=false;std::vector<LoginDatabasePreparedStatement>writes;
 LoginDatabasePreparedStatement*GetPreparedStatement(uint32 id){pending={id,{}};return &pending;}
 bool Query(LoginDatabasePreparedStatement*){return conflict;}
 void Execute(LoginDatabasePreparedStatement*p){writes.push_back(*p);}
}LoginDatabase;
namespace AccountMgr {
 static uint32 ownerSecurity=6,targetSecurity=0;static bool missing=false;
 bool GetName(uint32 id,std::string&name){name="ACCOUNT";return id!=0;}
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
#ifdef COUNTERFACTUAL_SETBIT
 Player p;ChatHandler h;h.target=&p;Require(Command(CaseSetBit::Selected::HandleDebugSet32BitCommand,h,"65536 1")&&p.writes==1&&p.flags[0]==1&&!p.badFieldWrites,"Original native16 field alias not reproduced");std::cout<<"COUNTERFACTUAL original field65536 narrows to native index0 and overwrites it;controlled setter,no actual server mutation\n";return 0;
#endif
#ifdef COUNTERFACTUAL_ITEMEXPIRE
 Player p;Item item;item.protectedRemove=true;p.ownedItems[7]=&item;Session ses{&p,1};ChatHandler h;h.session=&ses;moneyScripts.expired=0;Require(Command(CaseItemExpire::Selected::HandleDebugItemExpireCommand,h,"7")&&!item.removed&&moneyScripts.expired==1,"Original protected-item premature expiration hook not reproduced");std::cout<<"COUNTERFACTUAL original expiry hook fires despite protected removal leaving item owned;controlled native-behavior endpoint\n";return 0;
#endif
#ifdef COUNTERFACTUAL_ITEMSET
 Player p;Item small,wide;uint64 guid=(uint64(1)<<32)+7;p.ownedItems[7]=&small;p.ownedItems[guid]=&wide;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(Command(CaseItemSet::Selected::HandleDebugSetItemValueCommand,h,"4294967303 1 123")&&small.writes==1&&!wide.writes&&small.fields[1]==123,"Original item32 alias not reproduced");std::cout<<"COUNTERFACTUAL original native33-bit inventory GUID aliases low7 and changes wrong owned item;controlled inventory only\n";return 0;
#endif
#ifdef COUNTERFACTUAL_PARTY
 Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(Command(CaseParty::Selected::HandleDebugSendQuestPartyMsgCommand,h,"256")&&p.writes==1&&p.partyCode==0,"Original party byte wrap not reproduced");std::cout<<"COUNTERFACTUAL original quest-party code256 is sent as0 through uint8 narrowing;controlled sender only\n";return 0;
#endif
#ifdef COUNTERFACTUAL_BFTIMER
 battlefieldManager={};ChatHandler h;Require(Command(CaseBfTimer::Selected::HandleBattlefieldTimer,h,"1 4294968")&&battlefieldManager.first.timer==704,"Original timer wrap not reproduced");std::cout<<"COUNTERFACTUAL original4294968 seconds becomes704 milliseconds through uint32 wrap;controlled battlefield only\n";return 0;
#endif
#ifdef COUNTERFACTUAL_EXPANSION
 Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;LoginDatabase.writes.clear();Require(Command(CaseExpansion::Selected::HandleAccountAddonCommand,h,"256")&&LoginDatabase.writes.size()==1&&LoginDatabase.writes[0].fields.at(0)==0,"Original expansion wrap not reproduced");std::cout<<"COUNTERFACTUAL original account expansion256 passes narrowed gate and persists0;controlled prepared DB only\n";return 0;
#endif
#ifdef COUNTERFACTUAL_GOSTATE
 Player p;Session ses{&p,1};GameObject go;ChatHandler h;h.session=&ses;h.go=&go;Require(Command(CaseGoState::Selected::HandleGameObjectSetStateCommand,h,"7 0 256")&&go.writes==1&&go.byteValue==0,"Original byte narrowing not reproduced");std::cout<<"COUNTERFACTUAL original GO state256 narrows to0 and mutates controlled byte field;no real world writes\n";return 0;
#endif
#ifdef OLD_GO_GUID_BASELINE
 Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;std::string malformed="7x";Require(CaseGoGuid::Selected::GetGuidLowFromArgsOrLastTargetedGo(&h,malformed.c_str())==7,"Original prefix alias not reproduced");std::string prior="7 stale";strtok(prior.data()," ");std::string empty;Require(CaseGoGuid::Selected::GetGuidLowFromArgsOrLastTargetedGo(&h,empty.c_str())==7&&std::string(strtok(nullptr," "))=="stale","Original cursor reuse not reproduced");std::cout<<"COUNTERFACTUAL original GO helper aliases7x to7 and leaves unrelated token cursor;controlled memory,no world writes\n";return 0;
#endif
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

#ifdef REPAIR_DEST
 for(char const*arg:{"123 nan 1 2","123 1 inf 2","123 1 2 3x","123 1 2","123 bad 1 2"}){Player p;ChatHandler h;h.target=&p;Require(!Command(CaseDest::Selected::HandleCastDestCommand,h,arg)&&!p.spells,"Invalid destination cast");++cases;}
 for(char const*arg:{"123 1 2 3","123 -1 0 20000 triggered","123 0x1p1 2 3 t"}){Player p;ChatHandler h;h.target=&p;Require(Command(CaseDest::Selected::HandleCastDestCommand,h,arg)&&p.spells==1,"Native destination syntax/trigger lost");++cases;}
#endif
#ifdef REPAIR_CUSTOM
 for(char const*arg:{"123 nan","123 inf","123 1 2x","123 1 2 -inf","123 2147483648","123 -2147483649","123 1e1000"}){Player p;ChatHandler h;h.target=&p;Require(!Command(CaseCustom::Selected::HandleCastCustomCommand,h,arg)&&!p.spells,"Unsafe base-point integer cast");++cases;}
 for(char const*arg:{"123","123 1.9 -2.9 3","123 2147483647.9 -2147483648.9 0","123 0x1p1 0 0"}){Player p;ChatHandler h;h.target=&p;Require(Command(CaseCustom::Selected::HandleCastCustomCommand,h,arg)&&p.spells==1&&p.basePoints.size()==3,"Custom native defaults/truncation lost");if(std::string(arg)=="123 1.9 -2.9 3")Require(p.basePoints==std::vector<float>{1,-2,3},"Truncation semantics changed");++cases;}
 for(char const*arg:std::vector<char const*>{nullptr,"","nan","1x","1 2","inf"}){double out=7;Require(!Trinity::ParseCommandDouble(arg,out)&&out==7,"Double parser failed receiver contract");++cases;}
#endif
#ifdef REPAIR_VISUAL
 for(char const*arg:{""," ","1x","2147483648","-2147483649"}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;h.target=&p;Require(!Command(CaseVisual::Selected::HandleCastVisualCommand,h,arg)&&!p.writes,"Visual invalid native signed ID sent");++cases;}
 for(int32 id:{0,1,-1,INT32_MIN,INT32_MAX}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;h.target=&p;Require(Command(CaseVisual::Selected::HandleCastVisualCommand,h,std::to_string(id).c_str())&&p.writes==1&&p.lastVisual==id,"Signed visual packet contract changed");++cases;}
#endif
#ifdef REPAIR_ORPHAN
 for(char const*arg:{""," ","1x","2147483648","-2147483649"}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;h.target=&p;Require(!Command(CaseOrphan::Selected::HandleCastOrphanVisualCommand,h,arg)&&!p.writes,"Orphan invalid native signed ID sent");++cases;}
 for(int32 id:{0,1,-1,INT32_MIN,INT32_MAX}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;h.target=&p;Require(Command(CaseOrphan::Selected::HandleCastOrphanVisualCommand,h,std::to_string(id).c_str())&&p.writes==1&&p.lastVisual==id,"Signed orphan packet contract changed");++cases;}
#endif
#ifdef REPAIR_HONOR
 for(char const*arg:{""," ","1x","2147483648","-2147483649"}){Player p;ChatHandler h;h.target=&p;Require(!Command(CaseHonor::Selected::HandleModifyHonorCommand,h,arg)&&!p.writes&&!h.messages,"Invalid honor reached reward/report");++cases;}
 for(int32 amount:{0,1,-1,INT32_MIN,INT32_MAX})for(bool accepted:{false,true}){Player p;p.honorAccepted=accepted;ChatHandler h;h.target=&p;Require(Command(CaseHonor::Selected::HandleModifyHonorCommand,h,std::to_string(amount).c_str())==accepted&&p.honorAmount==amount&&p.writes==uint32(accepted)&&h.messages==uint32(accepted),"Honor native signed args/receipt not honored");++cases;}
#endif
#ifdef REPAIR_REP
 for(char const*arg:{""," ","1","1x 1","4294967297 1","1 1x","1 -","1 2147483648","1 -2147483649","3 1","4 1"}){Player p;ChatHandler h;h.target=&p;Require(!Command(CaseRep::Selected::HandleModifyRepCommand,h,arg)&&!p.reputation.writes,"Invalid reputation partially changed");++cases;}
 for(uint32 id:{1u,2u})for(int32 amount:{0,1,-1,INT32_MIN,INT32_MAX}){Player p;ChatHandler h;h.target=&p;std::string arg=std::to_string(id)+" "+std::to_string(amount);Require(Command(CaseRep::Selected::HandleModifyRepCommand,h,arg.c_str())&&p.reputation.writes==1&&p.reputation.delta==amount&&p.reputation.paragon==(id==2),"Native reputation/paragon route changed");++cases;}
#endif
#ifdef REPAIR_GOGUID
 for(char const*arg:{"-1","7x","4294967303","abc"}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;std::string text=arg;Require(!CaseGoGuid::Selected::GetGuidLowFromArgsOrLastTargetedGo(&h,text.c_str()),"GO malformed reference aliases object");++cases;}
 for(char const*arg:{"7","+7","4294967295"}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;std::string text=arg;Require(CaseGoGuid::Selected::GetGuidLowFromArgsOrLastTargetedGo(&h,text.c_str())==std::strtoul(arg,nullptr,10),"GO native32 input changed");++cases;}
 {Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;std::string previous="7 stale";strtok(previous.data()," ");std::string empty;Require(CaseGoGuid::Selected::GetGuidLowFromArgsOrLastTargetedGo(&h,empty.c_str())==7&&!strtok(nullptr," "),"GO fallback kept previous token cursor");++cases;}
#endif
#ifdef REPAIR_TURN
 for(char const*arg:{"7 nan","7 1 inf","7 1 2 3x","7 bad"}){Player p;Session ses{&p,1};GameObject go;ChatHandler h;h.session=&ses;h.go=&go;Require(!Command(CaseTurn::Selected::HandleGameObjectTurnCommand,h,arg)&&!go.writes&&!go.saves&&!go.destroys,"Invalid object orientation persisted");++cases;}
 for(char const*arg:{"7","7 1 2 3",""}){Player p;Session ses{&p,1};GameObject go;ChatHandler h;h.session=&ses;h.go=&go;Require(Command(CaseTurn::Selected::HandleGameObjectTurnCommand,h,arg)&&go.saves==1&&go.refreshes==1&&std::isfinite(go.rz),"Native GO rotation/last-target fallback changed");++cases;}
#endif
#ifdef REPAIR_MOVE
 for(char const*arg:{"7 nan 1 2","7 1 inf 2","7 1 2x 3","7 1 2","7 20000 2 3"}){Player p;Session ses{&p,1};GameObject go;ChatHandler h;h.session=&ses;h.go=&go;Require(!Command(CaseMove::Selected::HandleGameObjectMoveCommand,h,arg)&&!go.writes&&!go.saves&&!go.destroys,"Invalid object coordinate persisted");++cases;}
 for(char const*arg:{"7","7 1 2 20000",""}){Player p;Session ses{&p,1};GameObject go;ChatHandler h;h.session=&ses;h.go=&go;Require(Command(CaseMove::Selected::HandleGameObjectMoveCommand,h,arg)&&go.saves==1&&go.refreshes==1,"Native GO move/player fallback changed");++cases;}
#endif
#ifdef REPAIR_GOSIZE
 for(char const*arg:{"7 nan","7 inf","7 -1","7 1x","7"}){Player p;Session ses{&p,1};GameObject go;ChatHandler h;h.session=&ses;h.go=&go;objectManager.goData.personalSize=3;Require(!Command(CaseGoSize::Selected::HandleGameObjectSetSizeCommand,h,arg)&&!go.writes&&!go.saves&&objectManager.goData.personalSize==3,"Invalid GO size persisted");++cases;}
 for(char const*arg:{"7 0","7 2.5","7 0x1p1"}){Player p;Session ses{&p,1};GameObject go;ChatHandler h;h.session=&ses;h.go=&go;Require(Command(CaseGoSize::Selected::HandleGameObjectSetSizeCommand,h,arg)&&go.saves==1&&go.scale==(std::string(arg)=="7 0"?objectManager.goInfo.size:std::strtof(arg+2,nullptr)),"Native GO size/template-reset changed");++cases;}
#endif

#ifdef REPAIR_STAND
 for(char const*arg:{""," ","-1","1x","4294967297","3"}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(!Command(CaseStand::Selected::HandleModifyStandStateCommand,h,arg)&&!p.writes,"Invalid emote state accepted");++cases;}
 for(uint32 id:{0u,1u,2u}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(Command(CaseStand::Selected::HandleModifyStandStateCommand,h,std::to_string(id).c_str())&&p.writes==1&&p.flags[UNIT_FIELD_EMOTE_STATE]==id,"Native emote reset/set lost");++cases;}
#endif
#ifdef REPAIR_MOUNT
 for(char const*arg:{""," ","0","-1","1x","4294967297","70"}){Player p;ChatHandler h;h.target=&p;Require(!Command(CaseMount::Selected::HandleModifyMountCommand,h,arg)&&!p.writes,"Invalid mount selector changed player");++cases;}
 for(uint32 id:{1u,35u,69u}){Player p;p.flags[UNIT_FIELD_FLAGS]=0x400;ChatHandler h;h.target=&p;Require(Command(CaseMount::Selected::HandleModifyMountCommand,h,std::to_string(id).c_str())&&p.mount&&p.speeds[MOVE_RUN]==15&&p.speeds[MOVE_SWIM]==15&&p.flags[UNIT_FIELD_FLAGS]==(0x400|UNIT_FLAG_PVP),"Native mount overwrote unrelated flags/speeds");++cases;}
#endif
#ifdef REPAIR_GOPHASE
 for(char const*arg:{"7","7 0","7 -1","7 1x","7 4294967297"}){Player p;Session ses{&p,1};GameObject go;ChatHandler h;h.session=&ses;h.go=&go;Require(!Command(CaseGoPhase::Selected::HandleGameObjectSetPhaseCommand,h,arg)&&!go.writes&&!go.saves,"Invalid GO phase persisted");++cases;}
 for(uint32 mask:{1u,UINT32_MAX}){Player p;Session ses{&p,1};GameObject go;ChatHandler h;h.session=&ses;h.go=&go;Require(Command(CaseGoPhase::Selected::HandleGameObjectSetPhaseCommand,h,("7 "+std::to_string(mask)).c_str())&&go.phase==mask&&go.saves==1,"Native GO mask/save lost");++cases;}
#endif
#ifdef REPAIR_GOSTATE
 for(char const*arg:{"7","7 0","7 0 1x","7 0 -1","7 0 256","7 -3","7 5 1","7 4294967296 1","7 4 4294967296"}){Player p;Session ses{&p,1};GameObject go;ChatHandler h;h.session=&ses;h.go=&go;Require(!Command(CaseGoState::Selected::HandleGameObjectSetStateCommand,h,arg)&&!go.writes&&!go.despawns&&!go.customCalls,"Invalid GO state/width mutated object");++cases;}
 for(uint32 offset:{0u,1u,3u}){Player p;Session ses{&p,1};GameObject go;ChatHandler h;h.session=&ses;h.go=&go;Require(Command(CaseGoState::Selected::HandleGameObjectSetStateCommand,h,("7 "+std::to_string(offset)+" 255").c_str())&&go.byteOffset==offset&&go.byteValue==255,"Native GO byte route changed");++cases;}
 {Player p;Session ses{&p,1};GameObject go;ChatHandler h;h.session=&ses;h.go=&go;Require(Command(CaseGoState::Selected::HandleGameObjectSetStateCommand,h,"7 -1")&&go.despawns==1,"Native despawn animation lost");Require(Command(CaseGoState::Selected::HandleGameObjectSetStateCommand,h,"7 4 4294967295")&&go.customAnim==UINT32_MAX,"Native UInt32 custom animation narrowed");++cases;}
#endif
#ifdef REPAIR_GODAMAGE
 for(char const*arg:{"7 1x","7 2147483648","7 -2147483648","7 -2147483649"}){Player p;Session ses{&p,1};GameObject go;ChatHandler h;h.session=&ses;h.go=&go;Require(!Command(CaseGoDamage::Selected::HandleDamageObjectCommand,h,arg)&&!go.writes,"Unsafe GO damage negation dispatched");++cases;}
 for(int32 damage:{0,1,-1,INT32_MAX,INT32_MIN+1}){Player p;Session ses{&p,1};GameObject go;ChatHandler h;h.session=&ses;h.go=&go;Require(Command(CaseGoDamage::Selected::HandleDamageObjectCommand,h,("7 "+std::to_string(damage)).c_str())&&go.writes==1&&go.healthDelta==-damage&&go.healthActor==&p,"Native signed GO health/actor route changed");++cases;}
#endif
#ifdef REPAIR_GONEAR
 for(char const*arg:{" ","nan","inf","1x","1e20","-1e20"}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;WorldDatabase.queries=0;Require(!Command(CaseGoNear::Selected::HandleGameObjectNearCommand,h,arg)&&!WorldDatabase.queries,"Invalid GO radius reached SQL");++cases;}
 for(char const*arg:{"","0","-5","1e19"})for(bool rows:{false,true}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;WorldDatabase.queries=0;WorldDatabase.hasRow=rows;Require(Command(CaseGoNear::Selected::HandleGameObjectNearCommand,h,arg)&&WorldDatabase.queries==1&&std::isfinite(WorldDatabase.lastQuery.floats.at(7)),"Native GO finite distance query/list lost");++cases;}
#endif
#ifdef REPAIR_NPCNEAR
 for(char const*arg:{" ","nan","inf","1x","1e20","-1e20"}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;WorldDatabase.queries=0;Require(!Command(CaseNpcNear::Selected::HandleNpcNearCommand,h,arg)&&!WorldDatabase.queries,"Invalid NPC radius reached SQL");++cases;}
 for(char const*arg:{"","0","-5","1e19"})for(bool rows:{false,true}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;WorldDatabase.queries=0;WorldDatabase.hasRow=rows;Require(Command(CaseNpcNear::Selected::HandleNpcNearCommand,h,arg)&&WorldDatabase.queries==1&&std::isfinite(WorldDatabase.lastQuery.floats.at(7)),"Native NPC finite distance query/list lost");++cases;}
#endif
#ifdef REPAIR_NPCGROUP
 for(char const*arg:{""," ","-1","1x","4294967296","1 -1","1 1x","1 4294967296"}){Creature c;ChatHandler h;h.creature=&c;Require(!Command(CaseNpcGroup::Selected::HandleNpcSummonGroupCommand,h,arg)&&!c.groupCalls,"Invalid summon/despawn group dispatched");++cases;}
 for(uint32 group:{0u,1u,UINT32_MAX})for(uint32 del:{0u,1u,UINT32_MAX}){Creature c;ChatHandler h;h.creature=&c;Require(Command(CaseNpcGroup::Selected::HandleNpcSummonGroupCommand,h,(std::to_string(group)+" "+std::to_string(del)).c_str())&&c.groupCalls==1&&c.lastGroup==group&&c.groupDelete==bool(del),"Native explicit zero/group direction changed");++cases;}
#endif
#ifdef REPAIR_NPCENTRY
 for(char const*arg:{""," ","0","-1","1x","4294967297"}){Creature c;ChatHandler h;h.unit=&c;Require(!Command(CaseNpcEntry::Selected::HandleNpcSetEntryCommand,h,arg)&&!c.entryCalls,"Invalid NPC entry dispatched");++cases;}
 for(bool accepted:{false,true}){Creature c;c.entryAccepted=accepted;ChatHandler h;h.unit=&c;Require(Command(CaseNpcEntry::Selected::HandleNpcSetEntryCommand,h,"99")==accepted&&c.entryCalls==1&&c.writes==uint32(accepted),"Native entry rejection reported success");++cases;}
#endif
#ifdef REPAIR_HONORADD
 for(char const*arg:{""," ","1x","2147483648","-2147483649"}){Player p;ChatHandler h;h.target=&p;Require(!Command(CaseHonorAdd::Selected::HandleHonorAddCommand,h,arg)&&!p.writes,"Invalid native honor add dispatched");++cases;}
 for(int32 amount:{0,1,-1,INT32_MIN,INT32_MAX})for(bool accepted:{false,true}){Player p;p.honorAccepted=accepted;ChatHandler h;h.target=&p;Require(Command(CaseHonorAdd::Selected::HandleHonorAddCommand,h,std::to_string(amount).c_str())==accepted&&p.honorAmount==amount&&p.honorGroup==1&&p.honorToken&&p.writes==uint32(accepted),"Native honor-add signed amount/receipt/token route changed");++cases;}
#endif

#ifdef REPAIR_DEST
 {ChatHandler h;Require(!Command(CaseDest::Selected::HandleCastDestCommand,h,"123 1 2 3"),"Missing destination caster accepted");++cases;}
#endif
#ifdef REPAIR_CUSTOM
 {ChatHandler h;Require(!Command(CaseCustom::Selected::HandleCastCustomCommand,h,"123 1"),"Missing custom caster accepted");++cases;}
#endif
#ifdef REPAIR_VISUAL
 {Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(!Command(CaseVisual::Selected::HandleCastVisualCommand,h,"1")&&!p.writes,"Missing visual target accepted");++cases;}
#endif
#ifdef REPAIR_ORPHAN
 {Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(!Command(CaseOrphan::Selected::HandleCastOrphanVisualCommand,h,"1")&&!p.writes,"Missing orphan target accepted");++cases;}
#endif
#ifdef REPAIR_HONOR
 for(bool target:{false,true}){Player p;ChatHandler h;h.target=target?&p:nullptr;h.lower=true;Require(!Command(CaseHonor::Selected::HandleModifyHonorCommand,h,"1")&&!p.writes,"Honor missing/authority admission lost");++cases;}
#endif
#ifdef REPAIR_REP
 for(bool target:{false,true}){Player p;ChatHandler h;h.target=target?&p:nullptr;h.lower=true;Require(!Command(CaseRep::Selected::HandleModifyRepCommand,h,"1 1")&&!p.reputation.writes,"Reputation missing/authority admission lost");++cases;}
#endif
#ifdef REPAIR_MOUNT
 for(bool target:{false,true}){Player p;ChatHandler h;h.target=target?&p:nullptr;h.lower=true;Require(!Command(CaseMount::Selected::HandleModifyMountCommand,h,"1")&&!p.writes,"Mount missing/authority admission lost");++cases;}
#endif
#ifdef REPAIR_HONORADD
 for(bool target:{false,true}){Player p;ChatHandler h;h.target=target?&p:nullptr;h.lower=true;Require(!Command(CaseHonorAdd::Selected::HandleHonorAddCommand,h,"1")&&!p.writes,"Honor-add missing/authority admission lost");++cases;}
#endif
#ifdef REPAIR_NPCENTRY
 {Player p;ChatHandler h;h.target=&p;Require(!Command(CaseNpcEntry::Selected::HandleNpcSetEntryCommand,h,"99")&&!p.writes,"Player accepted as NPC entry target");++cases;}
#endif
#ifdef REPAIR_NPCGROUP
 {ChatHandler h;Require(!Command(CaseNpcGroup::Selected::HandleNpcSummonGroupCommand,h,"1"),"Missing NPC group target accepted");++cases;}
#endif

#ifdef REPAIR_BFSTART
 for(char const*arg:{""," ","-1","1x","4294967297","3"}){battlefieldManager={};ChatHandler h;Require(!Command(CaseBfStart::Selected::HandleBattlefieldStart,h,arg)&&!battlefieldManager.first.writes&&!battlefieldManager.second.writes,"Invalid battlefield start dispatched");++cases;}
 for(uint32 id:{1u,2u}){battlefieldManager={};ChatHandler h;Require(Command(CaseBfStart::Selected::HandleBattlefieldStart,h,std::to_string(id).c_str())&&(id==1?battlefieldManager.first:battlefieldManager.second).starts==1&&h.messages==uint32(id==1),"Native battlefield start/global notification changed");++cases;}
#endif
#ifdef REPAIR_BFEND
 for(char const*arg:{""," ","-1","1x","4294967297","3"}){battlefieldManager={};ChatHandler h;Require(!Command(CaseBfEnd::Selected::HandleBattlefieldEnd,h,arg)&&!battlefieldManager.first.writes,"Invalid battlefield end dispatched");++cases;}
 {battlefieldManager={};ChatHandler h;Require(Command(CaseBfEnd::Selected::HandleBattlefieldEnd,h,"1")&&battlefieldManager.first.ends==1&&battlefieldManager.first.winner,"Native end-battle force changed");++cases;}
#endif
#ifdef REPAIR_BFENABLE
 for(char const*arg:{""," ","-1","1x","4294967297","3"}){battlefieldManager={};ChatHandler h;Require(!Command(CaseBfEnable::Selected::HandleBattlefieldEnable,h,arg)&&!battlefieldManager.first.writes,"Invalid battlefield enable dispatched");++cases;}
 for(bool enabled:{false,true}){battlefieldManager={};battlefieldManager.first.enabled=enabled;ChatHandler h;Require(Command(CaseBfEnable::Selected::HandleBattlefieldEnable,h,"1")&&battlefieldManager.first.enabled!=enabled&&battlefieldManager.first.toggles==1,"Native battlefield toggle direction lost");++cases;}
#endif
#ifdef REPAIR_BFSWITCH
 for(char const*arg:{""," ","-1","1x","4294967297","3"}){battlefieldManager={};ChatHandler h;Require(!Command(CaseBfSwitch::Selected::HandleBattlefieldSwitch,h,arg)&&!battlefieldManager.first.writes,"Invalid battlefield switch dispatched");++cases;}
 {battlefieldManager={};ChatHandler h;Require(Command(CaseBfSwitch::Selected::HandleBattlefieldSwitch,h,"1")&&battlefieldManager.first.ends==1&&!battlefieldManager.first.winner,"Native battlefield switch differs from forced end");++cases;}
#endif
#ifdef REPAIR_BFTIMER
 for(char const*arg:{""," ","1","1x 1","-1 1","4294967297 1","1 nan","1 1x","1 -1","1 4294968","1 4294967295","3 1"}){battlefieldManager={};ChatHandler h;Require(!Command(CaseBfTimer::Selected::HandleBattlefieldTimer,h,arg)&&!battlefieldManager.first.writes&&!battlefieldManager.first.packets,"Invalid/overflow timer reached battlefield");++cases;}
 for(uint32 seconds:{0u,1u,4294967u}){battlefieldManager={};ChatHandler h;Require(Command(CaseBfTimer::Selected::HandleBattlefieldTimer,h,("1 "+std::to_string(seconds)).c_str())&&battlefieldManager.first.timer==seconds*1000&&battlefieldManager.first.writes==1&&battlefieldManager.first.packets==1,"Native milliseconds/broadcast changed");++cases;}
#endif
#ifdef REPAIR_EVENTINFO
 for(char const*arg:{""," ","-1","1x","4294967297","3","4"}){gameEventManager={};ChatHandler h;Require(!Command(CaseEventInfo::Selected::HandleEventInfoCommand,h,arg)&&!gameEventManager.writes,"Invalid world event diagnostic ID accepted");++cases;}
 for(bool active:{false,true}){gameEventManager={};if(!active)gameEventManager.active.clear();ChatHandler h;Require(Command(CaseEventInfo::Selected::HandleEventInfoCommand,h,"1")&&h.messages==1&&!gameEventManager.writes,"Native event info/timing became mutation");++cases;}
#endif
#ifdef REPAIR_EVENTSTART
 for(char const*arg:{""," ","-1","1x","4294967297","0","3","4","1"}){gameEventManager={};ChatHandler h;Require(!Command(CaseEventStart::Selected::HandleEventStartCommand,h,arg)&&!gameEventManager.writes,"Invalid/already active event started");++cases;}
 {gameEventManager={};ChatHandler h;Require(Command(CaseEventStart::Selected::HandleEventStartCommand,h,"2")&&gameEventManager.writes==1&&gameEventManager.active.count(2),"Native forced event start lost");Require(!Command(CaseEventStart::Selected::HandleEventStartCommand,h,"2")&&gameEventManager.writes==1,"Repeated event start mutated twice");++cases;}
#endif
#ifdef REPAIR_EVENTSTOP
 for(char const*arg:{""," ","-1","1x","4294967297","0","3","4","2"}){gameEventManager={};ChatHandler h;Require(!Command(CaseEventStop::Selected::HandleEventStopCommand,h,arg)&&!gameEventManager.writes,"Invalid/inactive event stopped");++cases;}
 {gameEventManager={};ChatHandler h;Require(Command(CaseEventStop::Selected::HandleEventStopCommand,h,"1")&&gameEventManager.writes==1&&!gameEventManager.active.count(1),"Native forced event stop lost");Require(!Command(CaseEventStop::Selected::HandleEventStopCommand,h,"1")&&gameEventManager.writes==1,"Repeated event stop mutated twice");++cases;}
#endif
#ifdef REPAIR_EXPANSION
 for(char const*arg:{""," ","-1","x","1x","7","256","262","4294967296"}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;LoginDatabase.writes.clear();Require(!Command(CaseExpansion::Selected::HandleAccountAddonCommand,h,arg)&&LoginDatabase.writes.empty(),"Invalid/wrapped self expansion persisted");++cases;}
 for(uint32 expansion:{0u,1u,6u}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;LoginDatabase.writes.clear();Require(Command(CaseExpansion::Selected::HandleAccountAddonCommand,h,std::to_string(expansion).c_str())&&LoginDatabase.writes.size()==1&&LoginDatabase.writes[0].id==LOGIN_UPD_EXPANSION&&LoginDatabase.writes[0].fields.at(0)==int32(expansion)&&LoginDatabase.writes[0].fields.at(1)==1,"Native self account/expansion SQL changed");++cases;}
#endif
#ifdef REPAIR_SETEXPANSION
 for(char const*arg:{""," ","ACCOUNT x","ACCOUNT 1x","ACCOUNT -1","ACCOUNT 7","ACCOUNT 256","ACCOUNT 262","ACCOUNT 4294967296"}){ChatHandler h;LoginDatabase.writes.clear();Require(!Command(CaseSetExpansion::Selected::HandleAccountSetAddonCommand,h,arg)&&LoginDatabase.writes.empty(),"Invalid target expansion persisted");++cases;}
 for(uint32 expansion:{0u,1u,6u})for(bool named:{false,true}){Player p;Session ses{&p,2};p.session=&ses;ChatHandler h;h.target=&p;LoginDatabase.writes.clear();std::string arg=(named?"ACCOUNT ":"")+std::to_string(expansion);Require(Command(CaseSetExpansion::Selected::HandleAccountSetAddonCommand,h,arg.c_str())&&LoginDatabase.writes.size()==1&&LoginDatabase.writes[0].fields.at(0)==int32(expansion)&&LoginDatabase.writes[0].fields.at(1)==2,"Named/selected account expansion SQL changed");++cases;}
 {Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;h.lower=true;LoginDatabase.writes.clear();Require(!Command(CaseSetExpansion::Selected::HandleAccountSetAddonCommand,h,"ACCOUNT 6")&&LoginDatabase.writes.empty(),"Target expansion strong authority bypass");++cases;}
#endif

#ifdef REPAIR_CINEMATIC
 for(char const*arg:{""," ","-1","1x","4294967296"}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(!Command(CaseCinematic::Selected::HandleDebugPlayCinematicCommand,h,arg)&&!p.writes,"Invalid cinematic reference sent");++cases;}
 for(uint32 id:{0u,1u,UINT32_MAX}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(Command(CaseCinematic::Selected::HandleDebugPlayCinematicCommand,h,std::to_string(id).c_str())&&p.writes==1&&p.lastCinematic==id,"Native raw cinematic ID changed");++cases;}
#endif
#ifdef REPAIR_MOVIE
 for(char const*arg:{""," ","-1","1x","4294967297","3"}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(!Command(CaseMovie::Selected::HandleDebugPlayMovieCommand,h,arg)&&!p.writes,"Invalid movie reference reached native playback");++cases;}
 for(uint32 id:{1u,2u}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(Command(CaseMovie::Selected::HandleDebugPlayMovieCommand,h,std::to_string(id).c_str())&&p.writes==1&&p.lastMovie==id,"Native known movie playback lost");++cases;}
#endif
#ifdef REPAIR_SOUND
 for(char const*arg:{""," ","-1","1x","4294967296"}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;h.target=&p;Require(!Command(CaseSound::Selected::HandleDebugPlaySoundCommand,h,arg)&&!p.writes,"Invalid sound reference sent");++cases;}
 for(bool selected:{false,true})for(uint32 id:{0u,1u,UINT32_MAX}){Player p;p.selection=selected;Session ses{&p,1};ChatHandler h;h.session=&ses;h.target=&p;Require(Command(CaseSound::Selected::HandleDebugPlaySoundCommand,h,std::to_string(id).c_str())&&p.writes==1&&p.lastSound==id&&p.distanceSound==selected,"Native selected distance/direct sound branch changed");++cases;}
 {Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(!Command(CaseSound::Selected::HandleDebugPlaySoundCommand,h,"1")&&!p.writes,"Missing sound actor accepted");++cases;}
#endif
#ifdef REPAIR_ANIM
 for(char const*arg:{""," ","-1","1x","4294967296"}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(!Command(CaseAnim::Selected::HandleDebugAnimCommand,h,arg)&&!p.writes,"Invalid emote reference sent");++cases;}
 for(uint32 id:{0u,1u,UINT32_MAX}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(Command(CaseAnim::Selected::HandleDebugAnimCommand,h,std::to_string(id).c_str())&&p.writes==1&&p.lastEmote==id,"Native raw emote ID changed");++cases;}
#endif
#ifdef REPAIR_STREAM
 for(char const*arg:{""," ","-32769","1x","32768","65536","4294967296"}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(!Command(CaseStream::Selected::HandleDebugStreamingMoviesCommand,h,arg)&&!p.packetCalls,"Invalid streaming file ID sent");++cases;}
 for(int16 id:{int16(-32768),int16(-1),int16(0),int16(1),int16(32767)}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(Command(CaseStream::Selected::HandleDebugStreamingMoviesCommand,h,std::to_string(id).c_str())&&p.packetCalls==1&&p.streamIDs==std::vector<int16>{id},"Native streaming signed16 payload changed");++cases;}
#endif
#ifdef REPAIR_EQUIP
 for(char const*arg:{""," ","-1","1x","256","4294967297"}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(!Command(CaseEquip::Selected::HandleDebugSendEquipErrorCommand,h,arg)&&!p.writes,"Invalid equipment byte sent");++cases;}
 for(uint32 id:{0u,1u,127u,255u}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(Command(CaseEquip::Selected::HandleDebugSendEquipErrorCommand,h,std::to_string(id).c_str())&&p.writes==1&&p.lastError==id&&p.errorChannel==1,"Native raw unsigned equipment byte changed");++cases;}
#endif
#ifdef REPAIR_SELL
 for(char const*arg:{""," ","-1","1x","16","4294967297"}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(!Command(CaseSell::Selected::HandleDebugSendSellErrorCommand,h,arg)&&!p.writes,"Out-of-native-range sell enum converted");++cases;}
 for(uint32 id:{0u,1u,9u,15u}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(Command(CaseSell::Selected::HandleDebugSendSellErrorCommand,h,std::to_string(id).c_str())&&p.writes==1&&p.lastError==id&&p.errorChannel==2,"Native representable sell code changed");++cases;}
#endif
#ifdef REPAIR_BUY
 for(char const*arg:{""," ","-1","1x","16","4294967297"}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(!Command(CaseBuy::Selected::HandleDebugSendBuyErrorCommand,h,arg)&&!p.writes,"Out-of-native-range buy enum converted");++cases;}
 for(uint32 id:{0u,1u,12u,15u}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(Command(CaseBuy::Selected::HandleDebugSendBuyErrorCommand,h,std::to_string(id).c_str())&&p.writes==1&&p.lastError==id&&p.errorChannel==3,"Native representable buy code changed");++cases;}
#endif
#ifdef REPAIR_PARTY
 for(char const*arg:{""," ","-1","1x","256","4294967297"}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(!Command(CaseParty::Selected::HandleDebugSendQuestPartyMsgCommand,h,arg)&&!p.writes,"Invalid quest-party byte sent");++cases;}
 for(uint32 id:{0u,1u,255u}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(Command(CaseParty::Selected::HandleDebugSendQuestPartyMsgCommand,h,std::to_string(id).c_str())&&p.writes==1&&p.partyCode==id,"Native raw quest-party byte changed");++cases;}
#endif
#ifdef REPAIR_JUMP
 for(char const*arg:{""," ","1 2 3 4","nan 2 3 4 5","1 inf 3 4 5","1 2 3x 4 5","1 2 3 nan 5","1 2 3 4 inf","20000 2 3 4 5","1 2 3 4 1e20"}){Player p;ChatHandler h;h.target=&p;Require(!Command(CaseJump::Selected::HandleDebugMoveJump,h,arg)&&!p.motion.jumps,"Invalid/nonfinite native parabola dispatched");++cases;}
 for(char const*arg:{"1 2 3 4 5","1 2 20000 4 5","-1 0 -100 -5 -5"}){Player p;ChatHandler h;h.target=&p;Require(Command(CaseJump::Selected::HandleDebugMoveJump,h,arg)&&p.motion.jumps==1&&p.motion.jumpArgs.size()==5,"Native finite jump request policy changed");++cases;}
 for(bool actor:{false,true}){Player p;p.unitEligible=false;ChatHandler h;h.target=actor?&p:nullptr;Require(!Command(CaseJump::Selected::HandleDebugMoveJump,h,"1 2 3 4 5")&&!p.motion.jumps,"Missing/non-unit jump actor accepted");++cases;}
#endif

#ifdef REPAIR_FIELDGET
 for(char const*arg:{""," ","-1","1x","65536","4 x","4 2147483648"}){Player p;ChatHandler h;h.target=&p;Require(!Command(CaseFieldGet::Selected::HandleDebugGetValueCommand,h,arg)&&!p.fieldReads&&!p.writes,"Invalid field/mode reached native getter");++cases;}
 for(char const*mode:{""," 1"," -1"," 0"}){Player p;ChatHandler h;h.target=&p;Require(Command(CaseFieldGet::Selected::HandleDebugGetValueCommand,h,("4"+std::string(mode)).c_str())&&p.fieldReads==1&&!p.writes,"Native integer/float read mode changed");++cases;}
#endif
#ifdef REPAIR_FIELDSET
 for(char const*arg:{""," ","4","-1 1","4x 1","65536 1","4 1x","4 -1","4 4294967296","4 1 x","4 nan 0","4 inf 0","4 1x 0"}){Player p;ChatHandler h;h.target=&p;Require(!Command(CaseFieldSet::Selected::HandleDebugSetValueCommand,h,arg)&&!p.writes&&!p.badFieldWrites,"Invalid raw field/value/mode reached setter");++cases;}
 for(uint32 value:{0u,1u,UINT32_MAX}){Player p;ChatHandler h;h.target=&p;Require(Command(CaseFieldSet::Selected::HandleDebugSetValueCommand,h,("4 "+std::to_string(value)).c_str())&&p.writes==1&&p.flags[4]==value,"Native unsigned raw field value changed");++cases;}
 for(char const*value:{"0","-1.5","0x1p1"}){Player p;ChatHandler h;h.target=&p;Require(Command(CaseFieldSet::Selected::HandleDebugSetValueCommand,h,("4 "+std::string(value)+" 0").c_str())&&p.writes==1&&p.floatFields[4]==std::strtof(value,nullptr),"Native finite raw float mode changed");++cases;}
 {ChatHandler h;Require(!Command(CaseFieldSet::Selected::HandleDebugSetValueCommand,h,"4 1"),"Missing field target accepted");++cases;}
#endif
#ifdef REPAIR_DYNAMICGET
 for(char const*arg:{""," ","1","-1 0","1x 0","3 0","0 -1","0 1x","0 32","4294967296 0","0 4294967296"}){Player p;ChatHandler h;h.target=&p;Require(!Command(CaseDynamicGet::Selected::HandleDebugGetDynamicValueCommand,h,arg)&&!p.dynamicReads&&!p.dynamicWrites,"Invalid dynamic selector reached reader");++cases;}
 for(uint32 field:{0u,2u})for(uint32 offset:{0u,31u}){Player p;ChatHandler h;h.target=&p;Require(Command(CaseDynamicGet::Selected::HandleDebugGetDynamicValueCommand,h,(std::to_string(field)+" "+std::to_string(offset)).c_str())&&p.dynamicReads==1&&!p.dynamicWrites,"Native zero/missing dynamic offset read changed");++cases;}
#endif
#ifdef REPAIR_DYNAMICSET
 for(char const*arg:{""," ","1 0","-1 0 1","1x 0 1","3 0 1","0 -1 1","0 1x 1","0 32 1","0 0 1x","0 0 -1","0 0 4294967296"}){Player p;ChatHandler h;h.target=&p;Require(!Command(CaseDynamicSet::Selected::HandleDebugSetDynamicValueCommand,h,arg)&&!p.dynamicWrites,"Invalid dynamic selector/value reached writer");++cases;}
 for(uint32 field:{0u,2u})for(uint32 offset:{0u,31u})for(uint32 value:{0u,UINT32_MAX}){Player p;ChatHandler h;h.target=&p;Require(Command(CaseDynamicSet::Selected::HandleDebugSetDynamicValueCommand,h,(std::to_string(field)+" "+std::to_string(offset)+" "+std::to_string(value)).c_str())&&p.dynamicWrites==1&&p.dynamicFields[field].size()==offset+1&&p.dynamicFields[field][offset]==value,"Native bounded dynamic resize/value changed");++cases;}
#endif
#ifdef REPAIR_ITEMGET
 for(char const*arg:{""," ","7","-1 1","7x 1","0 1","1099511627783 1","18446744073709551616 1","7 1x","7 65536","8 1"}){Player p;Item item;p.ownedItems[7]=&item;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(!Command(CaseItemGet::Selected::HandleDebugGetItemValueCommand,h,arg)&&!item.reads&&!item.writes,"Invalid/wrapped item read reached field");++cases;}
 for(uint64 guid:{uint64(7),(uint64(1)<<32)+7,ObjectGuid::GetMaxCounter(HighGuid::Item)}){Player p;Item item;p.ownedItems[guid]=&item;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(Command(CaseItemGet::Selected::HandleDebugGetItemValueCommand,h,(std::to_string(guid)+" 1").c_str())&&item.reads==1&&p.lastItem==guid&&!item.writes,"Native40-bit owned item read lost width");++cases;}
#endif
#ifdef REPAIR_ITEMSET
 for(char const*arg:{""," ","7 1","-1 1 1","7x 1 1","0 1 1","1099511627783 1 1","18446744073709551616 1 1","7 1x 1","7 65536 1","7 1 -1","7 1 1x","7 1 4294967296","8 1 1"}){Player p;Item item;p.ownedItems[7]=&item;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(!Command(CaseItemSet::Selected::HandleDebugSetItemValueCommand,h,arg)&&!item.writes,"Invalid/wrapped item write mutated field");++cases;}
 for(uint64 guid:{uint64(7),(uint64(1)<<32)+7,ObjectGuid::GetMaxCounter(HighGuid::Item)})for(uint32 value:{0u,UINT32_MAX}){Player p;Item small,wide;p.ownedItems[7]=&small;p.ownedItems[guid]=&wide;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(Command(CaseItemSet::Selected::HandleDebugSetItemValueCommand,h,(std::to_string(guid)+" 1 "+std::to_string(value)).c_str())&&wide.writes==1&&wide.fields[1]==value&&!small.writes&&p.lastItem==guid,"Native40-bit item write aliases small GUID");++cases;}
#endif
#ifdef REPAIR_ITEMEXPIRE
 for(char const*arg:{""," ","-1","7x","0","1099511627783","18446744073709551616","8"}){Player p;Item item;p.ownedItems[7]=&item;Session ses{&p,1};ChatHandler h;h.session=&ses;moneyScripts.expired=0;Require(!Command(CaseItemExpire::Selected::HandleDebugItemExpireCommand,h,arg)&&!p.destroyCalls&&!moneyScripts.expired&&!item.removed,"Invalid/wrapped item expired");++cases;}
 for(bool protectedRemove:{false,true}){Player p;Item small,wide;uint64 guid=(uint64(1)<<32)+7;wide.slot=2;wide.proto.entry=9;wide.protectedRemove=protectedRemove;p.ownedItems[7]=&small;p.ownedItems[guid]=&wide;Session ses{&p,1};ChatHandler h;h.session=&ses;moneyScripts.expired=0;Require(Command(CaseItemExpire::Selected::HandleDebugItemExpireCommand,h,std::to_string(guid).c_str())==!protectedRemove&&p.destroyCalls==1&&wide.removed==!protectedRemove&&!small.removed&&moneyScripts.expired==uint32(!protectedRemove),"Native protected deletion/expire receipt lost");if(!protectedRemove){Require(moneyScripts.expirePlayer==&p&&moneyScripts.expireTemplate==&wide.proto,"Expire hook actor/template changed");Require(!Command(CaseItemExpire::Selected::HandleDebugItemExpireCommand,h,std::to_string(guid).c_str())&&moneyScripts.expired==1,"Repeated expire notified twice");}++cases;}
#endif
#ifdef REPAIR_MOD32
 for(char const*arg:{""," ","4","-1 1","4x 1","65536 1","4 1x","4 2147483648","4 -2147483649"}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(!Command(CaseMod32::Selected::HandleDebugMod32ValueCommand,h,arg)&&!p.writes&&!p.fieldReads,"Invalid signed raw addition reached field");++cases;}
 for(auto const&row:std::vector<std::tuple<uint32,int32,uint32,bool>>{{0,0,0,true},{0,-1,UINT32_MAX,true},{UINT32_MAX,1,0,true},{uint32(INT32_MAX),1,uint32(INT32_MAX),false},{uint32(1)<<31,-1,uint32(1)<<31,false},{uint32(1)<<31,1,(uint32(1)<<31)+1,true},{0,INT32_MIN,uint32(1)<<31,true}}){Player p;p.flags[4]=std::get<0>(row);Session ses{&p,1};ChatHandler h;h.session=&ses;Require(Command(CaseMod32::Selected::HandleDebugMod32ValueCommand,h,("4 "+std::to_string(std::get<1>(row))).c_str())==std::get<3>(row)&&p.flags[4]==std::get<2>(row)&&p.writes==uint32(std::get<3>(row)),"Native signed field addition boundary/bit semantics changed");++cases;}
#endif
#ifdef REPAIR_SETBIT
 for(char const*arg:{""," ","4","-1 1","4x 1","65536 1","4 -1","4 1x","4 33","4294967300 1"}){Player p;ChatHandler h;h.target=&p;Require(!Command(CaseSetBit::Selected::HandleDebugSet32BitCommand,h,arg)&&!p.writes&&!p.badFieldWrites,"Invalid field index or bit reached native narrow setter");++cases;}
 for(uint32 bit:{0u,1u,31u,32u}){Player p;ChatHandler h;h.target=&p;Require(Command(CaseSetBit::Selected::HandleDebugSet32BitCommand,h,("4 "+std::to_string(bit)).c_str())&&p.writes==1&&p.flags[4]==(bit?(uint32(1)<<(bit-1)):0),"Native bit32/explicit zero reset changed");++cases;}
 {Player p;ChatHandler h;h.target=&p;Require(!Command(CaseSetBit::Selected::HandleDebugSet32BitCommand,h,(std::to_string(p.GetValuesCount())+" 1").c_str())&&!p.badFieldWrites&&!p.writes,"Past-end field reached native diagnostic setter");++cases;}
#endif
#ifdef REPAIR_KILLPOINTS
 for(char const*arg:{""," ","-1","1x","4294967296"}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(!Command(CaseKillPoints::Selected::HandleDebugKillPointsCommand,h,arg)&&p.m_killPoints==17,"Invalid kill points replaced value");++cases;}
 for(uint32 value:{0u,1u,UINT32_MAX}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(Command(CaseKillPoints::Selected::HandleDebugKillPointsCommand,h,std::to_string(value).c_str())&&p.m_killPoints==value,"Native UInt32 kill-point field narrowed");++cases;}
 {Session ses;ChatHandler h;h.session=&ses;Require(!Command(CaseKillPoints::Selected::HandleDebugKillPointsCommand,h,"1")&&!h.messages,"Missing kill-point actor reported success");++cases;}
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
 std::cout<<"PASS "<<cases<<" complete selected new-fifty production handler/helper contexts;native enums/coordinate/fall/counter sources,controlled endpoints,no live world/account/character mutation;not actual client traversal\n";
#endif
 return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
