#define MONK_NINE_ARRIVAL_TEST
#include "MonkChenEarlyPresentationTest.cpp"
#include "HaitOpeningActor.inc"
#include "HaitOpeningBeforeActor.inc"
#include "HaitOpeningBeforeInstance.inc"
uint32 Unit::ScriptedDamage(Unit* victim,uint32 damage){uint32 damagetype=0;
#include "HaitDamageDispatch.inc"
return damage;}
using Hait=npc_master_hait::npc_master_haitAI;
static_assert(offsetof(MonkHaitOpening::State,defeatPoint)==96,"Accepted opening prefix retains its deployed offsets");
struct Opening:Run
{
 Creature first,pupil,foe;GameObject door;
 Opening(){scenario.step=0;scenario.currentTree=nativeRoots[0];player.distance=4;player.quest.ScriptLifetime=std::make_shared<uint8>();first.map=pupil.map=foe.map=&map;first.guid={61,2};pupil.guid={62,2};foe.guid={63,2};first.entry=97778;first.spawn=362105;pupil.entry=98001;pupil.spawn=362111;foe.entry=98011;foe.spawn=362135;first.chuan=&pupil;pupil.demon=&foe;pupil.door=&door;first.nearest=&other;first.i_AI=new Hait(&first);pupil.i_AI=new CreatureAI(&pupil);foe.i_AI=new CreatureAI(&foe);map.creatures[first.guid]=&first;map.creatures[pupil.guid]=&pupil;map.creatures[foe.guid]=&foe;instance.OnCreatureCreate(&first);instance.OnCreatureCreate(&pupil);instance.OnCreatureCreate(&foe);}
 ~Opening(){delete first.i_AI;if(first.i_disabledAI!=first.i_AI)delete first.i_disabledAI;delete pupil.i_AI;delete foe.i_AI;}
 auto& opening(){return *instance.haitOpening;}
 void Accept(){instance.SetGuidData(MonkHaitOpening::AcceptData,player.guid);assert(instance.GetGuidData(MonkHaitOpening::AcceptedData)==player.guid);first.RemoveFlag(UNIT_FIELD_NPC_FLAGS,UNIT_NPC_FLAG_SPELLCLICK);first.i_AI->DoAction(1);assert(static_cast<Hait*>(first.i_AI)->_presentationFunctions.SizeQueue()==0);}
 void Tick(uint32 diff){clockNow+=diff;map.i_data=&instance;map.onUpdate=[this](uint32 elapsed){first.Lifecycle(elapsed);first.functions.Update(elapsed);pupil.functions.Update(elapsed);foe.functions.Update(elapsed);player.functions.Update(elapsed);};map.Update(diff);}
 void Launched(){assert(opening().launched&&door.uses==1&&pupil.walk&&pupil.motion.points==1&&foe.motion.points==1);assert(first.casts==std::vector<uint32>{193053}&&foe.unitFlags==(256|512|128));assert(foe.functions.SizeQueue()==2&&pupil.functions.SizeQueue()==2);assert(player.credits.empty()&&other.credits.empty());}
};
int main()
{
 _set_abort_behavior(0,_CALL_REPORTFAULT);std::cout<<std::unitbuf;uint32 cases=0;
 {Opening r;instance_monk_intro_before_opening old(&r.map);r.map.script=&old;r.map.i_data=&old;delete r.first.i_AI;r.first.i_AI=new npc_master_hait_before_opening::npc_master_hait_before_openingAI(&r.first);r.first.i_AI->DoAction(1);assert(r.first.AIM_Destroy());clockNow+=10000;r.map.Update(10000);assert(!r.door.uses&&r.first.casts.empty()&&!r.pupil.motion.points&&!r.foe.motion.points);++cases;}
 std::cout<<"COUNTERFACTUAL full pinnedc0f7fae1a actor/instance: native AIM_Destroy loses four accepted opening callbacks before10s; original door/movement/demon handoff never start. Controlled receipts/endpoints, not client traversal.\n";
 for(uint32 mode=0;mode<6;++mode)
 {Opening r;r.Accept();if(mode){assert(r.first.AIM_Destroy());if(mode==2)r.first.i_AI=new Hait(&r.first);if(mode==3)r.first.i_disabledAI=new Hait(&r.first);if(mode==4){r.first.i_AI=new Hait(&r.first);r.first.IsAIEnabled=false;}if(mode==5){r.first.i_AI=new Hait(&r.first);r.first.i_disabledAI=r.first.i_AI;r.first.i_AI=nullptr;}}r.Tick(999);assert(r.first.talks.empty());r.Tick(1);assert(Count(r.first,0)==1&&r.first.talks.back().second==r.player.guid);r.Tick(2999);assert(!Count(r.first,1));r.Tick(1);r.Tick(3000);r.Tick(2999);assert(!r.door.uses);r.Tick(1);r.Launched();for(uint32 n=0;n<100;++n)r.instance.Update(0);assert(Count(r.first,0)==1&&Count(r.first,1)==1&&Count(r.first,2)==1&&Count(r.first,3)==1&&r.door.uses==1);++cases;}
 for(uint32 gate=0;gate<14;++gate)
 {Opening r;switch(gate){case 0:r.player.openingGroup=0;break;case 1:r.player.openingPersonal=0;break;case 2:r.player.quest.ScriptLifetime.reset();break;case 3:r.player.type=1;break;case 4:r.player.status=0;break;case 5:r.player.reward=true;break;case 6:r.player.distance=6;break;case 7:r.player.phase=false;break;case 8:r.player.los=false;break;case 9:r.scenario.step=1;break;case 10:r.scenario.id=1;break;case 11:r.first.hidden=true;break;case 12:r.pupil.spawn=1;break;case 13:r.foe.deathState=CORPSE;break;}r.instance.SetGuidData(MonkHaitOpening::AcceptData,r.player.guid);r.Tick(40000);assert(r.opening().owner.IsEmpty()&&!r.door.uses&&r.first.talks.empty()&&r.player.credits.empty());++cases;}
 for(uint32 gate=0;gate<9;++gate)
 {Opening r;r.Accept();switch(gate){case 0:r.player.quest.ScriptLifetime.reset();break;case 1:r.player.quest.ScriptLifetime=std::make_shared<uint8>();break;case 2:r.player.map=nullptr;break;case 3:r.map.unloading=true;break;case 4:r.scenario.step=1;break;case 5:r.scenario.id=1;break;case 6:r.player.status=0;break;case 7:r.player.reward=true;break;case 8:r.player.type=1;break;}r.Tick(10000);assert(r.opening().ended&&!r.door.uses&&r.first.talks.empty());++cases;}
 for(uint32 missing=0;missing<5;++missing)
 {Opening r;r.Accept();auto id=missing==0?r.first.guid:missing==1?r.pupil.guid:r.foe.guid;if(missing<3)r.map.creatures.erase(id);if(missing==3)r.pupil.hidden=true;if(missing==4)r.foe.phaseVisible=false;r.Tick(10000);assert(!r.door.uses);r.map.creatures[r.first.guid]=&r.first;r.map.creatures[r.pupil.guid]=&r.pupil;r.map.creatures[r.foe.guid]=&r.foe;r.pupil.hidden=false;r.foe.phaseVisible=true;r.Tick(1);r.Launched();++cases;}
 {Opening r;r.Accept();r.map.creatures.erase(r.foe.guid);Creature replacement;replacement.map=&r.map;replacement.entry=98011;replacement.spawn=362135;replacement.guid={99,2};r.map.creatures[replacement.guid]=&replacement;r.pupil.demon=&replacement;r.Tick(10000);assert(!r.door.uses&&r.opening().demon==r.foe.guid);++cases;}
 for(uint32 mutation=0;mutation<4;++mutation)
 {Opening r;r.Accept();r.first.onTalk=[&]{if(mutation==0)r.instance.Update(0);if(mutation==1)r.player.quest.ScriptLifetime=std::make_shared<uint8>();if(mutation==2)r.map.creatures.erase(r.first.guid);if(mutation==3)r.map.unloading=true;};r.Tick(10000);r.first.onTalk={};assert(!r.opening().updating&&Count(r.first,0)==1);if(mutation==0)r.Launched();else assert(!r.door.uses);++cases;}
 // Reentry can unload an original actor during any external launch call.
 // Resume only its remaining commands, without replaying text/camera/door.
 for(uint32 endpoint=0;endpoint<4;++endpoint)
 {Opening r;r.Accept();r.Tick(7000);auto remove=[&]{r.map.creatures.erase(r.foe.guid);};if(endpoint==0)r.first.onTalk=[&]{if(r.first.talks.back().first==3)remove();};if(endpoint==1)r.first.onCast=remove;if(endpoint==2)r.pupil.motion.onPoint=remove;if(endpoint==3)r.door.onUse=remove;r.Tick(3000);assert(!r.opening().launched&&!r.foe.unitFlags);r.first.onTalk={};r.first.onCast={};r.pupil.motion.onPoint={};r.door.onUse={};r.map.creatures[r.foe.guid]=&r.foe;r.Tick(1);r.Launched();assert(Count(r.first,3)==1);++cases;}
 {Opening r;r.Accept();r.player.distance=100;r.Tick(10000);r.Launched();assert(!Count(r.first,0)&&Count(r.first,1)==1);++cases;}
 {Opening r;r.first.i_AI->DoAction(1);r.Tick(40000);assert(r.first.talks.empty()&&!r.door.uses&&r.opening().owner.IsEmpty());++cases;}
 {Opening r;r.Accept();r.first.nearest=&r.other;r.instance.SetGuidData(MonkHaitOpening::AcceptData,r.other.guid);r.Tick(10000);r.Launched();assert(r.opening().owner==r.player.guid&&r.first.talks[0].second==r.player.guid);++cases;}
 // Native per-Unit child clocks begin at actual10s launch, not the start of
 // an overdue frame. Cleanup remains on the Demon after Hait/Chuan loss.
 for(uint32 missing=0;missing<4;++missing)
 {Opening r;r.Accept();r.Tick(10000);r.Launched();r.Tick(19999);assert(r.pupil.casts.empty()&&r.foe.casts.empty());r.Tick(1);assert(r.pupil.casts==std::vector<uint32>{227140}&&r.foe.casts==std::vector<uint32>{193268}&&r.foe.vehicle.passenger);if(missing==1)r.map.creatures.erase(r.first.guid);if(missing==2)r.map.creatures.erase(r.pupil.guid);if(missing==3){assert(r.first.AIM_Destroy());}r.Tick(2999);assert(!r.foe.vehicle.removals);r.Tick(1);assert(r.foe.vehicle.removals==1&&!r.foe.vehicle.passenger&&!r.foe.unitFlags&&r.foe.attacks==(missing==1?0u:1u)&&r.player.credits.empty());++cases;}
 {Opening r;r.Accept();r.Tick(50000);r.Launched();r.Tick(19999);assert(r.foe.casts.empty());r.Tick(1);assert(r.foe.casts==std::vector<uint32>{193268});r.Tick(3000);assert(r.foe.attacks==1);++cases;}
 {Opening r;r.Accept();r.pupil.castAccepted=false;r.Tick(10000);r.Tick(23000);assert(!r.foe.unitFlags&&r.foe.vehicle.removals==1&&r.foe.attacks==1&&r.player.credits.empty());++cases;}
 {Opening r;r.Accept();r.Tick(10000);delete r.foe.i_AI;r.foe.i_AI=nullptr;r.Tick(23000);assert(!r.foe.unitFlags&&!r.foe.attacks);r.foe.i_AI=new CreatureAI(&r.foe);r.Tick(500);assert(r.foe.attacks==1);++cases;}
 {Opening r;clockNow=0xFFFFFE00;r.Accept();r.Tick(10000);assert(!r.door.uses);r.Tick(1);r.Launched();++cases;}
 // Actual post-scaling Unit dispatch reaches the instance with absent AI;
 // preserve only the original initial friendly actor's zero-damage policy.
 for(uint32 killer=0;killer<3;++killer)for(uint32 damage:{1u,1000000000u})
 {Opening r;assert(r.first.AIM_Destroy());Unit* attacker=killer==0?static_cast<Unit*>(&r.player):killer==1?&r.pet:&r.guardian;assert(attacker->ScriptedDamage(&r.first,damage)==0);assert(attacker->ScriptedDamage(&r.foe,damage)==damage);r.first.spawn=362271;assert(attacker->ScriptedDamage(&r.first,damage)==damage);r.first.spawn=362105;r.first.guid={90,2};assert(attacker->ScriptedDamage(&r.first,damage)==damage);++cases;}
 {Opening r;instance_monk_intro_before_opening old(&r.map);r.map.script=&old;assert(r.first.AIM_Destroy());assert(r.pet.ScriptedDamage(&r.first,1000000000u)==1000000000u);++cases;}
 {Opening r;r.Accept();r.map.creatures.erase(r.foe.guid);auto before=warnings;r.Tick(120000);assert(r.opening().ended&&warnings==before+1&&!r.door.uses);r.map.creatures[r.foe.guid]=&r.foe;r.Tick(1);assert(!r.door.uses);++cases;}
 std::cout<<"PASS "<<cases<<" actual instance-owned opening ASAN contexts: full predecessor AI-loss counterfactual, real1/4/7/10 deadlines, accepted full GUID/weak owner/marker admission, no-AI/replaced/suspended, missing actors/retry, once/reentry/cancel/fresh-clock/rollover, original Demon-owned20/23 casts/cleanup/attack and no fabricated credit. Injected120s expiry is expected and separate from recovery. Receipts/world/casts/vehicle/motion endpoints controlled; later46868 death/point1/client/full943/all36 remain OPEN.\n";
 RunEarlyPresentationCases();
 return 0;
}
