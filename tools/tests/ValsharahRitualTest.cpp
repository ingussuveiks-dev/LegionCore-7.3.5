#include <cassert>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>
using uint32=uint32_t;using int32=int32_t;using ObjectGuid=unsigned;using DamageEffectType=int;
enum {QUEST_STATUS_NONE=0,QUEST_STATUS_COMPLETE=1,QUEST_STATUS_INCOMPLETE=3,TEMPSUMMON_MANUAL_DESPAWN=0,REACT_PASSIVE=0,UNIT_FIELD_NPC_FLAGS=0,UNIT_NPC_FLAG_QUESTGIVER=2,UNIT_NPC_FLAG_GOSSIP=1,UNIT_STATE_CASTING=1};
struct Position {float x=0,y=0,z=0,o=0;float GetPositionX()const{return x;}float GetPositionY()const{return y;}float GetPositionZ()const{return z;}float GetOrientation()const{return o;}};
struct Player;struct Creature;struct ScriptedAI;
struct Unit {Position pos;Player* controller=nullptr;virtual ~Unit()=default;virtual Player* ToPlayer(){return nullptr;}virtual Creature* ToCreature(){return nullptr;}Player* GetCharmerOrOwnerPlayerOrPlayerItself(){return ToPlayer()?ToPlayer():controller;}float GetDistance(Position const& p)const{return std::sqrt((pos.x-p.x)*(pos.x-p.x)+(pos.y-p.y)*(pos.y-p.y)+(pos.z-p.z)*(pos.z-p.z));}float GetDistance(Unit const* u)const{return GetDistance(u->pos);}Position GetPosition(){return pos;}};
struct TempSummon {Unit* owner;Unit* GetSummoner();};
struct Motion {Position dest;unsigned moves=0,clears=0;void Clear(){++clears;}void MovePoint(unsigned,Position p){dest=p;++moves;}};
std::map<unsigned,Creature*> world;unsigned nextGuid=100;
struct Creature:Unit {unsigned entry,guid;bool alive=true,despawned=false,casting=false,combat=false;TempSummon summon;ScriptedAI* ai=nullptr;Motion motion;Creature(unsigned e,Unit* p=nullptr):entry(e),guid(++nextGuid),summon{p}{if(p)world[guid]=this;}TempSummon* ToTempSummon(){return summon.owner?&summon:nullptr;}Creature* ToCreature()override{return this;}unsigned GetGUID(){return guid;}unsigned GetEntry(){return entry;}bool IsAlive(){return alive&&!despawned;}void DespawnOrUnsummon(unsigned=0);void AddPlayerInPersonnalVisibilityList(unsigned){}void Mount(unsigned m){assert(m==67955);}void SetWalk(bool){}void RemoveAurasDueToSpell(unsigned){}Creature* SummonCreature(unsigned,Position,unsigned,unsigned,unsigned,unsigned);void SetReactState(unsigned){}void RemoveFlag(unsigned,unsigned){}Motion* GetMotionMaster(){return &motion;}bool HasUnitState(unsigned){return casting;}ScriptedAI* AI(){return ai;}};
struct Player:Unit {unsigned guid,map=1220;bool alive=true,teleport=false,movie=false,combat=false,vehicle=false,teleportSuccess=true;std::map<unsigned,int> quests;std::map<std::pair<unsigned,unsigned>,int> objectives;std::set<unsigned> rewarded;std::map<unsigned,std::vector<unsigned>> summons;std::vector<unsigned> credits,spells;std::vector<std::function<void()>> delayed;
 Creature* staticEnder=nullptr;bool visible=true;Creature* FindNearestCreature(unsigned e,float){assert(e==104921);return staticEnder;}bool canSeeOrDetect(Creature*){return visible;}
 Player(unsigned g):guid(g){}Player* ToPlayer()override{return this;}bool IsAlive(){return alive;}unsigned GetMapId(){return map;}bool IsBeingTeleported(){return teleport;}bool isWatchingMovie(){return movie;}bool isInCombat(){return combat;}bool GetVehicle(){return vehicle;}unsigned GetGUID(){return guid;}int GetQuestStatus(unsigned q){return quests[q];}bool GetQuestRewardStatus(unsigned q){return rewarded.count(q)!=0;}int GetQuestObjectiveData(unsigned q,unsigned o){return objectives[{q,o}];}std::vector<unsigned>* GetSummonList(unsigned e){return &summons[e];}
 Creature* SummonCreature(unsigned e,Position p,unsigned type,unsigned time,unsigned veh,unsigned personal){assert(type==0&&time==0&&veh==0&&personal==guid);auto c=new Creature(e,this);c->pos=p;summons[e].push_back(c->guid);return c;}
 void KilledMonsterCredit(unsigned e){credits.push_back(e);for(auto const& q:quests)if(q.second==3)objectives[{q.first,e}]=1;if(quests[38743]==3&&objectives[{38743,104799}]&&objectives[{38743,93065}])quests[38743]=1;}
 void CastSpell(Player* target,unsigned spell,bool trig){assert(target==this&&trig);spells.push_back(spell);if(spell==197487){KilledMonsterCredit(92742);quests[38377]=1;}if(spell==208446){KilledMonsterCredit(104764);if(quests[38687]==3)quests[38687]=1;if(quests[41763]==3)quests[41763]=1;}}
 unsigned lastDelay=0;void AddDelayedEvent(unsigned time,std::function<void()> fn){lastDelay=time;delayed.push_back(fn);}void Next(){assert(!delayed.empty());auto fn=delayed.front();delayed.erase(delayed.begin());fn();}
 bool TeleportTo(unsigned m,float x,float y,float z,float o){if(!teleportSuccess)return false;map=m;pos={x,y,z,o};teleport=true;return true;}
};
Unit* TempSummon::GetSummoner(){return owner;}
struct ObjectAccessor{static Creature* GetCreature(Player&,unsigned g){auto i=world.find(g);return i!=world.end()&&!i->second->despawned?i->second:nullptr;}};
struct ScriptedAI{Creature* me;std::vector<unsigned> casts;unsigned melee=0;ScriptedAI(Creature* c):me(c){c->ai=this;}virtual ~ScriptedAI()=default;virtual void UpdateAI(unsigned){}virtual void sGossipSelect(Player*,unsigned,unsigned){}virtual void DoAction(int){}virtual void OnSpellClick(Unit*){}virtual void DamageTaken(Unit*,unsigned&,DamageEffectType){}virtual void EnterEvadeMode(){}virtual void JustDied(Unit*){}virtual void JustSummoned(Creature*){}virtual void SummonedCreatureDespawn(Creature*){}virtual void SummonedCreatureDies(Creature*,Unit*){}void AttackStart(Player*){me->combat=true;}void Talk(unsigned,unsigned){}bool UpdateVictim(){return me->combat;}void DoCast(unsigned s){casts.push_back(s);}void DoMeleeAttackIfReady(){++melee;}};
struct SmartAI:ScriptedAI{using ScriptedAI::ScriptedAI;};
struct SpellScene{unsigned MiscValue;};struct Quest{unsigned id;unsigned GetQuestId()const{return id;}};
struct SceneTriggerScript{SceneTriggerScript(char const*){}virtual bool OnTrigger(Player*,SpellScene const*,std::string){return false;}};
struct PlayerScript{PlayerScript(char const*){}virtual void OnUpdate(Player*,unsigned){}virtual void OnLogout(Player*){}virtual void OnMapChanged(Player*){}virtual void OnQuestReward(Player*,Quest const*){}};
#define RegisterCreatureAI(x) ((void)0)
#include <set>
void Creature::DespawnOrUnsummon(unsigned){if(despawned)return;despawned=true;if(summon.owner)if(auto parent=summon.owner->ToCreature())if(parent->ai)parent->ai->SummonedCreatureDespawn(this);}
std::vector<ScriptedAI*> allocated;
unsigned failSummonEntry=0;
Creature* Creature::SummonCreature(unsigned e,Position p,unsigned,unsigned,unsigned,unsigned personal){if(e==failSummonEntry)return nullptr;auto c=new Creature(e,this);c->pos=p;allocated.push_back(new ScriptedAI(c));assert(personal!=0);if(ai)ai->JustSummoned(c);return c;}
struct SummonList {std::set<unsigned> ids;SummonList(Creature*){}void Summon(Creature* c){ids.insert(c->guid);}void Despawn(Creature* c){ids.erase(c->guid);}void DespawnAll(){auto copy=ids;for(auto id:copy)world[id]->DespawnOrUnsummon();ids.clear();}};
#include "ValRitual.inc"
int main(){
 using namespace ValsharahRituals;
 Player ritual{1};ritual.pos=Grove;StartRitual(&ritual);assert(ritual.delayed.empty());ritual.quests[38377]=3;StartRitual(&ritual);assert(ritual.credits.empty()&&ritual.delayed.size()==1&&ritual.lastDelay==30000);ritual.alive=false;ritual.Next();assert(ritual.credits.empty());ritual.alive=true;StartRitual(&ritual);ritual.pos.z+=100;ritual.Next();assert(ritual.credits.empty());ritual.pos=Grove;StartRitual(&ritual);ritual.Next();assert(ritual.credits==std::vector<unsigned>{92742}&&ritual.spells.back()==197487);FinishRitual(&ritual);assert(ritual.credits.size()==1);
 player_valsharah_rituals hooks;
 for(unsigned q:{38675u,41724u}){
  Player p{q};p.quests[q]=3;p.pos=EscortStart;hooks.OnUpdate(&p,500);auto escort=Existing(&p,103022);assert(escort);npc_valsharah_path_tyrande ai(escort);hooks.OnUpdate(&p,500);assert(p.summons[103022].size()==1);ai.UpdateAI(500);assert(escort->motion.moves==1&&p.credits.empty());
  for(unsigned i=0;i<6;++i){escort->pos=Path[i];p.pos=Path[i];p.pos.z+=100;ai.UpdateAI(500);assert(ai.point==i&&p.credits.empty());p.pos=Path[i];ai.UpdateAI(500);}
  assert(p.credits==std::vector<unsigned>{103022}&&escort->despawned);ai.UpdateAI(500);assert(p.credits.size()==1);
 }
 for(unsigned q:{41708u,41890u}){
  Player p{q},other{2};p.quests[q]=3;other.quests[q]=3;p.pos=VigilStart;Creature source{104739};source.pos=VigilStart;npc_valsharah_vigil_tyrande sourceAI(&source);sourceAI.sGossipSelect(&p,19405,0);assert(!Existing(&p,104739));sourceAI.sGossipSelect(&p,19405,1);auto vigil=Existing(&p,104739);assert(vigil);npc_valsharah_vigil_tyrande ai(vigil);sourceAI.sGossipSelect(&p,19405,1);assert(p.summons[104739].size()==1);ai.UpdateAI(1000);assert(ai.alive.empty());vigil->pos=Prayer;ai.UpdateAI(1000);assert(ai.phase==1&&ai.wait==8000);ai.UpdateAI(7999);assert(ai.alive.empty());ai.UpdateAI(1);assert(ai.alive.size()==1&&p.credits.empty());
  Creature unrelated{104646};ai.SummonedCreatureDies(&unrelated,&p);assert(ai.wave==0&&p.credits.empty());auto first=world[*ai.alive.begin()];assert(Owner(first)==&p);ai.SummonedCreatureDies(first,&p);ai.SummonedCreatureDespawn(first);assert(ai.wave==1&&ai.alive.empty());ai.UpdateAI(10000);assert(ai.alive.size()==2);auto a=world[*ai.alive.begin()];auto b=world[*ai.alive.rbegin()];ai.SummonedCreatureDies(a,&p);assert(ai.wave==1&&ai.alive.size()==1);ai.UpdateAI(60000);assert(ai.wave==1&&p.credits.empty());ai.SummonedCreatureDies(b,&p);assert(ai.wave==2);ai.UpdateAI(10000);assert(ai.alive.size()==1);auto last=world[*ai.alive.begin()];ai.SummonedCreatureDies(last,&p);assert(ai.phase==2&&p.credits.empty());ai.UpdateAI(1000);assert(p.credits.empty());vigil->pos=VigilStart;p.pos=Prayer;p.pos.z+=100;ai.UpdateAI(1000);assert(p.credits.empty());p.pos=VigilStart;ai.UpdateAI(1000);assert(p.credits==std::vector<unsigned>{103022}&&other.credits.empty()&&ai.stopping);
 }
 Player retry{3};retry.quests[41708]=3;retry.pos=VigilStart;Creature source{104739};source.pos=VigilStart;StartVigil(&retry,&source);auto vigil=Existing(&retry,104739);npc_valsharah_vigil_tyrande ai(vigil);vigil->pos=Prayer;ai.UpdateAI(1);ai.UpdateAI(8000);auto enemy=world[*ai.alive.begin()];enemy->DespawnOrUnsummon();assert(ai.stopping&&vigil->despawned&&retry.credits.empty());StartVigil(&retry,&source);auto second=Existing(&retry,104739);assert(second&&second!=vigil);npc_valsharah_vigil_tyrande restarted(second);second->pos=Prayer;restarted.UpdateAI(1);restarted.UpdateAI(8000);retry.alive=false;restarted.UpdateAI(1);assert(restarted.stopping&&restarted.alive.empty()&&retry.credits.empty());
 retry.alive=true;StartVigil(&retry,&source);auto third=Existing(&retry,104739);npc_valsharah_vigil_tyrande logout(third);third->pos=Prayer;logout.UpdateAI(1);logout.UpdateAI(8000);hooks.OnLogout(&retry);assert(logout.stopping&&logout.summons.ids.empty());
 Player escortRetry{4};escortRetry.quests[41724]=3;escortRetry.pos=EscortStart;hooks.OnUpdate(&escortRetry,500);auto escort=Existing(&escortRetry,103022);npc_valsharah_path_tyrande escortAI(escort);escortRetry.map=1;hooks.OnMapChanged(&escortRetry);assert(escort->despawned&&escortRetry.credits.empty());escortRetry.map=1220;hooks.OnUpdate(&escortRetry,500);assert(Existing(&escortRetry,103022)!=escort);
 Player failed{5};failed.quests[41708]=3;failed.pos=VigilStart;StartVigil(&failed,&source);auto fail=Existing(&failed,104739);npc_valsharah_vigil_tyrande failure(fail);fail->pos=Prayer;failure.UpdateAI(1);failSummonEntry=104643;failure.UpdateAI(8000);assert(failure.stopping&&failure.alive.empty()&&failed.credits.empty());failSummonEntry=0;
 for(auto c:allocated)delete c;for(auto const& item:world)delete item.second;
 std::cout<<"PASS: production ritual delay, no dead/distant/duplicate credit, both faction escorts with owner at every waypoint, personal death-gated 1/2/1 waves, unrelated/departed enemies, no bystander credit, wipe/logout/map recovery.\n";
}
