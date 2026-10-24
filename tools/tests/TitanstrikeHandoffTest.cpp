#include <atomic>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <vector>
using uint32=uint32_t;
using uint8=uint8_t;
using uint64=uint64_t;
using int32=int32_t;
using CriteriaTypes=uint32;
using SpellCastResult=uint32;
enum {CLASS_HUNTER=3,QUEST_STATUS_NONE=0,QUEST_STATUS_COMPLETE=1,QUEST_STATUS_INCOMPLETE=3,
    SPELL_CAST_OK=0,UNIT_FIELD_NPC_FLAGS=7,UNIT_NPC_FLAG_SPELLCLICK=16777216,CRITERIA_TYPE_SCRIPT_EVENT_2=92,
    CONVERSATION_FIELD_LAST_LINE_END_TIME=7,SMART_ACTION_ADD_NPC_FLAG=81,SMART_ACTION_REMOVE_NPC_FLAG=83,SMART_ACTION_UPDATE_ACHIEVEMENT_CRITERIA=205};
#define TC_LOG_ERROR(...) (++errors)
#define TC_LOG_DEBUG(...) ((void)0)
uint32 errors=0,currentTime=0;
uint32 getMSTime(){return currentTime;}
struct ObjectGuid {
    uint64 identity=0;
    bool operator<(ObjectGuid const& other) const{return identity<other.identity;}
    bool operator==(ObjectGuid const& other) const{return identity==other.identity;}
};
#include "Scope.inc"
struct Player;
struct Creature;
struct Map {} activeMap,otherMap;
struct WorldObject {
    ObjectGuid guid;
    bool inWorld=true,player=false;
    uint32 phase=1;
    std::weak_ptr<QuestObjectScope> _questObjectScope;
    bool _questObjectBound=false;
    ObjectGuid GetGUID() const{return guid;}
    bool IsPlayer() const{return player;}
    bool IsInWorld() const{return inWorld;}
    bool InSamePhase(WorldObject const* other) const{return other&&phase==other->phase;}
    void AddPlayerInPersonnalVisibilityList(ObjectGuid){}
    void BindQuestObjectScope(std::shared_ptr<QuestObjectScope> const&);
    bool InheritQuestObjectScope(WorldObject const*);
    bool IsQuestObjectExpired() const;
    bool CanTargetQuestObject(WorldObject const*) const;
};
struct Unit:WorldObject {
    bool alive=true;
    Map* map=&activeMap;
    uint32 flags=0;
    std::set<uint32> auras;
    bool IsAlive() const{return alive;}
    bool HasAura(uint32 identity)const{return auras.count(identity)!=0;}
    Map* GetMap(){return map;}
    Unit* ToUnit(){return this;}
    virtual Player* ToPlayer(){return nullptr;}
    void SetFlag(uint32 field,uint32 value){assert(field==7);flags|=value;}
    void RemoveFlag(uint32 field,uint32 value){assert(field==7);flags&=~value;}
};
struct FakeAI {Creature* actor;uint32 calls=0;void SetData(uint32,uint32);};
struct Creature:Unit {
    bool IsAIEnabled=true,hasAI=true,nativeAura=true,castOk=true,applyAura=true;
    uint32 casts=0;
    FakeAI ai{this};
    FakeAI* AI(){return hasAI?&ai:nullptr;}
    SpellCastResult CastSpell(Creature* target,uint32 spell,bool triggered){assert(target==this&&spell==211970&&triggered);++casts;if(castOk&&applyAura)auras.insert(spell);return castOk?0:1;}
};
void FakeAI::SetData(uint32 identity,uint32 value){assert(identity==1&&value==1);++calls;if(actor->nativeAura)actor->auras.insert(211970);actor->RemoveFlag(7,UNIT_NPC_FLAG_SPELLCLICK);}
struct CriteriaProgress {uint64 Counter=1;};
struct Achievement {std::map<uint32,CriteriaProgress> progress{{48020,{1}}};auto GetCriteriaProgressMap(){return &progress;}};
struct Scenario {uint32 id=1099,step=2;Achievement achievement;uint32 GetScenarioId(){return id;}uint32 GetCurrentStep(){return step;}auto& GetAchievementMgr(){return achievement;}} scenario;
struct ScenarioMgr {bool exists=true;Scenario* GetScenario(uint32 instance){return exists&&instance==42?&scenario:nullptr;}} scenarios;
auto sScenarioMgr=&scenarios;
struct QuestStatusData {uint32 Status=3;std::shared_ptr<uint8_t> ScriptLifetime;std::shared_ptr<QuestObjectScope> ScriptObjects;};
struct Player:Unit {
    uint32 instance=42,mapId=1609,klass=3,credits=0;
    bool rewarded=false,contact=true,teleport=false,hasQuest=true,creditAdvances=true;
    QuestStatusData quest;
    Creature* hati=nullptr;
    Creature* liveBoss=nullptr;
    std::vector<std::function<void()>> delayed;
    Player(){player=true;guid={42};}
    Player* ToPlayer() override{return this;}
    uint32 GetGUIDLow(){return uint32(guid.identity);}
    bool CanContact(){return contact;}
    bool IsBeingTeleported(){return teleport;}
    uint32 GetMapId(){return mapId;}
    uint32 GetInstanceId(){return instance;}
    uint32 getClass(){return klass;}
    bool GetQuestRewardStatus(uint32 identity){assert(identity==42185);return rewarded;}
    uint32 GetQuestStatus(uint32 identity){assert(identity==42185);return hasQuest?quest.Status:0;}
    QuestStatusData* getQuestStatus(uint32 identity){assert(identity==42185);return hasQuest?&quest:nullptr;}
    Creature* FindNearestCreature(uint32 identity,float distance,bool aliveOnly){assert(distance==100&&aliveOnly);return identity==103154?hati:liveBoss;}
    int GetPosition(){return 0;}
    void AddDelayedEvent(uint64 delay,std::function<void()>&& action){assert(delay==1000);delayed.push_back(std::move(action));}
    void AddQuestDelayedEvent(uint32,uint64,std::function<void()>&&);
    void BindQuestObject(uint32,WorldObject*);
    void UpdateAchievementCriteria(uint32 type,uint32 asset,uint32 second,uint32 third,Unit* invoker,bool native){assert(type==92&&asset==50997&&!second&&!third&&native);(void)invoker;++credits;scenario.achievement.progress[48094]={1};if(creditAdvances)scenario.step=3;}
    void RunDelayed(){auto work=std::move(delayed);delayed.clear();for(auto& action:work)action();}
    void Abandon(){if(quest.ScriptObjects)quest.ScriptObjects->Cancel();quest={};hasQuest=false;}
};
struct SpellInfo {} info;
bool creationOk=true;
uint32 payloadDuration=73800;
std::vector<struct Conversation*> created;
struct Conversation:WorldObject {
    uint32 duration=0,deadline=0;
    ObjectGuid casterGUID;
    std::map<ObjectGuid,uint32> playing;
    Conversation(){inWorld=false;}
    virtual ~Conversation()=default;
    virtual void Update(uint32){}
    ObjectGuid GetCasterGUID(){return casterGUID;}
    int32 GetDuration(){return int32(duration);}
    void SetDuration(int32 value){duration=value;}
    uint32 GetUInt32Value(uint32 field) const{assert(field==7);return deadline;}
    void SetUInt32Value(uint32 field,uint32 value){assert(field==7);deadline=value;}
    void Remove(){inWorld=false;}
    bool GenerateDynamicFieldData(uint32 identity,Unit*,uint32& value){assert(identity==2167);value=payloadDuration-5000;return creationOk;}
    bool CreateConversation(uint64 identity,uint32 triggerEntry,Unit* caster,SpellInfo const*,int){
        casterGUID=caster->GetGUID();guid={identity};phase=caster->phase;
#include "Lifetime.inc"
        inWorld=true;created.push_back(this);return true;
    }
    bool CanNeverSee2(WorldObject const*)const;
};
Player* activePlayer=nullptr;
struct ObjectAccessor {static Player* GetPlayer(WorldObject const&,ObjectGuid identity){return activePlayer&&activePlayer->guid==identity&&activePlayer->inWorld?activePlayer:nullptr;}};
enum class HighGuid {Conversation};
struct Generator {uint64 counter=100;uint64 Generate(){return ++counter;}} generator;
struct ObjectMgr {template<HighGuid>Generator* GetGenerator(){return &generator;}} objects;
auto sObjectMgr=&objects;
#include "ObjectScope.inc"
#include "QuestDelay.inc"
#include "QuestBind.inc"
#include "Playback.inc"
#include "Handoff.inc"
using ObjectList=std::list<Unit*>;
struct ActionEntry {struct {struct {uint32 flag=16777216;} unitFlag;struct {uint32 type=92,misc1=50997,misc2=0,misc3=0;} achievementCriteria;} action;} actionEntry;
struct Dispatcher {
    Unit* target;
    ObjectList* GetTargets(ActionEntry const&,Unit*){return new ObjectList{target};}
    Unit* GetBaseObject(){return target;}
    bool IsUnit(Unit*){return true;}
    Player* GetScriptCreditPlayer(Unit* value){return value->ToPlayer();}
    void Run(uint32 type){auto& e=actionEntry;Unit* unit=nullptr;switch(type){
#include "FlagAction.inc"
#include "CreditAction.inc"
    }}
};
void Clear(){for(auto* story:created)delete story;created.clear();creationOk=true;payloadDuration=73800;errors=0;scenario={};scenarios.exists=true;currentTime=0;}
Conversation* Start(Player& player,Creature& hati){activePlayer=&player;player.hati=&hati;TitanstrikeAftermath::Request(&player,&info,42);assert(created.size()==1);return created.back();}
int main(){
    _set_abort_behavior(0,_CALL_REPORTFAULT);
    Player player;Creature hati;player.hati=&hati;activePlayer=&player;
    hati.flags=2;Dispatcher flag{&hati},credit{&player};
#ifdef BASELINE
    flag.Run(81);credit.Run(205);
#else
    flag.Run(83);
#endif
    assert(scenario.step==2&&hati.flags==2);
    for(uint32 status:{1u,3u}){Clear();player=Player{};hati=Creature{};hati.ai.actor=&hati;player.quest.Status=status;auto* story=Start(player,hati);
        currentTime=999999;story->Update(1);assert(!player.credits&&!hati.flags&&story->IsInWorld());
        assert(!story->CanNeverSee2(&player));currentTime+=73800;story->Update(1);assert(!player.credits&&!hati.flags);
        ++currentTime;story->Update(1);assert(player.credits==1&&scenario.step==3&&(hati.flags&16777216)&&!story->IsInWorld());
        hati.flags=0;story->Update(1);assert(player.credits==1&&!hati.flags);
    }
    for(uint32 invalid=0;invalid<12;++invalid){Clear();player=Player{};hati=Creature{};player.hati=&hati;activePlayer=&player;
        if(invalid==0)player.hasQuest=false;if(invalid==1)player.rewarded=true;if(invalid==2)player.klass=9;if(invalid==3)player.mapId=1;
        if(invalid==4)player.instance=7;if(invalid==5)scenario.step=4;if(invalid==6)player.contact=false;if(invalid==7)player.inWorld=false;
        if(invalid==8)scenario.id=1;if(invalid==9)scenario.step=1;if(invalid==10)scenario.achievement.progress.clear();if(invalid==11){scenario.achievement.progress.clear();scenario.achievement.progress[30091]={1};}
        TitanstrikeAftermath::Request(&player,&info,42);assert(created.empty()&&player.delayed.empty()&&!player.credits);
    }
    for(uint32 failure=0;failure<4;++failure){Clear();player=Player{};hati=Creature{};hati.ai.actor=&hati;player.hati=&hati;activePlayer=&player;
        if(failure==0)creationOk=false;if(failure==1)payloadDuration=33200;if(failure==2)player.auras.insert(211934);if(failure==3)player.liveBoss=&hati;
        TitanstrikeAftermath::Request(&player,&info,42);assert(player.delayed.size()==1&&!player.credits);
        creationOk=true;payloadDuration=73800;player.auras.clear();player.liveBoss=nullptr;player.RunDelayed();assert(created.back()->IsInWorld()&&!player.credits);
    }
    Clear();player=Player{};hati=Creature{};player.hati=&hati;activePlayer=&player;creationOk=false;TitanstrikeAftermath::Request(&player,&info,42);player.Abandon();player.hasQuest=true;creationOk=true;player.RunDelayed();assert(created.empty());
    Clear();player=Player{};hati=Creature{};hati.ai.actor=&hati;auto* scoped=Start(player,hati);scoped->CanNeverSee2(&player);player.Abandon();player.hasQuest=true;currentTime=100000;scoped->Update(1);assert(!scoped->IsInWorld()&&!player.credits);
    Clear();player=Player{};hati=Creature{};hati.ai.actor=&hati;auto* missing=Start(player,hati);missing->CanNeverSee2(&player);currentTime=73801;player.hati=nullptr;missing->Update(1000);assert(missing->IsInWorld()&&!player.credits);player.hati=&hati;missing->Update(1000);assert(player.credits==1);
    Clear();player=Player{};hati=Creature{};hati.ai.actor=&hati;hati.nativeAura=false;hati.castOk=false;auto* failed=Start(player,hati);failed->CanNeverSee2(&player);currentTime=73801;failed->Update(1000);assert(!hati.flags&&!player.credits);hati.castOk=true;failed->Update(1000);assert(player.credits==1&&hati.casts==2);
    Clear();player=Player{};hati=Creature{};hati.ai.actor=&hati;auto* waiting=Start(player,hati);waiting->CanNeverSee2(&player);currentTime=73801;player.hati=nullptr;waiting->Update(30000);waiting->Update(30000);assert(waiting->IsInWorld()&&!player.credits&&errors==1);player.hati=&hati;waiting->Update(1000);assert(player.credits==1);
    Clear();player=Player{};hati=Creature{};hati.ai.actor=&hati;auto* first=Start(player,hati);first->CanNeverSee2(&player);TitanstrikeAftermath::Request(&player,&info,42);auto* second=created.back();second->CanNeverSee2(&player);currentTime=73801;first->Update(1);hati.flags=0;hati.auras.insert(211967);second->Update(1);assert(player.credits==1&&!hati.flags);
    Clear();player=Player{};hati=Creature{};hati.ai.actor=&hati;auto* shifted=Start(player,hati);shifted->CanNeverSee2(&player);player.phase=2;shifted->Update(1);assert(!shifted->IsInWorld()&&!player.credits);
    for(uint32 failure=0;failure<10;++failure){Clear();player=Player{};hati=Creature{};hati.ai.actor=&hati;auto* pending=Start(player,hati);pending->CanNeverSee2(&player);currentTime=73801;
        if(failure==0)hati.inWorld=false;if(failure==1)hati.alive=false;if(failure==2)hati.IsAIEnabled=false;if(failure==3)hati.hasAI=false;
        if(failure==4)hati.map=&otherMap;if(failure==5)hati.phase=2;if(failure==6)player.alive=false;if(failure==7)player.teleport=true;if(failure==8)player.creditAdvances=false;if(failure==9)player.liveBoss=&hati;
        pending->Update(1000);assert(pending->IsInWorld()&&!hati.flags);assert(player.credits==(failure==8?1u:0u));
        if(failure==8){pending->Update(1);assert(player.credits==1);scenario.step=3;}
        hati.inWorld=true;hati.alive=true;hati.IsAIEnabled=true;hati.hasAI=true;hati.map=&activeMap;hati.phase=1;player.alive=true;player.teleport=false;player.creditAdvances=true;player.liveBoss=nullptr;
        pending->Update(1000);assert(scenario.step==3&&(hati.flags&16777216));
    }
    Clear();player=Player{};hati=Creature{};hati.ai.actor=&hati;hati.nativeAura=false;hati.applyAura=false;auto* unapplied=Start(player,hati);unapplied->CanNeverSee2(&player);currentTime=73801;unapplied->Update(1000);assert(!player.credits&&!hati.flags);hati.applyAura=true;unapplied->Update(1000);assert(player.credits==1);
    Clear();player=Player{};hati=Creature{};player.hati=&hati;activePlayer=&player;creationOk=false;TitanstrikeAftermath::Request(&player,&info,42);for(uint32 attempt=0;attempt<30;++attempt)player.RunDelayed();assert(player.delayed.empty()&&created.empty()&&errors==2&&!player.credits);
    Clear();player=Player{};hati=Creature{};hati.ai.actor=&hati;auto* wrapped=Start(player,hati);currentTime=UINT32_MAX-1000;wrapped->CanNeverSee2(&player);currentTime+=73801;wrapped->Update(1);assert(player.credits==1);
    Clear();player=Player{};hati=Creature{};hati.ai.actor=&hati;player.creditAdvances=false;auto* unpublished=Start(player,hati);unpublished->CanNeverSee2(&player);TitanstrikeAftermath::Request(&player,&info,42);auto* repeated=created.back();repeated->CanNeverSee2(&player);currentTime=73801;unpublished->Update(1);repeated->Update(1);assert(player.credits==1&&!hati.flags);scenario.step=3;unpublished->Update(1000);repeated->Update(1000);assert(player.credits==1&&(hati.flags&16777216));
    Clear();player=Player{};hati=Creature{};hati.ai.actor=&hati;auto* staleBinding=Start(player,hati);staleBinding->CanNeverSee2(&player);hati.auras.insert(211967);currentTime=73801;staleBinding->Update(1);assert(staleBinding->IsInWorld()&&!player.credits&&!hati.flags);hati.auras.erase(211967);staleBinding->Update(1000);assert(player.credits==1);
    Clear();std::cout<<"PASS: actual handoff namespace, native published lifetime/playback, quest scopes/delayed attempt guards and old flag/criteria actions: no early Hati, bounded creation retries and quest-scoped1s actor/aura recovery, exactly-once progression, duplicate-active-binding protection, abandonment/reaccept and phase/context cleanup; controlled world/credit/cast/AI sinks, not client traversal.\n";
}
