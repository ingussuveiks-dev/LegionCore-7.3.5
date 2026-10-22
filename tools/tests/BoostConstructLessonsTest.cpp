#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>
using uint32=uint32_t;using uint64=uint64_t;using uint8=uint8_t;
#define TC_LOG_INFO(...) ((void)0)
int fallbackErrors=0;
#define TC_LOG_ERROR(...) (++fallbackErrors)
enum {CRITERIA_TYPE_CAST_SPELL=29,CRITERIA_TYPE_SCRIPT_EVENT_2=92,CRITERIA_TYPE_KILL_CREATURE=0,CRITERIA_CONSTRUCTS_DESTROYED=50571,CRITERIA_SPAR_COMPLETE=48772,DATA_SPAR_SURRENDER=1,NPC_ARCANE_CONSTRUCT=108478,NPC_TRAINING_DUMMY=107104,UNIT_FIELD_FLAGS=1,UNIT_FLAG_FLEEING=0x800000,UNIT_STATE_ROOT=1,REACT_PASSIVE=1,UNIT_FLAG_NON_ATTACKABLE=2,UNIT_STAND_STATE_KNEEL=1,LANG_UNIVERSAL=0,TEMPSUMMON_CORPSE_TIMED_DESPAWN=1};
struct ObjectGuid {
    uint32 id=0;static ObjectGuid const Empty;
    bool operator==(ObjectGuid const& b)const{return id==b.id;}
    bool operator!=(ObjectGuid const& b)const{return !(*this==b);}
    bool IsEmpty()const{return !id;}void Clear(){id=0;}
    std::string ToString()const{return std::to_string(id);}
};
ObjectGuid const ObjectGuid::Empty{};
struct Position {
    float x=0,y=0,z=0,o=0;
    Position(float X,float Y,float Z,float O):x(X),y(Y),z(Z),o(O){}
    float GetPositionX()const{return x;}float GetPositionY()const{return y;}float GetPositionZ()const{return z;}float GetOrientation()const{return o;}
    static float NormalizeOrientation(float v){return v;}
};
#include "Offset.inc"
#include "CombatStage.inc"
#include "Combat.inc"
#include "Surrender.inc"
#include "First.inc"
struct CriteriaEntry {uint32 Type,Asset;};
struct Criteria {CriteriaEntry const* Entry;};
struct CriteriaTree {std::vector<CriteriaTree const*> Children;struct Criteria const* Criteria=nullptr;};
#include "Criterion.inc"
#include "Needs.inc"
struct Scenario {
    uint32 step=7;uint8 count=13;
    uint32 GetCurrentStep(){return step;}uint8 GetStepCount(bool){return count;}
    uint32 GetScenarioCriteriaByStep(uint32 index){return index;}
};
struct ScenarioManager {Scenario* scenario=nullptr;Scenario* GetScenario(uint32){return scenario;}} scenarios;
ScenarioManager* sScenarioMgr=&scenarios;
struct AchievementManager {std::map<uint32,CriteriaTree const*> trees;CriteriaTree const* GetCriteriaTree(uint32 id){return trees[id];}} achievements;
AchievementManager* sAchievementMgr=&achievements;
struct Player;
struct Unit {
    Player* owner=nullptr;ObjectGuid guid;
    Player* GetCharmerOrOwnerPlayerOrPlayerItself(){return owner;}
    ObjectGuid GetGUID()const{return guid;}
};
struct Player:Unit {
    uint32 scenarioId=1083;std::vector<uint32> events;std::function<void(uint32)> onEvent;
    Player(){owner=this;guid.id=1;}
    char const* GetName(){return "trainee";}
    void SendActionButtons(int){}
    template<class... Args> void UpdateAchievementCriteria(uint32,uint32 event,Args...){events.push_back(event);if(onEvent)onEvent(event);}
};
uint32 GetScenarioFor(Player const* p){return p->scenarioId;}
void EnsureDoomguardShard(Player*,uint32){}
struct Creature:Unit {
    uint32 entry=NPC_ARCANE_CONSTRUCT;bool dead=false,despawned=false,root=false;int faction=14,react=0;uint32 flags=UNIT_FLAG_FLEEING;
    uint64 health=100,maxHealth=100;
    uint32 GetEntry(){return entry;}
    void DespawnOrUnsummon(uint32=0){despawned=true;}
    void setFaction(int v){faction=v;}void SetReactState(int v){react=v;}
    void RemoveFlag(int,uint32 flag){flags&=~flag;}void SetFlag(int,uint32 flag){flags|=flag;}
    void SetControlled(bool v,int){root=v;}
    uint64 GetHealth(Unit*){return health;}uint64 GetMaxHealth(Unit*){return maxHealth;}
    void RemoveAllAuras(){}void CombatStop(bool){}void SetStandState(int){}
    void MonsterSay(char const*,int,ObjectGuid const&){}
    Player* GetLootRecipient(){return nullptr;}
};
using TempSummon=Creature;
struct Transport {};
struct Instance {
    std::vector<std::unique_ptr<Creature>> creatures;
    uint32 GetInstanceId(){return 7;}
    Creature* GetCreature(ObjectGuid guid){for(auto& c:creatures)if(c->guid==guid&&!c->despawned)return c.get();return nullptr;}
};
struct Script {
    Instance data;Instance* instance=&data;Player trainee;Player* player=&trainee;Transport ship;bool hasShip=true;int failAfter=-1,spawned=0;
    Position deck{0,0,100,0};std::vector<Position> poses;
    uint32 _constructStep=uint32(-1),_constructRetryTimer=0,_visibilityTimer=0,_secondWaveTimer=0,_defenderTimer=0;
    bool _constructCredit=false,_trainingDummySpawned=false;
    ObjectGuid _trainingDummyGuid;
    uint8 _lastCombatStep=255;
    std::vector<ObjectGuid> _constructGuids,_deadConstructGuids,_passengerGuids,_sparringGuids,_creditedSparringGuids,_yieldedSparringGuids,_legionGuids,_defenderGuids;
    Player* GetPlayer()const{return player;}Transport* GetGunship(){return hasShip?&ship:nullptr;}Position const& Deck(){return deck;}
    Creature* SummonPassenger(Transport*,uint32 entry,Position const& pose,Unit* =nullptr,int=0,uint32=0){
        if(failAfter>=0&&spawned==failAfter)return nullptr;
        auto c=std::make_unique<Creature>();c->entry=entry;c->guid.id=100+uint32(data.creatures.size());auto raw=c.get();data.creatures.push_back(std::move(c));
        _passengerGuids.push_back(raw->guid);poses.push_back(pose);++spawned;return raw;
    }
    void SpawnSparringWave(uint8 count){_sparringGuids.clear();_creditedSparringGuids.clear();for(uint8 i=0;i<count;++i)_sparringGuids.push_back(SummonPassenger(&ship,111995,deck)->guid);}
    void ClearSparringWave(){_sparringGuids.clear();}
    void SpawnLegionWave(){++legionSpawns;}void SpawnExit(){++exitSpawns;}int legionSpawns=0,exitSpawns=0;
#include "Clear.inc"
#include "Spawn.inc"
#include "Death.inc"
#include "Retry.inc"
#include "Dummy.inc"
#include "ActiveSpar.inc"
#include "Damage.inc"
#include "SparCredit.inc"
#include "CreatureDies.inc"
#include "Next.inc"
    void Kill(ObjectGuid guid,Unit* killer){auto c=instance->GetCreature(guid);assert(c);uint32 damage=100000;OnCreatureDamageTaken(c,killer,damage);if(damage){c->dead=true;CreatureDies(c,killer);}}
};
int main(){
    CriteriaEntry event{92,50571},multi{29,2643},victory{29,34428};Criteria ce{&event},cm{&multi},cv{&victory};
    CriteriaTree kill{{},&ce},cast{{},&cm},vr{{},&cv},aoe{{&kill,&cast},nullptr},dummy;
    achievements.trees={{6,&dummy},{7,&aoe},{8,&dummy},{9,&dummy},{10,&dummy},{11,&dummy},{12,&dummy}};
    assert(NeedsArcaneConstructs(&aoe)&&NeedsArcaneConstructs(&vr)&&!NeedsArcaneConstructs(&dummy)&&!NeedsArcaneConstructs(nullptr));
    for(int faction=0;faction<2;++faction)for(int killerType=0;killerType<3;++killerType){
        std::vector<int> order{0,1,2};do{
            Scenario scenario;scenarios.scenario=&scenario;Script s;s.deck.o=faction?6.2787f:2.937f;
            Unit pet,guardian;pet.owner=&s.trainee;guardian.owner=&s.trainee;Unit* killer=killerType==0?static_cast<Unit*>(&s.trainee):killerType==1?&pet:&guardian;
            s.SetTrainingDummyForStep(&s.trainee,7,13);auto guids=s._constructGuids;
            assert(guids.size()==3&&!s._trainingDummySpawned&&s.trainee.events.empty());
            s.SetTrainingDummyForStep(&s.trainee,7,13);assert(s._constructGuids==guids);
            for(auto guid:guids){auto c=s.instance->GetCreature(guid);assert(c->entry==108478&&c->root&&c->react==REACT_PASSIVE&&c->faction==14&&!(c->flags&UNIT_FLAG_FLEEING));}
            for(int i=0;i<3;++i){s.Kill(guids[order[i]],killer);s.CreatureDies(s.instance->GetCreature(guids[order[i]]),killer);assert(s.trainee.events.size()==size_t(i==2));}
            assert(s.trainee.events[0]==50571&&s._constructRetryTimer==2000);
            s.UpdateConstructLesson(1999);assert(s._constructGuids==guids);s.UpdateConstructLesson(1);
            assert(s._constructGuids.size()==3&&s._constructGuids!=guids); // Refill if the required cast has not happened.
            auto refill=s._constructGuids;for(auto guid:refill)s.Kill(guid,killer);assert(s.trainee.events.size()==1);
            scenario.step=8;s.onScenarionNextStep(8);assert(s._constructGuids.empty()&&s._trainingDummySpawned&&!s._constructRetryTimer);
        }while(std::next_permutation(order.begin(),order.end()));
    }
    {Scenario scenario;scenarios.scenario=&scenario;Script s;s.SetTrainingDummyForStep(&s.trainee,6,13);auto old=s._trainingDummyGuid;
     s.SetTrainingDummyForStep(&s.trainee,7,13);assert(!s.instance->GetCreature(old));
     s.trainee.onEvent=[&](uint32 e){if(e==50571){scenario.step=8;s.onScenarionNextStep(8);}};
     auto guids=s._constructGuids;for(auto guid:guids)s.Kill(guid,&s.trainee);assert(s.trainee.events.size()==1&&s._constructGuids.empty()&&s._trainingDummySpawned&&!s._constructRetryTimer);}
    {Scenario scenario;scenarios.scenario=&scenario;Script s;s.failAfter=2;s.SetArcaneConstructsForStep(&s.trainee,7,13);assert(s._constructGuids.size()==2);
     auto partial=s._constructGuids;for(auto guid:partial)s.Kill(guid,&s.trainee);assert(s.trainee.events.empty());
     s.failAfter=-1;s.UpdateConstructLesson(1000);assert(s._constructGuids.size()==3);s.Kill(s._constructGuids.back(),&s.trainee);assert(s.trainee.events.size()==1);}
    {Scenario scenario;scenarios.scenario=&scenario;Script s;s.hasShip=false;s.SetArcaneConstructsForStep(&s.trainee,7,13);assert(s._constructRetryTimer==1000&&s._constructGuids.empty());
     s.hasShip=true;s.UpdateConstructLesson(1000);assert(s._constructGuids.size()==3);Unit outsider;
     auto c=s.instance->GetCreature(s._constructGuids.front());uint32 damage=100000;s.OnCreatureDamageTaken(c,&outsider,damage);assert(!damage);
     s.CreatureDies(c,nullptr);assert(s._constructGuids.empty()&&s.trainee.events.empty());s.UpdateConstructLesson(1000);assert(s._constructGuids.size()==3);
     Creature foreign;foreign.guid.id=999;assert(!s.HandleConstructDeath(&foreign,&s.trainee));
     auto guids=s._constructGuids;scenario.step=9;s.onScenarionNextStep(9);assert(s._constructGuids.empty()&&!s._trainingDummySpawned&&s._sparringGuids.size()==1);
     for(auto guid:guids){auto corpse=std::find_if(s.data.creatures.begin(),s.data.creatures.end(),[&](auto& p){return p->guid==guid;});s.CreatureDies(corpse->get(),&s.trainee);}assert(s.trainee.events.empty());
     auto spar=s.instance->GetCreature(s._sparringGuids.front());spar->health=5000;spar->maxHealth=5000;uint32 hit=50000;s.OnCreatureDamageTaken(spar,&s.trainee,hit);assert(!hit&&s.trainee.events.back()==48772&&!spar->dead);
     s.onScenarionNextStep(10);assert(s._secondWaveTimer==2000);s.SpawnSparringWave(2);scenario.step=10;
     Unit pet;pet.owner=&s.trainee;auto pair=s._sparringGuids;for(auto guid:pair){hit=50000;s.OnCreatureDamageTaken(s.instance->GetCreature(guid),&pet,hit);assert(!hit);}assert(s._creditedSparringGuids.size()==2);
     s.onScenarionNextStep(11);s.onScenarionNextStep(12);assert(s.legionSpawns==1&&s.exitSpawns==1&&fallbackErrors==0);}
    {Scenario scenario;scenarios.scenario=&scenario;Script s;achievements.trees[7]=&vr;s.SetArcaneConstructsForStep(&s.trainee,7,13);auto guids=s._constructGuids;
     for(auto guid:guids)s.Kill(guid,&s.trainee);assert(s.trainee.events.empty()&&s._constructRetryTimer==2000);s.UpdateConstructLesson(2000);assert(s._constructGuids.size()==3);achievements.trees[7]=&aoe;}
    {Scenario scenario;scenarios.scenario=&scenario;Script s;s.SetArcaneConstructsForStep(&s.trainee,7,13);auto old=s._constructGuids;
     s.instance->GetCreature(old[1])->DespawnOrUnsummon();s.UpdateConstructLesson(1000);assert(s._constructGuids.size()==3&&s._constructGuids!=old&&s.trainee.events.empty());
     auto current=s._constructGuids;s.UpdateConstructLesson(1000);assert(s._constructGuids==current);s.Kill(current[0],&s.trainee);s.instance->GetCreature(current[0])->DespawnOrUnsummon();
     s.UpdateConstructLesson(1000);assert(s._constructGuids==current);s.Kill(current[1],&s.trainee);s.Kill(current[2],&s.trainee);assert(s.trainee.events.size()==1);}
    std::cout<<"PASS: extracted construct spawn/selection, six death orders, both faction poses, player/pet/guardian one-hit kills, exact-once event, missing summon/transport, retry, early kills, reentrant completion, cleanup, native spar surrender/stage ordering and Victory Rush targets\n";
}
