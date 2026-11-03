// Actual production actor/controller/instance and native AI/map/criteria-tree
// dispatch; world/player/text/motion/criteria storage endpoints controlled.
#define MONK_NINE_ARRIVAL_TEST
#include "MonkChenEarlyPresentationTest.cpp"
#include "NineActor.inc"
#include "NineBaselineActor.inc"
#include "NineBaselineInstance.inc"
using Nine=npc_nine_czi::npc_nine_cziAI;
using OldNine=npc_nine_czi_arrival_baseline::npc_nine_czi_arrival_baselineAI;
struct Visit:Run
{
    Creature nine;
    Visit(){nine.map=&map;nine.entry=97774;nine.spawn=362252;nine.guid={81,2};nine.y=1730.58f;nine.nearest=&player;nine.i_AI=new Nine(&nine);map.creatures[nine.guid]=&nine;scenario.step=2;player.distance=3;other.distance=1;instance.OnCreatureCreate(&nine);}
    ~Visit(){delete nine.i_AI;if(nine.i_disabledAI&&nine.i_disabledAI!=nine.i_AI)delete nine.i_disabledAI;}
    Nine* ai(){return static_cast<Nine*>(nine.i_disabledAI?nine.i_disabledAI:nine.i_AI);}
    auto& receipt(){return instance.nineArrival;}
    void Arrive(){ai()->MoveInLineOfSight(&player);}
    void TickNine(uint32 ms){clockNow+=ms;map.i_data=&instance;map.onUpdate=[&](uint32 elapsed){nine.Lifecycle(elapsed);nine.functions.Update(elapsed);player.functions.Update(elapsed);};map.Update(ms);}
};
void NoArrival(Visit& r){assert(!r.receipt().started&&!r.scenario.progress[29069].Counter&&!Count(r.nine,7)&&!Count(r.nine,8)&&!r.nine.despawnRequests);}
void Accepted(Visit& r){assert(r.receipt().started&&!r.receipt().pending&&r.scenario.progress[29069].Counter==1&&r.scenario.step>=3&&Count(r.nine,7)==1);assert(r.player.credits==std::vector<uint32>{46902}&&r.other.credits.empty());assert(r.nine.talks.front().second==r.player.guid);}
void Departed(Visit& r){Accepted(r);assert(r.receipt().finished&&Count(r.nine,8)==1&&r.nine.despawnRequests==1&&r.nine.despawnDelay==3000&&r.nine.talks.back().second==r.player.guid);}

