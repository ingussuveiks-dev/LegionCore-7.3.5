#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <vector>
using uint32=uint32_t; using uint64=uint64_t; using int8=int8_t;
using SpellEffIndex=int; using DamageEffectType=int; using SpellCastResult=int;
enum { QUEST_STATUS_INCOMPLETE=1, QUEST_STATUS_COMPLETE=2, QUEST_STATUS_REWARDED=3,
 TEMPSUMMON_TIMED_DESPAWN=1, REACT_PASSIVE=0, REACT_AGGRESSIVE=1, FOLLOW_MOTION_TYPE=4, POINT_MOTION_TYPE=8,
 UNIT_FIELD_FLAGS=1, UNIT_FLAG_NON_ATTACKABLE=2, SPELL_CAST_OK=0, SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW=1,
 SPELL_FAILED_OUT_OF_RANGE=2, EFFECT_0=0, EFFECT_1=1, SPELL_EFFECT_REMOVE_AURA=164, SPELL_EFFECT_APPLY_AURA=6,
 SPELL_EFFECT_ANY=999, SPELL_EFFECT_DUMMY=3, SPELL_EFFECT_TRIGGER_SPELL=64 };
struct ObjectGuid { unsigned value=0; bool operator<(ObjectGuid b)const{return value<b.value;}bool operator==(ObjectGuid b)const{return value==b.value;} };
using GuidList=std::vector<ObjectGuid>;
struct Position { float x=0,y=0,z=0,o=0;float GetPositionX()const{return x;}float GetPositionY()const{return y;}float GetPositionZ()const{return z;} };
struct Player;struct Creature;struct TempSummon;
struct Unit {
 ObjectGuid guid;Position pos;unsigned map=1220;bool alive=true;
 virtual ~Unit()=default;virtual Player* ToPlayer(){return nullptr;}virtual Creature* ToCreature(){return nullptr;}
 virtual Player* GetCharmerOrOwnerPlayerOrPlayerItself(){return ToPlayer();}
 ObjectGuid GetGUID()const{return guid;}bool IsAlive()const{return alive;}
 float GetDistance(Position p){return std::sqrt((pos.x-p.x)*(pos.x-p.x)+(pos.y-p.y)*(pos.y-p.y)+(pos.z-p.z)*(pos.z-p.z));}
 bool IsWithinDistInMap(Unit* u,float r){return u && map==u->map && GetDistance(u->pos)<=r;}
 Position GetPosition(){return pos;}float GetPositionX(){return pos.x;}float GetPositionY(){return pos.y;}float GetPositionZ(){return pos.z;}
};
struct Motion { unsigned type=0,moves=0,id=0;Position destination;
 void MovePoint(unsigned p,float x,float y,float z,bool=true){type=POINT_MOTION_TYPE;id=p;destination={x,y,z,0};++moves;}
 void MovePoint(unsigned p,Position const& where){MovePoint(p,where.x,where.y,where.z);}
 void MoveFollow(Player*,float,float){type=FOLLOW_MOTION_TYPE;++moves;}unsigned GetCurrentMovementGeneratorType(){return type;}
};
struct CreatureAI { virtual ~CreatureAI()=default;virtual void Reset(){}virtual void DamageTaken(Unit*,uint32&,DamageEffectType){}virtual void UpdateAI(uint32){}virtual void MovementInform(uint32,uint32){}virtual void PassengerBoarded(Unit*,int8,bool){} };
struct Creature:Unit {
 unsigned entry=0,expiry=0,flags=0;uint64 spawn=0,health=100,maxHealth=100;float multiplier=1;ObjectGuid owner,viewer;
 bool removed=false;Motion motion;Creature* ToCreature()override{return this;}virtual TempSummon* ToTempSummon(){return nullptr;}
 unsigned GetEntry(){return entry;}uint64 GetDBTableGUIDLow(){return spawn;}
 void DespawnOrUnsummon(unsigned delay=0){if(delay)expiry=delay;else removed=true;}
 void SetReactState(int){}void SetDisableGravity(bool){}void SetCanFly(bool){}void CombatStop(bool){}
 void SetFlag(unsigned,unsigned value){flags|=value;}void RemoveFlag(unsigned,unsigned value){flags&=~value;}
 uint64 GetHealth(Unit*){return health;}uint64 GetMaxHealth(Unit*){return maxHealth;}float GetHealthMultiplierForTarget(Unit*){return multiplier;}
 Motion* GetMotionMaster(){return &motion;}
};
struct TempSummon:Creature {TempSummon* ToTempSummon()override{return this;}ObjectGuid GetSummonerGUID(){return owner;}Unit* GetSummoner();};
static std::map<ObjectGuid,TempSummon> creatures;static std::map<ObjectGuid,Player*> players;static unsigned serial=100;
struct Menu { unsigned id=0;unsigned GetMenuId(){return id;} };
struct TalkClass { Menu menu;Menu& GetGossipMenu(){return menu;}void SendCloseGossip(){} };
struct QuestObjective { unsigned ID=284778,ObjectID=107995; };
struct Quest { std::vector<QuestObjective> objectives{{}};auto const& GetObjectives()const{return objectives;} };
struct QuestStore { Quest quest;Quest const* GetQuestTemplate(unsigned id){assert(id==37862);return &quest;} } questStore;
auto sQuestDataStore=&questStore;
struct Player:Unit {
 Player(){guid={++serial};players[guid]=this;}~Player(){players.erase(guid);}Player* ToPlayer()override{return this;}
 unsigned zone=7334;bool los=true,failedSpawn=false;Creature* vehicle=nullptr;TalkClass talk;TalkClass* PlayerTalkClass=&talk;
 std::map<unsigned,int> quests;std::map<std::pair<unsigned,unsigned>,unsigned> objectives;std::map<unsigned,unsigned> items;
 std::map<unsigned,GuidList> summons;std::vector<unsigned> credits,casts;
 unsigned GetGUIDLow(){return guid.value;}unsigned GetMapId(){return map;}unsigned GetZoneId(){return zone;}
 int GetQuestStatus(unsigned q){return quests[q];}unsigned GetQuestObjectiveData(unsigned q,unsigned id){return objectives[{q,id}];}
 bool HasItemCount(unsigned id,unsigned n){return items[id]>=n;}void DestroyItemCount(unsigned id,unsigned n,bool){assert(items[id]>=n);items[id]-=n;}
 bool IsWithinLOSInMap(Unit*){return los;}GuidList* GetSummonList(unsigned id){return &summons[id];}
 Creature* GetVehicle(){return vehicle;}Creature* GetVehicleBase(){return vehicle;}void ExitVehicle(Position const* p){vehicle=nullptr;pos=*p;}
 void KilledMonsterCredit(unsigned id,ObjectGuid={}){credits.push_back(id);unsigned q=0;switch(id){case 90315:q=37853;break;case 90880:q=42271;break;case 90167:q=37859;break;case 90372:case 90479:q=37957;break;case 90406:q=37857;break;case 100386:q=37860;break;case 91155:case 108721:q=42756;break;default:assert(false);}++objectives[{q,id}];}
 void SetQuestObjectiveData(Quest const*,QuestObjective const* o,unsigned n){objectives[{37862,o->ObjectID}]=n;}
 TempSummon* SummonCreature(unsigned id,Position where,int,unsigned duration,int,ObjectGuid viewer){if(failedSpawn)return nullptr;ObjectGuid next{++serial};auto& c=creatures[next];c.guid=next;c.entry=id;c.pos=where;c.map=map;c.owner=guid;c.viewer=viewer;c.expiry=duration;summons[id].push_back(next);return &c;}
 void CastSpell(float,float,float,unsigned id,bool){casts.push_back(id);}void CastSpell(Creature*,unsigned id,bool){casts.push_back(id);}
 Creature* FindNearestCreature(unsigned id,float r){for(auto& pair:creatures)if(!pair.second.removed && pair.second.entry==id && IsWithinDistInMap(&pair.second,r))return &pair.second;return nullptr;}
};
Unit* TempSummon::GetSummoner(){auto it=players.find(owner);return it==players.end()?nullptr:it->second;}
struct ObjectAccessor {static Creature* GetCreature(Player& p,ObjectGuid id){auto it=creatures.find(id);return it==creatures.end()||it->second.removed||it->second.map!=p.map?nullptr:&it->second;}};
struct ScriptedAI:CreatureAI {Creature* me;ScriptedAI(Creature* c):me(c){}void Talk(unsigned){}bool UpdateVictim(){return true;}void DoMeleeAttackIfReady(){} };
struct PlayerScript {PlayerScript(char const*){}virtual ~PlayerScript()=default;virtual void OnLogout(Player*){}virtual void OnMapChanged(Player*){}virtual void OnUpdate(Player*,unsigned){} };
struct CreatureScript {CreatureScript(char const*){}virtual ~CreatureScript()=default;virtual bool OnGossipSelect(Player*,Creature*,unsigned,unsigned){return false;}virtual CreatureAI* GetAI(Creature*)const{return nullptr;} };
struct SpellInfo {unsigned Id=0;};struct Hook {unsigned count=0;void operator+=(int){++count;} };
struct SpellScript {Unit* caster=nullptr;Unit* target=nullptr;Creature* hit=nullptr;SpellInfo info;std::set<int> prevented;
 virtual ~SpellScript()=default;virtual void Register(){}Unit* GetCaster(){return caster;}Unit* GetExplTargetUnit(){return target;}Creature* GetHitCreature(){return hit;}SpellInfo const* GetSpellInfo(){return &info;}
 void PreventHitDefaultEffect(int i){prevented.insert(i);}Hook OnCheckCast,OnEffectHitTarget,OnEffectHit,OnEffectLaunch,OnEffectLaunchTarget;
};
#define PrepareSpellScript(x) public:
#define SpellCheckCastFn(...) 0
#define SpellEffectFn(...) 0
#define RegisterSpellScript(x) ((void)0)
#define RegisterCreatureAI(x) ((void)0)
struct Database {
 std::map<std::tuple<unsigned,unsigned,uint64>,unsigned> rows;
 bool PQuery(char const*,unsigned g,unsigned q,unsigned long long s){return rows.count({g,q,uint64(s)})!=0;}
 void DirectPExecute(char const*,unsigned g,unsigned q,unsigned progress){for(auto i=rows.begin();i!=rows.end();)if(std::get<0>(i->first)==g && std::get<1>(i->first)==q && i->second>progress)i=rows.erase(i);else ++i;}
 void DirectPExecute(char const*,unsigned g,unsigned q,unsigned long long s,unsigned n){assert(!rows.count({g,q,uint64(s)}));rows[{g,q,uint64(s)}]=n;}
} CharacterDatabase;

