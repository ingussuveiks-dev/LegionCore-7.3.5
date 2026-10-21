#include <algorithm>
#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>
using uint8=uint8_t;using uint32=uint32_t;using uint64=uint64_t;
#include "Status.inc"
enum {CLASS_WARLOCK=9,GARRISON_TYPE_GARRISON=2,GARRISON_TYPE_CLASS_ORDER=3,GARRISON_SHIPMENT_FLAG_REQUIRE_QUEST_NOT_COMPLETE=1,
 SPELL_EFFECT_HANDLE_HIT_TARGET=1,GARRISON_SUCCESS=0,GARRISON_ERROR_NO_GARRISON=1,DB_STATE_UNCHANGED=0,DB_STATE_NEW,DB_STATE_REMOVED,
 CONFIG_ARTIFACT_RESEARCH_TIMER,GAMEOBJECT_TYPE_GARRISON_SHIPMENT=45,GO_STATE_READY,GAMEOBJECT_FIELD_DISPLAY_ID,GAMEOBJECT_FIELD_SPELL_VISUAL_ID,
 BUILDING_DATA_SPECIAL_SPAWN,CHAR_DEL_CHARACTER_GARRISON_SHIPMENTS_DBID,DAY=86400};
using ObjectDBState=int;using SpellEffIndex=int;
namespace GameTime{uint32 now=1000;uint32 GetGameTime(){return now;}}
struct World{uint32 getIntConfig(int){return 3600;}} world;auto sWorld=&world;
struct CharShipmentEntry{uint32 ID,ContainerID,Duration,SpellID,OnCompleteSpellID,GarrFollowerID,MaxShipments,Flags,DummyItemID;};
struct CharShipmentContainerEntry{uint32 ID,GarrTypeID,GarrBuildingType,BaseCapacity;};
template<class T>struct Store{std::map<uint32,T> rows;T const* LookupEntry(uint32 id){auto i=rows.find(id);return i==rows.end()?nullptr:&i->second;}};
Store<CharShipmentEntry> sCharShipmentStore;Store<CharShipmentContainerEntry> sCharShipmentContainerStore;
namespace DB2Manager{using Rows=std::multimap<uint32,CharShipmentEntry const*>;using ShipmentConteinerMapBounds=std::pair<Rows::iterator,Rows::iterator>;}
struct DB2{DB2Manager::Rows rows;auto GetShipmentConteinerBounds(uint32 id){return rows.equal_range(id);}} sDB2Manager;
uint32 idxShipment(CharShipmentContainerEntry const* c){return c->GarrTypeID==GARRISON_TYPE_GARRISON?c->GarrBuildingType:c->ID;}
namespace WorldPackets{namespace Garrison{
struct Shipment{uint64 ShipmentID=0;uint32 ShipmentRecID=0,BuildingTypeID=0,CreationTime=0,end=0,ShipmentDuration=0;int DbState=0;bool finished=false;};
struct CreateShipmentResponse{uint32 ShipmentRecID=0;uint64 ShipmentID=0;int Result=0;CreateShipmentResponse* Write(){return this;}};
}}
struct Garrison;
struct GameObject{struct Info{int type=GAMEOBJECT_TYPE_GARRISON_SHIPMENT;struct{uint32 ShipmentContainer=129;}garrisonShipment;}info;
 struct Loot{std::vector<int> items;}loot;int refreshes=0;Info const* GetGOInfo(){return &info;}void SetGoState(int){}void ForceValuesUpdateAtIndex(int){++refreshes;}};
using Loot=GameObject::Loot;
struct Player{Garrison* g=nullptr;uint32 cls=9;std::map<uint32,QuestStatus> statuses;std::vector<uint32> credits;int packets=0;
 Garrison* GetGarrison(){return g;}uint32 getClass(){return cls;}QuestStatus GetQuestStatus(uint32 id){return statuses.count(id)?statuses[id]:QUEST_STATUS_NONE;}
 bool CheckShipment(CharShipmentEntry const*);void CastSpell(Player*,uint32,bool);void KilledMonsterCredit(uint32 id){credits.push_back(id);}
 template<class T>void SendDirectMessage(T){++packets;}void GetNearGameObjectListInGrid(std::list<GameObject*>&,float){}float GetVisibilityRange(){return 90;}
 bool IsPlayer(){return true;}Player* ToPlayer(){return this;}};
