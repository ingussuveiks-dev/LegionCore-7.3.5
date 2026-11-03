#define main HaitOpeningRetainedMain
#include "HaitOpeningTest.cpp"
#undef main
#include "BeforeDefeatOpening.inc"
#include "BeforeDefeatActor.inc"
#include "BeforeDefeatInstance.inc"
static_assert(sizeof(MonkHaitOpeningBeforeDefeat::State)==96,"Complete pinned opening retained");
static_assert(sizeof(MonkHaitOpening::State)==152,"Release-owned appended defeat state");
struct DemonRoot:CreatureAI
{
    Creature* guide;
    DemonRoot(Creature* demon,Creature* hait):CreatureAI(demon),guide(hait){}
    // Native World98011 event6/action45/target19 endpoint, with the actual
    // Unit::Kill AI-before-instance dispatch exported into this host.
    void JustDied(Unit*)override{if(guide->AI())guide->AI()->SetData(1,1);}
};
struct Defeat:Opening
{
    Defeat(){delete foe.i_AI;foe.i_AI=new DemonRoot(&foe,&first);Accept();Tick(10000);Tick(23000);assert(foe.unitFlags==0);}
    void Die(Unit* killer=nullptr){foe.deathState=CORPSE;Unit ally;ally.map=&map;(killer?killer:&ally)->DispatchDeath(&foe);}
    void Credited(){assert(opening().defeated&&opening().credited&&scenario.step==1&&scenario.progress[29068].Counter==1);assert(player.credits==std::vector<uint32>{46868}&&other.credits.empty()&&Count(first,5)==1);}
    void Arrive(){first.x=3805.70f;first.y=1833.99f;first.z=949.62f;first.spline.done=true;if(first.motion.pointOwner)first.motion.pointOwner->Finalize(first);first.motion.Retire();Tick(0);}
};
int main()
{
    _set_abort_behavior(0,_CALL_REPORTFAULT);std::cout<<std::unitbuf;uint32 cases=0;
    // Complete pinned actor+instance+96-byte opening, rather than a copied
    // failing callback. Proximity picks another player; AI retirement loses10s.
    {Opening r;instance_monk_intro_before_defeat old(&r.map);r.map.script=&old;r.map.i_data=&old;delete r.first.i_AI;r.first.i_AI=new npc_master_hait_before_defeat::npc_master_hait_before_defeatAI(&r.first);r.first.i_AI->SetData(1,1);assert(r.other.credits==std::vector<uint32>{46868}&&r.player.credits.empty());assert(r.first.AIM_Destroy());clockNow+=10000;r.map.Update(10000);assert(!r.first.motion.points&&!Count(r.first,6));++cases;}
    {Opening r;instance_monk_intro_before_defeat old(&r.map);r.map.script=&old;r.map.i_data=&old;delete r.foe.i_AI;r.foe.i_AI=new DemonRoot(&r.foe,&r.first);assert(r.first.AIM_Destroy());r.foe.deathState=CORPSE;r.pet.DispatchDeath(&r.foe);assert(r.player.credits.empty()&&r.other.credits.empty());++cases;}
    std::cout<<"COUNTERFACTUAL full pinned8314 actor/instance/opening plus native death order: wrong nearest recipient and lost10s outro, or no46868 with missing Hait AI. World event6/action45 endpoint controlled; no client reproduction claimed.\n";
    for(uint32 killer=0;killer<5;++killer)for(uint32 mode=0;mode<4;++mode)
    {Defeat r;if(mode){assert(r.first.AIM_Destroy());if(mode==2)r.first.i_AI=new Hait(&r.first);if(mode==3){assert(r.foe.AIM_Destroy());}}Unit ally;ally.map=&r.map;r.Die(killer==0?static_cast<Unit*>(&r.player):killer==1?&r.pet:killer==2?&r.guardian:killer==3?static_cast<Unit*>(&r.first):&ally);r.Credited();for(uint32 i=0;i<50;++i){r.instance.CreatureDies(&r.foe,&r.pet);if(r.first.AI())r.first.AI()->SetData(1,1);}r.Tick(1999);assert(!Count(r.pupil,3));r.Tick(1);assert(Count(r.pupil,3)==1);r.Tick(2000);assert(Count(r.pupil,4)==1);r.Tick(3000);assert(Count(r.pupil,5)==1);r.Tick(2999);assert(!r.first.motion.points);r.Tick(1);assert(r.opening().moving&&r.first.motion.points==1&&Count(r.first,6)==1&&!Count(r.first,7));r.Arrive();assert(r.opening().arrived&&Count(r.first,7)==1);r.Tick(2999);assert(!Count(r.first,8));r.Tick(1);assert(Count(r.first,8)==1);r.Tick(2999);assert(!Count(r.first,9));r.Tick(1);assert(Count(r.first,9)==1&&r.first.talks.back().second==r.player.guid&&r.opening().ended);r.Tick(100000);r.Credited();++cases;}
    for(uint32 gate=0;gate<17;++gate)
    {Defeat r;switch(gate){case 0:r.foe.entry=1;break;case 1:r.foe.spawn=1;break;case 2:r.foe.guid={901,2};break;case 3:r.foe.world=false;break;case 4:r.foe.predelete=true;break;case 5:r.map.creatures.erase(r.foe.guid);break;case 6:r.player.quest.ScriptLifetime.reset();break;case 7:r.player.quest.ScriptLifetime=std::make_shared<uint8>();break;case 8:r.player.status=0;break;case 9:r.player.reward=true;break;case 10:r.player.type=1;break;case 11:r.scenario.step=1;break;case 12:r.scenario.id=1;break;case 13:r.map.id=1;break;case 14:r.map.unloading=true;break;case 15:r.opening().launched=false;break;case 16:r.foe.unitFlags=256;break;}if(gate==13){r.foe.deathState=CORPSE;r.opening().Death(&r.foe,&r.guardian);}else r.Die(&r.guardian);assert(!r.opening().defeated&&r.player.credits.empty()&&r.other.credits.empty()&&!Count(r.first,5));++cases;}
    {Defeat r;r.instance.CreatureDies(&r.foe,&r.player);assert(!r.opening().defeated&&r.player.credits.empty());r.first.i_AI->SetData(1,1);r.first.i_AI->MovementInform(POINT_MOTION_TYPE,1);r.Tick(10000);assert(!r.opening().defeated&&!Count(r.first,5)&&!Count(r.first,7));++cases;}
    for(uint32 gate=0;gate<9;++gate)
    {Defeat r;switch(gate){case 0:r.player.alive=false;break;case 1:r.player.world=false;break;case 2:r.player.contact=false;break;case 3:r.player.teleport=true;break;case 4:r.player.phase=false;break;case 5:r.player.gm=true;break;case 6:r.player.distance=100.01f;break;case 7:r.map.creatures.erase(r.first.guid);break;case 8:r.first.hidden=true;break;}r.Die(&r.pet);assert(r.opening().defeated&&!r.opening().credited&&r.player.credits.empty()&&r.other.credits.empty());r.player.alive=r.player.world=r.player.contact=r.player.phase=true;r.player.teleport=r.player.gm=false;r.player.distance=100;r.first.hidden=false;r.map.creatures[r.first.guid]=&r.first;r.Tick(2000);r.Credited();++cases;}
    {Defeat r;r.player.accept=false;auto prior=warnings;r.Die(&r.guardian);assert(!r.opening().credited&&r.player.credits==std::vector<uint32>{46868}&&warnings==prior+1);r.Tick(1999);assert(r.player.credits.size()==1);r.player.accept=true;r.Tick(1);assert(r.opening().credited&&r.player.credits.size()==2&&r.scenario.step==1);r.Tick(2000);assert(r.player.credits.size()==2);++cases;}
    for(uint32 mutation=0;mutation<5;++mutation)
    {Defeat r;r.player.onCredit=[&]{switch(mutation){case 0:r.instance.Update(50000);r.instance.CreatureDies(&r.foe,&r.guardian);break;case 1:r.player.quest.ScriptLifetime=std::make_shared<uint8>();break;case 2:r.map.creatures.erase(r.first.guid);break;case 3:r.map.unloading=true;break;case 4:r.scenario.id=1;break;}};r.Die(&r.guardian);r.player.onCredit={};assert(!r.opening().defeatBusy&&r.player.credits==std::vector<uint32>{46868});if(!mutation)r.Credited();else assert(!Count(r.first,5));++cases;}
    for(uint32 missing=0;missing<3;++missing)
    {Defeat r;r.Die(&r.pet);if(missing==0)r.map.creatures.erase(r.pupil.guid);if(missing==1)r.pupil.hidden=true;if(missing==2)r.pupil.spawn=1;r.Tick(10000);assert(r.opening().moving&&r.first.motion.points==1&&!Count(r.pupil,3));r.pupil.spawn=362111;r.pupil.hidden=false;r.map.creatures[r.pupil.guid]=&r.pupil;r.Tick(1);assert(Count(r.pupil,3)==1&&Count(r.pupil,4)==1&&Count(r.pupil,5)==1&&r.first.motion.points==1);++cases;}
    {Defeat r;r.Die();r.first.motion.install=false;auto prior=warnings;r.Tick(10000);assert(!r.opening().moving&&warnings==prior+1&&r.first.motion.points==1);r.Tick(1999);assert(r.first.motion.points==1);r.first.motion.install=true;r.Tick(1);assert(r.opening().moving&&r.first.motion.points==2&&Count(r.first,6)==1);++cases;}
    for(uint32 badArrival=0;badArrival<4;++badArrival)
    {Defeat r;r.Die();r.Tick(10000);if(badArrival==0)r.first.i_AI->MovementInform(POINT_MOTION_TYPE,1);if(badArrival==1){r.first.x=3805.70f;r.first.y=1833.99f;r.first.z=949.62f;}if(badArrival==2)r.first.spline.done=true;if(badArrival==3){r.first.x=3805.70f;r.first.y=1833.99f;r.first.z=949.62f;r.first.spline.done=true;++r.first.spline.id;}r.Tick(1);assert(!r.opening().arrived&&!Count(r.first,7));if(badArrival>=2){r.Tick(2000);assert(r.opening().moving);}r.Arrive();assert(r.opening().arrived&&Count(r.first,7)==1);++cases;}
    {Defeat r;r.Die();r.Tick(10000);r.Arrive();r.player.distance=41;r.Tick(6000);assert(Count(r.first,8)==1&&!Count(r.first,9));r.first.nearest=&r.other;r.player.distance=40;r.Tick(1);assert(Count(r.first,9)==1&&r.first.talks.back().second==r.player.guid);++cases;}
    for(uint32 gate=0;gate<5;++gate)
    {Defeat r;r.Die();switch(gate){case 0:r.player.quest.ScriptLifetime=std::make_shared<uint8>();break;case 1:r.player.status=0;break;case 2:r.scenario.step=0;break;case 3:r.scenario.progress[29068].Counter=0;break;case 4:r.map.unloading=true;break;}r.Tick(10000);assert(r.opening().ended&&!r.first.motion.points&&Count(r.first,6)==0);++cases;}
    {Defeat r;r.Die();r.scenario.step=2;r.Tick(10000);assert(r.opening().moving);r.Arrive();r.Tick(6000);assert(r.opening().ended&&Count(r.first,9)==1);++cases;}
    {Defeat r;clockNow=0xFFFFFE00;r.Die();r.Tick(10000);r.Tick(1);assert(r.opening().moving);r.Arrive();r.Tick(6000);assert(Count(r.first,9)==1);++cases;}
    {Defeat r;r.Die();r.map.creatures.erase(r.first.guid);auto prior=warnings;r.Tick(120000);assert(r.opening().ended&&warnings==prior+1);r.map.creatures[r.first.guid]=&r.first;r.Tick(1);assert(!r.first.motion.points);++cases;}
    {Defeat r;r.player.onCredit=[&]{r.scenario.step=2;};r.Die(&r.pet);assert(r.opening().credited&&r.scenario.step==2&&Count(r.first,5)==1);r.Tick(10000);assert(r.opening().moving);++cases;}
    {Defeat r;Scenario replacement;r.player.accept=false;r.Die();replacement.step=0;manager.current=&replacement;r.Tick(2000);assert(r.opening().ended&&!r.first.motion.points);manager.current=&r.scenario;++cases;}
    {Defeat r;r.map.i_data=&r.instance;r.map.onUpdate=[&](uint32){r.Die(&r.guardian);};clockNow+=100000;r.map.Update(100000);assert(r.opening().credited&&!r.first.motion.points&&!Count(r.pupil,3));r.Tick(9999);assert(!r.first.motion.points);r.Tick(1);assert(r.opening().moving);++cases;}
    {Defeat r;r.Die();r.first.motion.onPoint=[&]{--r.first.spline.id;};r.Tick(10000);assert(!r.opening().moving&&r.opening().defeatPoint&&r.first.motion.points==1);r.first.motion.onPoint={};r.Tick(5000);assert(!r.opening().moving&&r.first.motion.points==1);++r.first.spline.id;r.Tick(1);assert(r.opening().moving&&r.first.motion.points==1);r.Arrive();assert(r.opening().arrived);++cases;}
    for(uint32 endpoint=0;endpoint<5;++endpoint)
    {Defeat r;bool fired=false;auto remove=[&]{if(!fired){fired=true;r.map.creatures.erase(r.first.guid);}};if(endpoint==0)r.first.onTalk=[&]{if(r.first.talks.back().first==5)remove();};if(endpoint==1)r.pupil.onTalk=remove;if(endpoint==2)r.first.onTalk=[&]{if(r.first.talks.back().first==6)remove();};if(endpoint==3)r.first.motion.onPoint=remove;if(endpoint==4)r.first.onTalk=[&]{if(r.first.talks.back().first==7)remove();};r.Die(&r.pet);r.Tick(10000);if(endpoint==4)r.Arrive();r.first.onTalk={};r.pupil.onTalk={};r.first.motion.onPoint={};r.map.creatures[r.first.guid]=&r.first;r.Tick(1);if(!r.opening().moving)r.Tick(10000);assert(r.opening().moving&&Count(r.first,5)==1&&Count(r.first,6)==1);r.Arrive();r.Tick(6000);assert(r.opening().ended&&Count(r.first,7)==1&&Count(r.first,8)==1&&Count(r.first,9)==1&&r.player.credits==std::vector<uint32>{46868});++cases;}
    std::cout<<"PASS "<<cases<<" actual Hait defeat ASAN contexts: complete pinned counterfactuals, native AI-before-instance death order, accepted owner/pet/guardian/ally, real46868/29068 acknowledgment/retry/exactly once, original full GUID/weak quest guards, missing/recreated AI/actors/reentry, authored2/4/7/10 and arrival3/6 clocks, actual owned point/spline/pose and interrupted/install retry, no fabricated completion. World/criteria/motion/visibility endpoints controlled; physical client/full943/all36 remain OPEN.\n";
}
