// Complete selected production callbacks; external world/group/cast endpoints modeled.
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <set>
#include <vector>
#include <mutex>
#include <thread>
using uint32=uint32_t;using uint64=uint64_t;using uint8=uint8_t;
enum{TYPEID_UNIT=3,TYPEID_PLAYER=4,QUEST_STATUS_INCOMPLETE=3,TEMPSUMMON_TIMED_DESPAWN=1,TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT=2,TEXT_GENERIC_2=2,CREATURE_SUMMON_GROUP_RESET=1,CREATURE_SUMMON_GROUP_COMBAT=2,DONE=3,ENCOUNTER_FRAME_DISENGAGE=1,ENCOUNTER_FRAME_INSTANCE_END=2,ENCOUNTER_FRAME_SET_ALLOWING_RELEASE=3,UNIT_FIELD_EMOTE_STATE=0,SPEED_CHARGE=42,SPELL_CAST_OK=0,ALLIANCE=0,HORDE=1};
enum class AnimTier{Ground};
#include "Constants.inc"
struct ObjectGuid{uint64 value=0;static ObjectGuid Empty;bool operator<(ObjectGuid const&o)const{return value<o.value;}};ObjectGuid ObjectGuid::Empty;
struct Player;struct Creature;struct Instance;
struct Record{int credits=0,casts=0,summons=0,talks=0,resets=0,moves=0,auras=0,groups=0,save=0,frames=0,scheduler=0,summonCleanup=0,areaCleanup=0,eventReset=0;bool failSummon=false;ObjectGuid eventPlayer;std::vector<uint32>spawned;}rec;
struct Unit{Player*player=nullptr;Unit*owner=nullptr;Player*charmer=nullptr;uint64 id=8;Player*ToPlayer(){return player;}bool IsPlayer(){return player!=nullptr;}uint32 GetTypeId(){return IsPlayer()?TYPEID_PLAYER:TYPEID_UNIT;}Unit*GetOwner(){return owner;}Player*GetCharmerOrOwnerPlayerOrPlayerItself(){return charmer?charmer:owner?owner->ToPlayer():player;}ObjectGuid GetGUID(){return ObjectGuid{id};}static Creature*GetCreature(Unit&,ObjectGuid);};
struct Player:Unit{int quest=QUEST_STATUS_INCOMPLETE;uint32 area=5087,team=ALLIANCE,failSpell=0,failRemaining=0;std::vector<uint32>castIds;uint32 GetCurrentAreaID(){return area;}uint32 GetTeam(){return team;}Player(){player=this;}int GetQuestStatus(uint32){return quest;}void KilledMonsterCredit(uint32,ObjectGuid=ObjectGuid::Empty){if(quest==QUEST_STATUS_INCOMPLETE)++rec.credits;}void GroupEventHappens(uint32,Unit*source){if(source&&quest==QUEST_STATUS_INCOMPLETE)++rec.credits;}void RewardPlayerAndGroupAtEvent(uint32,Unit*source){if(source&&quest==QUEST_STATUS_INCOMPLETE)++rec.credits;}template<class...T>int CastSpell(Unit*,uint32 spell,T...){++rec.casts;castIds.push_back(spell);if(spell==failSpell&&failRemaining){--failRemaining;return 1;}return SPELL_CAST_OK;}};
struct PlayerScript{explicit PlayerScript(char const*){}virtual~PlayerScript()=default;virtual void OnUpdate(Player*,uint32){}virtual void OnLogout(Player*){}};
#define CAST_PLR(x) static_cast<Player*>(x)
struct DummyAI{template<class...T>void Talk(T...){++rec.talks;}void EnterEvadeMode(){++rec.resets;}void SetGUID(ObjectGuid guid,int){rec.eventPlayer=guid;}}dummyAI;
struct Motion{template<class...T>void MoveCharge(T...){++rec.moves;}}motion;
std::map<uint64,Creature*>world;struct Map{Creature*GetCreature(ObjectGuid g){auto i=world.find(g.value);return i==world.end()?nullptr:i->second;}}map;
Creature*Unit::GetCreature(Unit&,ObjectGuid g){return map.GetCreature(g);}
struct Events{bool held=true,aborted=false,premature=false;void KillAllEvents(bool force){aborted=true;premature=force&&held;}void Reset(){++rec.eventReset;}};
struct Creature:Unit{uint32 entry=1;bool temp=true,nearby=true;Events m_Events;Creature*SummonCreature(uint32 id,...){++rec.summons;rec.spawned.push_back(id);return rec.failSummon?nullptr:this;}float GetOrientation(){return 0;}Creature*FindNearestCreature(uint32,float,bool=true){return nearby?this:nullptr;}static Creature*GetCreature(Creature&,ObjectGuid g){return map.GetCreature(g);}DummyAI*AI(){return &dummyAI;}Creature*ToTempSummon(){return temp?this:nullptr;}ObjectGuid GetSummonerGUID(){return ObjectGuid{1};}Map*GetMap(){return &map;}uint32 GetEntry(){return entry;}template<class...T>void SetUInt32Value(T...){}void SetAnimTier(AnimTier){}void SetCanFly(bool){}void RemoveAllAuras(){++rec.auras;}Motion*GetMotionMaster(){return &motion;}void SummonCreatureGroupDespawn(int){++rec.summonCleanup;}void RemoveAllAreaObjects(){++rec.areaCleanup;}};
struct Instance{bool ready=true;std::map<uint32,uint32>states;uint32 GetData(uint32){return ready;}void SetData(uint32 type,uint32 value){states[type]=value;}void SetBossState(uint32,uint32){++rec.groups;}void SaveToDB(){++rec.save;}template<class...T>void SendEncounterUnit(T...){++rec.frames;}void DoRemoveAurasDueToSpellOnPlayers(uint32){++rec.auras;}}instance;
struct TextMgr{template<class...T>void SendChat(T...){++rec.talks;}}textMgr;auto sCreatureTextMgr=&textMgr;
struct BaseAI{Creature*me;Instance*instance=&::instance;bool _creditBloodQuickening=true;ObjectGuid LordIllidanGUID{1};explicit BaseAI(Creature*c):me(c){}virtual~BaseAI()=default;virtual void JustDied(Unit*){}void _JustDied(){++rec.groups;}void CleanAuras(){++rec.auras;}template<class...T>void Talk(T...){++rec.talks;}};
struct BossAI{Creature*me;Instance*instance=&::instance;uint32 _bossId=4;Events events;struct Scheduler{void CancelAll(){++rec.scheduler;}}scheduler;struct Summons{void DespawnAll(){++rec.summonCleanup;}}summons;explicit BossAI(Creature*c):me(c){}void _JustDied();};
bool maxRng=false;uint32 urand(uint32 lo,uint32 hi){return maxRng?hi:lo;}
#include "Selected.inc"
void Check(bool condition,char const*why){if(!condition){std::cerr<<why<<'\n';std::abort();}}
int main(){Creature actor,parent;world={{1,&parent}};Player player;player.id=100;Unit pet;pet.owner=&player;pet.charmer=&player;
#if defined(CASE_BOSSDEATH)
 BossAI ai(&actor);ai._JustDied();
#ifdef COUNTERFACTUAL
 Check(actor.m_Events.premature,"old BossAI force deletes referenced spell event");
#else
 Check(actor.m_Events.aborted&&!actor.m_Events.premature,"abort without premature deletion");Check(rec.groups==1&&rec.save==1&&rec.frames==3&&rec.areaCleanup==1&&rec.summonCleanup==3&&rec.scheduler==1,"boss progression/cleanup retained");
#endif
#elif defined(CASE_HYJAL)
 player_hyjal_protectors_random script;Player a,b,outsider;a.id=100;b.id=200;outsider.id=300;b.team=HORDE;outsider.area=0;
#ifdef COUNTERFACTUAL
 script.summoned=false; // isolate sharing from the separate uninitialized-member defect
#endif
 script.OnUpdate(&a,5000);script.OnUpdate(&b,5000);
#ifdef COUNTERFACTUAL
 Check(a.castIds.size()==4&&b.castIds.empty(),"original global state blocks second player");script.OnUpdate(&outsider,5000);script.OnUpdate(&a,5000);Check(a.castIds.size()==8,"unrelated outside player resets original completed state");
#else
 Check(a.castIds.size()==4&&b.castIds.size()==4,"both factions have independent four-slot episodes");script.OnUpdate(&outsider,5000);script.OnUpdate(&a,5000);Check(a.castIds.size()==4,"unrelated outside player cannot replay accepted casts");
 Player retry;retry.id=400;retry.failSpell=96544;retry.failRemaining=1;script.OnUpdate(&retry,5000);Check(retry.castIds.size()==4,"initial four cast requests");script.OnUpdate(&retry,4999);Check(retry.castIds.size()==4,"retry not before5s");script.OnUpdate(&retry,1);Check(retry.castIds.size()==5&&retry.castIds.back()==96544,"only failed slot retried,selection retained");script.OnUpdate(&retry,5000);Check(retry.castIds.size()==5,"accepted slots exactly once");
 retry.quest=0;script.OnUpdate(&retry,1);Check(!script.progress.count(retry.GetGUID()),"inactive quest clears only own episode");retry.quest=QUEST_STATUS_INCOMPLETE;script.OnUpdate(&retry,5000);Check(retry.castIds.size()==9,"quest retry starts fresh episode");
 script.OnLogout(&a);Check(!script.progress.count(a.GetGUID())&&script.progress.count(b.GetGUID()),"logout removes only actual player");script.OnUpdate(&a,4999);Check(a.castIds.size()==4,"fresh episode starts with full authored delay");script.OnUpdate(&a,1);Check(a.castIds.size()==8,"fresh episode gets four cast slots");
 Player c,d;c.id=10001;d.id=10002;auto tick=[&](Player*p){for(unsigned i=0;i<100;++i)script.OnUpdate(p,5000);script.OnLogout(p);};std::thread first(tick,&c),second(tick,&d);first.join();second.join();Check(c.castIds.size()==4&&d.castIds.size()==4,"serialized shared-map access for independent players");
#endif
#elif defined(CASE_WEBBEDPOOL)
 DeathFrame ai(&actor);maxRng=true;ai.JustDied(&player);Check(rec.spawned.size()==1,"native nonrescue roll must summon");
#ifdef COUNTERFACTUAL
 Check(rec.spawned[0]==possibleSpawns[30]&&rec.spawned[0]!=possibleSpawns[31],"original pool excludes last authored entry");
#else
 Check(rec.spawned[0]==possibleSpawns[31],"last authored outcome reachable");
#endif
#else
 DeathFrame ai(&actor);
#if defined(CASE_SILVERHAND)
 actor.entry=SH_VICAR;
#endif
#if defined(CASE_TORLOTH)
 pet.owner=nullptr; // charm has no owner; native owner/charmer resolver still finds player.
#endif
 ai.JustDied(&pet);
#ifdef COUNTERFACTUAL
#if defined(CASE_GHOUL)
 Check(rec.summons==0,"original pet blow loses Darrowshire spirit");
#elif defined(CASE_DIMWIND)
 Check(rec.eventPlayer.value==0,"original pet blow fails player handoff");
#else
 Check(rec.credits==0,"original nonplayer blow loses player continuation");
#endif
#else
#if defined(CASE_GHOUL)
 Check(rec.summons==1,"pet player owner receives original spirit summon");
#elif defined(CASE_DIMWIND)
 Check(rec.eventPlayer.value==player.id,"summoner receives eligible player GUID,not pet");
#else
 Check(rec.credits==1,"eligible owner receives native credit");
#endif
 rec={};ai.JustDied(nullptr);Check(rec.credits==0&&rec.eventPlayer.value==0,"null killer no player progression");
#if defined(CASE_GHOUL)
 Check(rec.summons==0,"null killer no spirit");
#elif defined(CASE_TORLOTH)
 Check(rec.talks==1&&rec.resets==1,"null killer retains Illidan cleanup");
#elif defined(CASE_BLOODQUEEN)
 Check(rec.moves==1&&instance.states[DATA_BLOOD_QUICKENING_STATE]==DONE,"null killer retains Minchar/encounter cleanup");
#endif
 Unit unowned;rec={};ai.JustDied(&unowned);Check(rec.credits==0&&rec.eventPlayer.value==0,"unowned killer no player progression");rec={};ai.JustDied(&player);
#if defined(CASE_GHOUL)
 Check(rec.summons==1,"ordinary player spirit unchanged");
#elif defined(CASE_DIMWIND)
 Check(rec.eventPlayer.value==player.id,"ordinary player GUID unchanged");actor.temp=false;rec={};ai.JustDied(&pet);Check(rec.eventPlayer.value==0,"ordinary DB scamp cannot notify foreign summoner");actor.temp=true;world.clear();rec={};ai.JustDied(&pet);Check(rec.eventPlayer.value==0,"missing actual summoner safe");
#else
 Check(rec.credits==1,"ordinary player credit retained");
#endif
#if defined(CASE_WEBBED)
 rec={};rec.failSummon=true;ai.JustDied(&player);Check(rec.credits==0,"failed rescue summon cannot grant credit");maxRng=true;rec={};ai.JustDied(&pet);Check(rec.credits==0&&rec.summons==1,"nonrescue spawn unchanged");
#elif defined(CASE_BALEFUL)
 player.quest=0;rec={};ai.JustDied(&pet);Check(rec.credits==0&&rec.casts==0&&rec.talks==1,"active39049 prerequisite and independent death text retained");
#elif defined(CASE_SENTRY)
 actor.nearby=false;rec={};ai.JustDied(&pet);Check(rec.credits==0,"actual live golem prerequisite retained");actor.nearby=true;player.quest=0;rec={};ai.JustDied(&pet);Check(rec.credits==0,"both faction quest admission retained");
#elif defined(CASE_SILVERHAND)
 instance.ready=false;rec={};ai.JustDied(&pet);Check(rec.credits==0,"all-five completion gate retained");ai.instance=nullptr;rec={};ai.JustDied(&pet);Check(rec.credits==0,"missing instance safe");
#elif defined(CASE_BLOODQUEEN)
 ai._creditBloodQuickening=false;rec={};ai.JustDied(&pet);Check(rec.credits==0&&rec.moves==0,"inactive timed quest cannot grant extra credit");
#endif
#endif
#endif
 std::cout<<"PASS actual complete selected production callback under ASAN;synthetic/model endpoints;native/client scopes separate\n";
}
