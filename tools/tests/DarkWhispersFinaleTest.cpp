#include <algorithm>
#include <limits>
#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
using uint32=uint32_t;using uint64=uint64_t;using DamageEffectType=int;
constexpr int SMART_EVENT_DAMAGED=9,SMART_EVENT_UPDATE_IC=0,SMART_EVENT_HEALTH_PCT=2;
#include "EventFlags.inc"
#define TC_LOG_INFO(...) ((void)0)
struct Creature;struct InstanceScript;
struct DefaultAI {void DamageTakenAfterScaling(struct Unit*,uint32&,DamageEffectType){}};
using CreatureAI=DefaultAI;
struct Unit {bool pet=false;uint32 Deal(Creature*,uint32);};
struct Creature {
    bool IsAIEnabled=false;DefaultAI defaultAI;DefaultAI* i_disabledAI=nullptr;DefaultAI* AI(){return &defaultAI;}
    uint32 entry=106652;uint64 hp=100000,maxHp=100000,targetHp=1000;
    float multiplier=1;bool alive=true,combat=true;InstanceScript* instance=nullptr;
    uint32 GetEntry(){return entry;}bool IsAlive(){return alive;}bool isInCombat(){return combat;}
    uint64 GetHealth(){return hp;}uint64 GetHealth(Unit*){return targetHp;}uint64 GetMaxHealth(){return maxHp;}
    float GetHealthPct(){return 100.f*hp/maxHp;}void SetHealth(uint64 v){hp=v;targetHp=v;}
    float GetHealthMultiplierForTarget(Unit*){return multiplier;}
    Creature* ToCreature(){return this;}InstanceScript* GetInstanceScript(){return instance;}
};
#include "ProtectFinale.inc"
struct Script {void ProcessEventsFor(int,Unit*,uint32){}};
struct SmartAI {Creature* me;uint32 mInvincibilityHpLevel=0;Script script;Script* GetScript(){return &script;}void DamageTaken(Unit*,uint32&,DamageEffectType);};
#include "SmartDamage.inc"
struct InstanceScript {uint32 scenario=1097;void OnCreatureDamageTaken(Creature* c,Unit* a,uint32& damage){DarkWhispersArtifact::ProtectFinale(c,a,damage,scenario);}};
uint32 Unit::Deal(Creature* victim,uint32 damage)
{
    DamageEffectType damagetype=0;
    SmartAI ai{victim};ai.DamageTaken(this,damage,0);
    uint64 health=victim->GetHealth(this);
#include "NativeScaledHook.inc"
    if(health<=damage)victim->alive=false;
    return damage;
}
struct Row {
    int entry=0,id=0,link=0,type=0,flags=0,e1=0,e2=0,e3=0,e4=0,action=0,a1=0,a2=0,target=0,t1=0,t2=0;
    float x=0,y=0,z=0;bool runOnce=false;
    struct {int event_flags=0;struct {uint32 min=0,max=0,repeatMin=0,repeatMax=0;} minMaxRepeat;} event;
};
struct Finale {
    Creature* me;std::map<int,Row> rows;int step=4,ritual=0,defeat=0,chests=0,handoffs=0,talks=0,casts=0;
    bool castWorks=false,pet=true,despawned=false;float playerDistance=10;
    void RecalcTimer(Row&,uint32,uint32){}
    void ProcessAction(Row& e){
        e.runOnce=true;
        switch(e.action){
            case 1:++talks;break;
            case 11:++casts;if(!castWorks)return;break; // native failed CAST returns before LINK
            case 205:{
                bool player=e.target==21?playerDistance<=e.t1:(e.target==7&&!pet);
                if(player&&step==4){if(e.a2==50961)++ritual;else if(e.a2==50962){++defeat;if(ritual)step=5;}}
                break;
            }
            case 45:assert(e.a1==2&&e.a2==2);++handoffs;break;
            case 50:assert(e.a1==249821&&step==5);assert(e.x==1004.36f&&e.y==4931.7f);++chests;break;
            case 41:despawned=true;break;
            default:assert(false);
        }
        if(e.link){assert(rows.count(e.link));auto linked=rows.at(e.link);assert(linked.type==61);ProcessAction(linked);}
    }
    void Poll(Row& e){
        if((e.flags&1)&&e.runOnce)return;
        // Fixture conditions are checked separately against the real SQL.
        if((e.id==2||e.id==3||e.id==4)&&step!=4)return;
        switch(e.type){
            #include "HealthEvent.inc"
            default:break;
        }
    }
    void Tick(){if(!despawned)for(auto& row:rows)Poll(row.second);}
};
int main(int argc,char** argv)
{
    assert(argc==2);std::ifstream in(argv[1]);std::map<int,Row> rows;std::string line;
    while(std::getline(in,line)){
        Row r;std::istringstream s(line);s>>r.entry>>r.id>>r.link>>r.type>>r.flags>>r.e1>>r.e2>>r.e3>>r.e4>>r.action>>r.a1>>r.a2>>r.target>>r.t1>>r.t2>>r.x>>r.y>>r.z;
        assert(s);if(r.entry!=106652)continue;
        r.event.event_flags=r.flags;r.event.minMaxRepeat={uint32(r.e1),uint32(r.e2),uint32(r.e3),uint32(r.e4)};assert(rows.emplace(r.id,r).second);
    }
    assert(rows.size()==9&&rows.at(0).link==0&&rows.at(2).type==0&&rows.at(2).target==21);
    assert(rows.at(3).action==11&&rows.at(3).a1==152821&&rows.at(3).link==0);
    assert(rows.at(4).type==2&&rows.at(4).link==5&&rows.at(5).target==21);
    // Full-health first-hit kills and a tiny raw hit becoming lethal only
    // after native scaling must both survive until the native health poll.
    for(bool pet:{false,true})for(float scale:{0.001f,0.01f,1.f,100.f})for(bool visual:{false,true}){
        Unit attacker;attacker.pet=pet;InstanceScript instance;Creature boss;boss.instance=&instance;boss.multiplier=scale;
        assert(attacker.Deal(&boss,200000)==0&&boss.alive&&boss.hp==1);
        assert(attacker.Deal(&boss,200000)==0&&boss.alive&&boss.hp==1);
        Finale f{&boss,rows};f.pet=pet;f.castWorks=visual;
        f.Tick();assert(f.step==5&&f.ritual==1&&f.defeat==1&&f.chests==1&&f.handoffs==1&&f.talks==1&&f.casts==1&&f.despawned);
        for(int i=0;i<10;++i)f.Tick();assert(f.defeat==1&&f.chests==1);
    }
    Unit attacker;InstanceScript instance;Creature scaled;scaled.instance=&instance;scaled.multiplier=.001f;
    assert(attacker.Deal(&scaled,2)==0&&scaled.alive&&scaled.hp==1);
    Creature ordinary;ordinary.instance=&instance;assert(attacker.Deal(&ordinary,2)==2&&ordinary.hp==100000);
    for(auto scenario:{0u,988u,998u}){instance.scenario=scenario;Creature other;other.instance=&instance;uint32 damage=2000;DarkWhispersArtifact::ProtectFinale(&other,&attacker,damage,scenario);assert(damage==2000&&other.hp==100000);}
    Creature other;other.entry=106692;uint32 damage=2000;DarkWhispersArtifact::ProtectFinale(&other,&attacker,damage,1097);assert(damage==2000);
    other.entry=106652;other.alive=false;DarkWhispersArtifact::ProtectFinale(&other,&attacker,damage,1097);assert(damage==2000);
    other.alive=true;DarkWhispersArtifact::ProtectFinale(&other,nullptr,damage,1097);assert(damage==2000);
    Creature boss;Finale high{&boss,rows};high.Tick();assert(high.ritual==1&&!high.defeat&&!high.chests);
    boss.SetHealth(7000);high.Tick();assert(high.step==5&&high.chests==1); // normal low-health retreat
    for(int step:{0,1,2,3,5,6}){boss.SetHealth(1);Finale early{&boss,rows};early.step=step;early.Tick();assert(!early.ritual&&!early.defeat&&!early.chests);}
    boss.combat=false;Finale idle{&boss,rows};idle.Tick();assert(!idle.ritual&&!idle.defeat&&!idle.chests);
    std::cout<<"PASS: native post-scaling protection and health poll; first-hit/pet/DoT-equivalent lethal damage, normal retreat, failed visual, exact-once credit/skull/cleanup, stage and actor isolation\n";
}
