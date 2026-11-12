#include "SecondTenTen20261112FifthFixture.h"
#include "Selected.inc"
int main(int argc,char**argv){try{Require(argc==2,"case");int test=std::stoi(argv[1]);Creature me(1);Player player(2);
switch(test){
case 0:{Fifth0 ai(&me);auto ghost=new Creature(3);me.summoned=ghost;ai.WaypointReached(23);delete ghost;me.summoned=nullptr;int before=talks;ai.WaypointReached(24);ai.WaypointReached(25);Require(talks==before,"A vanished Uther cannot speak through stale escort storage");ai.WaypointReached(23);ai.WaypointReached(24);ai.escort=&player;ai.WaypointReached(26);Require(player.credits==1,"Native eligible escort group credit remains independent of optional ghost dialogue");break;}
case 1:{Fifth1 ai(&me);auto old=new Creature(3);me.summoned=old;ai.JustReachedHome();delete old;Creature next(4);me.summoned=&next;ai.JustReachedHome();Require(me.lastTarget==next.guid.value,"Native reset channels to the newly returned rift");me.summoned=nullptr;ai.JustReachedHome();Require(next.disappears==1,"Only the current surviving rift is retired");ai.JustReachedHome();break;}
case 2:{GameObject go(3);Fifth2 ai(&go);auto owner=new Player(4);ai.IsSummonedBy(owner);delete owner;ai.UpdateAI(5500);Require(go.loot==1&&!go.destroyed,"Personal cage finite cleanup survives original player loss");ai.UpdateAI(5500);Require(go.loot==1,"Cage cleanup remains one shot");Player current(5);Fifth2 live(&go);live.IsSummonedBy(&current);live.UpdateAI(5500);Require(go.destroyed==1,"Live owner receives private cage cleanup");break;}
case 3:{Fifth3 ai(&me);auto owner=new Player(3);auto target=new Creature(4);target->pos.m_positionX=10;owner->auraTargets={target};me.owner=owner;ai.InitializeAI();delete target;ai.UpdateAI(1);Require(!me.casts&&me.removedAuras==2,"Missing wind-spirit target releases both visual auras");delete owner;ai.UpdateAI(1);Require(me.removedAuras==2,"One-shot launch cannot repeat after owner destruction");Player live(5);Creature next(6);live.auraTargets={&next};me.owner=&live;ai.InitializeAI();ai.UpdateAI(1);Require(me.lastSpell==196052&&me.lastTarget==next.guid.value,"Living actual owner and selected aura target retain launch");me.owner=new Player(7);me.owner->auraTargets={&next};ai.InitializeAI();delete me.owner;ai.UpdateAI(1);Require(me.removedAuras==4,"Missing owner releases visual state even while target remains");break;}
case 4:{Fifth4 ai(&me);ai.Reset();auto rider=new Player(3);ai.PassengerBoarded(rider,0,true);delete rider;SpellInfo fling{SPELL_FLING};ai.SpellHitTarget(&player,&fling);Require(me.vehicle.removals==1,"Passenger removal completes despite stale rider loss");ai.PassengerBoarded(&player,0,true);ai.PassengerBoarded(&player,0,false);ai.UpdateAI(100);Require(!me.casts,"Unboarded passenger cannot trigger delayed fling");Player live(4);ai.PassengerBoarded(&live,0,true);me.vehicle.onRemove=[&]{ai.PassengerBoarded(&live,0,false);};ai.SpellHitTarget(&player,&fling);Require(live.casts==1&&live.lastTarget==player.guid.value,"Real unboarding callback cannot lose the captured legitimate rider");ai.PassengerBoarded(&live,0,true);me.vehicle.onRemove=[&]{live.map=2;};ai.SpellHitTarget(&player,&fling);Require(live.casts==1,"Post-removal map transfer cannot receive a stale fling");me.vehicle.onRemove={};break;}
case 5:{Fifth5 ai(&me);auto illidan=new Creature(3);
#ifdef FIXED_ACTOR_CALLBACKS
ai.illidanGuid=illidan->guid;
#else
ai.illidan_saved=illidan;
#endif
delete illidan;Creature shadow(4);shadow.entry=NPC_SHADOW_OF_MEPHISTROTH;ai.JustSummoned(&shadow);Require(!shadow.ai.attacks,"A new shadow cannot attack a departed Illidan");Creature current(5);
#ifdef FIXED_ACTOR_CALLBACKS
ai.illidanGuid=current.guid;
#else
ai.illidan_saved=&current;
#endif
ai.JustSummoned(&shadow);Require(shadow.ai.target==current.guid.value,"Living actual Illidan remains the shadow target");break;}
case 6:{Fifth6 ai(&me);auto partner=new Player(3);ai.checkWebPlayer=true;SpellInfo spell{SPELL_TANGLED_WEB_DMG};ai.SpellHitTarget(partner,&spell);delete partner;ai.SpellHitTarget(&player,&spell);Require(!player.casts,"Missing original web partner cannot receive a stale beam");Player next(4);ai.checkWebPlayer=true;ai.SpellHitTarget(&next,&spell);ai.SpellHitTarget(&player,&spell);Require(player.lastTarget==next.guid.value&&player.lastSpell==SPELL_TANGLED_BEAM,"Normal actual web pairing retains original beam");break;}
case 7:{Fifth7 ai(&me);me.allowVictim=false;ai.event=true;ai.queue=false;auto actor=new Creature(3);
#ifdef FIXED_ACTOR_CALLBACKS
ai.elisandaGuid=actor->guid;
#else
ai.elisanda=actor;
#endif
delete actor;ai.UpdateAI(1000);Require(ai.event&&ai.text==0,"Failed intro-actor recreation must retain dialogue stage");Creature replacement(4);me.summoned=&replacement;ai.UpdateAI(5000);Require(ai.text==1&&talks==1,"Authored actor retry resumes the pending dialogue once");ai.UpdateAI(5000);ai.UpdateAI(5000);Require(!ai.event&&replacement.despawns==1,"Normal alternating intro reaches original cleanup");break;}
case 8:{Fifth8 ai(&me);me.pos.m_positionX=-2000;auto twin=new Creature(3);twin->pos.m_positionX=-2001;
#ifdef FIXED_ACTOR_CALLBACKS
ai.kohcromGuid=twin->guid;
#else
ai.pKohcrom=twin;
#endif
delete twin;ai.bKohcrom=true;ai.events.ScheduleEvent(EVENT_UPDATE_HEALTH,0);ai.UpdateAI(1);Require(me.health==1000000,"A missing twin cannot read or corrupt shared health");Creature current(4);current.pos.m_positionX=-2001;current.health=800000;
#ifdef FIXED_ACTOR_CALLBACKS
ai.kohcromGuid=current.guid;
#else
ai.pKohcrom=&current;
#endif
ai.UpdateAI(3000);Require(me.health==900000&&current.health==900000,"Native live-twin shared health cadence remains");break;}
case 9:{Fifth9 ai(&me);Creature ally(3);ally.health=400000;me.nearest[NPC_MINISTER_OF_AIR]=&ally;ai.events.ScheduleEvent(EVENT_GREATER_HEAL,0);ai.UpdateAI(1);Require(me.lastTarget==ally.guid.value&&me.lastSpell==SPELL_GREATER_HEAL,"Missing first ally type cannot block another eligible injured ally");Creature healthy(4);me.nearest[NPC_EXECUTOR_OF_THE_CALIPH]=&healthy;ai.events.ScheduleEvent(EVENT_GREATER_HEAL,0);int before=me.casts;ai.UpdateAI(1);Require(me.casts==before+1&&me.lastTarget==ally.guid.value,"A healthy earlier type cannot block one eligible heal");break;}
default:throw std::runtime_error("unknown case");}
std::cout<<"PASS complete production affected callbacks with removed actors and preserved ordinary transitions; native/client traversal separate\n";return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
