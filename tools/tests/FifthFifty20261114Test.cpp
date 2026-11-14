// Actual selected production functions; all services below are controlled and never access a database.
#include <algorithm>
#include <any>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cctype>
#include <ctime>
#include <iostream>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include "../../src/server/game/Chat/CommandIntegerArgument.h"
#include "../../src/server/game/Chat/CommandFloatArgument.h"
using uint8=uint8_t;using uint16=uint16_t;using uint32=uint32_t;using uint64=uint64_t;using int32=int32_t;using int64=int64_t;
#define UI64LIT(N) UINT64_C(N)
#define UI64FMTD "%llu"
#define TC_LOG_ERROR(...) ((void)0)
#define TC_LOG_INFO(...) ((void)0)
#define TC_LOG_DEBUG(...) ((void)0)
#define stricmp _stricmp
static void Require(bool b,char const* message){if(!b)throw std::runtime_error(message);}
#include "Constants.inc"
enum class HighGuid{Player,Creature,GameObject};
struct ObjectGuid{using LowType=uint64;uint64 value=0;HighGuid type=HighGuid::Player;static ObjectGuid Empty;static uint64 GetMaxCounter(HighGuid){return 0xffffffffffULL;}template<HighGuid T>static ObjectGuid Create(uint64 id){return{id,T};}template<HighGuid T>static ObjectGuid Create(uint32,uint32,uint64 id){return{id,T};}uint64 GetGUIDLow()const{return value;}explicit operator bool()const{return value!=0;}};ObjectGuid ObjectGuid::Empty;
struct Map;struct Transport;struct Session;struct Creature;struct Player;
struct Aura{int32 duration=1;uint8 charges=1;unsigned changes=0;void SetDuration(int32 n){duration=n;++changes;}void SetCharges(uint8 n){charges=n;++changes;}};
struct Unit{virtual~Unit()=default;virtual Player*ToPlayer(){return nullptr;}Aura aura;bool hasAura=true;unsigned removals=0;Aura*GetAura(uint32){return hasAura?&aura:nullptr;}void RemoveAurasDueToSpell(uint32){++removals;hasAura=false;}};using WorldObject=Unit;
struct Visuals{uint32 m_visMainhand=0,m_visHead=0,m_visShoulders=0,m_visChest=0,m_visWaist=0,m_visLegs=0,m_visFeet=0,m_visWrists=0,m_visHands=0,m_visBack=0,m_visOffhand=0,m_visRanged=0,m_visTabard=0,m_visShirt=0;};
struct PhaseManager{uint32 GetPhaseMaskForSpawn(){return 2;}};
struct Packet{int32 reason=0,a=-1,b=-1,spell=0;};
struct Player:Unit{static uint32 deletions;static void DeleteOldCharacters(uint32);static void DeleteFromDB(ObjectGuid,uint32,bool,bool){++deletions;}Player*ToPlayer()override{return this;}Session*session=nullptr;Transport*transport=nullptr;Map*map=nullptr;Visuals*m_vis=nullptr;uint32 slot=0,item=0,modelCalls=0,flags=0,packets=0,questCode=0;bool modelAccept=true,inBG=false;Packet lastPacket;PhaseManager phase;
 ~Player(){delete m_vis;}ObjectGuid GetGUID(){return {7,HighGuid::Player};}char const*GetName(){return "Controlled";}Session*GetSession(){return session;}Map*GetMap(){return map;}Transport*GetTransport(){return transport;}PhaseManager&GetPhaseMgr(){return phase;}uint32 GetPhaseMask(){return 2;}float GetPositionX(){return 1;}float GetPositionY(){return 2;}float GetPositionZ(){return 3;}float GetOrientation(){return 4;}float GetTransOffsetX(){return 5;}float GetTransOffsetY(){return 6;}float GetTransOffsetZ(){return 7;}float GetTransOffsetO(){return 1;}uint32 GetMapId(){return 1;}
 bool HandleChangeSlotModel(uint32 id,uint32 s){++modelCalls;slot=s;item=id;return modelAccept;}bool GetCommandStatus(uint32 f){return(flags&f)!=0;}void SetCommandStatusOn(uint32 f){flags|=f;}void SetCommandStatusOff(uint32 f){flags&=~f;}void SendCanTakeQuestResponse(uint32 n,void*){++packets;questCode=n;}void SendDirectMessage(Packet const*p){Require(p,"Missing packet");++packets;lastPacket=*p;}bool InBattleground(){return inBG;}
};
struct Session{Player*player=nullptr;uint32 reason=0,responses=0;Player*GetPlayer(){return player;}void SendPetBattleRequestFailed(uint8 n){reason=n;++responses;}std::string GetRemoteAddress(){return "192.0.2.1";}};
struct Map{Creature*selected=nullptr;Creature*GetCreature(ObjectGuid){return selected;}uint32 GetId(){return 1;}uint32 GetSpawnMode(){return 0;}};
struct CreatureData{uint32 id=0,phaseMask=0,mapid=1;float posX=0,posY=0,posZ=0,orientation=0;};
struct CreatureTemplate{};struct ItemTemplate{};struct GameObjectTemplate{};struct GameObjectData{uint32 id=99,mapid=1;};
static bool createAccept=true,loadAccept=true;static uint64 loadedGuid=0;static uint32 creates=0,saves=0,cleanups=0;static int liveCreatures=0;static std::vector<Creature*>loadedCreatures;
struct Creature:Unit{uint64 db=0,liveGuid=0;bool active=false,totem=false,pet=false;void setActive(bool b){active=b;}bool isActiveObject(){return active;}bool isTotem(){return totem;}float GetPositionX(){return 1;}float GetPositionY(){return 2;}float GetPositionZ(){return 3;}uint32 path=77,entry=VISUAL_WAYPOINT;Creature(){++liveCreatures;}~Creature(){--liveCreatures;}bool Create(uint64 guid,Map*,uint32,uint32,uint32,uint32,float,float,float,float){++creates;db=guid;return createAccept;}void SaveToDB(uint32,uint64,uint32);void CleanupsBeforeDelete(){++cleanups;}bool LoadCreatureFromDB(uint64 guid,Map*){loadedGuid=guid;db=guid;if(loadAccept)loadedCreatures.push_back(this);return loadAccept;}uint64 GetDBTableGUIDLow(){return db;}bool isPet(){return pet;}uint32 GetWaypointPath(){return path;}uint32 GetEntry(){return entry;}ObjectGuid GetGUID(){return{liveGuid?liveGuid:db,HighGuid::Creature};}char const*GetName(){return "Controlled NPC";}bool hasLootRecipient(){return false;}Player*GetLootRecipient(){return nullptr;}};
struct GameTele{uint32 id=7;};
struct ObjectMgr{CreatureTemplate creature;ItemTemplate item;GameObjectTemplate go;GameObjectData goData;GameTele tele;bool templatePresent=true;uint32 catalogCalls=0,deletes=0,grids=0,teleId=0;std::string teleName;uint64 next=(1ULL<<32)+7;std::map<uint64,CreatureData>data;struct Generator{ObjectMgr*owner;uint64 Generate(){return owner->next++;}}generator{this};template<HighGuid>Generator*GetGenerator(){return &generator;}CreatureTemplate const*GetCreatureTemplate(uint32 id){++catalogCalls;return templatePresent&&id==7?&creature:nullptr;}ItemTemplate const*GetItemTemplate(uint32 id){return id==7?&item:nullptr;}GameObjectTemplate const*GetGameObjectTemplate(uint32 id){return id==7?&go:nullptr;}CreatureData&NewOrExistCreatureData(uint64 guid){return data[guid];}std::map<uint64,CreatureData>&_creatureDataStore=data;void DeleteCreatureData(uint64 const&guid);void RemoveCreatureFromGrid(uint64 guid,CreatureData const*p){Require(p&&data.count(guid),"Wrong spawn grid cleanup");++deletes;}void AddCreatureToGrid(uint64 guid,CreatureData const*p){Require(p&&data.count(guid),"Invalid native grid data");++grids;}CreatureData const*GetCreatureData(uint64 id){auto it=data.find(id);return it==data.end()?nullptr:&it->second;}GameObjectData const*GetGOData(uint64 id){return id==(1ULL<<32)+7?&goData:nullptr;}GameTele const*GetGameTele(uint32 id){teleId=id;teleName.clear();return id==7?&tele:nullptr;}GameTele const*GetGameTele(std::string const&name){teleId=0;teleName=name;return name=="7th"||name=="Home"||name=="0"?&tele:nullptr;}
 static uint32 GetPlayerAccountIdByGUID(ObjectGuid guid){return guid.value==7?1:0;}static ObjectGuid GetPlayerGUIDByName(std::string const&name){return name=="Controlled"?ObjectGuid{7,HighGuid::Player}:ObjectGuid{};}static bool GetPlayerNameByGUID(ObjectGuid,std::string&){return false;}
}objectManager;auto sObjectMgr=&objectManager;
void Creature::SaveToDB(uint32 map,uint64,uint32 mask){++saves;auto&d=sObjectMgr->data[db];d.mapid=map;d.phaseMask=mask;}
struct Transport{bool accepted=true;unsigned calls=0;std::unique_ptr<Creature>passenger;struct Info{struct{uint32 SpawnMap=9;}moTransport;}info;Info*GetGOInfo(){return &info;}Creature*CreateNPCPassenger(uint64 guid,CreatureData const*){++calls;if(!accepted)return nullptr;passenger=std::make_unique<Creature>();passenger->db=guid;return passenger.get();}};
struct Field{uint64 value=0;std::string text;uint32 GetUInt32(){return uint32(value);}uint64 GetUInt64(){return value;}float GetFloat(){return float(value);}char const*GetCString(){return text.c_str();}std::string GetString(){return text;}};
struct QueryRow{Field fields[12];Field*Fetch(){return fields;}bool NextRow(){return false;}};using PreparedQueryResult=std::shared_ptr<QueryRow>;using QueryResult=PreparedQueryResult;
struct Statement{uint32 id=0;std::map<uint32,uint64>integers;std::map<uint32,float>floats;std::map<uint32,std::string>strings;void setUInt32(uint32 k,uint32 n){integers[k]=n;}void setUInt64(uint32 k,uint64 n){integers[k]=n;}void setFloat(uint32 k,float n){floats[k]=n;}void setString(uint32 k,std::string const&s){strings[k]=s;}};using WorldDatabasePreparedStatement=Statement;using CharacterDatabasePreparedStatement=Statement;using LoginDatabasePreparedStatement=Statement;
struct Database{Statement pending,last;unsigned queries=0;std::vector<Statement>writes;std::map<uint32,bool>rows;bool hasRow=false;uint64 first=0;std::string sql;int64 rawValue=0;Statement*GetPreparedStatement(uint32 id){pending={};pending.id=id;return &pending;}PreparedQueryResult Query(Statement*p){++queries;last=*p;if(!(rows.count(p->id)?rows[p->id]:hasRow))return nullptr;auto row=std::make_shared<QueryRow>();row->fields[0].value=first;return row;}void Execute(Statement*p){writes.push_back(*p);}void EscapeString(std::string&){}void PExecute(char const*fmt,uint8,uint64 n){sql=fmt;rawValue=int64(n);++queries;}template<class...T>void PExecute(char const*fmt,char const*,int64 n,T...){sql=fmt;rawValue=n;++queries;}template<class...T>void PExecute(char const*fmt,char const*,char const*,T...){sql=fmt;++queries;}}WorldDatabase,CharacterDatabase;
struct World{int32 keepDays=30;int32 getIntConfig(uint32){return keepDays;}}world;auto sWorld=&world;
struct DeletedInfo{ObjectGuid Guid;std::string name;uint32 accountId=0;std::string accountName;time_t deleteDate=0;};using DeletedInfoList=std::list<DeletedInfo>;
namespace AccountMgr{uint32 GetId(std::string const&s){return s=="CONTROLLED"?1:0;}bool GetName(uint32 n,std::string&s){s="CONTROLLED";return n==1;}}
static bool Utf8ToUpperOnlyLatin(std::string&s){for(char&c:s)c=char(std::toupper(static_cast<unsigned char>(c)));return true;}
static bool normalizePlayerName(std::string&s){if(s.empty()||!std::isalpha(static_cast<unsigned char>(s[0])))return false;return true;}
struct CharacterDataStore{uint32 CheckPlayerName(std::string const&,uint32,bool){return CHAR_NAME_SUCCESS;}}characterStore;auto sCharacterDataStore=&characterStore;
struct Accessor{Player*FindPlayerByName(std::string const&){return nullptr;}}accessor;auto sObjectAccessor=&accessor;
struct TalentEntry{uint32 SpellID=123;};struct GlyphPropertiesEntry{uint32 SpellID=456;};struct TalentStore{TalentEntry const*LookupEntry(uint32){return nullptr;}}sTalentStore;struct GlyphStore{GlyphPropertiesEntry const*LookupEntry(uint32){return nullptr;}}sGlyphPropertiesStore;
struct ItemSpecOverrideEntry{uint32 SpecID=0;};struct ItemSparseEntry{};struct ItemEntry{};struct ItemCatalog{bool present=true;ItemSparseEntry sparse;ItemEntry item;}itemCatalog;
struct SparseStore{ItemSparseEntry const*LookupEntry(uint32 id){return itemCatalog.present&&id==7?&itemCatalog.sparse:nullptr;}}sItemSparseStore;struct ItemStore{ItemEntry const*LookupEntry(uint32 id){return itemCatalog.present&&id==7?&itemCatalog.item:nullptr;}}sItemStore;
struct ItemSpecStats{uint32 ItemSpecStatCount=0;ItemSpecStats(ItemEntry const*i,ItemSparseEntry const*s){Require(i&&s,"Null native ItemSpecStats input");}};
struct DB2Manager{std::vector<ItemSpecOverrideEntry const*> const*GetItemSpecOverrides(uint32){return nullptr;}}db2;auto&sDB2Manager=db2;
struct GameEventMgr{using ActiveEvents=std::set<uint16>;ActiveEvents active;ActiveEvents const&GetActiveEventList(){return active;}}events;auto sGameEventMgr=&events;
struct Battleground{};namespace MS{namespace Battlegrounds{enum class BattlegroundTypeId{ArenaAll};}}
#include "NativeBracket.inc"
struct BattlegroundManager{Battleground bg;bool exists=true;Battleground*GetBattlegroundTemplate(MS::Battlegrounds::BattlegroundTypeId){return exists?&bg:nullptr;}}battlegrounds;auto sBattlegroundMgr=&battlegrounds;
struct ChatHandler{Session*session=nullptr;Player*target=nullptr;Unit*unit=nullptr;Creature*creature=nullptr;std::string lastText;unsigned messages=0,reached=0;bool error=false;uint64 observed=0;int64 signedValue=0;std::vector<uint64>captured;Session*GetSession(){return session;}uint32 GetSessionDbcLocale(){return 0;}Player*getSelectedPlayer(){return target;}Unit*getSelectedUnit(){return unit?unit:target;}Unit*getSelectedObject(){return unit?unit:target;}Creature*getSelectedCreature(){return creature;}template<class...T>void SendSysMessage(T...){++messages;}void PSendSysMessage(char const*fmt,char const*name,uint64 guid,uint64 db,char const*owner){char buffer[256];std::snprintf(buffer,sizeof(buffer),fmt,name,guid,db,owner);lastText=buffer;++messages;}template<class...T>void PSendSysMessage(T...){++messages;}void SetSentErrorMessage(bool b){error=b;}char*extractKeyFromLink(char*,char const*,char** =nullptr);char*extractKeyFromLink(char*,char const*const*,int*,char** =nullptr);char*extractQuotedArg(char*);void extractOptFirstArg(char*,char**,char**);uint32 extractSpellIdFromLink(char*);ObjectGuid extractGuidFromLink(char*);GameTele const*extractGameTeleFromLink(char*);std::string extractPlayerNameFromLink(char*text){return text?text:"";}};
class CommandArgs{public:enum CommandArgsType{ARG_INT,ARG_UINT,ARG_FLOAT,ARG_STRING,ARG_QUOTE_ENCLOSED_STRING};bool _validArgs=false;ChatHandler*_handler=nullptr;char const*_charArgs=nullptr;std::vector<std::any>_args;void Initialize(std::initializer_list<CommandArgsType>);};
constexpr uint32 SMSG_CAST_FAILED=1;namespace WorldPackets{namespace Spells{struct CastFailed{Packet packet;int32 SpellID=0,Reason=0,FailedArg1=-1,FailedArg2=-1;explicit CastFailed(uint32){}Packet const*Write(){packet={Reason,FailedArg1,FailedArg2,SpellID};return &packet;}};}}
uint32 Player::deletions=0;namespace GameTime{static time_t now=1700000000;time_t GetGameTime(){return now;}}
#include "NativeDeletion.inc"
#include "NativeChatLinks.inc"
struct ControlledGroup{uint32 members;uint32 GetMembersCount(){return members;}};
#include "NativeCleanupAndRemaining.inc"
#include "Selected.inc"
static unsigned cases=0;static char const*activeCase="core";
template<class Fn>static bool Run(Fn fn,ChatHandler&h,std::string text){++cases;return fn(&h,text.data());}
struct Context{Map map;Player player;Session session;ChatHandler handler;Creature npc;Context(){session.player=&player;player.session=&session;player.map=&map;handler.session=&session;handler.target=&player;handler.creature=&npc;npc.db=(1ULL<<32)+7;}};
static void ResetSpawns(){for(Creature*p:loadedCreatures)delete p;loadedCreatures.clear();objectManager.data.clear();objectManager.next=(1ULL<<32)+7;objectManager.deletes=objectManager.grids=objectManager.catalogCalls=0;objectManager.templatePresent=true;createAccept=loadAccept=true;loadedGuid=0;creates=saves=cleanups=0;}
#include "Cases.inc"
