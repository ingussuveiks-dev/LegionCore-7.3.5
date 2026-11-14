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
#include <list>
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
struct Session;struct Player;struct Creature;struct GameObject;struct CharTitlesEntry;struct AchievementEntry{uint32 ID;};struct Map;
struct ItemPosCount{uint16 pos=1;};using ItemPosCountVec=std::vector<ItemPosCount>;
struct AuraEffect{int32 amount=0;uint32 writes=0;void SetAmount(int32 value){++writes;amount=value;}};
struct ItemTemplate{uint32 entry=7,stack=1,set=7;int32 maxCount=0;uint32 GetId()const{return entry;}uint32 GetItemSet()const{return set;}uint32 GetMaxStackSize()const{return stack;}int32 GetMaxCount()const{return maxCount;}};using ItemTemplateContainer=std::map<uint32,ItemTemplate>;
struct Item{bool bound=true;void SetBinding(bool value){bound=value;}uint64 xp=100;uint32 writes=0,states=0,reads=0;Player*owner=nullptr;std::map<uint16,uint32>fields;ItemTemplate proto;uint8 bag=0,slot=1;bool protectedRemove=false,removed=false;uint64 GetUInt64Value(uint32 field){Require(field==ITEM_FIELD_ARTIFACT_XP,"Wrong artifact field read");return xp;}void SetUInt64Value(uint32 field,uint64 value){Require(field==ITEM_FIELD_ARTIFACT_XP,"Wrong artifact field write");++writes;xp=value;}void SetState(uint32 state,Player*player){Require(state==ITEM_CHANGED,"Wrong item state");++states;owner=player;}uint16 GetValuesCount(){return ITEM_END;}uint32 GetUInt32Value(uint16 field){Require(field<ITEM_END,"Bad item read index");++reads;return fields[field];}void SetUInt32Value(uint16 field,uint32 value){Require(field<ITEM_END,"Bad item write index");++writes;fields[field]=value;}uint8 GetBagSlot(){return bag;}uint8 GetSlot(){return slot;}ItemTemplate const*GetTemplate(){return &proto;}};
struct PhaseManager{uint32 writes=0,mask=0;void SetCustomPhase(uint32 value){++writes;mask=value;}};
struct WorldPacket{std::vector<int16>ids;};
namespace WorldPackets{namespace Misc{struct StreamingMovie{std::vector<int16>MovieIDs;WorldPacket packet;WorldPacket const*Write(){packet.ids=MovieIDs;return &packet;}};}}
struct MotionMaster{uint32 initializes=0,jumps=0;std::vector<float>jumpArgs;void Initialize(){++initializes;}void MoveJump(float x,float y,float z,float xy,float vz){++jumps;jumpArgs={x,y,z,xy,vz};}};
struct Unit {
 bool combat=false;uint32 stops=0;void CombatStop(){++stops;combat=false;}bool isInCombat(){return combat;}virtual bool IsAlive(){return true;}AuraEffect aura;uint32 effectReads=0,lastEffect=0;bool auraPresent=true;AuraEffect*GetAuraEffect(uint32,uint8 index){++effectReads;lastEffect=index;return auraPresent?&aura:nullptr;}
 virtual uint32 GetEntry(){return 0;}
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
 bool canStore=true;uint32 storeCalls=0,notices=0,failStoreCall=0;std::vector<std::unique_ptr<Item>>storedItems;
 InventoryResult CanStoreNewItem(uint8,uint8,ItemPosCountVec&dest,uint32,uint32){dest={{1}};return canStore?EQUIP_ERR_OK:EQUIP_ERR_BAG_FULL;}
 Item*StoreNewItem(ItemPosCountVec const&,uint32 id,bool update){Require(update,"Native inventory update flag changed");++storeCalls;if(storeCalls==failStoreCall)return nullptr;auto item=std::make_unique<Item>();item->proto.entry=id;item->owner=this;storedItems.push_back(std::move(item));return storedItems.back().get();}
 void SendNewItem(Item*item,uint32 count,bool,bool){Require(item&&count==1,"Invalid item grant notification");++notices;}
 void SendEquipError(InventoryResult result,Item*,Item*,uint32){Require(result==EQUIP_ERR_BAG_FULL,"Wrong native inventory error");++errorChannel;}
 Map*GetMap();bool mapAvailable=true;uint32 removedItems=0,removedId=0,removedCount=0;void DestroyItemCount(uint32 id,uint32 count,bool update,bool unequip){Require(update&&!unequip,"Removal native flags changed");++removedItems;removedId=id;removedCount=count;}
 struct HostileRefs{uint32 deletes=0;void deleteReferences(){++deletes;}}refs;HostileRefs&getHostileRefManager(){return refs;}
 bool goSummonAccepted=true;uint32 goSummons=0,goEntry=0,goTimer=0;GameObject*SummonGameObject(uint32,float,float,float,float,float,float,float,float,uint32);
 bool summonAccepted=true;uint32 summons=0,summonEntry=0;Creature*SummonCreature(uint32,Unit const&,TempSummonType,uint32);
 uint8 level=110;static uint8 dbLevel;uint8 getLevel(){return level;}void GiveLevel(uint8 value){++writes;level=value;}static uint8 GetLevelFromDB(ObjectGuid){return dbLevel;}uint64 GetGuildId(){return 7;}static uint64 GetGuildIdFromDB(ObjectGuid){return 7;}