#include "../../src/server/scripts/Legion/azurewing_repose.cpp"

Creature* Static(unsigned entry,Position pos,uint64 spawn=0){ObjectGuid id{++serial};auto& c=creatures[id];c.guid=id;c.entry=entry;c.pos=pos;c.spawn=spawn;return &c;}
int main()
{
 using namespace Azurewing;
 spell_azurewing_pylon pylon;pylon.Register();assert(pylon.OnEffectLaunch.count==1 && pylon.OnEffectLaunchTarget.count==1 && pylon.OnEffectHit.count==0);
 spell_azurewing_whelp_pickup pickup;pickup.Register();assert(pickup.OnEffectLaunch.count==1 && pickup.OnEffectLaunchTarget.count==1);
 Player p,other;p.pos={634,6679,54,0};other.pos=p.pos;
 Static(89975,p.pos);spell_azurewing_pool pool;pool.caster=&p;p.quests[37853]=QUEST_STATUS_INCOMPLETE;
 assert(pool.Check()!=SPELL_CAST_OK);p.items[122095]=6;p.los=false;assert(pool.Check()!=SPELL_CAST_OK);p.los=true;
 assert(pool.Check()==SPELL_CAST_OK);pool.Throw(0);assert(Done(&p,37853,90315) && p.items[122095]==0 && p.casts.back()==179913);
 p.items[122095]=6;pool.Throw(0);assert(p.items[122095]==6);pool.SuppressMissingTrigger(1);assert(pool.prevented.count(1));
 p.quests[42271]=other.quests[42271]=QUEST_STATUS_INCOMPLETE;
 Creature* whelp=Static(90880,p.pos,338304);assert(!Revive(&p,whelp,180713));
 for(Player* player:{&p,&other}){player->items[122292]=1;player->items[122306]=4;}
 Creature* portable=Personal(&p,91037,p.pos);pickup.caster=&other;pickup.target=portable;other.items[122292]=0;assert(pickup.Check()!=SPELL_CAST_OK);
 pickup.caster=&p;p.items[122292]=0;assert(pickup.Check()==SPELL_CAST_OK);p.items[122292]=other.items[122292]=1;assert(pickup.Check()!=SPELL_CAST_OK);
 assert(Revive(&p,whelp,180713));assert(!Revive(&p,whelp,180713));assert(Revive(&other,whelp,180713));
 // Persistence is in the database, not a process-local duplicate set.
 player_azurewing_recovery recovery;recovery.OnLogout(&p);assert(!Revive(&p,whelp,180713));
 p.objectives[{42271,90880}]=0;assert(Revive(&p,whelp,180713));assert((other.objectives[{42271,90880}]==1));
 Creature* next=Static(90880,p.pos,338305);p.los=false;assert(!Revive(&p,next,180713));p.los=true;next->pos.x+=100;assert(!Revive(&p,next,180713));
 p.quests[37859]=QUEST_STATUS_INCOMPLETE;Creature* drained=Static(90167,p.pos,266388);p.items[122188]=1;
 assert(Revive(&p,drained,180463));assert(!Revive(&p,drained,180463));assert(!Revive(&p,whelp,180463));
 spell_azurewing_revive spell;spell.caster=&p;spell.target=drained;spell.info.Id=180463;spell.Credit(1);assert(spell.prevented.count(1));
 // A scaled one-shot surrenders; an ordinary smaller hit does not.
 Creature runas;runas.health=100;runas.maxHealth=100;runas.multiplier=100;p.quests[37957]=QUEST_STATUS_INCOMPLETE;
 npc_azurewing_runas_duel duel(&runas);uint32 damage=1000;duel.DamageTaken(&p,damage,0);assert(damage==1000 && !Done(&p,37957,90372));
 damage=1000000000;duel.DamageTaken(&p,damage,0);assert(damage==0 && Done(&p,37957,90372));damage=100;duel.DamageTaken(&p,damage,0);assert(damage==0);
 duel.Reset();assert(!(runas.flags&UNIT_FLAG_NON_ATTACKABLE));
 recovery.OnUpdate(&p,1000);assert(Owned(&p,90476) && !Done(&p,37957,90479));
 Creature* follower=Owned(&p,90476);p.pos=EscortHome;follower->pos=EscortHome;Static(89978,EscortHome);
 p.los=false;recovery.OnUpdate(&p,1000);assert(!Done(&p,37957,90479));p.los=true;recovery.OnUpdate(&p,1000);assert(Done(&p,37957,90479));assert(!Owned(&p,90476));
 // Failed summon and distant owner cannot complete a guide route.
 p.quests[37857]=QUEST_STATUS_INCOMPLETE;p.talk.menu.id=18200;npc_azurewing_runas_start start;Creature giver;giver.pos=p.pos;
 p.failedSpawn=true;start.OnGossipSelect(&p,&giver,0,0);assert(!Owned(&p,90406));p.failedSpawn=false;start.OnGossipSelect(&p,&giver,0,0);
 Creature* guide=Owned(&p,90406);assert(guide && guide->viewer==p.guid);npc_azurewing_runas_guide guideAI(guide);
 for(unsigned i=0;i<10;++i){p.pos=guide->pos;guideAI.UpdateAI(1000);assert(guideAI.moving);guide->pos=guide->motion.destination;guideAI.MovementInform(POINT_MOTION_TYPE,i+1);}
 p.pos.x+=100;guideAI.UpdateAI(1000);assert(!Done(&p,37857,90406));p.pos=guide->pos;guideAI.UpdateAI(1000);assert(Done(&p,37857,90406));
 // Optional flight remains available for a completed but un-rewarded quest.
 p.quests[37862]=QUEST_STATUS_COMPLETE;p.talk.menu.id=18196;giver.pos=p.pos;npc_azurewing_stellagosa_return flight;
 flight.OnGossipSelect(&p,&giver,0,0);Creature* ride=Owned(&p,107995);assert(ride && !Done(&p,37862,107995));
 npc_azurewing_stellagosa_return::AI flightAI(ride);flightAI.UpdateAI(1000);assert(ride->motion.moves==0);
 flightAI.PassengerBoarded(&other,0,true);assert(!flightAI.boarded);p.vehicle=ride;flightAI.PassengerBoarded(&p,0,true);
 for(unsigned i=0;i<11;++i){flightAI.UpdateAI(1000);assert(flightAI.moving);ride->pos=ride->motion.destination;p.pos=ride->pos;flightAI.MovementInform(POINT_MOTION_TYPE,i+1);assert(!Done(&p,37862,107995));}
 flightAI.UpdateAI(1000);assert(Done(&p,37862,107995) && !p.vehicle && p.pos.z==75.0f);
 // Leaving mid-flight and failed boarding cannot grant the optional objective.
 other.quests[37862]=QUEST_STATUS_INCOMPLETE;Creature* interrupted=Personal(&other,107995,other.pos);
 npc_azurewing_stellagosa_return::AI interruptedAI(interrupted);other.vehicle=interrupted;interruptedAI.PassengerBoarded(&other,0,true);
 interruptedAI.UpdateAI(1000);other.vehicle=nullptr;interruptedAI.PassengerBoarded(&other,0,false);interruptedAI.UpdateAI(5000);
 assert(!Done(&other,37862,107995) && interrupted->removed);
 Creature* unboarded=Personal(&other,107995,other.pos);npc_azurewing_stellagosa_return::AI unboardedAI(unboarded);
 unboardedAI.UpdateAI(5000);assert(unboarded->removed && !Done(&other,37862,107995));
 // Each player's final encounter requires two real, separate kill credits.
 p.quests[42756]=other.quests[42756]=QUEST_STATUS_INCOMPLETE;p.pos=other.pos=Orbyth;
 recovery.OnUpdate(&p,1000);recovery.OnUpdate(&other,1000);assert(Owned(&p,91155) && Owned(&other,91155) && !Owned(&p,108721));
 assert(!Done(&p,42756,91155));Owned(&p,91155)->alive=false;p.KilledMonsterCredit(91155);
 recovery.OnUpdate(&p,1000);assert(Owned(&p,108721) && !Owned(&other,108721) && !Done(&p,42756,108721));
 p.quests[42756]=0;recovery.OnUpdate(&p,1000);assert(!Owned(&p,108721) && Owned(&other,91155));
 recovery.OnLogout(&other);assert(!Owned(&other,91155));
 std::cout<<"PASS: production Azurewing pool, persistent distinct revivals, scaled surrender, actual escort arrival, boarded optional flight and sequential per-player finale.\n";
}
