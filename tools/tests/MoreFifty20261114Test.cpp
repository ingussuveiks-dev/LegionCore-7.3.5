// Selected production handlers and declared admission prefixes. External services
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
using int8=int8_t;using int16=int16_t;using uint8=uint8_t;using uint16=uint16_t;using uint32=uint32_t;using int32=int32_t;using uint64=uint64_t;using int64=int64_t;
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
struct Session;struct Player;struct Creature;struct GameObject;struct CharTitlesEntry;struct AchievementEntry{uint32 ID;};
struct ItemTemplate{uint32 entry=7;};
struct Item{uint64 xp=100;uint32 writes=0,states=0,reads=0;Player*owner=nullptr;std::map<uint16,uint32>fields;ItemTemplate proto;uint8 bag=0,slot=1;bool protectedRemove=false,removed=false;uint64 GetUInt64Value(uint32 field){Require(field==ITEM_FIELD_ARTIFACT_XP,"Wrong artifact field read");return xp;}void SetUInt64Value(uint32 field,uint64 value){Require(field==ITEM_FIELD_ARTIFACT_XP,"Wrong artifact field write");++writes;xp=value;}void SetState(uint32 state,Player*player){Require(state==ITEM_CHANGED,"Wrong item state");++states;owner=player;}uint16 GetValuesCount(){return ITEM_END;}uint32 GetUInt32Value(uint16 field){Require(field<ITEM_END,"Bad item read index");++reads;return fields[field];}void SetUInt32Value(uint16 field,uint32 value){Require(field<ITEM_END,"Bad item write index");++writes;fields[field]=value;}uint8 GetBagSlot(){return bag;}uint8 GetSlot(){return slot;}ItemTemplate const*GetTemplate(){return &proto;}};
struct PhaseManager{uint32 writes=0,mask=0;void SetCustomPhase(uint32 value){++writes;mask=value;}};
struct WorldPacket{std::vector<int16>ids;};
namespace WorldPackets{namespace Misc{struct StreamingMovie{std::vector<int16>MovieIDs;WorldPacket packet;WorldPacket const*Write(){packet.ids=MovieIDs;return &packet;}};}}
struct MotionMaster{uint32 initializes=0,jumps=0;std::vector<float>jumpArgs;void Initialize(){++initializes;}void MoveJump(float x,float y,float z,float xy,float vz){++jumps;jumpArgs={x,y,z,xy,vz};}};
struct Unit {
 bool IsVehicle(){return true;}
 uint32 moveFlags=0,extraFlags=0,movementUpdates=0;uint32 GetUnitMovementFlags(){return moveFlags;}uint32 GetExtraUnitMovementFlags(){return extraFlags;}void SetUnitMovementFlags(uint32 v){++writes;moveFlags=v;}void SetExtraUnitMovementFlags(uint32 v){++writes;extraFlags=v;}void SendMovementFlagUpdate(){++movementUpdates;}
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
struct Player:Unit{Player*ToPlayer()override{return this;}
 uint32 swaps=0;uint16 skillId=0,skillLevel=0,skillMax=0;bool positionsValid=true;
 bool IsValidPos(uint8,uint8,bool){return positionsValid;}void SwapItem(uint16,uint16){++swaps;}
 uint16 GetSkillValue(uint32){return 1;}uint16 GetPureMaxSkillValue(uint32){return 100;}uint16 GetSkillStep(uint16){return 2;}void SetSkill(uint16 id,uint16,uint16 level,uint16 max){++writes;skillId=id;skillLevel=level;skillMax=max;}
 void SetTitle(CharTitlesEntry const*,bool=false);char const*GetName(){return "Controlled";}uint64 GetGUIDLow(){return 7;}
 uint32 achievements=0,kills=0;std::vector<uint32>criteria;
 void CompletedAchievement(AchievementEntry const*entry){++writes;achievements=entry->ID;}
 void KilledMonsterCredit(uint32 id){++kills;lastTarget=id;}
 void UpdateAchievementCriteria(CriteriaTypes type,uint32 a,uint32 b,uint32 c,Unit*,bool){++writes;criteria={uint32(type),a,b,c};}