 uint32 swaps=0;uint16 skillId=0,skillLevel=0,skillMax=0;bool positionsValid=true;
 bool IsValidPos(uint8,uint8,bool){return positionsValid;}void SwapItem(uint16,uint16){++swaps;}
 uint16 GetSkillValue(uint32){return 1;}uint16 GetPureMaxSkillValue(uint32){return 100;}uint16 GetSkillStep(uint16){return 2;}void SetSkill(uint16 id,uint16,uint16 level,uint16 max){++writes;skillId=id;skillLevel=level;skillMax=max;}
 void SetTitle(CharTitlesEntry const*,bool=false);char const*GetName(){return "Controlled";}uint64 GetGUIDLow(){return 7;}
 uint32 achievements=0,kills=0;std::vector<uint32>criteria;
 void CompletedAchievement(AchievementEntry const*entry){++writes;achievements=entry->ID;}
 void KilledMonsterCredit(uint32 id){++kills;lastTarget=id;}
 Unit*criteriaActor=nullptr;void UpdateAchievementCriteria(CriteriaTypes type,uint32 a,uint32 b,uint32 c,Unit*actor,bool ignore){Require(ignore,"Native criteria ignore-group mode changed");++writes;criteria={uint32(type),a,b,c};criteriaActor=actor;}

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
uint8 Player::dbLevel=110;
#include "NativeRating.inc"
#include "NativeSpellPower.inc"

namespace WorldPackets{namespace Misc{struct PhaseShiftDataPhase{uint16 Id;explicit PhaseShiftDataPhase(uint16 id):Id(id){}};}}
struct Session{Player*player=nullptr;uint32 account=1;uint32 phasePackets=0;std::vector<uint16>phaseValues;void SendSetPhaseShift(std::vector<WorldPackets::Misc::PhaseShiftDataPhase>const&p,std::vector<uint16>const&t,std::vector<uint16>const&w,std::vector<uint16>const&s){++phasePackets;phaseValues=t;for(auto v:p)phaseValues.push_back(v.Id);phaseValues.insert(phaseValues.end(),w.begin(),w.end());phaseValues.insert(phaseValues.end(),s.begin(),s.end());}
 Player*GetPlayer(){return player;}uint32 GetAccountId(){return account;}};
struct CreatureTemplate{uint32 faction=1;float scale=1.5f;std::string Name[1]={"Controlled NPC"};};
struct CreatureData{float personalSize=3;};
struct CreatureAI{uint32 writes=0,key=0,value=0,evades=0;void SetData(uint32 a,uint32 b){++writes;key=a;value=b;}void EnterEvadeMode(){++evades;}};

struct ControlledFormation{uint64 GetId(){return (uint64(1)<<32)+7;}};
struct Creature:Unit{bool vendor=true,formation=false;ControlledFormation group;bool isVendor(){return vendor;}ControlledFormation*GetFormation(){return formation?&group:nullptr;}uint32 entry=0,path=77,faction=1,level=1,saves=0,nativeDisplay=0,outfit=0,moveType=0,deathCalls=0,respawns=0,delay=0;bool pet=false,alive=true,hasAI=true;float radius=0;CreatureTemplate templateData;CreatureAI ai;MotionMaster motion;
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
Creature controlledSummon;
Creature*Player::SummonCreature(uint32 entry,Unit const&pose,TempSummonType type,uint32 time){Require(&pose==this&&type==TEMPSUMMON_CORPSE_DESPAWN&&time==120,"Native temporary summon pose/lifecycle changed");++summons;summonEntry=entry;return summonAccepted?&controlledSummon:nullptr;}
struct WorldConfig{uint32 amount=100,amountWrites=0,securityWrites=0,securityLoads=0;AccountTypes security=SEC_PLAYER;void SetPlayerAmountLimit(uint32 value){++amountWrites;amount=value;}void SetPlayerSecurityLimit(AccountTypes value){++securityWrites;security=value;}uint32 GetPlayerAmountLimit(){return amount;}AccountTypes GetPlayerSecurityLimit(){return security;}void LoadDBAllowedSecurityLevel(){++securityLoads;}
 uint32 shutdowns=0,lastTime=0;uint8 lastMask=0,lastExit=0;bool getBoolConfig(uint32){return true;}void ShutdownServ(uint32 time,uint32 mask,uint8 code){++shutdowns;lastTime=time;lastMask=mask;lastExit=code;}uint32 getIntConfig(WorldIntConfigs key){Require(key==CONFIG_MAX_PLAYER_LEVEL,"Wrong level config");return 120;}}worldConfig;auto sWorld=&worldConfig;
struct UpdateClock{uint32 writes=0,last=0;void SetRecordUpdateTimeInterval(uint32 value){++writes;last=value;}}sWorldUpdateTime;
struct SkillLineEntry{FactionName DisplayName[1];};struct SkillStore{SkillLineEntry row;SkillLineEntry const*LookupEntry(uint32){return &row;}}sSkillLineStore;
struct AreaTableEntry{uint32 ID;};struct AreaStore{AreaTableEntry row;AreaTableEntry const*LookupEntry(uint32 id){row.ID=id;return &row;}}sAreaTableStore;
struct GameObjectTemplate{uint32 type=3,displayId=99;uint32 GetLootId()const{return 7;}std::string name="Controlled GO";float size=1.5f;};
struct GameObjectData{uint32 id=99;float personalSize=3;};
struct GameObject:Unit{uint32 despawns=0,customCalls=0,customAnim=0,byteOffset=0,byteValue=0;int32 healthDelta=0;Unit*healthActor=nullptr;void SetByteValue(uint32 field,uint8 offset,uint8 value){Require(field==GAMEOBJECT_FIELD_BYTES_1&&offset<4,"Wrong GO byte field");++writes;byteOffset=offset;byteValue=value;}void SendCustomAnim(uint32 id){++customCalls;customAnim=id;}void SendObjectDeSpawnAnim(ObjectGuid){++despawns;}void ModifyHealth(int32 delta,Unit*actor){++writes;healthDelta=delta;healthActor=actor;}GameObjectTemplate info;uint32 saves=0,refreshes=0,destroys=0,visibility=0;float rz=0,ry=0,rx=0;ObjectGuid GetGUID(){return {7};}uint32 GetEntry(){return 99;}GameObjectTemplate const*GetGOInfo(){return &info;}void Relocate(float x,float y,float z){++writes;position.x=x;position.y=y;position.z=z;}void Relocate(float x,float y,float z,float o){Relocate(x,y,z);position.o=o;}void RelocateStationaryPosition(float,float,float,float){++writes;}void SetWorldRotationAngles(float z,float y,float x){++writes;rz=z;ry=y;rx=x;}void DestroyForNearbyPlayers(){++destroys;}void UpdateObjectVisibility(){++visibility;}void SaveToDB(){++saves;}void Refresh(){++refreshes;}};
struct MapManager{static bool IsValidMapCoord(uint32 id,float x,float y,float z){return id==1&&Trinity::IsValidMapCoord(x,y,z);}};
struct ObjectManager{ItemTemplateContainer itemSet{{7,{7,1,7,0}},{8,{8,1,7,0}}};ItemTemplateContainer const*GetItemTemplateStore(){return &itemSet;}ItemTemplate item;ItemTemplate const*GetItemTemplate(uint32 id){return id==7?&item:nullptr;}uint64 linkedFrom=0,linkedTo=0;uint32 links=0;bool linkAccepted=true;bool SetCreatureLinkedRespawn(uint64 const&from,uint64 const&to){++links;linkedFrom=from;linkedTo=to;return linkAccepted;}uint32 GetDBCLocaleIndex(){return 0;}CreatureTemplate data;CreatureData spawn;GameObjectTemplate goInfo;GameObjectData goData;uint32 GetXPForLevel(uint32 level){return level*100;}CreatureTemplate const*GetCreatureTemplate(uint32){return &data;}CreatureData&NewOrExistCreatureData(uint64 id){Require(id==7,"Wrong spawn");return spawn;}GameObjectData const*GetGOData(uint32 id){return id==7?&goData:nullptr;}GameObjectTemplate const*GetGameObjectTemplate(uint32 id){return id==99?&goInfo:nullptr;}GameObjectData&NewGOData(uint64 id){Require(id==7,"Wrong GO spawn");return goData;}}objectManager;auto sObjectMgr=&objectManager;
struct WorldDatabasePreparedStatement{uint32 id=0;std::map<uint32,uint64>values;std::map<uint32,float>floats;void setString(uint32,std::string const&){}void setUInt16(uint32 i,uint16 v){values[i]=v;}void setUInt32(uint32 i,uint32 v){values[i]=v;}void setUInt64(uint32 i,uint64 v){values[i]=v;}void setUInt8(uint32 i,uint8 v){values[i]=v;}void setFloat(uint32 i,float v){Require(std::isfinite(v),"Nonfinite DB write");floats[i]=v;}};
struct Field{uint64 integer=0;float number=0;uint64 GetUInt64(){return integer;}uint32 GetUInt32(){return uint32(integer);}uint16 GetUInt16(){return uint16(integer);}float GetFloat(){return number;}};
struct QueryRow{Field fields[6]{{(uint64(1)<<32)+7,0},{99,0},{0,1},{0,2},{0,3},{1,0}};Field*Fetch(){return fields;}Field&operator[](size_t index){return fields[index];}bool NextRow(){return false;}};
using PreparedQueryResult=std::shared_ptr<QueryRow>;
using QueryResult=PreparedQueryResult;
struct WorldService{void EscapeString(std::string&){}struct Aggregate{bool present=false;uint32 value=0;};std::map<uint32,Aggregate>aggregates;bool rawHasRow=true;uint32 rawFirst=0,rawQueries=0;std::vector<double>rawTail;uint32 rawWrites=0;uint64 rawId=0;template<class...T>void PExecute(char const*,uint64 id,T...args){++rawWrites;rawId=id;rawTail={double(args)...};}QueryResult PQuery(char const*,uint32 id){++rawQueries;rawId=id;if(!rawHasRow)return nullptr;auto result=std::make_shared<QueryRow>();result->fields[0]={rawFirst,0};return result;}WorldDatabasePreparedStatement pending,lastQuery;uint32 queries=0;bool hasRow=false;std::vector<WorldDatabasePreparedStatement>writes;WorldDatabasePreparedStatement*GetPreparedStatement(uint32 id){pending={id,{},{}};return &pending;}void Execute(WorldDatabasePreparedStatement*row){writes.push_back(*row);}PreparedQueryResult Query(WorldDatabasePreparedStatement*row){++queries;lastQuery=*row;auto it=aggregates.find(row->id);if(it!=aggregates.end()){if(!it->second.present)return nullptr;auto result=std::make_shared<QueryRow>();result->fields[0]={it->second.value,0};return result;}return hasRow?std::make_shared<QueryRow>():nullptr;}}WorldDatabase;
struct ChatHandler {
 int64 signedValue=0;
 bool offline=false,targetAvailable=true;std::vector<uint64>captured;float scalar=0;
 void extractOptFirstArg(char*,char**,char**);std::string playerLink(std::string const&s){return s;}
 bool extractPlayerTarget(char*args,Player**p,ObjectGuid*guid=nullptr,std::string*name=nullptr){if(args)strtok(args," ");if(!targetAvailable||(!target&&!offline))return false;*p=offline?nullptr:target;if(guid)*guid={7,HighGuid::Player};if(name)*name="Controlled";return true;}

