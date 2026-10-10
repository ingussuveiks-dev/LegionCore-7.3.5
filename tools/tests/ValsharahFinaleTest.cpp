#include <cassert>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
using uint8=uint8_t;using uint64=uint64_t;using QuestStatus=int;
#include <set>
#include <string>
#include <vector>
using uint32=uint32_t;using int32=int32_t;using ObjectGuid=unsigned;using DamageEffectType=int;
enum {QUEST_STATUS_NONE=0,QUEST_STATUS_COMPLETE=1,QUEST_STATUS_INCOMPLETE=3,TEMPSUMMON_MANUAL_DESPAWN=0,REACT_PASSIVE=0,UNIT_FIELD_NPC_FLAGS=0,UNIT_NPC_FLAG_QUESTGIVER=2,UNIT_STATE_CASTING=1};
#include "QuestStatusData.inc"
struct Position {float x=0,y=0,z=0,o=0;float GetPositionX()const{return x;}float GetPositionY()const{return y;}float GetPositionZ()const{return z;}float GetOrientation()const{return o;}};
struct Player;struct Creature;struct ScriptedAI;
struct Unit {Position pos;Player* controller=nullptr;virtual ~Unit()=default;virtual Player* ToPlayer(){return nullptr;}Player* GetCharmerOrOwnerPlayerOrPlayerItself(){return ToPlayer()?ToPlayer():controller;}float GetDistance(Position const& p)const{return std::sqrt((pos.x-p.x)*(pos.x-p.x)+(pos.y-p.y)*(pos.y-p.y)+(pos.z-p.z)*(pos.z-p.z));}float GetDistance(Unit const* u)const{return GetDistance(u->pos);}Position GetPosition(){return pos;}};
struct TempSummon {Player* owner;Unit* GetSummoner();};
struct Motion {Position dest;unsigned moves=0,clears=0;void Clear(){++clears;}void MovePoint(unsigned,Position p){dest=p;++moves;}};
std::map<unsigned,Creature*> world;unsigned nextGuid=100;
struct Creature:Unit {unsigned entry,guid;bool alive=true,despawned=false,casting=false,combat=false;TempSummon summon;ScriptedAI* ai=nullptr;Motion motion;Creature(unsigned e,Player* p=nullptr):entry(e),guid(++nextGuid),summon{p}{if(p)world[guid]=this;}TempSummon* ToTempSummon(){return summon.owner?&summon:nullptr;}unsigned GetEntry(){return entry;}bool IsAlive(){return alive&&!despawned;}void DespawnOrUnsummon(unsigned=0){despawned=true;}void SetReactState(unsigned){}void RemoveFlag(unsigned,unsigned){}Motion* GetMotionMaster(){return &motion;}bool HasUnitState(unsigned){return casting;}ScriptedAI* AI(){return ai;}};
struct Player:Unit {unsigned guid,map=1220;bool alive=true,teleport=false,movie=false,combat=false,vehicle=false,teleportSuccess=true;std::map<unsigned,int> quests;std::map<std::pair<unsigned,unsigned>,int> objectives;std::set<unsigned> rewarded;std::map<unsigned,std::vector<unsigned>> summons;std::vector<unsigned> credits,spells;std::vector<std::function<void()>> delayed;
 std::map<unsigned,QuestStatusData> attempts;
 QuestStatusData* getQuestStatus(unsigned q){if(!quests[q])return nullptr;auto& status=attempts[q];status.Status=quests[q];return &status;}
 void RemoveActiveQuest(unsigned q){quests.erase(q);attempts.erase(q);}
 void AddQuestDelayedEvent(uint32,uint64,std::function<void()>&&);
 Creature* staticEnder=nullptr;bool visible=true;Creature* FindNearestCreature(unsigned e,float){assert(e==104921);return staticEnder;}bool canSeeOrDetect(Creature*){return visible;}
 Player(unsigned g):guid(g){}Player* ToPlayer()override{return this;}bool IsAlive(){return alive;}unsigned GetMapId(){return map;}bool IsBeingTeleported(){return teleport;}bool isWatchingMovie(){return movie;}bool isInCombat(){return combat;}bool GetVehicle(){return vehicle;}unsigned GetGUID(){return guid;}int GetQuestStatus(unsigned q){return quests[q];}bool GetQuestRewardStatus(unsigned q){return rewarded.count(q)!=0;}int GetQuestObjectiveData(unsigned q,unsigned o){return objectives[{q,o}];}std::vector<unsigned>* GetSummonList(unsigned e){return &summons[e];}
 Creature* SummonCreature(unsigned e,Position p,unsigned type,unsigned time,unsigned veh,unsigned personal){assert(type==0&&time==0&&veh==0&&personal==guid);auto c=new Creature(e,this);c->pos=p;summons[e].push_back(c->guid);return c;}
 void KilledMonsterCredit(unsigned e){credits.push_back(e);for(auto const& q:quests)if(q.second==3)objectives[{q.first,e}]=1;if(quests[38743]==3&&objectives[{38743,104799}]&&objectives[{38743,93065}])quests[38743]=1;}
 void CastSpell(Player* target,unsigned spell,bool trig){assert(target==this&&trig);spells.push_back(spell);if(spell==208446){KilledMonsterCredit(104764);if(quests[38687]==3)quests[38687]=1;if(quests[41763]==3)quests[41763]=1;}}
 void AddDelayedEvent(uint64,std::function<void()> fn){delayed.push_back(fn);}void Next(){assert(!delayed.empty());auto fn=delayed.front();delayed.erase(delayed.begin());fn();}
 bool TeleportTo(unsigned m,float x,float y,float z,float o){if(!teleportSuccess)return false;map=m;pos={x,y,z,o};teleport=true;return true;}
};
#include "QuestDelayedEvent.inc"
Unit* TempSummon::GetSummoner(){return owner;}
struct ObjectAccessor{static Creature* GetCreature(Player&,unsigned g){auto i=world.find(g);return i!=world.end()&&!i->second->despawned?i->second:nullptr;}};
struct ScriptedAI{Creature* me;std::vector<unsigned> casts;unsigned melee=0;ScriptedAI(Creature* c):me(c){c->ai=this;}virtual ~ScriptedAI()=default;virtual void UpdateAI(unsigned){}virtual void sGossipSelect(Player*,unsigned,unsigned){}virtual void DoAction(int){}virtual void OnSpellClick(Unit*){}virtual void DamageTaken(Unit*,unsigned&,DamageEffectType){}virtual void EnterEvadeMode(){}virtual void JustDied(Unit*){}bool UpdateVictim(){return me->combat;}void DoCast(unsigned s){casts.push_back(s);}void DoMeleeAttackIfReady(){++melee;}};
struct SmartAI:ScriptedAI{using ScriptedAI::ScriptedAI;};
struct SpellScene{unsigned MiscValue;};struct Quest{unsigned id;unsigned GetQuestId()const{return id;}};
struct SceneTriggerScript{SceneTriggerScript(char const*){}virtual bool OnTrigger(Player*,SpellScene const*,std::string){return false;}};
struct PlayerScript{PlayerScript(char const*){}virtual void OnUpdate(Player*,unsigned){}virtual void OnLogout(Player*){}virtual void OnMapChanged(Player*){}virtual void OnQuestReward(Player*,Quest const*){}};
#define RegisterCreatureAI(x) ((void)0)
#include "ValFinale.inc"
int main(){
 using namespace ValsharahFinale;
 for(unsigned quest:{38687u,41763u}){
  Player p{quest},other{1};p.quests[quest]=3;other.quests[quest]=3;Creature starter{104728};p.pos=starter.pos={3320.36f,5958.03f,250.16f,0};npc_valsharah_search_tyrande startAI(&starter);
  startAI.sGossipSelect(&p,19419,0);assert(!Existing(&p,104728));startAI.sGossipSelect(&p,19419,1);auto escort=Existing(&p,104728);assert(escort&&p.credits.empty());npc_valsharah_search_tyrande ai(escort);
  ai.UpdateAI(1000);assert(escort->motion.moves==1&&p.credits.empty());p.pos=Entrance;ai.UpdateAI(1000);assert(p.credits.empty());escort->pos=Entrance;ai.UpdateAI(1000);assert(p.credits==std::vector<unsigned>{104799});ai.UpdateAI(1000);
  scene_valsharah_choice scene;SpellScene sceneId{1246};scene.OnTrigger(&p,&sceneId,"complete");assert(!p.GetQuestObjectiveData(quest,104764));
  for(unsigned i=0;i<3;++i){unsigned e=Illusions[quest==41763][i];auto illusion=Existing(&p,e);assert(illusion);npc_valsharah_malfurion_search clickAI(illusion);other.pos=illusion->pos;clickAI.OnSpellClick(&other);assert(other.credits.empty());p.pos=Entrance;clickAI.OnSpellClick(&p);assert(!p.GetQuestObjectiveData(quest,e));p.pos=illusion->pos;clickAI.OnSpellClick(&p);assert(p.GetQuestObjectiveData(quest,e));auto count=p.credits.size();clickAI.OnSpellClick(&p);assert(p.credits.size()==count);}
  p.pos=Found;p.pos.z+=100;ai.UpdateAI(1000);assert(!ai.sceneStarted);p.pos=Found;ai.UpdateAI(1000);assert(ai.sceneStarted&&!p.GetQuestObjectiveData(quest,104764));auto spells=p.spells.size();ai.UpdateAI(1000);assert(p.spells.size()==spells);scene.OnTrigger(&p,&sceneId,"complete");assert(p.GetQuestStatus(quest)==1&&p.GetQuestObjectiveData(quest,104764));
 }
 Player p{2};p.quests[38743]=3;Creature departure{104799};p.pos=departure.pos={3547.35f,6420.47f,168.439f,0};npc_valsharah_temple_departure departureAI(&departure);
 p.combat=true;departureAI.sGossipSelect(&p,19474,0);assert(p.delayed.empty());p.combat=false;p.teleportSuccess=false;departureAI.sGossipSelect(&p,19474,0);assert(p.delayed.empty());p.teleportSuccess=true;departureAI.sGossipSelect(&p,19474,0);assert(p.credits.empty());p.Next();assert(p.credits.empty()&&!Existing(&p,93065));p.teleport=false;p.Next();assert(p.credits==std::vector<unsigned>{104799}&&Existing(&p,93065));auto boss=Existing(&p,93065);TempleRecovery(&p);assert(Existing(&p,93065)==boss);
 npc_valsharah_ysera_finale bossAI(boss);boss->combat=true;bossAI.UpdateAI(15000);assert(bossAI.casts==std::vector<unsigned>({208292,190406})&&bossAI.melee==1&&p.GetQuestStatus(38743)==3);bossAI.EnterEvadeMode();assert(boss->despawned&&p.GetQuestStatus(38743)==3);TempleRecovery(&p);boss=Existing(&p,93065);assert(boss);npc_valsharah_ysera_finale retryAI(boss);Player outsider{3};unsigned damage=1;retryAI.DamageTaken(&outsider,damage,0);retryAI.JustDied(&outsider);assert(p.GetQuestStatus(38743)==3);
 Unit pet;pet.controller=&p;retryAI.DamageTaken(&pet,damage,0);retryAI.JustDied(&pet);assert(p.GetQuestStatus(38743)==1&&p.spells.back()==194213);p.movie=true;p.Next();assert(!Existing(&p,104921));player_valsharah_finale hooks;p.movie=false;hooks.OnUpdate(&p,1000);assert(Existing(&p,104921));hooks.OnUpdate(&p,1000);assert(p.summons[104921].size()==1);Quest reward{38743};p.rewarded.insert(38743);hooks.OnQuestReward(&p,&reward);assert(!Existing(&p,104921));
 Player resume{4};resume.quests[41763]=3;resume.objectives[{41763,104799}]=1;resume.pos=Searches[1];hooks.OnUpdate(&resume,1000);assert(Existing(&resume,104728)&&resume.credits.empty());hooks.OnLogout(&resume);assert(!Existing(&resume,104728));resume.map=1;hooks.OnUpdate(&resume,1000);assert(!Existing(&resume,104728));
 Player missing{5};missing.quests[38687]=3;missing.pos=Found;missing.objectives[{38687,104799}]=1;for(unsigned e:Illusions[0])missing.objectives[{38687,e}]=1;auto controller=Spawn(&missing,104728,Entrance);npc_valsharah_search_tyrande fallback(controller);fallback.UpdateAI(1000);assert(fallback.sceneStarted);fallback.UpdateAI(90000);assert(missing.GetQuestStatus(38687)==1);
 Player done{6};done.quests[38743]=1;done.pos=Temple;Creature staticEnder{104921};done.staticEnder=&staticEnder;TempleRecovery(&done);assert(!Existing(&done,104921));done.visible=false;TempleRecovery(&done);assert(Existing(&done,104921));
 Player dead{7};dead.quests[38743]=3;dead.objectives[{38743,104799}]=1;dead.pos=Temple;dead.alive=false;hooks.OnUpdate(&dead,1000);assert(!Existing(&dead,93065));dead.alive=true;hooks.OnUpdate(&dead,1000);auto retry=Existing(&dead,93065);assert(retry&&dead.credits.empty());npc_valsharah_ysera_finale deathAI(retry);dead.alive=false;deathAI.UpdateAI(1000);assert(retry->despawned);dead.alive=true;hooks.OnUpdate(&dead,1000);assert(Existing(&dead,93065)!=retry);dead.map=1;hooks.OnMapChanged(&dead);assert(!Existing(&dead,93065));
 Player wrongArrival{8};wrongArrival.quests[38743]=3;wrongArrival.pos=Found;Arrive(&wrongArrival,0);assert(wrongArrival.credits.empty()&&!Existing(&wrongArrival,93065));
 Player abandoned{90};abandoned.quests[38743]=3;abandoned.pos=departure.pos;StartBattle(&abandoned,&departure);assert(abandoned.delayed.size()==1);abandoned.RemoveActiveQuest(38743);abandoned.quests[38743]=3;abandoned.teleport=false;abandoned.Next();assert(abandoned.credits.empty()&&!Existing(&abandoned,93065));
 Player duringRetry{91};duringRetry.quests[38743]=3;duringRetry.pos=departure.pos;StartBattle(&duringRetry,&departure);duringRetry.Next();assert(duringRetry.delayed.size()==1);duringRetry.RemoveActiveQuest(38743);duringRetry.quests[38743]=3;duringRetry.teleport=false;duringRetry.Next();assert(duringRetry.credits.empty());
 for(auto const& item:world)if(item.second->summon.owner)delete item.second;
 std::cout<<"PASS: actual production search/finale handlers; both factions, physical follow and clicks, scene guard, callback recovery, teleport acknowledgment, real combat/pet participation, wipe retry, personal isolation and persistent completion recovery.\n";
}