 int16 m_baseRatingValue[MAX_COMBAT_RATING]{};
 std::vector<uint32>ratingMasks;uint32 versUpdates=0;int32 spellPower=0;std::vector<float>attackValues;std::vector<uint32>rescale;
 uint32 mailType=0,mailResult=0;
 void SendMailResult(uint32 id,MailResponseType type,MailResponseResult result){Require(id==0,"Mail diagnostic id changed");++writes;mailType=type;mailResult=result;}
 void SendUpdateCR(int32 mask){ratingMasks.push_back(mask);++writes;}
 void ApplyRatingMod(CombatRating,int32,bool);
 uint32 m_baseSpellPower=0;uint32 GetBaseSpellPowerBonus(){return m_baseSpellPower;}
 void ApplySpellPowerBonus(int32,bool);void UpdateSpellDamageAndHealingBonus(){++writes;}
 void HandleStatModifier(UnitMods,UnitModifierType,float value,bool apply){Require(apply,"Wrong native attackpower sign");++writes;attackValues.push_back(value);}
 void SetStatFloatValue(uint16 field,float value){SetFloatValue(field,value);}
 void UpdateVersality(){++versUpdates;}
 void RescaleAllForTimeWalk(uint32 level,uint32 max,uint32 min){++writes;rescale={level,max,min};}
};
#include "NativeRating.inc"
#include "NativeSpellPower.inc"

namespace WorldPackets{namespace Misc{struct PhaseShiftDataPhase{uint16 Id;explicit PhaseShiftDataPhase(uint16 id):Id(id){}};}}
struct Session{Player*player=nullptr;uint32 account=1;uint32 phasePackets=0;std::vector<uint16>phaseValues;void SendSetPhaseShift(std::vector<WorldPackets::Misc::PhaseShiftDataPhase>const&p,std::vector<uint16>const&t,std::vector<uint16>const&w,std::vector<uint16>const&s){++phasePackets;phaseValues=t;for(auto v:p)phaseValues.push_back(v.Id);phaseValues.insert(phaseValues.end(),w.begin(),w.end());phaseValues.insert(phaseValues.end(),s.begin(),s.end());}
 Player*GetPlayer(){return player;}uint32 GetAccountId(){return account;}};
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
struct WorldConfig{uint32 shutdowns=0,lastTime=0;uint8 lastMask=0,lastExit=0;bool getBoolConfig(uint32){return true;}void ShutdownServ(uint32 time,uint32 mask,uint8 code){++shutdowns;lastTime=time;lastMask=mask;lastExit=code;}uint32 getIntConfig(WorldIntConfigs key){Require(key==CONFIG_MAX_PLAYER_LEVEL,"Wrong level config");return 120;}}worldConfig;auto sWorld=&worldConfig;
struct UpdateClock{uint32 writes=0,last=0;void SetRecordUpdateTimeInterval(uint32 value){++writes;last=value;}}sWorldUpdateTime;
struct SkillLineEntry{FactionName DisplayName[1];};struct SkillStore{SkillLineEntry row;SkillLineEntry const*LookupEntry(uint32){return &row;}}sSkillLineStore;
struct AreaTableEntry{uint32 ID;};struct AreaStore{AreaTableEntry row;AreaTableEntry const*LookupEntry(uint32 id){row.ID=id;return &row;}}sAreaTableStore;
struct GameObjectTemplate{std::string name="Controlled GO";float size=1.5f;};
struct GameObjectData{uint32 id=99;float personalSize=3;};
struct GameObject:Unit{uint32 despawns=0,customCalls=0,customAnim=0,byteOffset=0,byteValue=0;int32 healthDelta=0;Unit*healthActor=nullptr;void SetByteValue(uint32 field,uint8 offset,uint8 value){Require(field==GAMEOBJECT_FIELD_BYTES_1&&offset<4,"Wrong GO byte field");++writes;byteOffset=offset;byteValue=value;}void SendCustomAnim(uint32 id){++customCalls;customAnim=id;}void SendObjectDeSpawnAnim(ObjectGuid){++despawns;}void ModifyHealth(int32 delta,Unit*actor){++writes;healthDelta=delta;healthActor=actor;}GameObjectTemplate info;uint32 saves=0,refreshes=0,destroys=0,visibility=0;float rz=0,ry=0,rx=0;ObjectGuid GetGUID(){return {7};}uint32 GetEntry(){return 99;}GameObjectTemplate const*GetGOInfo(){return &info;}void Relocate(float x,float y,float z){++writes;position.x=x;position.y=y;position.z=z;}void Relocate(float x,float y,float z,float o){Relocate(x,y,z);position.o=o;}void RelocateStationaryPosition(float,float,float,float){++writes;}void SetWorldRotationAngles(float z,float y,float x){++writes;rz=z;ry=y;rx=x;}void DestroyForNearbyPlayers(){++destroys;}void UpdateObjectVisibility(){++visibility;}void SaveToDB(){++saves;}void Refresh(){++refreshes;}};
struct MapManager{static bool IsValidMapCoord(uint32 id,float x,float y,float z){return id==1&&Trinity::IsValidMapCoord(x,y,z);}};
struct ObjectManager{uint32 GetDBCLocaleIndex(){return 0;}CreatureTemplate data;CreatureData spawn;GameObjectTemplate goInfo;GameObjectData goData;uint32 GetXPForLevel(uint32 level){return level*100;}CreatureTemplate const*GetCreatureTemplate(uint32){return &data;}CreatureData&NewOrExistCreatureData(uint64 id){Require(id==7,"Wrong spawn");return spawn;}GameObjectData const*GetGOData(uint32 id){return id==7?&goData:nullptr;}GameObjectTemplate const*GetGameObjectTemplate(uint32 id){return id==99?&goInfo:nullptr;}GameObjectData&NewGOData(uint64 id){Require(id==7,"Wrong GO spawn");return goData;}}objectManager;auto sObjectMgr=&objectManager;
struct WorldDatabasePreparedStatement{uint32 id=0;std::map<uint32,uint64>values;std::map<uint32,float>floats;void setUInt16(uint32 i,uint16 v){values[i]=v;}void setUInt32(uint32 i,uint32 v){values[i]=v;}void setUInt64(uint32 i,uint64 v){values[i]=v;}void setUInt8(uint32 i,uint8 v){values[i]=v;}void setFloat(uint32 i,float v){Require(std::isfinite(v),"Nonfinite DB write");floats[i]=v;}};
struct Field{uint64 integer=0;float number=0;uint64 GetUInt64(){return integer;}uint32 GetUInt32(){return uint32(integer);}uint16 GetUInt16(){return uint16(integer);}float GetFloat(){return number;}};
struct QueryRow{Field fields[6]{{(uint64(1)<<32)+7,0},{99,0},{0,1},{0,2},{0,3},{1,0}};Field*Fetch(){return fields;}bool NextRow(){return false;}};
using PreparedQueryResult=std::shared_ptr<QueryRow>;
struct WorldService{WorldDatabasePreparedStatement pending,lastQuery;uint32 queries=0;bool hasRow=false;std::vector<WorldDatabasePreparedStatement>writes;WorldDatabasePreparedStatement*GetPreparedStatement(uint32 id){pending={id,{},{}};return &pending;}void Execute(WorldDatabasePreparedStatement*row){writes.push_back(*row);}PreparedQueryResult Query(WorldDatabasePreparedStatement*row){++queries;lastQuery=*row;return hasRow?std::make_shared<QueryRow>():nullptr;}}WorldDatabase;
struct ChatHandler {
 uint64 observed=0;
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
struct CharTitlesEntry{int32 MaskID;FactionName*Name=&factionName;};
void Player::SetTitle(CharTitlesEntry const*title,bool remove){++writes;if(remove)titles&=~(uint64(1)<<title->MaskID);else titles|=uint64(1)<<title->MaskID;}
struct AchievementStore{AchievementEntry entry{7};AchievementEntry const*LookupEntry(uint32 id){return id==7?&entry:nullptr;}}sAchievementStore;
struct CharacterService{uint32 writes=0;uint64 last=0;template<class...T>void PExecute(char const*,uint32 id,T...){++writes;last=id;}}CharacterDatabase;
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
#include "Selected.inc"
template<class Fn>static bool Command(Fn fn,ChatHandler&h,char const*args){std::string text=args;return fn(&h,text.c_str());}
int main(){try{unsigned cases=0;
#ifdef COUNTERFACTUAL_MAILRESULTINDEPENDENTOPERANDS
 {Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(Command(CaseMailResultIndependentOperands::Selected::HandleSendMailStatus,h,"1 2")&&p.mailResult==1,"Pinned mail operand conflation not reproduced");std::cout<<"COUNTERFACTUAL original ignores second mail result operand;controlled packet endpoint only\n";return 0;}
#endif
#ifdef COUNTERFACTUAL_MOVEMENTFLAGSATOMICOPERANDS
 {Player p;ChatHandler h;h.target=&p;Require(Command(CaseMovementFlagsAtomicOperands::Selected::HandleDebugMoveflagsCommand,h,"1 bad")&&p.writes==2&&p.moveFlags==1,"Pinned partial movement flags not reproduced");std::cout<<"COUNTERFACTUAL original applies malformed two-field request;controlled fields only\n";return 0;}
#endif
#ifdef COUNTERFACTUAL_TICKETCLOSEBYIDEXACTID
 {ChatHandler h;Require(Command(CaseTicketCloseByIdExactId::Selected::HandleGMTicketCloseByIdCommand,h,"7x")&&h.observed==7,"Pinned ticket alias not reproduced");std::cout<<"COUNTERFACTUAL original7x selects ticket7 before service boundary;no live ticket mutation\n";return 0;}
#endif
#ifdef COUNTERFACTUAL_SERVERSHUTDOWNEXACTDEADLINE
 {ChatHandler h;worldConfig.shutdowns=0;Require(Command(CaseServerShutDownExactDeadline::Selected::HandleServerShutDownCommand,h,"1x")&&worldConfig.shutdowns==1&&worldConfig.lastTime==1,"Pinned shutdown parse not reproduced");std::cout<<"COUNTERFACTUAL original1x schedules one-second shutdown;controlled world endpoint only\n";return 0;}
#endif
#ifdef COUNTERFACTUAL_HIDEAREAEXACTIDBITS
 {Player p;ChatHandler h;h.target=&p;Require(Command(CaseHideAreaExactIdBits::Selected::HandleHideAreaCommand,h,"1")&&p.flags[PLAYER_FIELD_EXPLORED_ZONES]==2,"Pinned hide toggles on not reproduced");Require(Command(CaseHideAreaExactIdBits::Selected::HandleHideAreaCommand,h,"1")&&p.flags[PLAYER_FIELD_EXPLORED_ZONES]==0,"Pinned hide double toggle not reproduced");std::cout<<"COUNTERFACTUAL original hide command exposes unexplored area and toggles on repeat;controlled fields only\n";return 0;}
#endif

#ifdef REPAIR_HASTECACHECHECKED
 for(char const*arg:{""," ","x","1x","2147483648","-2147483649","32768","-32769"}){Player p;ChatHandler h;h.target=&p;Require(!Command(CaseHasteCacheChecked::Selected::HandleDebugModifyHasteCommand,h,arg)&&!p.writes,"Invalid rating delta mutated cache");++cases;}
 for(int32 delta:{-32768,-1,0,1,32767}){Player p;ChatHandler h;h.target=&p;Require(Command(CaseHasteCacheChecked::Selected::HandleDebugModifyHasteCommand,h,std::to_string(delta).c_str())&&p.writes==3,"Native rating delta rejected");for(auto rating:{CR_HASTE_MELEE,CR_HASTE_RANGED,CR_HASTE_SPELL})Require(p.m_baseRatingValue[rating]==delta,"Native cache delta changed");++cases;}
 for(auto rating:{CR_HASTE_MELEE,CR_HASTE_RANGED,CR_HASTE_SPELL})for(int sign:{-1,1}){Player p;p.m_baseRatingValue[rating]=sign>0?32767:-32768;auto original=p.m_baseRatingValue[rating];ChatHandler h;h.target=&p;Require(!Command(CaseHasteCacheChecked::Selected::HandleDebugModifyHasteCommand,h,std::to_string(sign).c_str())&&!p.writes&&p.m_baseRatingValue[rating]==original,"Multi-cache operation partially applied/wrapped");++cases;}
#endif

#ifdef REPAIR_HITCACHECHECKED
 for(char const*arg:{""," ","x","1x","2147483648","-2147483649","32768","-32769"}){Player p;ChatHandler h;h.target=&p;Require(!Command(CaseHitCacheChecked::Selected::HandleDebugModifyHitCommand,h,arg)&&!p.writes,"Invalid rating delta mutated cache");++cases;}
 for(int32 delta:{-32768,-1,0,1,32767}){Player p;ChatHandler h;h.target=&p;Require(Command(CaseHitCacheChecked::Selected::HandleDebugModifyHitCommand,h,std::to_string(delta).c_str())&&p.writes==3,"Native rating delta rejected");for(auto rating:{CR_HIT_MELEE,CR_HIT_RANGED,CR_HIT_SPELL})Require(p.m_baseRatingValue[rating]==delta,"Native cache delta changed");++cases;}
 for(auto rating:{CR_HIT_MELEE,CR_HIT_RANGED,CR_HIT_SPELL})for(int sign:{-1,1}){Player p;p.m_baseRatingValue[rating]=sign>0?32767:-32768;auto original=p.m_baseRatingValue[rating];ChatHandler h;h.target=&p;Require(!Command(CaseHitCacheChecked::Selected::HandleDebugModifyHitCommand,h,std::to_string(sign).c_str())&&!p.writes&&p.m_baseRatingValue[rating]==original,"Multi-cache operation partially applied/wrapped");++cases;}
#endif

#ifdef REPAIR_MASTERYCACHECHECKED
 for(char const*arg:{""," ","x","1x","2147483648","-2147483649","32768","-32769"}){Player p;ChatHandler h;h.target=&p;Require(!Command(CaseMasteryCacheChecked::Selected::HandleDebugModifyMasteryCommand,h,arg)&&!p.writes,"Invalid rating delta mutated cache");++cases;}
 for(int32 delta:{-32768,-1,0,1,32767}){Player p;ChatHandler h;h.target=&p;Require(Command(CaseMasteryCacheChecked::Selected::HandleDebugModifyMasteryCommand,h,std::to_string(delta).c_str())&&p.writes==1,"Native rating delta rejected");for(auto rating:{CR_MASTERY})Require(p.m_baseRatingValue[rating]==delta,"Native cache delta changed");++cases;}
 for(auto rating:{CR_MASTERY})for(int sign:{-1,1}){Player p;p.m_baseRatingValue[rating]=sign>0?32767:-32768;auto original=p.m_baseRatingValue[rating];ChatHandler h;h.target=&p;Require(!Command(CaseMasteryCacheChecked::Selected::HandleDebugModifyMasteryCommand,h,std::to_string(sign).c_str())&&!p.writes&&p.m_baseRatingValue[rating]==original,"Multi-cache operation partially applied/wrapped");++cases;}
#endif

#ifdef REPAIR_SPELLPOWERSIGNEDDELTA
 for(auto row:std::vector<std::tuple<uint32,int32,bool,uint32>>{{100,INT32_MAX,false,100},{100,1,true,101},{100,-101,true,0},{100,-2147483647,true,0},{100,INT32_MIN,false,100},{uint32(INT32_MAX),0,true,uint32(INT32_MAX)},{UINT32_MAX,-1,false,UINT32_MAX}}){Player p;p.m_baseSpellPower=std::get<0>(row);ChatHandler h;h.target=&p;Require(Command(CaseSpellpowerSignedDelta::Selected::HandleDebugModifySpellpowerCommand,h,std::to_string(std::get<1>(row)).c_str())==std::get<2>(row)&&p.m_baseSpellPower==std::get<3>(row)&&p.writes==uint32(std::get<2>(row)),"Native spellpower absolute/sign/base sum contract lost");++cases;}
 for(char const*arg:{""," ","x","1x","2147483648","-2147483649","-2147483648"}){Player p;ChatHandler h;h.target=&p;Require(!Command(CaseSpellpowerSignedDelta::Selected::HandleDebugModifySpellpowerCommand,h,arg)&&!p.writes,"Invalid stat delta mutated player");++cases;}
 for(int32 delta:{-2147483647,-1,0,1,INT32_MAX}){Player p;ChatHandler h;h.target=&p;Require(Command(CaseSpellpowerSignedDelta::Selected::HandleDebugModifySpellpowerCommand,h,std::to_string(delta).c_str())&&p.writes,"Native signed stat delta lost");++cases;}
#endif

#ifdef REPAIR_ATTACKPOWERSIGNEDDELTA
 for(char const*arg:{""," ","x","1x","2147483648","-2147483649"}){Player p;ChatHandler h;h.target=&p;Require(!Command(CaseAttackpowerSignedDelta::Selected::HandleDebugModifyAttackpowerCommand,h,arg)&&!p.writes,"Invalid stat delta mutated player");++cases;}
 for(int32 delta:{INT32_MIN,-1,0,1,INT32_MAX}){Player p;ChatHandler h;h.target=&p;Require(Command(CaseAttackpowerSignedDelta::Selected::HandleDebugModifyAttackpowerCommand,h,std::to_string(delta).c_str())&&p.writes,"Native signed stat delta lost");++cases;}
#endif

#ifdef REPAIR_CRITCHANCESIGNEDDELTA
 for(char const*arg:{""," ","x","1x","2147483648","-2147483649"}){Player p;ChatHandler h;h.target=&p;Require(!Command(CaseCritChanceSignedDelta::Selected::HandleDebugModifyCritChanceCommand,h,arg)&&!p.writes,"Invalid stat delta mutated player");++cases;}
 for(int32 delta:{INT32_MIN,-1,0,1,INT32_MAX}){Player p;ChatHandler h;h.target=&p;Require(Command(CaseCritChanceSignedDelta::Selected::HandleDebugModifyCritChanceCommand,h,std::to_string(delta).c_str())&&p.writes,"Native signed stat delta lost");++cases;}
#endif

#ifdef REPAIR_VERSATILITYATOMICSUMS
 for(char const*arg:{""," ","x","1x","2147483648","-2147483649","-1"}){Player p;ChatHandler h;h.target=&p;Require(!Command(CaseVersatilityAtomicSums::Selected::HandleDebugModifyVersalityCommand,h,arg)&&!p.writes&&!p.versUpdates,"Bad versatility changed player");++cases;}
 for(auto rating:{CR_VERSATILITY_DAMAGE_DONE,CR_VERSATILITY_HEALING_DONE,CR_VERSATILITY_DAMAGE_TAKEN})for(int sign:{-1,1}){Player p;for(auto cr:{CR_VERSATILITY_DAMAGE_DONE,CR_VERSATILITY_HEALING_DONE,CR_VERSATILITY_DAMAGE_TAKEN})p.flags[PLAYER_FIELD_COMBAT_RATINGS+cr]=100;p.flags[PLAYER_FIELD_COMBAT_RATINGS+rating]=sign>0?UINT32_MAX:0;auto original=p.flags;ChatHandler h;h.target=&p;Require(!Command(CaseVersatilityAtomicSums::Selected::HandleDebugModifyVersalityCommand,h,std::to_string(sign).c_str())&&!p.writes&&!p.versUpdates&&p.flags==original,"Versatility partial/unsigned wrap");++cases;}
 for(int32 delta:{-100,0,100,INT32_MAX}){Player p;for(auto cr:{CR_VERSATILITY_DAMAGE_DONE,CR_VERSATILITY_HEALING_DONE,CR_VERSATILITY_DAMAGE_TAKEN})p.flags[PLAYER_FIELD_COMBAT_RATINGS+cr]=100;ChatHandler h;h.target=&p;Require(Command(CaseVersatilityAtomicSums::Selected::HandleDebugModifyVersalityCommand,h,std::to_string(delta).c_str())&&p.writes==3&&p.versUpdates==1,"Valid versatility changed native updates");++cases;}
#endif
#ifdef REPAIR_EFFECTIVELEVELNATIVEWIDTHS
 for(char const*arg:{""," ","x","1x","-1","256","1 65536","1 1 -1","1 1 65536"}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(!Command(CaseEffectiveLevelNativeWidths::Selected::HandleDebugSetEffectiveLevel,h,arg)&&!p.writes&&!h.messages,"Bad scaling value reached rescale");++cases;}
 for(char const*arg:{"0","1","255 65535 65535","110 950 800"}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(Command(CaseEffectiveLevelNativeWidths::Selected::HandleDebugSetEffectiveLevel,h,arg)&&p.writes==1,"Native scaling limits lost");++cases;}
 {Session ses;ChatHandler h;h.session=&ses;Require(!Command(CaseEffectiveLevelNativeWidths::Selected::HandleDebugSetEffectiveLevel,h,"1")&&!h.messages,"Missing scaling player reported success");++cases;}
#endif
#ifdef REPAIR_PHASESHIFTNATIVEWIDTHS
 for(char const*arg:{""," ","x","1x","-1","65536","1 65536","1 1 -1","1 1 1 65536"}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(!Command(CasePhaseShiftNativeWidths::Selected::HandleDebugSendSetPhaseShiftCommand,h,arg)&&!ses.phasePackets,"Bad phase id sent packet");++cases;}
 for(char const*arg:{"0","65535","1 2 3 4","0 0 0 0"}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(Command(CasePhaseShiftNativeWidths::Selected::HandleDebugSendSetPhaseShiftCommand,h,arg)&&ses.phasePackets==1,"Native phase raw IDs/omission lost");++cases;}
#endif
#ifdef REPAIR_MAILRESULTINDEPENDENTOPERANDS
 for(char const*arg:{""," ","x","1x","-1","6","4294967296","1 7","1 -1","1 4294967296","1 1x"}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(!Command(CaseMailResultIndependentOperands::Selected::HandleSendMailStatus,h,arg)&&!p.writes,"Bad mail diagnostic sent");++cases;}
 for(uint32 type=0;type<=5;++type)for(uint32 result:{0u,1u,6u,14u,15u,16u,17u,18u,19u,21u}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(Command(CaseMailResultIndependentOperands::Selected::HandleSendMailStatus,h,(std::to_string(type)+" "+std::to_string(result)).c_str())&&p.writes==1&&p.mailType==type&&p.mailResult==result,"Independent mail operands conflated");++cases;}
 for(uint32 type=0;type<=5;++type){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(Command(CaseMailResultIndependentOperands::Selected::HandleSendMailStatus,h,std::to_string(type).c_str())&&p.mailType==type&&p.mailResult==type,"One-operand legacy mail form lost");++cases;}
#endif


#ifdef REPAIR_TITLEADDEXACTID
 for(char const*arg:{""," ","x","2x","4294967298","-2","2147483648"}){Player p;ChatHandler h;h.target=&p;Require(!Command(CaseTitleAddExactId::Selected::HandleTitlesAddCommand,h,arg)&&!p.writes,"Invalid title mutated actor");++cases;}
 for(int mode:{0,1,2}){Player p;ChatHandler h;h.target=mode==2?nullptr:&p;h.lower=mode==1;bool ok=Command(CaseTitleAddExactId::Selected::HandleTitlesAddCommand,h,"2");Require(ok==(mode==0)&&bool(p.writes)==(mode==0),"Title security/selection/native write changed");++cases;}
#endif

#ifdef REPAIR_TITLECURRENTEXACTID
 for(char const*arg:{""," ","x","2x","4294967298","-2","2147483648"}){Player p;ChatHandler h;h.target=&p;Require(!Command(CaseTitleCurrentExactId::Selected::HandleTitlesCurrentCommand,h,arg)&&!p.writes,"Invalid title mutated actor");++cases;}
 for(int mode:{0,1,2}){Player p;ChatHandler h;h.target=mode==2?nullptr:&p;h.lower=mode==1;bool ok=Command(CaseTitleCurrentExactId::Selected::HandleTitlesCurrentCommand,h,"2");Require(ok==(mode==0)&&bool(p.writes)==(mode==0),"Title security/selection/native write changed");++cases;}
#endif

#ifdef REPAIR_TITLEREMOVEEXACTID
 for(char const*arg:{""," ","x","2x","4294967298","-2","2147483648"}){Player p;ChatHandler h;h.target=&p;Require(!Command(CaseTitleRemoveExactId::Selected::HandleTitlesRemoveCommand,h,arg)&&!p.writes,"Invalid title mutated actor");++cases;}
 for(int mode:{0,1,2}){Player p;ChatHandler h;h.target=mode==2?nullptr:&p;h.lower=mode==1;bool ok=Command(CaseTitleRemoveExactId::Selected::HandleTitlesRemoveCommand,h,"2");Require(ok==(mode==0)&&bool(p.writes)==(mode==0),"Title security/selection/native write changed");++cases;}
#endif

#ifdef REPAIR_ACHIEVEMENTADDEXACTID
 for(char const*arg:{""," ","x","7x","4294967303","-7","0"}){Player p;ChatHandler h;h.target=&p;Require(!Command(CaseAchievementAddExactId::Selected::HandleAchievementAddCommand,h,arg)&&!p.writes,"Bad achievement ID awarded");++cases;}
 {Player p;ChatHandler h;h.target=&p;Require(Command(CaseAchievementAddExactId::Selected::HandleAchievementAddCommand,h,"7")&&p.achievements==7,"Actual achievement lookup changed");++cases;}
#endif
#ifdef REPAIR_ACHIEVEMENTDELETEEXACTID
 for(char const*arg:{""," ","x","7x","4294967303","-7","0"}){Player p;Session ses{&p,1};ChatHandler h;h.target=&p;h.session=&ses;CharacterDatabase.writes=0;Require(!Command(CaseAchievementDeleteExactId::Selected::HandleAchievementDeleteCommand,h,arg)&&!CharacterDatabase.writes,"Bad achievement ID deleted");++cases;}
 {Player p;Session ses{&p,1};p.session=&ses;ChatHandler h;h.session=&ses;CharacterDatabase.writes=0;Require(Command(CaseAchievementDeleteExactId::Selected::HandleAchievementDeleteCommand,h,"4294967295")&&CharacterDatabase.writes==2&&CharacterDatabase.last==UINT32_MAX,"Native account/character deletion ID lost");++cases;}
#endif
#ifdef REPAIR_ACHIEVEMENTCRITERIAATOMICOPERANDS
 for(char const*arg:{""," ","x","1x","-1","213","0 x","0 1 x","0 1 1 x","0 4294967296"}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(!Command(CaseAchievementCriteriaAtomicOperands::Selected::HandleAchievementCriteriaCommand,h,arg)&&!p.writes&&!p.kills,"Bad criteria partly awarded");++cases;}
 for(char const*arg:{"0","0 7 0 0","212 4294967295 4294967295 4294967295"}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(Command(CaseAchievementCriteriaAtomicOperands::Selected::HandleAchievementCriteriaCommand,h,arg)&&p.writes==1&&p.criteria.size()==4,"Native criteria domain/defaults lost");++cases;}
#endif
#ifdef REPAIR_UPDATEFIELDEXACTOPERANDS
 for(char const*arg:{""," ","x","1x","-1","4294967296","1 x","1 -1","1 4294967296"}){Player p;ChatHandler h;h.target=&p;Require(!Command(CaseUpdateFieldExactOperands::Selected::HandleDebugUpdateCommand,h,arg)&&!p.writes&&!p.fieldReads,"Bad debug update reaches field");++cases;}
 {Player p;ChatHandler h;h.target=&p;Require(Command(CaseUpdateFieldExactOperands::Selected::HandleDebugUpdateCommand,h,"1 4294967295")&&p.flags[1]==UINT32_MAX&&p.writes==1,"Native update write width lost");Require(Command(CaseUpdateFieldExactOperands::Selected::HandleDebugUpdateCommand,h,"1")&&p.fieldReads==1,"Native read mode lost");++cases;}
#endif
#ifdef REPAIR_MOVEMENTFLAGSATOMICOPERANDS
 for(char const*arg:{" ","x","1x","-1","4294967296","1 x","1 -1","1 4294967296"}){Player p;ChatHandler h;h.target=&p;Require(!Command(CaseMovementFlagsAtomicOperands::Selected::HandleDebugMoveflagsCommand,h,arg)&&!p.writes&&!p.movementUpdates,"Bad flags partly changed");++cases;}
 for(char const*arg:{"","0","4294967295 4294967295"}){Player p;ChatHandler h;h.target=&p;Require(Command(CaseMovementFlagsAtomicOperands::Selected::HandleDebugMoveflagsCommand,h,arg)&&p.movementUpdates==uint32(*arg!=0),"Native flags read/zero/full mask lost");++cases;}
#endif
#ifdef REPAIR_SESSIONACCOUNTEXACTWIDTH
 for(char const*arg:{"x","1x","-1","4294967296","0"}){Player p;ChatHandler h;Require(!Command(CaseSessionAccountExactWidth::Selected::HandleDebugSession,h,arg)&&!h.databaseReached,"Bad account reached lookup");++cases;}
 for(uint32 id:{1u,UINT32_MAX}){ChatHandler h;Require(Command(CaseSessionAccountExactWidth::Selected::HandleDebugSession,h,std::to_string(id).c_str())&&h.observed==id,"Native account width lost before lookup");++cases;}
 {Player p;Session ses{&p,7};p.session=&ses;ChatHandler h;h.target=&p;Require(Command(CaseSessionAccountExactWidth::Selected::HandleDebugSession,h,"")&&h.observed==7,"Selected account fallback lost");++cases;}
#endif
#ifdef REPAIR_VEHICLEKITEXACTID
 for(char const*arg:{""," ","x","1x","-1","4294967296"}){Player p;Creature c;ChatHandler h;h.unit=&c;Require(!Command(CaseVehicleKitExactId::Selected::HandleDebugSetVehicleIdCommand,h,arg)&&!h.databaseReached,"Bad vehicle id reached destructive kit replacement");++cases;}
 {Creature c;ChatHandler h;h.unit=&c;Require(Command(CaseVehicleKitExactId::Selected::HandleDebugSetVehicleIdCommand,h,"4294967295")&&h.observed==UINT32_MAX,"Vehicle id native domain narrowed");++cases;}
#endif


#ifdef REPAIR_TICKETASSIGNTOEXACTID
 for(char const*arg:{""," ","x","7x","-1","4294967303","18446744073709551616"}){ChatHandler h;Require(!Command(CaseTicketAssignToExactId::Selected::HandleGMTicketAssignToCommand,h,arg)&&!h.databaseReached,"Invalid ticket selector reached service boundary");++cases;}
 for(uint32 id:{0u,7u,UINT32_MAX}){ChatHandler h;Require(Command(CaseTicketAssignToExactId::Selected::HandleGMTicketAssignToCommand,h,std::to_string(id).c_str())&&h.databaseReached==1&&h.observed==id,"Native ticket id altered before service boundary");++cases;}
#endif

#ifdef REPAIR_TICKETCLOSEBYIDEXACTID
 for(char const*arg:{""," ","x","7x","-1","4294967303","18446744073709551616"}){ChatHandler h;Require(!Command(CaseTicketCloseByIdExactId::Selected::HandleGMTicketCloseByIdCommand,h,arg)&&!h.databaseReached,"Invalid ticket selector reached service boundary");++cases;}
 for(uint32 id:{0u,7u,UINT32_MAX}){ChatHandler h;Require(Command(CaseTicketCloseByIdExactId::Selected::HandleGMTicketCloseByIdCommand,h,std::to_string(id).c_str())&&h.databaseReached==1&&h.observed==id,"Native ticket id altered before service boundary");++cases;}
#endif

#ifdef REPAIR_TICKETCOMMENTEXACTID
 for(char const*arg:{""," ","x","7x","-1","4294967303","18446744073709551616"}){ChatHandler h;Require(!Command(CaseTicketCommentExactId::Selected::HandleGMTicketCommentCommand,h,arg)&&!h.databaseReached,"Invalid ticket selector reached service boundary");++cases;}
 for(uint32 id:{0u,7u,UINT32_MAX}){ChatHandler h;Require(Command(CaseTicketCommentExactId::Selected::HandleGMTicketCommentCommand,h,std::to_string(id).c_str())&&h.databaseReached==1&&h.observed==id,"Native ticket id altered before service boundary");++cases;}
#endif

#ifdef REPAIR_TICKETCOMPLETEEXACTID
 for(char const*arg:{""," ","x","7x","-1","4294967303","18446744073709551616"}){ChatHandler h;Require(!Command(CaseTicketCompleteExactId::Selected::HandleGMTicketCompleteCommand,h,arg)&&!h.databaseReached,"Invalid ticket selector reached service boundary");++cases;}
 for(uint32 id:{0u,7u,UINT32_MAX}){ChatHandler h;Require(Command(CaseTicketCompleteExactId::Selected::HandleGMTicketCompleteCommand,h,std::to_string(id).c_str())&&h.databaseReached==1&&h.observed==id,"Native ticket id altered before service boundary");++cases;}
#endif

#ifdef REPAIR_TICKETDELETEBYIDEXACTID
 for(char const*arg:{""," ","x","7x","-1","4294967303","18446744073709551616"}){ChatHandler h;Require(!Command(CaseTicketDeleteByIdExactId::Selected::HandleGMTicketDeleteByIdCommand,h,arg)&&!h.databaseReached,"Invalid ticket selector reached service boundary");++cases;}
 for(uint32 id:{0u,7u,UINT32_MAX}){ChatHandler h;Require(Command(CaseTicketDeleteByIdExactId::Selected::HandleGMTicketDeleteByIdCommand,h,std::to_string(id).c_str())&&h.databaseReached==1&&h.observed==id,"Native ticket id altered before service boundary");++cases;}
#endif

#ifdef REPAIR_TICKETESCALATEEXACTID
 for(char const*arg:{""," ","x","7x","-1","4294967303","18446744073709551616"}){ChatHandler h;Require(!Command(CaseTicketEscalateExactId::Selected::HandleGMTicketEscalateCommand,h,arg)&&!h.databaseReached,"Invalid ticket selector reached service boundary");++cases;}
 for(uint32 id:{0u,7u,UINT32_MAX}){ChatHandler h;Require(Command(CaseTicketEscalateExactId::Selected::HandleGMTicketEscalateCommand,h,std::to_string(id).c_str())&&h.databaseReached==1&&h.observed==id,"Native ticket id altered before service boundary");++cases;}
#endif

#ifdef REPAIR_TICKETUNASSIGNEXACTID
 for(char const*arg:{""," ","x","7x","-1","4294967303","18446744073709551616"}){ChatHandler h;Require(!Command(CaseTicketUnAssignExactId::Selected::HandleGMTicketUnAssignCommand,h,arg)&&!h.databaseReached,"Invalid ticket selector reached service boundary");++cases;}
 for(uint32 id:{0u,7u,UINT32_MAX}){ChatHandler h;Require(Command(CaseTicketUnAssignExactId::Selected::HandleGMTicketUnAssignCommand,h,std::to_string(id).c_str())&&h.databaseReached==1&&h.observed==id,"Native ticket id altered before service boundary");++cases;}
#endif

#ifdef REPAIR_TICKETGETBYIDEXACTID
 for(char const*arg:{""," ","x","7x","-1","4294967303","18446744073709551616"}){ChatHandler h;Require(!Command(CaseTicketGetByIdExactId::Selected::HandleGMTicketGetByIdCommand,h,arg)&&!h.databaseReached,"Invalid ticket selector reached service boundary");++cases;}
 for(uint32 id:{0u,7u,UINT32_MAX}){ChatHandler h;Require(Command(CaseTicketGetByIdExactId::Selected::HandleGMTicketGetByIdCommand,h,std::to_string(id).c_str())&&h.databaseReached==1&&h.observed==id,"Native ticket id altered before service boundary");++cases;}
#endif

#ifdef REPAIR_TICKETRESPONSESHAREDEXACTID
 for(char const*arg:{""," ","x","7x","-1","4294967303","18446744073709551616"}){ChatHandler h;Require(!Command(CaseTicketResponseSharedExactId::Selected::_HandleGMTicketResponseAppendCommand,h,arg)&&!h.databaseReached,"Invalid ticket selector reached service boundary");++cases;}
 for(uint32 id:{0u,7u,UINT32_MAX}){ChatHandler h;Require(Command(CaseTicketResponseSharedExactId::Selected::_HandleGMTicketResponseAppendCommand,h,std::to_string(id).c_str())&&h.databaseReached==1&&h.observed==id,"Native ticket id altered before service boundary");++cases;}
#endif

#ifdef REPAIR_INSTANCEUNBINDEXACTSELECTORS
 for(char const*arg:{""," ","x","1x","-1","65537","1 255","1 -2","1 35","all x","all -257"}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(!Command(CaseInstanceUnbindExactSelectors::Selected::HandleInstanceUnbindCommand,h,arg)&&!h.databaseReached,"Invalid map/difficulty entered bind mutation");++cases;}
 for(char const*arg:{"all","all -1","all 0","1","65535 34"}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(Command(CaseInstanceUnbindExactSelectors::Selected::HandleInstanceUnbindCommand,h,arg)&&h.databaseReached==1,"Native all-map/default difficulty domain lost");++cases;}
#endif

#ifdef REPAIR_SERVERSHUTDOWNEXACTDEADLINE
 for(char const*arg:{""," ","x","1x","-1","2147483648","4294967296","0 x","0 1x","0 -1","0 126","0 4294967296"}){ChatHandler h;worldConfig.shutdowns=0;Require(!Command(CaseServerShutDownExactDeadline::Selected::HandleServerShutDownCommand,h,arg)&&!worldConfig.shutdowns,"Malformed shutdown changed lifecycle");++cases;}
 for(int32 time:{0,1,INT32_MAX})for(int code:{-1,0,125}){ChatHandler h;worldConfig.shutdowns=0;std::string arg=std::to_string(time)+(code<0?"":" "+std::to_string(code));Require(Command(CaseServerShutDownExactDeadline::Selected::HandleServerShutDownCommand,h,arg.c_str())&&worldConfig.shutdowns==1&&worldConfig.lastTime==uint32(time)&&worldConfig.lastMask==0&&worldConfig.lastExit==(code<0?0:code),"Native shutdown mode/deadline/code changed");++cases;}
#endif

#ifdef REPAIR_SERVERRESTARTEXACTDEADLINE
 for(char const*arg:{""," ","x","1x","-1","2147483648","4294967296","0 x","0 1x","0 -1","0 126","0 4294967296"}){ChatHandler h;worldConfig.shutdowns=0;Require(!Command(CaseServerRestartExactDeadline::Selected::HandleServerRestartCommand,h,arg)&&!worldConfig.shutdowns,"Malformed shutdown changed lifecycle");++cases;}
 for(int32 time:{0,1,INT32_MAX})for(int code:{-1,0,125}){ChatHandler h;worldConfig.shutdowns=0;std::string arg=std::to_string(time)+(code<0?"":" "+std::to_string(code));Require(Command(CaseServerRestartExactDeadline::Selected::HandleServerRestartCommand,h,arg.c_str())&&worldConfig.shutdowns==1&&worldConfig.lastTime==uint32(time)&&worldConfig.lastMask==1&&worldConfig.lastExit==(code<0?2:code),"Native shutdown mode/deadline/code changed");++cases;}
#endif

#ifdef REPAIR_SERVERIDLERESTARTEXACTDEADLINE
 for(char const*arg:{""," ","x","1x","-1","2147483648","4294967296","0 x","0 1x","0 -1","0 126","0 4294967296"}){ChatHandler h;worldConfig.shutdowns=0;Require(!Command(CaseServerIdleRestartExactDeadline::Selected::HandleServerIdleRestartCommand,h,arg)&&!worldConfig.shutdowns,"Malformed shutdown changed lifecycle");++cases;}
 for(int32 time:{0,1,INT32_MAX})for(int code:{-1,0,125}){ChatHandler h;worldConfig.shutdowns=0;std::string arg=std::to_string(time)+(code<0?"":" "+std::to_string(code));Require(Command(CaseServerIdleRestartExactDeadline::Selected::HandleServerIdleRestartCommand,h,arg.c_str())&&worldConfig.shutdowns==1&&worldConfig.lastTime==uint32(time)&&worldConfig.lastMask==3&&worldConfig.lastExit==(code<0?2:code),"Native shutdown mode/deadline/code changed");++cases;}
#endif

#ifdef REPAIR_SERVERIDLESHUTDOWNEXACTDEADLINE
 for(char const*arg:{""," ","x","1x","-1","2147483648","4294967296","0 x","0 1x","0 -1","0 126","0 4294967296"}){ChatHandler h;worldConfig.shutdowns=0;Require(!Command(CaseServerIdleShutDownExactDeadline::Selected::HandleServerIdleShutDownCommand,h,arg)&&!worldConfig.shutdowns,"Malformed shutdown changed lifecycle");++cases;}
 for(int32 time:{0,1,INT32_MAX})for(int code:{-1,0,125}){ChatHandler h;worldConfig.shutdowns=0;std::string arg=std::to_string(time)+(code<0?"":" "+std::to_string(code));Require(Command(CaseServerIdleShutDownExactDeadline::Selected::HandleServerIdleShutDownCommand,h,arg.c_str())&&worldConfig.shutdowns==1&&worldConfig.lastTime==uint32(time)&&worldConfig.lastMask==2&&worldConfig.lastExit==(code<0?0:code),"Native shutdown mode/deadline/code changed");++cases;}
#endif

#ifdef REPAIR_SERVERDIFFINTERVALEXACTVALUE
 for(char const*arg:{""," ","x","1x","-1","2147483648","4294967296"}){ChatHandler h;sWorldUpdateTime.writes=0;Require(!Command(CaseServerDiffIntervalExactValue::Selected::HandleServerSetDiffTimeCommand,h,arg)&&!sWorldUpdateTime.writes,"Bad interval changed clock");++cases;}
 for(int32 value:{0,1,INT32_MAX}){ChatHandler h;sWorldUpdateTime.writes=0;Require(Command(CaseServerDiffIntervalExactValue::Selected::HandleServerSetDiffTimeCommand,h,std::to_string(value).c_str())&&sWorldUpdateTime.writes==1&&sWorldUpdateTime.last==uint32(value),"Native interval/reset lost");++cases;}
#endif
#ifdef REPAIR_DISABLEADDEXACTIDFLAGS
 for(char const*arg:{""," ","x 1 reason","1x 1 reason","-1 1 reason","0 1 reason","4294967296 1 reason","1 -1 reason","1 256 reason","1 x reason","1 1"}){ChatHandler h;Require(!Command(CaseDisableAddExactIdFlags::Selected::HandleAddDisables,h,arg)&&!h.databaseReached,"Bad disable id/flags reached persistence");++cases;}
 for(uint32 id:{1u,UINT32_MAX})for(uint32 flags:{0u,255u}){ChatHandler h;Require(Command(CaseDisableAddExactIdFlags::Selected::HandleAddDisables,h,(std::to_string(id)+" "+std::to_string(flags)+" reason").c_str())&&h.observed==((uint64(id)<<8)|flags),"Native disable fields changed before persistence");++cases;}
#endif
#ifdef REPAIR_DISABLEREMOVEEXACTID
 for(char const*arg:{""," ","x","1x","-1","0","4294967296"}){ChatHandler h;Require(!Command(CaseDisableRemoveExactId::Selected::HandleRemoveDisables,h,arg)&&!h.databaseReached,"Bad disable removal reached persistence");++cases;}
 for(uint32 id:{1u,UINT32_MAX}){ChatHandler h;Require(Command(CaseDisableRemoveExactId::Selected::HandleRemoveDisables,h,std::to_string(id).c_str())&&h.observed==id,"Native disable removal id lost");++cases;}
#endif
#ifdef REPAIR_ITEMMOVEEXACTSLOTS
 for(char const*arg:{""," ","x 1","1x 2","-1 1","256 1","1 x","1 -1","1 256"}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(!Command(CaseItemMoveExactSlots::Selected::HandleItemMoveCommand,h,arg)&&!p.swaps,"Wrapped slot moved inventory");++cases;}
 for(bool valid:{false,true}){Player p;p.positionsValid=valid;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(Command(CaseItemMoveExactSlots::Selected::HandleItemMoveCommand,h,"1 2")==valid&&p.swaps==uint32(valid),"Native slot validation bypassed");++cases;}
 {Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(Command(CaseItemMoveExactSlots::Selected::HandleItemMoveCommand,h,"255 255")&&!p.swaps,"Same-slot no-op lost");++cases;}
#endif
#ifdef REPAIR_WEATHERFINITETYPEDREQUEST
 for(char const*arg:{""," ","x 1","1x 1","-1 1","4294967296 1","127 1","1 x","1 nan","1 inf","1 1x"}){ChatHandler h;Require(!Command(CaseWeatherFiniteTypedRequest::Selected::HandleChangeWeather,h,arg)&&!h.databaseReached,"Bad weather reached zone update");++cases;}
 for(uint32 type:{0u,1u,2u,3u,86u,90u})for(char const*grade:{"-1","0","1","0x1p-1"}){ChatHandler h;Require(Command(CaseWeatherFiniteTypedRequest::Selected::HandleChangeWeather,h,(std::to_string(type)+" "+grade).c_str())&&h.observed==type,"Native weather type/finite grade lost");++cases;}
#endif
#ifdef REPAIR_SKILLNATIVEWIDTHS
 for(char const*arg:{""," ","x 1","1x 1","-1 1","65537 1","1 x","1 -1","1 65536","1 0","1 101","1 1 x","1 1 65536","1 2 1"}){Player p;ChatHandler h;h.target=&p;Require(!Command(CaseSkillNativeWidths::Selected::HandleSetSkillCommand,h,arg)&&!p.writes,"Wrapped skill/level/max changed skill");++cases;}
 for(char const*arg:{"1 1","65535 65535 65535","1 100"}){Player p;ChatHandler h;h.target=&p;Require(Command(CaseSkillNativeWidths::Selected::HandleSetSkillCommand,h,arg)&&p.writes==1,"Native skill widths/default cap lost");++cases;}
#endif

#ifdef REPAIR_SCENARIOEXACTID
 for(char const*arg:{""," ","x","7x","-1","4294967303","18446744073709551616"}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(!Command(CaseScenarioExactId::Selected::HandleSetScenarioCommand,h,arg)&&!h.databaseReached&&!p.writes,"Malformed identifier reached native service boundary");++cases;}
 for(uint32 id:{0u,7u,UINT32_MAX}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(Command(CaseScenarioExactId::Selected::HandleSetScenarioCommand,h,std::to_string(id).c_str())&&h.databaseReached==1&&h.observed==id,"Native identifier width changed before downstream admission");++cases;}
#endif

#ifdef REPAIR_CONVERSATIONEXACTID
 for(char const*arg:{""," ","x","7x","-1","4294967303","18446744073709551616"}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(!Command(CaseConversationExactId::Selected::HandleConversationCommand,h,arg)&&!h.databaseReached&&!p.writes,"Malformed identifier reached native service boundary");++cases;}
 for(uint32 id:{0u,7u,UINT32_MAX}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(Command(CaseConversationExactId::Selected::HandleConversationCommand,h,std::to_string(id).c_str())&&h.databaseReached==1&&h.observed==id,"Native identifier width changed before downstream admission");++cases;}
#endif

#ifdef REPAIR_AREACASTACTIONEXACTID
 for(char const*arg:{""," ","x","7x","-1","4294967303","18446744073709551616"}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(!Command(CaseAreaCastActionExactId::Selected::HandleCastActionCommand,h,arg)&&!h.databaseReached&&!p.writes,"Malformed identifier reached native service boundary");++cases;}
 for(uint32 id:{0u,7u,UINT32_MAX}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(Command(CaseAreaCastActionExactId::Selected::HandleCastActionCommand,h,std::to_string(id).c_str())&&h.databaseReached==1&&h.observed==id,"Native identifier width changed before downstream admission");++cases;}
#endif

#ifdef REPAIR_AREATRIGGEREXACTID
 for(char const*arg:{""," ","x","7x","-1","4294967303","18446744073709551616"}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(!Command(CaseAreaTriggerExactId::Selected::HandleAreaTriggerCommand,h,arg)&&!h.databaseReached&&!p.writes,"Malformed identifier reached native service boundary");++cases;}
 for(uint32 id:{0u,7u,UINT32_MAX}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(Command(CaseAreaTriggerExactId::Selected::HandleAreaTriggerCommand,h,std::to_string(id).c_str())&&h.databaseReached==1&&h.observed==id,"Native identifier width changed before downstream admission");++cases;}
#endif

#ifdef REPAIR_REWARDPACKEXACTID
 for(char const*arg:{""," ","x","7x","-1","4294967303","18446744073709551616"}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(!Command(CaseRewardPackExactId::Selected::HandlePvpRewardCommand,h,arg)&&!h.databaseReached&&!p.writes,"Malformed identifier reached native service boundary");++cases;}
 for(uint32 id:{0u,7u,UINT32_MAX}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(Command(CaseRewardPackExactId::Selected::HandlePvpRewardCommand,h,std::to_string(id).c_str())&&h.databaseReached==1&&h.observed==id,"Native identifier width changed before downstream admission");++cases;}
#endif

#ifdef REPAIR_GRAVEYARDEXACTID
 for(char const*arg:{""," ","x","7x","-1","4294967303","18446744073709551616"}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(!Command(CaseGraveyardExactId::Selected::HandleLinkGraveCommand,h,arg)&&!h.databaseReached&&!p.writes,"Malformed identifier reached native service boundary");++cases;}
 for(uint32 id:{0u,7u,UINT32_MAX}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(Command(CaseGraveyardExactId::Selected::HandleLinkGraveCommand,h,std::to_string(id).c_str())&&h.databaseReached==1&&h.observed==id,"Native identifier width changed before downstream admission");++cases;}
#endif

#ifdef REPAIR_TRANSFERREASONNATIVEDOMAIN
 for(char const*arg:{""," ","x","1x","-1","27","257","4294967296"}){ChatHandler h;Require(!Command(CaseTransferReasonNativeDomain::Selected::HandleTransferAbortReasonCommand,h,arg)&&!h.databaseReached,"Wrapped/outside-native transfer reason sent");++cases;}
 for(uint32 id:{0u,1u,26u}){ChatHandler h;Require(Command(CaseTransferReasonNativeDomain::Selected::HandleTransferAbortReasonCommand,h,std::to_string(id).c_str())&&h.observed==id,"Native transfer diagnostic reason lost");++cases;}
#endif
#ifdef REPAIR_GLOBALSOUNDEXACTID
 for(char const*arg:{""," ","x","7x","-1","4294967303"}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(!Command(CaseGlobalSoundExactId::Selected::HandlePlayAllCommand,h,arg)&&!p.writes,"Bad sound sent");++cases;}
 for(uint32 id:{0u,7u,UINT32_MAX}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(Command(CaseGlobalSoundExactId::Selected::HandlePlayAllCommand,h,std::to_string(id).c_str())&&p.writes==1&&p.lastSound==id,"Native sound id/target changed");++cases;}
#endif

#ifdef REPAIR_SHOWAREAEXACTIDBITS
 for(char const*arg:{""," ","x","31x","-1","4294967327","10240"}){Player p;ChatHandler h;h.target=&p;Require(!Command(CaseShowAreaExactIdBits::Selected::HandleShowAreaCommand,h,arg)&&!p.writes,"Bad area changed explored bit");++cases;}
 for(uint32 id:{0u,1u,31u,32u,10239u}){Player p;ChatHandler h;h.target=&p;uint16 field=PLAYER_FIELD_EXPLORED_ZONES+uint16(id/32);uint32 original=0x5a5a5a5a;p.flags[field]=original;Require(Command(CaseShowAreaExactIdBits::Selected::HandleShowAreaCommand,h,std::to_string(id).c_str()),"Native area id rejected");uint32 expected=original|(uint32(1)<<(id%32));Require(p.flags[field]==expected,"Wrong area bit modified");Require(Command(CaseShowAreaExactIdBits::Selected::HandleShowAreaCommand,h,std::to_string(id).c_str())&&p.flags[field]==expected,"Repeated area command toggles visibility");++cases;}
#endif

#ifdef REPAIR_HIDEAREAEXACTIDBITS
 for(char const*arg:{""," ","x","31x","-1","4294967327","10240"}){Player p;ChatHandler h;h.target=&p;Require(!Command(CaseHideAreaExactIdBits::Selected::HandleHideAreaCommand,h,arg)&&!p.writes,"Bad area changed explored bit");++cases;}
 for(uint32 id:{0u,1u,31u,32u,10239u}){Player p;ChatHandler h;h.target=&p;uint16 field=PLAYER_FIELD_EXPLORED_ZONES+uint16(id/32);uint32 original=0x5a5a5a5a;p.flags[field]=original;Require(Command(CaseHideAreaExactIdBits::Selected::HandleHideAreaCommand,h,std::to_string(id).c_str()),"Native area id rejected");uint32 expected=original&~(uint32(1)<<(id%32));Require(p.flags[field]==expected,"Wrong area bit modified");Require(Command(CaseHideAreaExactIdBits::Selected::HandleHideAreaCommand,h,std::to_string(id).c_str())&&p.flags[field]==expected,"Repeated area command toggles visibility");++cases;}
#endif

// MORE_CASES
std::cout<<"PASS "<<cases<<" selected production command cases;controlled endpoints;not client traversal\n";return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