struct GarrShipment{uint32 ShipmentID=0,ContainerID=129,questReq=41797;uint32 selectShipment(Player*)const;};
struct GarrBuildingEntry{uint32 ShipmentCapacity=4;};struct BuildingStore{GarrBuildingEntry row;GarrBuildingEntry const* LookupEntry(uint32){return &row;}} sGarrBuildingStore;
struct Statement{void setUInt64(int,uint64){}} databaseStatement;
using CharacterDatabasePreparedStatement=Statement;
struct DB{int deletes=0;Statement* GetPreparedStatement(int){return &databaseStatement;}void Execute(Statement*){++deletes;}} CharacterDatabase;
struct Mgr{uint64 next=1;uint64 GenerateShipmentDbId(){return next++;}} sGarrisonMgr;
using ShipmentSet=std::list<WorldPackets::Garrison::Shipment>;
struct Garrison{Player* _owner=nullptr;std::set<uint32> talents;std::map<uint32,ShipmentSet> _shipments;std::vector<uint32> followers;
 struct Plot{struct{struct Packet{uint32 GarrBuildingID=0;}*PacketInfo=nullptr;}BuildingInfo;};
 Plot* GetPlotWithBuildingType(uint32){return nullptr;}uint32 GetShipmentMaxMod(){return 0;}bool hasTallent(uint32 id){return talents.count(id)!=0;}
 uint32 GetSpecialSpawnBuildingTime(uint32){return 0;}void SetBuildingData(uint32,int,uint32){}
 void AddFollower(uint32 id){followers.push_back(id);}void CreateGarrisonShipment(uint32);uint64 PlaceShipment(uint32,uint32,uint32=0,uint64=0);
 void CompleteShipments(GameObject*);void FreeShipmentChest(uint32);void UpdateReady();};
#include "Select.inc"
#include "Place.inc"
#include "Create.inc"
#include "Free.inc"
#include "Collect.inc"
#include "Update.inc"
#include "Check.inc"
struct Effect{int MiscValue=0;};struct SpellInfo{Effect effect;Effect const* GetEffect(int,int){return &effect;}};
struct Spell{int effectHandleMode=SPELL_EFFECT_HANDLE_HIT_TARGET,m_diffMode=0;Player* unitTarget=nullptr;SpellInfo* m_spellInfo=nullptr;
 void EffectCreateGarrisonShipment(SpellEffIndex);void EffectKillCreditPersonal(SpellEffIndex);};
#include "EffectCreate.inc"
#include "EffectCredit.inc"
std::map<uint32,uint32> createEffects,creditEffects,costs;
void Player::CastSpell(Player*,uint32 id,bool){SpellInfo info;Spell spell;spell.unitTarget=this;spell.m_spellInfo=&info;
 if(createEffects.count(id)){info.effect.MiscValue=createEffects[id];spell.EffectCreateGarrisonShipment(0);}
 if(creditEffects.count(id)){info.effect.MiscValue=creditEffects[id];spell.EffectKillCreditPersonal(0);}}