uint32 RunNineArrivalCases(bool includeRetained=true)
{
    auto retained=includeRetained?RunEarlyPresentationCases():0;uint32 cases=0;
    // Full pinned actor+instance, not a handwritten alternative controller.
    for(uint32 mode=0;mode<4;++mode)
    {
        Visit r;instance_monk_intro_nine_baseline previous(&r.map);r.map.script=&previous;r.map.i_data=&previous;
        delete r.nine.i_AI;r.nine.i_AI=new OldNine(&r.nine);auto old=[&]{return static_cast<OldNine*>(r.nine.i_AI);};
        previous.OnCreatureCreate(&r.nine);r.map.onUpdate=[&](uint32 ms){r.nine.Lifecycle(ms);};
        if(mode==1)r.scenario.step=1;
        if(mode==2){r.other.status=0;r.nine.nearest=&r.other;}
        old()->MoveInLineOfSight(&r.player);
        if(mode==0){clockNow+=4000;r.map.Update(4000);assert(r.scenario.step==3&&Count(r.nine,8)==1&&r.nine.despawnRequests==1&&r.nine.despawnDelay==3000);}
        if(mode==1){assert(old()->_introDone&&!r.scenario.progress[29069].Counter);r.scenario.step=2;old()->MoveInLineOfSight(&r.player);assert(!r.scenario.progress[29069].Counter&&Count(r.nine,7)==1);}
        if(mode==2)assert(r.player.credits.empty()&&r.other.credits==std::vector<uint32>{46902}&&r.nine.talks.front().second==r.other.guid);
        if(mode==3){assert(r.nine.AIM_Destroy());r.nine.i_AI=new OldNine(&r.nine);clockNow+=4000;r.map.Update(4000);assert(!Count(r.nine,8)&&!r.nine.despawnRequests);old()->MoveInLineOfSight(&r.player);assert(Count(r.nine,7)==2&&r.player.credits.size()==2);}
        ++cases;
    }
    std::cout<<"COUNTERFACTUAL pinnedf67c49e3b full Nine actor/instance: early native future-tree rejection consumes introDone, wrong nearest recipient, AI destruction loses4s/3s cleanup and recreation replays7/event attempt; ordinary control passes.\n";
    for(uint32 mode=0;mode<5;++mode)
    {
        Visit r;r.other.status=0;r.nine.nearest=&r.other;r.Arrive();Accepted(r);
        if(mode){assert(r.nine.AIM_Destroy());if(mode==2)r.nine.i_AI=new Nine(&r.nine);if(mode==3)r.nine.i_disabledAI=new Nine(&r.nine);if(mode==4){r.nine.i_AI=new Nine(&r.nine);r.nine.IsAIEnabled=false;}}
        r.TickNine(3999);assert(!Count(r.nine,8)&&!r.nine.despawnRequests);r.TickNine(1);Departed(r);
        for(uint32 i=0;i<100;++i){r.instance.Update(100000);if(r.nine.i_AI)r.nine.i_AI->MoveInLineOfSight(&r.player);}r.TickNine(100000);Departed(r);++cases;
    }
    for(uint32 early=0;early<2;++early)
    {
        Visit r;r.scenario.step=early;r.Arrive();assert(r.receipt().pending);NoArrival(r);assert(r.player.credits.empty());
        r.TickNine(100000);NoArrival(r);assert(r.player.credits.empty());r.scenario.step=2;r.TickNine(0);Accepted(r);r.TickNine(4000);Departed(r);++cases;
    }
    // Early or rejected admission cannot consume a future visit. These cover
    // native first-entry flags/class/quest/map/contact/private/phase and radius.
    for(uint32 gate=0;gate<21;++gate)
    {
        Visit r;Map elsewhere;
        switch(gate){case 0:r.player.type=1;break;case 1:r.player.status=0;break;case 2:r.player.reward=true;break;case 3:r.player.slot=25;break;case 4:r.player.world=false;break;case 5:r.player.contact=false;break;case 6:r.player.gm=true;break;case 7:r.player.teleport=true;break;case 8:r.player.phase=false;break;case 9:r.player.alive=false;break;case 10:r.player.visible=false;break;case 11:r.player.los=false;break;case 12:r.player.distance=7.01f;break;case 13:r.player.map=&elsewhere;break;case 14:r.nine.world=false;break;case 15:r.nine.predelete=true;break;case 16:r.nine.deathState=CORPSE;break;case 17:r.nine.hidden=true;break;case 18:r.nine.y=1736.01f;break;case 19:r.scenario.id=1;break;case 20:r.map.unloading=true;break;}
        r.Arrive();r.TickNine(5000);NoArrival(r);assert(r.player.credits.empty()&&!r.receipt().pending);
        r.player.type=CLASS_MONK;r.player.status=QUEST_STATUS_INCOMPLETE;r.player.reward=false;r.player.slot=0;r.player.world=r.player.contact=r.player.phase=r.player.alive=r.player.visible=r.player.los=true;r.player.gm=r.player.teleport=false;r.player.distance=7;r.player.map=&r.map;r.nine.world=true;r.nine.predelete=r.nine.hidden=false;r.nine.deathState=ALIVE;r.nine.y=1736;r.scenario.id=943;r.map.unloading=false;
        r.Arrive();Accepted(r);r.TickNine(4000);Departed(r);++cases;
    }
    {Visit r;Unit pet;r.ai()->MoveInLineOfSight(&pet);NoArrival(r);r.Arrive();Accepted(r);++cases;}
    {Visit r;r.scenario.step=3;r.scenario.progress[29069].Counter=1;r.Arrive();assert(!r.receipt().started&&r.player.credits.empty()&&!Count(r.nine,7));++cases;}
    for(uint32 cancel=0;cancel<9;++cancel)
    {
        Visit r;r.Arrive();Accepted(r);Scenario replacement=r.scenario;
        switch(cancel){case 0:r.player.quest.ScriptLifetime.reset();break;case 1:r.player.quest.ScriptLifetime=std::make_shared<uint8>();break;case 2:r.player.map=nullptr;break;case 3:r.player.status=0;break;case 4:r.player.reward=true;break;case 5:r.scenario.step=2;break;case 6:r.scenario.progress[29069].Counter=0;break;case 7:r.map.unloading=true;break;case 8:manager.current=&replacement;break;}
        r.TickNine(4000);assert(!Count(r.nine,8)&&!r.nine.despawnRequests&&r.player.credits==std::vector<uint32>{46902});++cases;
    }
    for(uint32 gate=0;gate<7;++gate)
    {
        Visit r;r.Arrive();switch(gate){case 0:r.player.world=false;break;case 1:r.player.contact=false;break;case 2:r.player.phase=false;break;case 3:r.player.visible=false;break;case 4:r.player.gm=true;break;case 5:r.player.teleport=true;break;case 6:r.player.alive=false;break;}
        r.TickNine(4000);assert(!Count(r.nine,8)&&r.nine.despawnRequests==1&&r.nine.despawnDelay==3000);++cases;
    }
    {Visit r;r.Arrive();r.player.distance=30;r.player.los=false;r.TickNine(4000);Departed(r);++cases;}
    for(uint32 gate=0;gate<5;++gate)
    {
        Visit r;r.Arrive();switch(gate){case 0:r.map.creatures.erase(r.nine.guid);break;case 1:r.nine.world=false;break;case 2:r.nine.predelete=true;break;case 3:r.nine.deathState=CORPSE;break;case 4:r.nine.hidden=true;break;}
        r.TickNine(4000);assert(!Count(r.nine,8)&&!r.nine.despawnRequests);
        r.map.creatures[r.nine.guid]=&r.nine;r.nine.world=true;r.nine.predelete=r.nine.hidden=false;r.nine.deathState=ALIVE;r.TickNine(1);Departed(r);++cases;
    }
    {Visit r;r.Arrive();Creature impostor;impostor.map=&r.map;impostor.entry=97774;impostor.spawn=362252;impostor.guid={82,2};r.map.creatures.erase(r.nine.guid);r.map.creatures[impostor.guid]=&impostor;r.instance.OnCreatureCreate(&impostor);r.TickNine(4000);assert(!Count(impostor,8)&&!impostor.despawnRequests&&!r.nine.despawnRequests&&r.receipt().source==r.nine.guid);++cases;}
    {Visit r;r.player.accept=false;auto before=warnings;r.Arrive();NoArrival(r);assert(r.receipt().pending&&r.player.credits.size()==1&&warnings==before+1);for(uint32 i=0;i<100;++i)r.Arrive();r.TickNine(1999);assert(r.player.credits.size()==1);r.player.accept=true;r.TickNine(1);assert(r.receipt().started&&r.scenario.progress[29069].Counter==1&&r.player.credits==(std::vector<uint32>{46902,46902})&&Count(r.nine,7)==1);r.TickNine(4000);assert(Count(r.nine,8)==1&&r.nine.despawnRequests==1&&warnings==before+1);++cases;}
    for(uint32 source=0;source<3;++source)for(uint32 effect=0;effect<7;++effect)
    {
        Visit r;Scenario replacement=r.scenario;
        auto callback=[&]{switch(effect){case 0:r.instance.Update(100000);break;case 1:r.receipt().Cancel();break;case 2:r.player.quest.ScriptLifetime=std::make_shared<uint8>();break;case 3:r.map.unloading=true;break;case 4:r.map.creatures.erase(r.nine.guid);break;case 5:manager.current=&replacement;break;case 6:assert(r.nine.AIM_Destroy());break;}};
        if(source==0)r.player.onCredit=callback;
        else r.nine.onTalk=[&]{if(r.nine.talks.back().first==(source==1?7u:8u))callback();};
        r.Arrive();r.TickNine(4000);r.player.onCredit={};r.nine.onTalk={};assert(!r.receipt().busy&&r.player.credits==std::vector<uint32>{46902});
        if(effect==0||effect==6)Departed(r);
        else{assert(!r.nine.despawnRequests&&Count(r.nine,8)==(source==2?1u:0u));}
        ++cases;
    }
    {Visit r;r.player.onCredit=[&]{r.scenario.step=4;};r.Arrive();Accepted(r);r.player.onCredit={};r.TickNine(4000);Departed(r);++cases;}
    {Visit r;r.map.i_data=&r.instance;r.map.onUpdate=[&](uint32){r.Arrive();};clockNow+=100000;r.map.Update(100000);Accepted(r);assert(!Count(r.nine,8));r.TickNine(3999);assert(!Count(r.nine,8));r.TickNine(1);Departed(r);++cases;}
    {Visit r;clockNow=0xFFFFFE00;r.Arrive();r.TickNine(4000);assert(!Count(r.nine,8));r.TickNine(1);Departed(r);++cases;}
    {Visit r;r.Arrive();r.TickNine(100000);Departed(r);assert(r.nine.despawnDelay==3000);++cases;}
    {Visit r;r.scenario.step=1;r.Arrive();r.player.quest.ScriptLifetime=std::make_shared<uint8>();r.scenario.step=2;r.TickNine(0);NoArrival(r);assert(r.player.credits.empty());r.Arrive();Accepted(r);++cases;}
    {Visit r;r.scenario.step=1;r.Arrive();r.scenario.step=0;r.TickNine(0);NoArrival(r);assert(!r.receipt().pending);++cases;}
    // A newly observed original spawn belongs to a fresh attempt only after
    // the previous weak quest lifetime has ended. Active attempts keep GUIDs.
    {Visit r;r.Arrive();auto original=r.nine.guid;auto oldLife=r.receipt().member.attempt;r.player.quest.ScriptLifetime=std::make_shared<uint8>();r.map.creatures.erase(original);r.nine.guid={83,2};r.map.creatures[r.nine.guid]=&r.nine;r.instance.OnCreatureCreate(&r.nine);assert(r.receipt().source==r.nine.guid&&!r.receipt().started&&oldLife.expired());r.scenario.step=2;r.scenario.progress[29069].Counter=0;r.Arrive();assert(r.receipt().started&&r.receipt().member.attempt.lock()==r.player.quest.ScriptLifetime&&r.player.credits.size()==2&&Count(r.nine,7)==2);++cases;}
    {Visit r;r.nine.spawn=362182;r.nine.y=2007.45f;r.ai()->MoveInLineOfSight(&r.player);assert(Count(r.nine,1)==1&&!r.receipt().pending);r.scenario.step=1;r.ai()->SetData(1,1);r.TickNine(3000);assert(r.player.credits==std::vector<uint32>{46734}&&r.scenario.step==2&&r.nine.motion.jumps==1&&r.nine.casts==std::vector<uint32>{193248});++cases;}
    std::cout<<"PASS "<<cases<<" actual Nine arrival ASAN contexts plus retained "<<retained<<" Chen early/67 outro/81 routes/50 arrival/70 rescue: native future-tree rejection recovery, actual visitor/full source/original weak attempt, real29069 receipt before dialogue, AI-independent4s/native3s despawn, native retry, cancellation/source/scenario identity/reentry/rollover/birth clocks and unchanged first role. World/taxi/network endpoints controlled; actual client/full943/all36 remain OPEN.\n";
    return cases;
}
#ifndef MONK_NINE_RESCUE_TEST
int main(){RunNineArrivalCases();}
#endif
