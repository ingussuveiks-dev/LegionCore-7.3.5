#include <cassert>
#include <cstdint>
#include <iostream>
using uint32=uint32_t;
struct Unit;
constexpr int UNIT_FIELD_FLAGS=1,UNIT_FLAG_PLAYER_CONTROLLED=2,MOTION_SLOT_ACTIVE=1;
constexpr int SMART_EVENT_AGGRO=4,ENCOUNTER_FRAME_ENGAGE=1,ENCOUNTER_FRAME_INSTANCE_START=2,IN_PROGRESS=1;
enum MovementGeneratorType{WAYPOINT_MOTION_TYPE,POINT_MOTION_TYPE,IDLE_MOTION_TYPE};
struct Motion {Unit* chased=nullptr;MovementGeneratorType GetMotionSlotType(int){return IDLE_MOTION_TYPE;}void MoveChase(Unit* who){chased=who;}};
struct Vehicle {Unit* passenger=nullptr;Unit* GetPassenger(int seat){assert(seat==0);return passenger;}};
struct SpellInfo{uint32 Id=212497;};struct Spell{SpellInfo info;SpellInfo* m_spellInfo=&info;};
constexpr int CURRENT_GENERIC_SPELL=0,CURRENT_AUTOREPEAT_SPELL=1,CURRENT_CHANNELED_SPELL=2;
struct Unit{
 uint32 entry=106693,mapId=1498;int map=1,phase=1,casts=0,exits=0,despawns=0;bool alive=true,combat=false,creature=true,castOK=true;
 Vehicle* kit=nullptr;Vehicle* vehicle=nullptr;Unit* lastTarget=nullptr;Unit* victim=nullptr;uint32 lastSpell=0;Spell* m_currentSpells[3]{};Motion motion;
 uint32 GetEntry(){return entry;}uint32 GetMapId(){return mapId;}int GetMap(){return map;}bool InSamePhase(Unit* p){return p->phase==phase;}
 bool IsAlive(){return alive;}bool isInCombat(){return combat;}Vehicle* GetVehicleKit(){return kit;}Vehicle* GetVehicle(){return vehicle;}
 Unit* ToCreature(){return creature?this:nullptr;}
 void CastSpell(Unit* target,uint32 spell,bool triggered){assert(triggered);++casts;lastTarget=target;lastSpell=spell;
  if(castOK){target->kit->passenger=this;vehicle=target->kit;}}
 void ExitVehicle(){++exits;if(vehicle){vehicle->passenger=nullptr;vehicle=nullptr;}}
 void DespawnOrUnsummon(){++despawns;alive=false;}
 bool HasFlag(int,int){return false;}bool Attack(Unit* who,bool){victim=who;return true;}
 Motion* GetMotionMaster(){return &motion;}void StopMoving(){}int GetPosition(){return 1;}
 void InterruptSpell(int slot,bool,bool){m_currentSpells[slot]=nullptr;}
 void InterruptNonMeleeSpells(bool withDelayed,uint32 spell_id=0,bool withInstant=true);
};
#include "Interrupt.inc"
#include "Intro.inc"
struct Events{Unit* invoker=nullptr;void ProcessEventsFor(int type,Unit* enemy){assert(type==SMART_EVENT_AGGRO);invoker=enemy;}};
struct Instance{bool CheckRequiredBosses(int,uint32){return true;}void SendEncounterUnit(int,Unit*){}void SetBossState(int,int){}};
struct SmartAI{Unit* me;Events events;Instance* instance=nullptr;int _bossId=0,mLastOOCPos=0;bool mCanCombatMove=true,mRun=true;
 Events* GetScript(){return &events;}void EnterEvadeMode(){}void SetRun(bool){}
 void EnterCombat(Unit* enemy);void AttackStart(Unit* who);
};
#include "EnterCombat.inc"
#include "AttackStart.inc"
int main(){
 Unit meph,skull;meph.entry=106692;Vehicle hand;meph.kit=&hand;
 DarkWhispersIntro::RideSkull(&meph,&skull);assert(skull.casts==1&&skull.lastSpell==211702&&skull.lastTarget==&meph&&hand.passenger==&skull&&skull.despawns==0);
 for(int i=0;i<10000;++i)DarkWhispersIntro::RideSkull(&meph,&skull);assert(skull.casts==1);
 DarkWhispersIntro::DespawnSkull(&meph);assert(skull.despawns==1&&skull.exits==1&&!hand.passenger);
 Unit retry;retry.castOK=false;DarkWhispersIntro::RideSkull(&meph,&retry);assert(!retry.vehicle&&retry.despawns==0);
 retry.castOK=true;DarkWhispersIntro::RideSkull(&meph,&retry);assert(retry.vehicle&&retry.casts==2);
 for(int mode=0;mode<7;++mode){Unit a,b;Vehicle v;a.entry=106692;a.kit=&v;if(mode==0)a.entry=1;if(mode==1)b.entry=1;if(mode==2)a.mapId=1;if(mode==3)b.map=2;if(mode==4)b.phase=2;if(mode==5)b.alive=false;if(mode==6)a.kit=nullptr;DarkWhispersIntro::RideSkull(&a,&b);assert(!b.casts);}
 // Never eject a player, a different actor, or a passenger in another instance.
 for(int mode=0;mode<3;++mode){Unit p;hand.passenger=&p;if(mode==0)p.entry=1;if(mode==1)p.creature=false;if(mode==2)p.map=2;DarkWhispersIntro::DespawnSkull(&meph);assert(!p.exits&&!p.despawns);}
 Unit head,ritual[3];Spell channel[3];for(int i=0;i<3;++i){ritual[i].entry=106698;ritual[i].m_currentSpells[2]=&channel[i];assert(!DarkWhispersIntro::StopBeam(&ritual[i],&head));}
 // Execute native SmartAI combat entry and chase with the actual attacker.
 Unit player,pet;pet.entry=416;
 for(Unit* attacker:{&player,&pet}){SmartAI ai{&ritual[0]};ritual[0].m_currentSpells[2]=&channel[0];ritual[0].combat=true;
  ai.EnterCombat(attacker);ai.AttackStart(attacker);
  assert(ai.events.invoker==attacker&&ritual[0].victim==attacker&&ritual[0].motion.chased==attacker);
  assert(!ritual[0].m_currentSpells[2]&&DarkWhispersIntro::StopBeam(&ritual[0],&head));
  assert(ritual[1].m_currentSpells[2]&&ritual[2].m_currentSpells[2]);
  assert(!DarkWhispersIntro::StopBeam(&ritual[1],&head)&&!DarkWhispersIntro::StopBeam(&ritual[2],&head));
  ritual[0].combat=false;assert(!DarkWhispersIntro::StopBeam(&ritual[0],&head));}
 ritual[0].alive=false;assert(DarkWhispersIntro::StopBeam(&ritual[0],&head));
 assert(DarkWhispersIntro::StopBeam(nullptr,&head));head.entry=1;assert(!DarkWhispersIntro::StopBeam(nullptr,&head));
 std::cout<<"PASS: production skull ride/cleanup, failed boarding retry, 10,000 duplicate attempts, foreign actors/maps/phases; native channel interruption and per-caster beam lifetime\n";
}