 uint64 observed=0;
 GameObject*go=nullptr;GameObject*GetObjectGlobalyWithGuidOrNearWithDbGuid(uint32 id,uint32 entry){Require(id==7&&entry==99,"Wrong GO lookup");return go;}
 Player*target=nullptr;Unit*unit=nullptr;Creature*creature=nullptr;Session*session=nullptr;bool lower=false,error=false,report=false;uint32 messages=0,databaseReached=0;
 ChatHandler()=default;explicit ChatHandler(Player*){}
 WorldObject*getSelectedObject(){return unit?unit:target;}
 Session*GetSession(){return session;}Player*getSelectedPlayer(){return target;}Unit*getSelectedUnit(){return unit?unit:target;}Creature*getSelectedCreature(){return creature;}
 bool HasLowerSecurityAccount(Player*,uint32,bool strong){Require(strong,"Account addon strong authority gate lost");return lower;}
 template<class...T>void SendGlobalGMSysMessage(T...){++messages;}char const*GetTrinityString(uint32){return "active";}
 bool HasLowerSecurity(Player*,ObjectGuid,bool=false){return lower;}bool needReportToTarget(Player*){return report;}
 std::string GetNameLink(Player* =nullptr){return "fixture";}
 template<class...T>void PSendSysMessage(T...){++messages;}template<class...T>void SendSysMessage(T...){++messages;}
 void SetSentErrorMessage(bool b){error=b;}
 char*extractKeyFromLink(char*,char const*,char** =nullptr);
 char*extractKeyFromLink(char*,char const*const*,int*,char** =nullptr);
 uint32 extractSpellIdFromLink(char*);char*extractQuotedArg(char*);

};
#include "NativeOptFirst.inc"
struct Guild{std::string name="Controlled";std::string const&GetName(){return name;}uint32 writes=0;uint8 rank=3;bool accepts=true;bool ChangeMemberRank(ObjectGuid,uint8 value){if(!accepts)return false;++writes;rank=value;return true;}};
struct GuildManager{Guild guild;bool exists=true;Guild*GetGuildById(uint64 id){return exists&&id==7?&guild:nullptr;}}guildManager;auto sGuildMgr=&guildManager;
struct ObjectMgr{static uint32 GetPlayerAccountIdByGUID(ObjectGuid){return 7;}}; struct WaypointManager{std::vector<uint32>reloads;void ReloadPath(uint32 id){reloads.push_back(id);}}waypointManager;auto sWaypointMgr=&waypointManager;
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
using CharacterDatabasePreparedStatement=WorldDatabasePreparedStatement;
struct CharacterService{CharacterDatabasePreparedStatement pending,last;CharacterDatabasePreparedStatement*GetPreparedStatement(uint32 id){pending={id,{},{}};return &pending;}void Execute(CharacterDatabasePreparedStatement*p){last=*p;++writes;}uint32 writes=0;uint64 lastId=0;template<class...T>void PExecute(char const*,uint32 id,T...){++writes;lastId=id;}}CharacterDatabase;
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
GameObject controlledGo; GameObject*Player::SummonGameObject(uint32 id,float x,float y,float z,float angle,float r0,float r1,float r2,float r3,uint32 time){Require(x==GetPositionX()&&y==GetPositionY()&&z==GetPositionZ()&&angle==GetOrientation()&&r0==0&&r1==0&&std::abs(r2-std::sin(angle/2))<.00001f&&std::abs(r3-std::cos(angle/2))<.00001f,"Native GO pose/rotation changed");++goSummons;goEntry=id;goTimer=time;return goSummonAccepted?&controlledGo:nullptr;}
struct ConfigManager{int32 GetIntDefault(char const*,int32 fallback){return fallback;}}configManager;auto sConfigMgr=&configManager;
namespace lfg{using LfgDungeonSet=std::set<uint32>;}
struct LfgManager{bool enabled=true;uint32 joins=0,optionWrites=0,dumps=0,options=3;uint8 roles=0;bool verbose=false;lfg::LfgDungeonSet dungeons;bool isOptionEnabled(uint32){return enabled;}void JoinLfg(Player*,uint8 value,lfg::LfgDungeonSet const&ids){++joins;roles=value;dungeons=ids;}void SetOptions(uint32 value){++optionWrites;options=value;}uint32 GetOptions(){return options;}std::string DumpQueueInfo(bool value){++dumps;verbose=value;return "Controlled queue";}}lfgManager;auto sLFGMgr=&lfgManager;
struct InstanceMap{uint32 GetInstanceId(){return 7;}};
struct Map{InstanceMap instance;bool inInstance=true;InstanceMap*ToInstanceMap(){return inInstance?&instance:nullptr;}}controlledMap;
Map*Player::GetMap(){return mapAvailable?&controlledMap:nullptr;}
struct Scenario{uint8 count=3,last=0;uint32 writes=0;uint8 GetStepCount(bool bonus){Require(bonus,"Bonus scenario steps excluded");return count;}void SetCurrentStep(uint8 step){Require(step<count,"Bad step reached native error boundary");++writes;last=step;}};
struct ScenarioManager{bool exists=true;Scenario scenario;Scenario*GetScenario(uint32 id){return exists&&id==7?&scenario:nullptr;}}scenarioManager;auto sScenarioMgr=&scenarioManager;
struct Accessor{Player*named=nullptr;Player*FindPlayerByName(std::string const&){return named;}}accessor;auto sObjectAccessor=&accessor;
struct TalentEntry{uint32 SpellID;};struct GlyphPropertiesEntry{uint32 SpellID;};
struct TalentStore{TalentEntry row{123};TalentEntry const*LookupEntry(uint32 id){return id==7?&row:nullptr;}}sTalentStore;
struct GlyphStore{GlyphPropertiesEntry row{456};GlyphPropertiesEntry const*LookupEntry(uint32 id){return id==8?&row:nullptr;}}sGlyphPropertiesStore;
#include "NativeChatLinks.inc"
#include "NativeItemRemoval.inc"
#include "Selected.inc"
template<class Fn>static bool Command(Fn fn,ChatHandler&h,char const*args){std::string text=args;return fn(&h,text.data());}
int main(){try{unsigned cases=0;
#ifdef COUNTERFACTUAL_QUESTCOMPLETEEXACTID
 {Player p;ChatHandler h;h.target=&p;Require(Command(CaseQuestCompleteExactId::Selected::HandleQuestComplete,h,"7x")&&h.observed==7,"Pinned quest alias not reproduced");std::cout<<"COUNTERFACTUAL original quest7x selects quest7 before native catalog/progression;no live quest mutation\n";return 0;}
#endif
#ifdef COUNTERFACTUAL_NPCRESPAWNLINKFULLGUID
 {Creature c;c.dbGuid=(uint64(1)<<32)+9;ChatHandler h;h.creature=&c;objectManager.links=0;objectManager.linkAccepted=true;Require(Command(CaseNpcRespawnLinkFullGuid::Selected::HandleNpcSetLinkCommand,h,"4294967303")&&objectManager.links==1&&objectManager.linkedTo==7,"Pinned linked GUID truncation not reproduced");std::cout<<"COUNTERFACTUAL original64-bit respawn target narrows to32-bit7;controlled setter only\n";return 0;}
#endif
#ifdef COUNTERFACTUAL_SCRIPTWAYPOINTAPPENDCHECKEDORDINAL
 {Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;WorldDatabase.rawHasRow=true;WorldDatabase.rawFirst=UINT32_MAX;WorldDatabase.rawWrites=0;Require(Command(CaseScriptWaypointAppendCheckedOrdinal::Selected::HandleScriptWpAddCommand,h,"7")&&WorldDatabase.rawWrites==1&&WorldDatabase.rawTail[0]==0,"Pinned waypoint counter wrap not reproduced");std::cout<<"COUNTERFACTUAL original next waypoint ordinal wraps tozero;controlled SQL endpoint only\n";return 0;}
#endif
#ifdef COUNTERFACTUAL_EXPLORESELECTEDMASKREPLACEMENT
 {Player selected,caller;caller.flags[PLAYER_FIELD_EXPLORED_ZONES]=5;selected.flags[PLAYER_FIELD_EXPLORED_ZONES]=7;Session ses{&caller,1};ChatHandler h;h.target=&selected;h.session=&ses;Require(Command(CaseExploreSelectedMaskReplacement::Selected::HandleExploreCheatCommand,h,"0")&&caller.writes==PLAYER_EXPLORED_ZONES_SIZE&&!selected.writes&&caller.flags[PLAYER_FIELD_EXPLORED_ZONES]==5&&selected.flags[PLAYER_FIELD_EXPLORED_ZONES]==7,"Pinned exploration target/clear failure not reproduced");std::cout<<"COUNTERFACTUAL original clear touches executor instead of selected player and SetFlag0 leaves bits unchanged;controlled fields only\n";return 0;}
#endif
#ifdef COUNTERFACTUAL_SPELLLINKNATIVENUMERICPAYLOAD
 {ChatHandler h;std::string input="123x";Require(h.extractSpellIdFromLink(input.data())==123,"Pinned raw spell alias not reproduced");input="|cffffffff|Htalent:8|h[Missing Talent]|h|r";Require(h.extractSpellIdFromLink(input.data())==8,"Pinned missing talent fallthrough not reproduced");std::cout<<"COUNTERFACTUAL original spell123x aliases123 and missing talent8 falls through as spell8;native link-key parsers,controlled stores\n";return 0;}
#endif

#ifdef REPAIR_QUESTAUTOCOMPLETEEXACTID
 for(char const*arg:{""," ","x","7x","-1","4294967303","18446744073709551616"}){Player p;ChatHandler h;h.target=&p;Require(!Command(CaseQuestAutocompleteExactId::Selected::HandleQuestAutocomplete,h,arg)&&!h.databaseReached&&!p.writes,"Malformed quest selector reaches native effect");++cases;}
 for(uint32 id:{0u,7u,UINT32_MAX}){Player p;ChatHandler h;h.target=&p;Require(Command(CaseQuestAutocompleteExactId::Selected::HandleQuestAutocomplete,h,std::to_string(id).c_str())&&h.databaseReached==1&&h.observed==id,"Quest id changed before native catalog/status admission");++cases;}
 {ChatHandler h;Require(!Command(CaseQuestAutocompleteExactId::Selected::HandleQuestAutocomplete,h,"7")&&!h.databaseReached,"Quest command accepted missing player");++cases;}
#endif

#ifdef REPAIR_QUESTADDEXACTID
 for(char const*arg:{""," ","x","7x","-1","4294967303","18446744073709551616"}){Player p;ChatHandler h;h.target=&p;Require(!Command(CaseQuestAddExactId::Selected::HandleQuestAdd,h,arg)&&!h.databaseReached&&!p.writes,"Malformed quest selector reaches native effect");++cases;}
 for(uint32 id:{0u,7u,UINT32_MAX}){Player p;ChatHandler h;h.target=&p;Require(Command(CaseQuestAddExactId::Selected::HandleQuestAdd,h,std::to_string(id).c_str())&&h.databaseReached==1&&h.observed==id,"Quest id changed before native catalog/status admission");++cases;}
 {ChatHandler h;Require(!Command(CaseQuestAddExactId::Selected::HandleQuestAdd,h,"7")&&!h.databaseReached,"Quest command accepted missing player");++cases;}
#endif

#ifdef REPAIR_QUESTREMOVEEXACTID
 for(char const*arg:{""," ","x","7x","-1","4294967303","18446744073709551616"}){Player p;ChatHandler h;h.target=&p;Require(!Command(CaseQuestRemoveExactId::Selected::HandleQuestRemove,h,arg)&&!h.databaseReached&&!p.writes,"Malformed quest selector reaches native effect");++cases;}
 for(uint32 id:{0u,7u,UINT32_MAX}){Player p;ChatHandler h;h.target=&p;Require(Command(CaseQuestRemoveExactId::Selected::HandleQuestRemove,h,std::to_string(id).c_str())&&h.databaseReached==1&&h.observed==id,"Quest id changed before native catalog/status admission");++cases;}
 {ChatHandler h;Require(!Command(CaseQuestRemoveExactId::Selected::HandleQuestRemove,h,"7")&&!h.databaseReached,"Quest command accepted missing player");++cases;}
#endif

#ifdef REPAIR_QUESTCOMPLETEEXACTID
 for(char const*arg:{""," ","x","7x","-1","4294967303","18446744073709551616"}){Player p;ChatHandler h;h.target=&p;Require(!Command(CaseQuestCompleteExactId::Selected::HandleQuestComplete,h,arg)&&!h.databaseReached&&!p.writes,"Malformed quest selector reaches native effect");++cases;}
 for(uint32 id:{0u,7u,UINT32_MAX}){Player p;ChatHandler h;h.target=&p;Require(Command(CaseQuestCompleteExactId::Selected::HandleQuestComplete,h,std::to_string(id).c_str())&&h.databaseReached==1&&h.observed==id,"Quest id changed before native catalog/status admission");++cases;}
 {ChatHandler h;Require(!Command(CaseQuestCompleteExactId::Selected::HandleQuestComplete,h,"7")&&!h.databaseReached,"Quest command accepted missing player");++cases;}
#endif

#ifdef REPAIR_QUESTREWARDEXACTID
 for(char const*arg:{""," ","x","7x","-1","4294967303","18446744073709551616"}){Player p;ChatHandler h;h.target=&p;Require(!Command(CaseQuestRewardExactId::Selected::HandleQuestReward,h,arg)&&!h.databaseReached&&!p.writes,"Malformed quest selector reaches native effect");++cases;}
 for(uint32 id:{0u,7u,UINT32_MAX}){Player p;ChatHandler h;h.target=&p;Require(Command(CaseQuestRewardExactId::Selected::HandleQuestReward,h,std::to_string(id).c_str())&&h.databaseReached==1&&h.observed==id,"Quest id changed before native catalog/status admission");++cases;}
 {ChatHandler h;Require(!Command(CaseQuestRewardExactId::Selected::HandleQuestReward,h,"7")&&!h.databaseReached,"Quest command accepted missing player");++cases;}
#endif

#ifdef REPAIR_QUESTSTATUSEXACTID
 for(char const*arg:{""," ","x","7x","-1","4294967303","18446744073709551616"}){Player p;ChatHandler h;h.target=&p;Require(!Command(CaseQuestStatusExactId::Selected::HandleQuestStatus,h,arg)&&!h.databaseReached&&!p.writes,"Malformed quest selector reaches native effect");++cases;}
 for(uint32 id:{0u,7u,UINT32_MAX}){Player p;ChatHandler h;h.target=&p;Require(Command(CaseQuestStatusExactId::Selected::HandleQuestStatus,h,std::to_string(id).c_str())&&h.databaseReached==1&&h.observed==id,"Quest id changed before native catalog/status admission");++cases;}
 {ChatHandler h;Require(!Command(CaseQuestStatusExactId::Selected::HandleQuestStatus,h,"7")&&!h.databaseReached,"Quest command accepted missing player");++cases;}
#endif

#ifdef REPAIR_CHARACTERLEVELEXACTVALUE
 for(char const*arg:{""," ","1x","-1","0","2147483648","4294967296"})for(bool offline:{false,true}){Player p;Session ses{&p,1};ChatHandler h;h.target=&p;h.session=&ses;h.offline=offline;CharacterDatabase.writes=0;Require(!Command(CaseCharacterLevelExactValue::Selected::HandleCharacterLevelCommand,h,arg)&&!p.writes&&!CharacterDatabase.writes,"Bad absolute level mutated online/offline character");++cases;}
 for(bool offline:{false,true})for(int level:{1,110,255,INT32_MAX}){Player p;Session ses{&p,1};ChatHandler h;h.target=&p;h.session=&ses;h.offline=offline;CharacterDatabase.writes=0;Require(Command(CaseCharacterLevelExactValue::Selected::HandleCharacterLevelCommand,h,std::to_string(level).c_str()),"Valid absolute level rejected");uint32 wanted=uint32(std::min(level,int(MAX_LEVEL)));if(offline)Require(CharacterDatabase.writes==1&&CharacterDatabase.last.values[0]==wanted&&CharacterDatabase.last.values[1]==7&&!p.writes,"Wrong offline level/XP command");else Require(p.level==wanted&&p.writes==2&&!CharacterDatabase.writes,"Wrong online level/XP update");++cases;}
 {Player p;Session ses{&p,1};ChatHandler h;h.target=&p;h.session=&ses;Require(Command(CaseCharacterLevelExactValue::Selected::HandleCharacterLevelCommand,h,"Controlled")&&p.level==110&&p.writes==2,"Name-only current-level reset lost");++cases;}
#endif
#ifdef REPAIR_LEVELDELTACHECKEDSUM
 for(char const*arg:{"1x","2147483648","-2147483649","4294967296"})for(bool offline:{false,true}){Player p;Session ses{&p,1};ChatHandler h;h.target=&p;h.session=&ses;h.offline=offline;CharacterDatabase.writes=0;Require(!Command(CaseLevelDeltaCheckedSum::Selected::HandleLevelUpCommand,h,arg)&&!p.writes&&!CharacterDatabase.writes,"Bad level delta mutated character");++cases;}
 for(bool offline:{false,true})for(int32 delta:{INT32_MIN,-111,-1,0,1,INT32_MAX}){Player p;Session ses{&p,1};ChatHandler h;h.target=&p;h.session=&ses;h.offline=offline;CharacterDatabase.writes=0;uint32 wanted=uint32(std::clamp<int64>(int64(110)+delta,1,STRONG_MAX_LEVEL));Require(Command(CaseLevelDeltaCheckedSum::Selected::HandleLevelUpCommand,h,std::to_string(delta).c_str()),"Full signed native delta rejected");if(offline)Require(CharacterDatabase.last.values[0]==wanted&&CharacterDatabase.writes==1,"Offline level sum wrapped");else Require(p.level==wanted&&p.writes==2,"Online level sum wrapped");++cases;}
 for(char const*arg:{"","Controlled"}){Player p;Session ses{&p,1};ChatHandler h;h.target=&p;h.session=&ses;Require(Command(CaseLevelDeltaCheckedSum::Selected::HandleLevelUpCommand,h,arg)&&p.level==111,"Default level-up/name-only mode lost");++cases;}
#endif
#ifdef REPAIR_GUILDRANKNATIVEWIDTH
 for(char const*arg:{""," ","1x","-1","256","4294967296"}){Player p;ChatHandler h;h.target=&p;guildManager.guild.writes=0;Require(!Command(CaseGuildRankNativeWidth::Selected::HandleGuildRankCommand,h,arg)&&!guildManager.guild.writes,"Wrapped rank reached guild mutation");++cases;}
 for(bool accepted:{false,true})for(uint32 rank:{0u,3u,255u}){Player p;ChatHandler h;h.target=&p;guildManager.guild.accepts=accepted;guildManager.guild.writes=0;Require(Command(CaseGuildRankNativeWidth::Selected::HandleGuildRankCommand,h,std::to_string(rank).c_str())==accepted&&guildManager.guild.writes==uint32(accepted),"Native guild-rank admission/result changed");++cases;}guildManager.guild.accepts=true;
#endif
#ifdef REPAIR_EVENTOBJECTATOMICTEMPLATEREQUEST
 for(char const*arg:{""," ","x","7x","-1","4294967303","1 nan","1 inf","1 1x","1 1 x","1 1 -1","1 1 4294967296","1 1 1 x","1 1 1 4294967296"}){ChatHandler h;Require(!Command(CaseEventObjectAtomicTemplateRequest::Selected::HandleEOAddCommand,h,arg)&&!h.databaseReached,"Invalid event template partially reached persistence");++cases;}
 for(char const*arg:{"1","1 0","1 -1","1 3.4028234e38 4294967295 4294967295","4294967295 0x1p1 0 0"}){ChatHandler h;Require(Command(CaseEventObjectAtomicTemplateRequest::Selected::HandleEOAddCommand,h,arg)&&h.databaseReached==1&&h.captured.size()==3&&std::isfinite(h.scalar),"Native event default/finite grade/ID widths lost before persistence");++cases;}
#endif


#ifdef REPAIR_EVENTOBJECTDELETEFULLGUID
 for(char const*arg:{" ","x","7x","-1","0","18446744073709551616"}){ChatHandler h;WorldDatabase.rawWrites=0;Require(!Command(CaseEventObjectDeleteFullGuid::Selected::HandleEODeleteCommand,h,arg)&&!WorldDatabase.rawWrites,"Malformed EO selector deleted a row");++cases;}
 for(uint64 id:{uint64(7),(uint64(1)<<32)+7,UINT64_MAX}){ChatHandler h;WorldDatabase.rawWrites=0;Require(Command(CaseEventObjectDeleteFullGuid::Selected::HandleEODeleteCommand,h,std::to_string(id).c_str())&&WorldDatabase.rawWrites==1&&WorldDatabase.rawId==id,"EO row GUID truncated before SQL");++cases;}
#endif
#ifdef REPAIR_EVENTOBJECTFINITEQUERYRADIUS
 for(char const*arg:{" ","x","7x","nan","inf","1e1000","1e20"}){ChatHandler h;Require(!Command(CaseEventObjectFiniteQueryRadius::Selected::HandleEONearCommand,h,arg)&&!h.databaseReached,"Invalid/nonfinite squared EO radius entered query");++cases;}
 for(char const*arg:{"","0","-7","10","1.8e19"}){ChatHandler h;Require(Command(CaseEventObjectFiniteQueryRadius::Selected::HandleEONearCommand,h,arg)&&h.databaseReached==1&&std::isfinite(h.scalar*h.scalar),"Native finite radius/default/square domain lost");++cases;}
#endif
#ifdef REPAIR_NPCDELETEFULLCOUNTER
 for(char const*arg:{" ","x","7x","-1","0","1099511627783","18446744073709551616"}){ChatHandler h;Require(!Command(CaseNpcDeleteFullCounter::Selected::HandleNpcDeleteCommand,h,arg)&&!h.databaseReached,"Malformed/aliased NPC GUID reached delete lookup");++cases;}
 for(uint64 id:{uint64(7),(uint64(1)<<32)+7,ObjectGuid::GetMaxCounter(HighGuid::Creature)}){ChatHandler h;Require(Command(CaseNpcDeleteFullCounter::Selected::HandleNpcDeleteCommand,h,std::to_string(id).c_str())&&h.databaseReached==1&&h.observed==id,"Valid40-bit creature counter truncated");++cases;}
#endif
#ifdef REPAIR_VENDORADDATOMICNATIVEWIDTHS
 for(char const*arg:{""," ","x","7x","-1","0","4294967303","7 x","7 -1","7 1 x","7 1 1 x","7 1 1 1 x","7 1 1 1 -1","7 1 1 1 18446744073709551616"}){Creature c;ChatHandler h;h.creature=&c;Require(!Command(CaseVendorAddAtomicNativeWidths::Selected::HandleNpcAddVendorItemCommand,h,arg)&&!h.databaseReached,"Malformed vendor tuple partly reached persistence");++cases;}
 for(char const*arg:{"7","7 0 0 0 0","4294967295 4294967295 4294967295 4294967295 18446744073709551615"}){Creature c;ChatHandler h;h.creature=&c;Require(Command(CaseVendorAddAtomicNativeWidths::Selected::HandleNpcAddVendorItemCommand,h,arg)&&h.databaseReached==1&&h.captured.size()==5,"Native vendor optional widths/defaults lost");++cases;}
 {Creature c;ChatHandler h;h.creature=&c;Require(Command(CaseVendorAddAtomicNativeWidths::Selected::HandleNpcAddVendorItemCommand,h,"7 1 2 3 4294967303")&&h.captured[4]==(uint64(1)<<32)+7,"Vendor money truncated to signed32");++cases;}
#endif
#ifdef REPAIR_VENDORDELETEEXACTITEM
 for(char const*arg:{""," ","x","7x","-1","4294967303"}){Creature c;ChatHandler h;h.creature=&c;Require(!Command(CaseVendorDeleteExactItem::Selected::HandleNpcDeleteVendorItemCommand,h,arg)&&!h.databaseReached,"Malformed item selector reached vendor removal");++cases;}
 for(uint32 id:{0u,7u,UINT32_MAX}){Creature c;ChatHandler h;h.creature=&c;Require(Command(CaseVendorDeleteExactItem::Selected::HandleNpcDeleteVendorItemCommand,h,std::to_string(id).c_str())&&h.observed==id,"Vendor item id changed before native lookup");++cases;}
#endif
#ifdef REPAIR_NPCMOVEMENTEXACTGUIDREQUEST
 for(char const*arg:{""," ","x","7x","-1","18446744073709551616","7 x","7 2147483648","7 -2147483649"}){ChatHandler h;Require(!Command(CaseNpcMovementExactGuidRequest::Selected::HandleNpcAddMoveCommand,h,arg)&&!h.databaseReached,"Malformed movement GUID/request reached DB");++cases;}
 for(uint64 id:{uint64(0),uint64(7),(uint64(1)<<32)+7,UINT64_MAX})for(int32 wait:{INT32_MIN,0,INT32_MAX}){ChatHandler h;Require(Command(CaseNpcMovementExactGuidRequest::Selected::HandleNpcAddMoveCommand,h,(std::to_string(id)+" "+std::to_string(wait)).c_str())&&h.observed==id&&h.signedValue==wait,"Movement GUID/native optional integer lost before native lookup/clamp");++cases;}
#endif
#ifdef REPAIR_NPCWHISPERFULLPLAYERCOUNTER
 for(char const*arg:{""," ","x message","7x message","-1 message","0 message","1099511627783 message","18446744073709551616 message","7"}){ChatHandler h;Require(!Command(CaseNpcWhisperFullPlayerCounter::Selected::HandleNpcWhisperCommand,h,arg)&&!h.databaseReached,"Invalid/aliased whisper recipient reached dispatch");++cases;}
 for(uint64 id:{uint64(7),(uint64(1)<<32)+7,ObjectGuid::GetMaxCounter(HighGuid::Player)}){ChatHandler h;Require(Command(CaseNpcWhisperFullPlayerCounter::Selected::HandleNpcWhisperCommand,h,(std::to_string(id)+" message").c_str())&&h.observed==id,"Full native player counter lost before whisper");++cases;}
#endif
#ifdef REPAIR_NPCTEMPORARYENTRYRECEIPT
 for(char const*arg:{""," ","x","7x","-1","0","4294967303"}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(!Command(CaseNpcTemporaryEntryReceipt::Selected::HandleNpcAddTempSpawnCommand,h,arg)&&!p.summons,"Malformed temp entry summoned another creature");++cases;}
 for(bool accepted:{false,true})for(uint32 id:{7u,UINT32_MAX}){Player p;p.summonAccepted=accepted;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(Command(CaseNpcTemporaryEntryReceipt::Selected::HandleNpcAddTempSpawnCommand,h,std::to_string(id).c_str())==accepted&&p.summons==1&&p.summonEntry==id,"Temp spawn native result/entry/lifecycle changed");++cases;}
#endif
#ifdef REPAIR_NPCFORMATIONNATIVEGUIDWIDTHS
 for(char const*arg:{""," ","x","7x","-1","18446744073709551616"}){Creature c;ChatHandler h;h.creature=&c;Require(!Command(CaseNpcFormationNativeGuidWidths::Selected::HandleNpcAddFormationCommand,h,arg)&&!h.databaseReached,"Malformed formation leader reached persistence");++cases;}
 for(uint64 id:{uint64(0),uint64(7),(uint64(1)<<32)+7,UINT64_MAX}){Creature c;ChatHandler h;h.creature=&c;Require(Command(CaseNpcFormationNativeGuidWidths::Selected::HandleNpcAddFormationCommand,h,std::to_string(id).c_str())&&h.observed==id,"Native formation leader width lost");++cases;}
 {Creature c;c.formation=true;ChatHandler h;h.creature=&c;Require(!Command(CaseNpcFormationNativeGuidWidths::Selected::HandleNpcAddFormationCommand,h,"7")&&!h.databaseReached,"Existing formation overwritten");++cases;}
#endif
#ifdef REPAIR_NPCRESPAWNLINKFULLGUID
 for(char const*arg:{""," ","x","7x","-1","18446744073709551616"}){Creature c;ChatHandler h;h.creature=&c;objectManager.links=0;Require(!Command(CaseNpcRespawnLinkFullGuid::Selected::HandleNpcSetLinkCommand,h,arg)&&!objectManager.links,"Malformed respawn link reached native setter");++cases;}
 for(bool accepted:{false,true})for(uint64 id:{uint64(0),uint64(7),(uint64(1)<<32)+7,UINT64_MAX}){Creature c;c.dbGuid=(uint64(1)<<32)+9;ChatHandler h;h.creature=&c;objectManager.links=0;objectManager.linkAccepted=accepted;Require(Command(CaseNpcRespawnLinkFullGuid::Selected::HandleNpcSetLinkCommand,h,std::to_string(id).c_str())==accepted&&objectManager.links==1&&objectManager.linkedFrom==c.dbGuid&&objectManager.linkedTo==id,"Native respawn link/unlink width/result changed");++cases;}objectManager.linkAccepted=true;
#endif


#ifdef REPAIR_GAMEOBJECTADDTYPEDRESPAWN
 for(char const*arg:{""," ","x","99x","-1","0","4294967395","99 x","99 2147483648","99 -2147483649"}){ChatHandler h;Require(!Command(CaseGameObjectAddTypedRespawn::Selected::HandleGameObjectAddCommand,h,arg)&&!h.databaseReached,"Bad GO entry/respawn reached creation");++cases;}
 for(int32 time:{INT32_MIN,-1,0,1,INT32_MAX}){ChatHandler h;Require(Command(CaseGameObjectAddTypedRespawn::Selected::HandleGameObjectAddCommand,h,("99 "+std::to_string(time)).c_str())&&h.observed==99&&h.signedValue==time,"Native signed respawn time changed before creation");++cases;}
#endif
#ifdef REPAIR_GAMEOBJECTTEMPORARYTIMERRECEIPT
 for(char const*arg:{""," ","x","99x","-1","4294967395","99 x","99 -1","99 2147483648","99 4294967296"}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(!Command(CaseGameObjectTemporaryTimerReceipt::Selected::HandleGameObjectAddTempCommand,h,arg)&&!p.goSummons,"Wrapped temporary respawn/entry reached summon");++cases;}
 for(bool accepted:{false,true})for(uint32 time:{0u,300u,uint32(INT32_MAX)}){Player p;p.goSummonAccepted=accepted;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(Command(CaseGameObjectTemporaryTimerReceipt::Selected::HandleGameObjectAddTempCommand,h,("99 "+std::to_string(time)).c_str())==accepted&&p.goSummons==1&&p.goEntry==99&&p.goTimer==time,"Native GO summon receipt/timer/pose lost");++cases;}
#endif
#ifdef REPAIR_GAMEOBJECTINFOEXPLICITSELECTOR
 for(char const*arg:{" ","x","99x","-1","4294967395"}){ChatHandler h;Require(!Command(CaseGameObjectInfoExplicitSelector::Selected::HandleGameObjectInfoCommand,h,arg)&&!h.messages,"Bad explicit GO selector reported unrelated info");++cases;}
 {ChatHandler h;Require(Command(CaseGameObjectInfoExplicitSelector::Selected::HandleGameObjectInfoCommand,h,"99")&&h.messages==5,"Explicit GO entry ignored");++cases;}
 {GameObject go;ChatHandler h;h.unit=&go;Require(Command(CaseGameObjectInfoExplicitSelector::Selected::HandleGameObjectInfoCommand,h,"")&&h.messages==5,"Selected GO info fallback lost");++cases;}
 {ChatHandler h;Require(!Command(CaseGameObjectInfoExplicitSelector::Selected::HandleGameObjectInfoCommand,h,"")&&!h.messages,"Missing GO info selection accepted");++cases;}
#endif
#ifdef REPAIR_WAYPOINTAPPENDCHECKEDCOUNTERS
 for(char const*arg:{"x","7x","-1","4294967303"}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;WorldDatabase.queries=0;WorldDatabase.writes.clear();Require(!Command(CaseWaypointAppendCheckedCounters::Selected::HandleWpAddCommand,h,arg)&&!WorldDatabase.queries&&WorldDatabase.writes.empty(),"Bad path id entered DB append");++cases;}
 for(uint32 prior:{0u,1u,UINT32_MAX-1,UINT32_MAX}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;WorldDatabase.aggregates={{WORLD_SEL_WAYPOINT_DATA_MAX_POINT,{true,prior}}};WorldDatabase.writes.clear();bool ok=Command(CaseWaypointAppendCheckedCounters::Selected::HandleWpAddCommand,h,"7");Require(ok==(prior!=UINT32_MAX)&&WorldDatabase.writes.size()==size_t(prior!=UINT32_MAX),"Point wrap not rejected atomically");if(ok){auto const&row=WorldDatabase.writes.back();Require(row.id==WORLD_INS_WAYPOINT_DATA&&row.values.at(0)==7&&row.values.at(1)==uint64(prior)+1&&row.floats.at(2)==1&&row.floats.at(3)==2&&row.floats.at(4)==3,"Wrong appended path/point/pose");}++cases;}
 for(uint32 maximum:{0u,44u,UINT32_MAX}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;WorldDatabase.aggregates={{WORLD_SEL_WAYPOINT_DATA_MAX_ID,{true,maximum}},{WORLD_SEL_WAYPOINT_DATA_MAX_POINT,{true,0}}};WorldDatabase.writes.clear();bool ok=Command(CaseWaypointAppendCheckedCounters::Selected::HandleWpAddCommand,h,"");Require(ok==(maximum!=UINT32_MAX)&&WorldDatabase.writes.size()==size_t(maximum!=UINT32_MAX),"Auto path id wrapped");if(ok)Require(WorldDatabase.writes.back().values.at(0)==uint64(maximum)+1,"Auto path id changed");++cases;}
 for(bool pathQuery:{false,true}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;WorldDatabase.aggregates={{WORLD_SEL_WAYPOINT_DATA_MAX_ID,{!pathQuery,0}},{WORLD_SEL_WAYPOINT_DATA_MAX_POINT,{false,0}}};WorldDatabase.writes.clear();Require(!Command(CaseWaypointAppendCheckedCounters::Selected::HandleWpAddCommand,h,"")&&WorldDatabase.writes.empty(),"Failed aggregate query continued to insert");++cases;}
 {Player p;Creature c;c.path=77;Session ses{&p,1};ChatHandler h;h.session=&ses;h.creature=&c;WorldDatabase.aggregates={{WORLD_SEL_WAYPOINT_DATA_MAX_POINT,{true,2}}};WorldDatabase.writes.clear();Require(Command(CaseWaypointAppendCheckedCounters::Selected::HandleWpAddCommand,h,"")&&WorldDatabase.writes.back().values.at(0)==77,"Selected path fallback lost");++cases;}WorldDatabase.aggregates.clear();
#endif
#ifdef REPAIR_WAYPOINTLOADNATIVESELECTORS
 for(char const*arg:{""," ","x","7x","-1","4294967303"}){Creature c;c.entry=7;ChatHandler h;h.creature=&c;Require(!Command(CaseWaypointLoadNativeSelectors::Selected::HandleWpLoadCommand,h,arg)&&!h.databaseReached,"Bad path selector reached addon update");++cases;}
 for(uint32 id:{1u,7u,UINT32_MAX}){Creature c;c.entry=7;c.dbGuid=(uint64(1)<<32)+7;ChatHandler h;h.creature=&c;Require(Command(CaseWaypointLoadNativeSelectors::Selected::HandleWpLoadCommand,h,std::to_string(id).c_str())&&h.observed==id,"Valid native path id altered");++cases;}
#endif
#ifdef REPAIR_WAYPOINTRELOADEXACTID
 for(char const*arg:{""," ","x","7x","-1","0","4294967303"}){ChatHandler h;waypointManager.reloads.clear();Require(!Command(CaseWaypointReloadExactId::Selected::HandleWpReloadCommand,h,arg)&&waypointManager.reloads.empty(),"Bad id reloaded unrelated path");++cases;}
 for(uint32 id:{1u,7u,UINT32_MAX}){ChatHandler h;waypointManager.reloads.clear();Require(Command(CaseWaypointReloadExactId::Selected::HandleWpReloadCommand,h,std::to_string(id).c_str())&&waypointManager.reloads==std::vector<uint32>{id},"Native path reload width lost");++cases;}
#endif
#ifdef REPAIR_SCRIPTWAYPOINTAPPENDCHECKEDORDINAL
 for(char const*arg:{"x","7x","-1","4294967303","7 x","7 -1","7 4294967296"}){ChatHandler h;WorldDatabase.rawWrites=0;WorldDatabase.rawQueries=0;Require(!Command(CaseScriptWaypointAppendCheckedOrdinal::Selected::HandleScriptWpAddCommand,h,arg)&&!WorldDatabase.rawWrites&&!WorldDatabase.rawQueries,"Bad script waypoint tuple entered DB");++cases;}
 for(uint32 prior:{0u,7u,UINT32_MAX-1,UINT32_MAX})for(uint32 wait:{0u,UINT32_MAX}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;WorldDatabase.rawFirst=prior;WorldDatabase.rawHasRow=true;WorldDatabase.rawWrites=0;bool ok=Command(CaseScriptWaypointAppendCheckedOrdinal::Selected::HandleScriptWpAddCommand,h,("7 "+std::to_string(wait)).c_str());Require(ok==(prior!=UINT32_MAX)&&WorldDatabase.rawWrites==uint32(ok),"Script ordinal wrap reached insert");if(ok)Require(WorldDatabase.rawId==7&&WorldDatabase.rawTail[0]==double(uint64(prior)+1)&&WorldDatabase.rawTail[4]==wait,"Script entry/ordinal/wait altered");++cases;}
 {Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;WorldDatabase.rawHasRow=false;WorldDatabase.rawWrites=0;Require(!Command(CaseScriptWaypointAppendCheckedOrdinal::Selected::HandleScriptWpAddCommand,h,"7")&&!WorldDatabase.rawWrites,"Failed script aggregate query continued");++cases;}WorldDatabase.rawHasRow=true;
#endif

#ifdef REPAIR_WORLDQUESTEXACTGENERATIONID
 for(char const*arg:{""," ","x","7x","-1","0","4294967303"}){ChatHandler h;Require(!Command(CaseWorldQuestExactGenerationId::Selected::HandleWorldQuestAdd,h,arg)&&!h.databaseReached,"Bad identifier reached native service");++cases;}
 for(uint32 id:{1u,7u,UINT32_MAX}){ChatHandler h;Require(Command(CaseWorldQuestExactGenerationId::Selected::HandleWorldQuestAdd,h,std::to_string(id).c_str())&&h.observed==id,"Native identifier lost before service");++cases;}
#endif

#ifdef REPAIR_SPELLSCENEEXACTID
 for(char const*arg:{""," ","x","7x","-1","0","4294967303"}){ChatHandler h;Require(!Command(CaseSpellSceneExactId::Selected::HandleSendSpellScene,h,arg)&&!h.databaseReached,"Bad identifier reached native service");++cases;}
 for(uint32 id:{1u,7u,UINT32_MAX}){ChatHandler h;Require(Command(CaseSpellSceneExactId::Selected::HandleSendSpellScene,h,std::to_string(id).c_str())&&h.observed==id,"Native identifier lost before service");++cases;}
#endif

#ifdef REPAIR_TRANSMOGREMOVALEXACTITEM
 for(char const*arg:{"Controlled x","Controlled 7x","Controlled -1","Controlled 0","Controlled 4294967303"}){ChatHandler h;h.offline=true;Require(!Command(CaseTransmogRemovalExactItem::Selected::HandleDelTransmog,h,arg)&&!h.databaseReached,"Bad offline transmog item reached deletion");++cases;}
 for(uint32 id:{1u,7u,UINT32_MAX}){ChatHandler h;h.offline=true;Require(Command(CaseTransmogRemovalExactItem::Selected::HandleDelTransmog,h,("Controlled "+std::to_string(id)).c_str())&&h.observed==id,"Offline native item ID altered");++cases;}
 {Player p;ChatHandler h;h.target=&p;Require(!Command(CaseTransmogRemovalExactItem::Selected::HandleDelTransmog,h,"Controlled 7")&&!h.databaseReached,"Online transmog deletion admission changed");++cases;}
#endif


#ifdef REPAIR_COMBATSTOPMISSINGSELECTION
 {Player named;accessor.named=&named;ChatHandler h;Require(Command(CaseCombatStopMissingSelection::Selected::HandleCombatStopCommand,h,"Controlled")&&named.stops==1&&named.refs.deletes==1,"Named combat stop failed without selected unit");++cases;}accessor.named=nullptr;
 {ChatHandler h;Require(!Command(CaseCombatStopMissingSelection::Selected::HandleCombatStopCommand,h,"")&&!h.messages,"Missing combat target crashed/changed actor");++cases;}
 for(bool denied:{false,true}){Player p;ChatHandler h;h.target=&p;h.lower=denied;Require(Command(CaseCombatStopMissingSelection::Selected::HandleCombatStopCommand,h,"")==!denied&&p.stops==uint32(!denied)&&p.refs.deletes==uint32(!denied),"Player combat stop authority/cleanup changed");++cases;}
 for(bool ai:{false,true}){Creature c;c.combat=true;c.hasAI=ai;ChatHandler h;h.unit=&c;Require(Command(CaseCombatStopMissingSelection::Selected::HandleCombatStopCommand,h,"")==ai&&c.ai.evades==uint32(ai),"NPC AI evade/missing AI route changed");++cases;}
#endif
#ifdef REPAIR_SCENARIOSTEPNATIVEBOUNDS
 for(char const*arg:{"x","1x","-1","256","4294967296","3"}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;scenarioManager.scenario.writes=0;Require(!Command(CaseScenarioStepNativeBounds::Selected::HandleScenarioStepCommand,h,arg)&&!scenarioManager.scenario.writes,"Bad/wrapped step reached native scenario error");++cases;}
 for(char const*arg:{"","0","1","2"}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;scenarioManager.scenario.writes=0;Require(Command(CaseScenarioStepNativeBounds::Selected::HandleScenarioStepCommand,h,arg)&&scenarioManager.scenario.writes==1,"Native step/default zero lost");++cases;}
 {Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;scenarioManager.scenario.count=0;scenarioManager.scenario.writes=0;Require(!Command(CaseScenarioStepNativeBounds::Selected::HandleScenarioStepCommand,h,"0")&&!scenarioManager.scenario.writes,"Zero-step scenario accepted");scenarioManager.scenario.count=3;++cases;}
 for(int missing:{0,1,2}){Player p;p.mapAvailable=missing!=0;controlledMap.inInstance=missing!=1;scenarioManager.exists=missing!=2;Session ses{&p,1};ChatHandler h;h.session=&ses;scenarioManager.scenario.writes=0;Require(!Command(CaseScenarioStepNativeBounds::Selected::HandleScenarioStepCommand,h,"0")&&!scenarioManager.scenario.writes,"Missing map/instance/scenario accepted");++cases;}controlledMap.inInstance=true;scenarioManager.exists=true;
#endif
#ifdef REPAIR_DEBUGCRITERIAATOMICNATIVEOPERANDS
 for(char const*arg:{""," ","x","1x","-1","213","0 x","0 1 x","0 1 1 x","0 4294967296"}){Player p;ChatHandler h;h.target=&p;Require(!Command(CaseDebugCriteriaAtomicNativeOperands::Selected::HandleDebugUpdateCriteriaCommand,h,arg)&&!p.writes,"Malformed criteria partly progressed");++cases;}
 for(char const*arg:{"0","212 4294967295 4294967295 4294967295","0 1 2 3 self"}){Player p;ChatHandler h;h.target=&p;Require(Command(CaseDebugCriteriaAtomicNativeOperands::Selected::HandleDebugUpdateCriteriaCommand,h,arg)&&p.writes==1&&p.criteria.size()==4,"Native criteria type/default/operand width lost");if(std::strstr(arg,"self"))Require(p.criteriaActor==&p,"Native fifth-token player anchor lost");++cases;}
#endif
#ifdef REPAIR_VEHICLEENTERCHECKEDSELECTORS
 for(char const*arg:{""," ","x","1x","-2","4294967296","1 x","1 128","1 -129"}){Player p;Session ses{&p,1};ChatHandler h;h.target=&p;h.session=&ses;Require(!Command(CaseVehicleEnterCheckedSelectors::Selected::HandleDebugEnterVehicleCommand,h,arg)&&!h.databaseReached,"Malformed entry/seat reached boarding");++cases;}
 for(int64 entry:{int64(-1),int64(0),int64(7),int64(UINT32_MAX)})for(int32 seat:{-128,-1,0,127}){Player p;Session ses{&p,1};ChatHandler h;h.target=&p;h.session=&ses;Require(Command(CaseVehicleEnterCheckedSelectors::Selected::HandleDebugEnterVehicleCommand,h,(std::to_string(entry)+" "+std::to_string(seat)).c_str())&&h.observed==uint64(entry)&&h.signedValue==seat,"Native sentinels/seat byte/entry width changed before boarding");++cases;}
 {Player p;ChatHandler h;h.target=&p;Require(!Command(CaseVehicleEnterCheckedSelectors::Selected::HandleDebugEnterVehicleCommand,h,"0")&&!h.databaseReached,"Missing boarding player accepted");++cases;}
#endif
#ifdef REPAIR_VEHICLESPAWNATOMICSELECTORS
 for(char const*arg:{""," ","x","7x","-1","4294967303","7 x","7 -1","7 4294967296"}){ChatHandler h;Require(!Command(CaseVehicleSpawnAtomicSelectors::Selected::HandleDebugSpawnVehicleCommand,h,arg)&&!h.databaseReached,"Malformed vehicle tuple entered summon/create");++cases;}
 for(char const*arg:{"7","0 0","4294967295 4294967295"}){ChatHandler h;Require(Command(CaseVehicleSpawnAtomicSelectors::Selected::HandleDebugSpawnVehicleCommand,h,arg)&&h.captured.size()==3&&h.databaseReached==1,"Native optional vehicle id/width lost before creation");++cases;}
#endif
#ifdef REPAIR_LFGJOINATOMICNATIVEWIDTHS
 for(char const*arg:{""," ","1","x 7","1x 7","-1 7","256 7","1 x","1 7x","1 -1","1 4294967303","1 7 x"}){Player p;ChatHandler h;h.target=&p;lfgManager.joins=0;Require(!Command(CaseLfgJoinAtomicNativeWidths::Selected::HandleLfgJoinCommand,h,arg)&&!lfgManager.joins,"Malformed LFG tuple partly queued");++cases;}
 for(uint32 role:{0u,1u,15u,255u}){Player p;ChatHandler h;h.target=&p;lfgManager.joins=0;Require(Command(CaseLfgJoinAtomicNativeWidths::Selected::HandleLfgJoinCommand,h,(std::to_string(role)+" 7 7 4294967295").c_str())&&lfgManager.joins==1&&lfgManager.roles==role&&lfgManager.dungeons==lfg::LfgDungeonSet({7,UINT32_MAX}),"Native LFG role width/dungeon set dedup changed");++cases;}
 {Player p;ChatHandler h;h.target=&p;lfgManager.enabled=false;lfgManager.joins=0;Require(!Command(CaseLfgJoinAtomicNativeWidths::Selected::HandleLfgJoinCommand,h,"1 7")&&!lfgManager.joins,"Disabled LFG admission bypassed");lfgManager.enabled=true;++cases;}
#endif
#ifdef REPAIR_LFGOPTIONSCHECKEDNATIVEMASK
 for(char const*arg:{"x","1x","-2","4294967296","9223372036854775808"}){ChatHandler h;lfgManager.optionWrites=0;Require(!Command(CaseLfgOptionsCheckedNativeMask::Selected::HandleLfgOptionsCommand,h,arg)&&!lfgManager.optionWrites,"Malformed LFG mask disabled/changed options");++cases;}
 for(uint32 value:{0u,1u,UINT32_MAX}){ChatHandler h;lfgManager.optionWrites=0;Require(Command(CaseLfgOptionsCheckedNativeMask::Selected::HandleLfgOptionsCommand,h,std::to_string(value).c_str())&&lfgManager.optionWrites==1&&lfgManager.options==value,"Native options mask narrowed");++cases;}
 for(char const*arg:{"","-1"}){ChatHandler h;lfgManager.optionWrites=0;Require(Command(CaseLfgOptionsCheckedNativeMask::Selected::HandleLfgOptionsCommand,h,arg)&&!lfgManager.optionWrites,"LFG display/sentinel changed options");++cases;}
#endif
#ifdef REPAIR_LFGQUEUEVERBOSITYEXACTVALUE
 for(char const*arg:{"x","1x","2147483648","-2147483649","1 2"}){ChatHandler h;lfgManager.dumps=0;Require(!Command(CaseLfgQueueVerbosityExactValue::Selected::HandleLfgQueueInfoCommand,h,arg)&&!lfgManager.dumps,"Bad queue verbosity reached dump");++cases;}
 for(char const*arg:{"","0","1","-1","-2147483648","2147483647"}){ChatHandler h;lfgManager.dumps=0;Require(Command(CaseLfgQueueVerbosityExactValue::Selected::HandleLfgQueueInfoCommand,h,arg)&&lfgManager.dumps==1&&lfgManager.verbose==(*arg&&std::strtoll(arg,nullptr,10)!=0),"Native boolean verbosity/default changed");++cases;}
#endif
#ifdef REPAIR_EXPLORESELECTEDMASKREPLACEMENT
 for(char const*arg:{""," ","x","1x","2147483648","-2147483649"}){Player selected,caller;Session ses{&caller,1};ChatHandler h;h.target=&selected;h.session=&ses;Require(!Command(CaseExploreSelectedMaskReplacement::Selected::HandleExploreCheatCommand,h,arg)&&!caller.writes&&!selected.writes,"Bad exploration request changed fields");++cases;}
 for(int32 flag:{INT32_MIN,-1,0,1,INT32_MAX}){Player selected,caller;Session ses{&caller,1};ChatHandler h;h.target=&selected;h.session=&ses;selected.flags[PLAYER_FIELD_EXPLORED_ZONES-1]=42;selected.flags[PLAYER_FIELD_EXPLORED_ZONES+PLAYER_EXPLORED_ZONES_SIZE]=43;for(uint16 i=0;i<PLAYER_EXPLORED_ZONES_SIZE;++i)selected.flags[PLAYER_FIELD_EXPLORED_ZONES+i]=0x5a5a5a5a;Require(Command(CaseExploreSelectedMaskReplacement::Selected::HandleExploreCheatCommand,h,std::to_string(flag).c_str())&&selected.writes==PLAYER_EXPLORED_ZONES_SIZE&&!caller.writes,"Exploration modified executor instead of selected player");for(uint16 i=0;i<PLAYER_EXPLORED_ZONES_SIZE;++i)Require(selected.flags[PLAYER_FIELD_EXPLORED_ZONES+i]==(flag?UINT32_MAX:0),"Exploration clear failed to replace masks");Require(selected.flags[PLAYER_FIELD_EXPLORED_ZONES-1]==42&&selected.flags[PLAYER_FIELD_EXPLORED_ZONES+PLAYER_EXPLORED_ZONES_SIZE]==43,"Exploration changed neighboring field");++cases;}
 {Player p;ChatHandler h;h.target=&p;h.lower=true;Require(!Command(CaseExploreSelectedMaskReplacement::Selected::HandleExploreCheatCommand,h,"1")&&!p.writes,"Selected player authority gate missing");++cases;}
#endif
#ifdef REPAIR_SERVERPLAYERLIMITNATIVEDOMAIN
 for(char const*arg:{" ","x","1x","-8","-2147483648","4294967296","9223372036854775808"}){ChatHandler h;worldConfig=WorldConfig{};Require(!Command(CaseServerPlayerLimitNativeDomain::Selected::HandleServerPLimitCommand,h,arg)&&!worldConfig.amountWrites&&!worldConfig.securityWrites&&!worldConfig.securityLoads,"Malformed limit changed admission");++cases;}
 for(uint32 value:{0u,1u,UINT32_MAX}){ChatHandler h;worldConfig=WorldConfig{};Require(Command(CaseServerPlayerLimitNativeDomain::Selected::HandleServerPLimitCommand,h,std::to_string(value).c_str())&&worldConfig.amountWrites==1&&worldConfig.amount==value&&!worldConfig.securityWrites,"Player amount limit native width lost");++cases;}
 for(int level=1;level<=SEC_CONSOLE;++level){ChatHandler h;worldConfig=WorldConfig{};Require(Command(CaseServerPlayerLimitNativeDomain::Selected::HandleServerPLimitCommand,h,std::to_string(-level).c_str())&&worldConfig.securityWrites==1&&worldConfig.security==level&&!worldConfig.amountWrites,"Native security limit domain changed");++cases;}
 for(char const*arg:{"","player","moderator","gamemaster","administrator","reset"}){ChatHandler h;worldConfig=WorldConfig{};Require(Command(CaseServerPlayerLimitNativeDomain::Selected::HandleServerPLimitCommand,h,arg),"Native limit keywords/display lost");if(std::strcmp(arg,"reset")==0)Require(worldConfig.securityLoads==1&&worldConfig.amount==100,"Reset keyword failed native reload/default");++cases;}
#endif


#ifdef REPAIR_AURAEFFECTAMOUNTNATIVEADMISSION
 for(char const*arg:{"123 31 1x","123 x","123 -1","123 32","123 256","123 0 x","123 0 2147483648","123 0 -2147483649"}){Player p;ChatHandler h;h.target=&p;Require(!Command(CaseAuraEffectAmountNativeAdmission::Selected::HandleAuraAmountCommand,h,arg)&&!p.effectReads&&!p.aura.writes,"Bad effect/amount reached native aura array");++cases;}
 for(uint32 effect:{0u,31u})for(int32 value:{INT32_MIN,-1,0,INT32_MAX}){Player p;ChatHandler h;h.target=&p;Require(Command(CaseAuraEffectAmountNativeAdmission::Selected::HandleAuraAmountCommand,h,("123 "+std::to_string(effect)+" "+std::to_string(value)).c_str())&&p.effectReads==1&&p.lastEffect==effect&&p.aura.writes==1&&p.aura.amount==value,"Native aura slot/int32 amount changed");++cases;}
 {Player p;p.auraPresent=false;ChatHandler h;h.target=&p;Require(Command(CaseAuraEffectAmountNativeAdmission::Selected::HandleAuraAmountCommand,h,"123")&&p.effectReads==1&&!p.aura.writes,"Missing aura/default amount route changed");++cases;}
#endif
#ifdef REPAIR_DAMAGENATIVEOPERANDADMISSION
 for(char const*arg:{""," ","x","7x","2147483648","-2147483649","7 x","7 1x","7 -1","7 7","7 4294967296"}){Player caller;caller.selection=true;Creature target;Session ses{&caller,1};ChatHandler h;h.unit=&target;h.session=&ses;Require(!Command(CaseDamageNativeOperandAdmission::Selected::HandleDamageCommand,h,arg)&&!h.databaseReached,"Malformed damage/school reached combat effect");++cases;}
 for(char const*arg:{"1","2147483647","7 0","7 6"}){Player caller;caller.selection=true;Creature target;Session ses{&caller,1};ChatHandler h;h.unit=&target;h.session=&ses;Require(Command(CaseDamageNativeOperandAdmission::Selected::HandleDamageCommand,h,arg)&&h.databaseReached==1&&h.observed>0,"Native damage/default school domain lost");++cases;}
 for(char const*arg:{"0","-1","-2147483648"}){Player caller;caller.selection=true;Creature target;Session ses{&caller,1};ChatHandler h;h.unit=&target;h.session=&ses;Require(Command(CaseDamageNativeOperandAdmission::Selected::HandleDamageCommand,h,arg)&&!h.databaseReached,"Native nonpositive no-op changed");++cases;}
#endif
#ifdef REPAIR_ITEMCONTEXTATOMICNATIVEIDS
 for(char const*arg:{""," ","7","x 1","7x 1","-1 1","4294967303 1","7 x","7 -1","7 4294967296"}){ChatHandler h;Require(!Command(CaseItemContextAtomicNativeIds::Selected::HandleAddItemModCommand,h,arg)&&!h.databaseReached,"Bad item/context partially reached grant");++cases;}
 for(char const*arg:{"0 0","7 1","4294967295 4294967295"}){ChatHandler h;Require(Command(CaseItemContextAtomicNativeIds::Selected::HandleAddItemModCommand,h,arg)&&h.captured.size()==2&&h.databaseReached==1,"Native item/context widths lost before lookup");++cases;}
#endif
#ifdef REPAIR_ITEMSETEXACTSELECTOR
 for(char const*arg:{""," ","x","7x","-1","0","4294967303"}){ChatHandler h;Require(!Command(CaseItemSetExactSelector::Selected::HandleAddItemSetCommand,h,arg),"Bad item-set ID reached catalog/grants");++cases;}
 for(bool self:{false,true}){Player caller,other;Session ses{&caller,1};ChatHandler h;h.session=&ses;h.target=self?&caller:&other;Player&receiver=self?caller:other;Require(Command(CaseItemSetExactSelector::Selected::HandleAddItemSetCommand,h,"7")&&receiver.storeCalls==2&&receiver.storedItems.size()==2&&caller.notices==2,"Native known set did not grant its two catalog items");for(auto const&item:receiver.storedItems)Require(item->bound==!self,"Self/other item binding policy changed");if(!self)Require(other.notices==2,"Recipient grant notice lost");++cases;}
 for(uint32 id:{1u,UINT32_MAX}){Player p;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(!Command(CaseItemSetExactSelector::Selected::HandleAddItemSetCommand,h,std::to_string(id).c_str())&&p.storedItems.empty(),"Unknown native item-set granted inventory");++cases;}
 for(uint32 fail:{1u,2u}){Player p;p.failStoreCall=fail;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(!Command(CaseItemSetExactSelector::Selected::HandleAddItemSetCommand,h,"7")&&h.error&&p.storeCalls==fail&&p.storedItems.size()==fail-1&&p.notices==fail-1,"Failed item creation dereferenced/notified a null item or discarded prior grants");++cases;}
 {Player p;p.canStore=false;Session ses{&p,1};ChatHandler h;h.session=&ses;Require(Command(CaseItemSetExactSelector::Selected::HandleAddItemSetCommand,h,"7")&&p.storedItems.empty()&&p.errorChannel==2,"Native full-bag reporting/partial-set policy changed");++cases;}
#endif
#ifdef REPAIR_ITEMGRANTATOMICCOUNTANDBONUSES
 for(char const*arg:{""," ","x","7x","-1","4294967303","7 x","7 2147483648","7 -2147483649","7 1 x","7 1 1:2x","7 1 -1","7 1 4294967296"}){ChatHandler h;Require(!Command(CaseItemGrantAtomicCountAndBonuses::Selected::HandleAddItemCommand,h,arg)&&!h.databaseReached,"Invalid item/count/bonus partly reached mutation");++cases;}
 for(int32 count:{INT32_MIN,-1,0,1,INT32_MAX}){ChatHandler h;Require(Command(CaseItemGrantAtomicCountAndBonuses::Selected::HandleAddItemCommand,h,("7 "+std::to_string(count)+" 1:4294967295").c_str())&&h.observed==7&&h.signedValue==(count?count:1)&&h.captured==std::vector<uint64>({1,UINT32_MAX}),"Native signed count/zero normalization/bonus widths lost");++cases;}
 {ChatHandler h;WorldDatabase.aggregates[WORLD_SEL_ITEM_TEMPLATE_BY_NAME]={true,7};Require(Command(CaseItemGrantAtomicCountAndBonuses::Selected::HandleAddItemCommand,h,"[Controlled Item] 2 1:2")&&h.observed==7&&h.signedValue==2,"Manual item-name route lost");WorldDatabase.aggregates.clear();++cases;}
 for(int32 count:{INT32_MIN,-2147483647,-1}){Player p;ChatHandler h;Require(CheckedRemoval(&p,7,count,&h)&&p.removedItems==1&&p.removedId==7&&p.removedCount==uint32(-int64(count)),"Actual negative-count removal branch overflowed/changed amount");++cases;}
#endif
#ifdef REPAIR_MYTHICKEYATOMICNATIVEFIELDS
 for(char const*arg:{"","x","7x","-1","4294967303","7 x","7 -1","7 2 x","7 2 1 x","7 2 1 1 x","7 2 1 1 4294967296"}){ChatHandler h;Require(!Command(CaseMythicKeyAtomicNativeFields::Selected::HandleAddMythicKeyCommand,h,arg)&&!h.databaseReached,"Malformed key tuple entered item creation");++cases;}
 for(char const*arg:{" ","7","7 0 0 0 0","4294967295 4294967295 4294967295 4294967295 4294967295"}){ChatHandler h;Require(Command(CaseMythicKeyAtomicNativeFields::Selected::HandleAddMythicKeyCommand,h,arg)&&h.databaseReached==1&&h.captured.size()==5,"Native key defaults/UInt32 fields lost before item validation");++cases;}
#endif
#ifdef REPAIR_MAILITEMSBOUNDEDATOMICATTACHMENTS
 for(char const*tail:{"x","7x","-1","4294967303","7:x","7:1x","7:-1","7:0","7:4294967296","7:1:2","7:4294967295"}){ChatHandler h;h.offline=true;objectManager.item.stack=1;objectManager.item.maxCount=0;Require(!Command(CaseMailItemsBoundedAtomicAttachments::Selected::HandleSendItemsCommand,h,(std::string("Controlled \"Subject\" \"Body\" ")+tail).c_str())&&!h.databaseReached,"Malformed/unbounded attachments reached mail persistence");++cases;}
 for(uint32 count:{1u,12u,13u,UINT32_MAX}){ChatHandler h;h.offline=true;objectManager.item.stack=1;bool ok=Command(CaseMailItemsBoundedAtomicAttachments::Selected::HandleSendItemsCommand,h,(std::string("Controlled \"Subject\" \"Body\" 7:")+std::to_string(count)).c_str());Require(ok==(count<=MAX_MAIL_ITEMS)&&h.databaseReached==uint32(ok),"Attachment preflight allowed oversized expansion");if(ok)Require(h.observed==count&&h.captured.size()==count*2,"Native stack split/count changed");++cases;}
 {ChatHandler h;h.offline=true;objectManager.item.stack=20;Require(Command(CaseMailItemsBoundedAtomicAttachments::Selected::HandleSendItemsCommand,h,"Controlled \"Subject\" \"Body\" 7:21 7:1")&&h.observed==3&&h.captured==std::vector<uint64>({7,20,7,1,7,1}),"Native attachment ordering/stack split changed");++cases;}
 {ChatHandler h;h.offline=true;objectManager.item.stack=0;Require(!Command(CaseMailItemsBoundedAtomicAttachments::Selected::HandleSendItemsCommand,h,"Controlled \"Subject\" \"Body\" 7:1")&&!h.databaseReached,"Zero stack size divided/looped");++cases;}objectManager.item.stack=1;
 {ChatHandler h;h.offline=true;objectManager.item.maxCount=1;Require(!Command(CaseMailItemsBoundedAtomicAttachments::Selected::HandleSendItemsCommand,h,"Controlled \"Subject\" \"Body\" 7:2")&&!h.databaseReached,"Native max-count admission bypassed");++cases;}objectManager.item.maxCount=0;
#endif
#ifdef REPAIR_MAILMONEYNATIVECAP
 for(char const*money:{"x","7x","-1","0","100000000000","18446744073709551616"}){ChatHandler h;h.offline=true;Require(!Command(CaseMailMoneyNativeCap::Selected::HandleSendMoneyCommand,h,(std::string("Controlled \"Subject\" \"Body\" ")+money).c_str())&&!h.databaseReached,"Malformed/above-cap money reached draft");++cases;}
 for(uint64 money:{uint64(1),uint64(INT32_MAX)+1,MAX_MONEY_AMOUNT}){ChatHandler h;h.offline=true;Require(Command(CaseMailMoneyNativeCap::Selected::HandleSendMoneyCommand,h,(std::string("Controlled \"Subject\" \"Body\" ")+std::to_string(money)).c_str())&&h.observed==money,"Native64-bit capped mail balance lost");++cases;}
#endif
#ifdef REPAIR_GUILDLOOKUPFULLNATIVEID
 for(char const*arg:{""," ","x","7x","-1","0","18446744073709551616"}){ChatHandler h;Require(!Command(CaseGuildLookupFullNativeId::Selected::HandleLookupGuildCommand,h,arg)&&!h.messages,"Malformed guild lookup reported another record");++cases;}
 for(uint64 id:{uint64(7),(uint64(1)<<32)+7,UINT64_MAX}){ChatHandler h;Require(Command(CaseGuildLookupFullNativeId::Selected::HandleLookupGuildCommand,h,std::to_string(id).c_str())&&h.messages==1,"Native guild lookup width/result lost");++cases;}
#endif
#ifdef REPAIR_SPELLLINKNATIVENUMERICPAYLOAD
 for(char const*arg:{""," ","x","123x","-1","4294967419","18446744073709551616","|cffffffff|Hspell:123x|h[Bad Spell]|h|r","|cffffffff|Htalent:8|h[Missing Talent]|h|r","|cffffffff|Hglyph:0:8x|h[Bad Glyph]|h|r","|cffffffff|Hglyph:0:-1|h[Bad Glyph]|h|r","|cffffffff|Hglyph:0:4294967304|h[Bad Glyph]|h|r"}){ChatHandler h;std::string buffer=arg;Require(h.extractSpellIdFromLink(buffer.data())==0,"Malformed/missing native spell-link payload aliased a spell");++cases;}
 for(auto const&row:std::vector<std::pair<std::string,uint32>>{{"123",123},{"4294967295",UINT32_MAX},{"|Hspell:123|h[Spell Name]|h|r",123},{"|cffffffff|Henchant:123|h[Recipe Name]|h|r",123},{"|cffffffff|Htrade:123:1:2|h[Trade Name]|h|r",123},{"|cffffffff|Htalent:7:1|h[Talent Name]|h|r",123},{"|cffffffff|Hglyph:0:8|h[Glyph Name]|h|r",456}}){ChatHandler h;auto buffer=row.first;Require(h.extractSpellIdFromLink(buffer.data())==row.second,"Native spell/talent/glyph/enchant/trade mapping changed");++cases;}
 {ChatHandler h;std::string input="|cffffffff|Hspell:123|h[Spell Name With Spaces]|h|r 7 9";Require(h.extractSpellIdFromLink(input.data())==123,"Native link extraction failed");char*first=strtok(nullptr," ");char*second=strtok(nullptr," ");Require(first&&second&&std::strcmp(first,"7")==0&&std::strcmp(second,"9")==0,"Link parser lost later command operands");++cases;}
#endif

// NEW_CASES
std::cout<<"PASS "<<cases<<" selected production command cases;controlled endpoints;not client traversal\n";return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