int main(int argc,char**argv){assert(argc==2);std::ifstream f(argv[1]);assert(f);std::string line;
 while(std::getline(f,line)){std::istringstream s(line);char kind;s>>kind;
  if(kind=='S'){CharShipmentEntry row;s>>row.ID>>row.ContainerID>>row.Duration>>row.SpellID>>row.OnCompleteSpellID>>row.GarrFollowerID>>row.MaxShipments>>row.Flags>>row.DummyItemID;assert(s);sCharShipmentStore.rows[row.ID]=row;}
  else if(kind=='C'){CharShipmentContainerEntry row;s>>row.ID>>row.GarrTypeID>>row.GarrBuildingType>>row.BaseCapacity;sCharShipmentContainerStore.rows[row.ID]=row;}
  else if(kind=='E'){uint32 id,type,misc;s>>id>>type>>misc;if(type==216)createEffects[id]=misc;if(type==90)creditEffects[id]=misc;}
  else if(kind=='R'){uint32 id,currency,amount;s>>id>>currency>>amount;assert(currency==1220);costs[id]=amount;}
  else assert(false);
 }
 assert(sCharShipmentStore.rows.size()==3&&sCharShipmentContainerStore.rows.at(129).GarrTypeID==3);
 auto tutorial=sCharShipmentStore.LookupEntry(321),normal=sCharShipmentStore.LookupEntry(255),advanced=sCharShipmentStore.LookupEntry(300);
 assert(tutorial&&normal&&advanced&&tutorial->Duration==5&&tutorial->MaxShipments==1&&tutorial->GarrFollowerID==649);
 assert(normal->Duration==1800&&advanced->Duration==1800&&normal->MaxShipments==4&&advanced->GarrFollowerID==738);
 for(auto& row:sCharShipmentStore.rows){assert(!row.second.DummyItemID&&costs.at(row.second.SpellID)==20&&creditEffects.at(row.second.OnCompleteSpellID)==106208);}
 std::vector<uint32> order{255,300,321};unsigned permutations=0;
 do{sDB2Manager.rows.clear();for(auto id:order)sDB2Manager.rows.emplace(129,sCharShipmentStore.LookupEntry(id));
  Player p;Garrison g;g._owner=&p;p.g=&g;GarrShipment chooser;
  for(auto status:{QUEST_STATUS_NONE,QUEST_STATUS_COMPLETE,QUEST_STATUS_FAILED}){p.statuses[41797]=status;assert(!chooser.selectShipment(&p));}
  p.statuses[41797]=QUEST_STATUS_INCOMPLETE;assert(chooser.selectShipment(&p)==321);g.talents.insert(362);assert(chooser.selectShipment(&p)==321);
  p.statuses[41797]=QUEST_STATUS_REWARDED;assert(chooser.selectShipment(&p)==300);g.talents.clear();assert(chooser.selectShipment(&p)==255);
  p.statuses[41797]=QUEST_STATUS_INCOMPLETE;uint32 selected=chooser.selectShipment(&p);assert(p.CheckShipment(sCharShipmentStore.LookupEntry(selected)));
  p.CastSpell(&p,sCharShipmentStore.LookupEntry(selected)->SpellID,false);assert(g._shipments[129].size()==1&&g._shipments[129].front().end==GameTime::now+5);
  GameObject chest;g.UpdateReady();g.CompleteShipments(&chest);assert(g.followers.empty()&&p.credits.empty()&&g._shipments[129].size()==1);
  GameTime::now+=5;g.UpdateReady();assert(g._shipments[129].front().finished);g.CompleteShipments(&chest);
  assert(g.followers==std::vector<uint32>{649}&&p.credits==std::vector<uint32>{106208}&&g._shipments[129].empty());
  g.CompleteShipments(&chest);assert(g.followers.size()==1&&p.credits.size()==1);
  // A saved 30-minute order from the old selector remains collectible without
  // rewriting its timestamps or granting the quest/follower ahead of time.
  Player saved;Garrison old;old._owner=&saved;saved.g=&old;old.PlaceShipment(255,100,1900,77);GameTime::now=1900;old.UpdateReady();old.CompleteShipments(&chest);
  assert(old.followers==std::vector<uint32>{649}&&saved.credits==std::vector<uint32>{106208}&&old._shipments[129].empty());
  ++permutations;
 }while(std::next_permutation(order.begin(),order.end()));
 assert(permutations==6);std::cout<<"PASS: native DB2 durations/currency/followers, all six selector orders, five-second creation, readiness, collection/credit/cleanup once, saved-order recovery and normal/upgraded selection\n";
}
