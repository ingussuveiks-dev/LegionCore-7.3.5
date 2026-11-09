// Full extracted death callbacks/condition case/login transport branch.
// External world/quest/cast/conversation endpoints below are controlled models.
#include <algorithm>
#include <cassert>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <map>
#include <list>
#include <string>
#include <vector>
using uint8=uint8_t;using uint32=uint32_t;using uint64=uint64_t;
struct ObjectGuid{using LowType=uint64;uint64 value=0;static ObjectGuid Empty;explicit operator bool()const{return value!=0;}uint64 GetCounter()const{return value;}};ObjectGuid ObjectGuid::Empty;
enum{QUEST_STATUS_INCOMPLETE=1,SPELL_CAST_OK=0,TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT=1,TEMPSUMMON_MANUAL_DESPAWN=2,TEXT_GENERIC_1=1,TEXT_GENERIC_2=2,TEXT_GENERIC_3=3,UNIT_STATE_STUNNED=1,ITEM_FIELD_ARTIFACT_XP=0,CONDITION_ARTIFACT_POWER=57,f_transguid=0};
int randomValue=0;int ModelRand(){return randomValue;}uint32 urand(uint32 lo,uint32 hi){return std::min(hi,uint32(std::max(int(lo),randomValue)));}
struct Player;struct Creature;struct Item{uint64 xp=(uint64(1)<<32)+17;bool equipped=true;bool IsEquipped(){return equipped;}uint64 GetUInt64Value(int){return xp;}};
struct Record{int credits=0,casts=0,talk=0,summons=0,moves=0,despawns=0,kills=0,unstuns=0,conversations=0;std::vector<uint32>creditIds,spellIds;bool failSummon=false,failCast=false,failConversation=false;}rec;
struct Motion{template<class...T>void MovePoint(T...){++rec.moves;}}motion;
struct DummyAI{template<class...T>void Talk(T...){++rec.talk;}}dummyAI;
struct Unit{Player*player=nullptr;Player*owner=nullptr;uint64 id=1;Player*ToPlayer(){return player;}Player*GetCharmerOrOwnerPlayerOrPlayerItself(){return owner?owner:player;}ObjectGuid GetGUID(){return ObjectGuid{id};}template<class...T>int CastSpell(Unit*,uint32 spell,T...){++rec.casts;rec.spellIds.push_back(spell);return rec.failCast?1:SPELL_CAST_OK;}Creature*SummonCreature(uint32,...);static Creature*GetCreature(Unit&,ObjectGuid);};
struct Player:Unit{int quest=QUEST_STATUS_INCOMPLETE;Item artifact;Player(){player=this;}int GetQuestStatus(uint32){return quest;}void KilledMonsterCredit(uint32 id,ObjectGuid=ObjectGuid::Empty){++rec.credits;rec.creditIds.push_back(id);}void RewardPlayerAndGroupAtEvent(uint32 id,Unit*source){if(source)KilledMonsterCredit(id);}Item*GetItemByEntry(uint32){return &artifact;}Item*GetArtifactWeapon(){return &artifact;}};
std::map<uint64,Creature*>world;std::vector<Player*>nearby;
struct Creature:Unit{DummyAI*AI(){return &dummyAI;}Motion*GetMotionMaster(){return &motion;}void GetPlayerListInGrid(std::list<Player*>&out,float){out.assign(nearby.begin(),nearby.end());}void DespawnOrUnsummon(int){++rec.despawns;}void Kill(Creature*,bool){++rec.kills;}void RemoveAura(uint32){++rec.unstuns;}void ClearUnitState(int){++rec.unstuns;}void SetWalk(bool){}}summoned;
Creature*Unit::SummonCreature(uint32,...){++rec.summons;return rec.failSummon?nullptr:&summoned;}Creature*Unit::GetCreature(Unit&,ObjectGuid g){auto i=world.find(g.value);return i==world.end()?nullptr:i->second;}
namespace ObjectAccessor{Creature*GetCreature(Unit&u,ObjectGuid g){return Unit::GetCreature(u,g);}}
struct TextMgr{template<class...T>void SendChat(T...){++rec.talk;}}textMgr;auto sCreatureTextMgr=&textMgr;
enum class HighGuid{Conversation};struct Generator{uint64 Generate(){return 999;}}generator;struct ObjectMgr{template<HighGuid>Generator*GetGenerator(){return &generator;}}objectMgr;auto sObjectMgr=&objectMgr;
struct Conversation{bool CreateConversation(uint64,uint32,Player*,void*,Player&){++rec.conversations;return !rec.failConversation;}};
struct BaseAI{Creature*me;explicit BaseAI(Creature*p):me(p){}virtual~BaseAI()=default;virtual void JustDied(Unit*){}template<class...T>void Talk(T...){++rec.talk;}};
struct Field{uint64 value=0;uint64 GetUInt64(){return value;}};
struct Movement{struct Transport{int resets=0;void Reset(){++resets;}}transport;};
std::string fallbackLog;void LogError(char const*,char const*format,...){char buffer[512];va_list args;va_start(args,format);vsnprintf(buffer,sizeof(buffer),format,args);va_end(args);fallbackLog=buffer;}
#define TC_LOG_ERROR(...) LogError(__VA_ARGS__)
#define UI64FMTD "%llu"
#define rand ModelRand
#include "Selected.inc"
void Check(bool value,char const*why){if(!value){std::cerr<<why<<'\n';std::abort();}}
int main(){
#if defined(CASE_ARTIFACTCONDITION)
 Player p;ConditionFrame c;
#ifdef COUNTERFACTUAL
 Check(c.Meets(&p),"old high AP wraps under max100");
#else
 Check(!c.Meets(&p),"high AP must fail max100");c.ConditionValue1=128943;Check(!c.Meets(&p),"named artifact same64bit bound");p.artifact.equipped=false;Check(c.Meets(&p),"unequipped named item retains native zero convention");p.artifact.equipped=true;c.ConditionValue3=0;c.ConditionValue2=100;Check(c.Meets(&p),"unbounded minimum");p.artifact.xp=99;Check(!c.Meets(&p),"minimum native comparison");Unit npc;Check(!c.Meets(&npc),"nonplayer refused");
#endif
#elif defined(CASE_TRANSPORT)
 TransportFrame t;t.fields[0].value=uint64(1)<<32;t.Load();
#ifdef COUNTERFACTUAL
 Check(!t.bind,"old high-only counter loses saved transport presence");
#else
 Check(t.bind&&fallbackLog.find("4294967296")!=std::string::npos&&fallbackLog.find("1099511627993")!=std::string::npos,"native unsupported transport fallback sees and logs full counters");for(auto map:{1554u,1557u,1460u}){TransportFrame supported;supported.mapId=map;supported.fields[0].value=uint64(1)<<32;Check(!supported.Load()&&!supported.bind&&supported.m_movementInfo.transport.resets==1,"native tutorial reconnect cleanup retained");}TransportFrame zero;Check(!zero.Load()&&!zero.bind,"ordinary zero counter unchanged");
#endif
#else
 Player p,q;Unit pet;pet.owner=&p;Creature actor,tyranna,leryssa,arlos;world={{1,&tyranna},{2,&leryssa},{3,&arlos}};nearby={&p,&q};DeathFrame ai(&actor);ai.JustDied(&pet);
#ifdef COUNTERFACTUAL
 Check(rec.credits==0&&rec.casts==0&&rec.conversations==0,"original pet blow loses player continuation");
#else
#if defined(CASE_PROLIFICA)
 Check(rec.conversations==1&&rec.summons==3&&rec.moves==3,"owner receives original conversation and actor outro");
#else
 Check(rec.credits>0,"owner receives original credit");
#endif
 rec={};ai.TyrannaGuid=ObjectGuid{1};ai.JustDied(nullptr);
#if defined(CASE_TALBOT)
 Check(rec.credits==0&&rec.kills==1&&rec.moves==1,"null killer retains actual actor cleanup only");
#elif defined(CASE_BELIASH)
 Check(rec.credits==0&&rec.casts==0&&rec.despawns==1,"null killer retains Tyranna cleanup");
#else
 Check(rec.credits==0&&rec.casts==0&&rec.conversations==0,"null killer refuses player progression");
#endif
 Unit unowned;rec={};ai.JustDied(&unowned);Check(rec.credits==0&&rec.casts==0&&rec.conversations==0,"unowned killer refuses player progression");rec={};ai.JustDied(&p);
#if defined(CASE_PROLIFICA)
 Check(rec.conversations==1&&rec.summons==3,"ordinary player outro unchanged");rec={};rec.failConversation=rec.failSummon=true;ai.JustDied(&pet);Check(rec.conversations==1&&rec.summons==3&&rec.moves==0,"failed optional conversation/summons no invalid movement");
#elif defined(CASE_TALBOT)
 Check(rec.credits==1&&rec.kills==1&&rec.moves==1,"ordinary native group event and actors unchanged");world.erase(2);rec={};ai.JustDied(&pet);Check(rec.credits==0,"missing required actors retain refusal");
#else
 Check(rec.credits>0,"ordinary player credit retained");
#endif
#if defined(CASE_NETHERWEB) || defined(CASE_NERUBAR)
 p.quest=0;rec={};ai.JustDied(&pet);Check(rec.credits==0&&rec.casts==0&&rec.summons==0,"inactive/rewarded quest gate retained");p.quest=QUEST_STATUS_INCOMPLETE;rec={};rec.failCast=rec.failSummon=true;ai.JustDied(&pet);Check(rec.credits==0,"failed real rescue action cannot grant rescue credit");randomValue=90;rec={};ai.JustDied(&pet);Check(rec.credits==0,"native nonrescue RNG branch retained");
#endif
#endif
#endif
 std::cout<<"PASS extracted complete callback/condition/login branch;synthetic inputs,modeled world/cast/group endpoints;not full-core/client traversal\n";
}
