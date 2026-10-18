#include "VrektControlFixture.inc"
struct AggressorAI:CreatureAI {
 using CreatureAI::CreatureAI;int initializes=0,respawns=0,updates=0,los=0,attacks=0,enters=0,deaths=0,evades=0;
 void InitializeAI()override{++initializes;}void JustRespawned()override{++respawns;}
 void UpdateAI(uint32)override{++updates;}void MoveInLineOfSight(Unit*)override{++los;}
 void AttackStart(Unit*)override{++attacks;me->combat=true;}void EnterCombat(Unit*)override{++enters;}
 void JustDied(Unit*)override{++deaths;}void EnterEvadeMode(){++evades;me->combat=false;me->evading=true;}
};
#include "vrekt.h"
int temporaryDestroyed=0;
struct PetAI:CreatureAI {using CreatureAI::CreatureAI;int updates=0;~PetAI(){++temporaryDestroyed;}void UpdateAI(uint32)override{++updates;}};
struct PossessedAI:PetAI {using PetAI::PetAI;};
#include "VrektCharmSwap.inc"
#include "VrektVisibility.inc"
#include "VrektLifecycle.inc"
 // These are the actual later alive-branch blocks, with intervening world,
 // Unit update, throttling/death/regen services outside this fixture.
 if(alive){
#include "VrektRestoreAI.inc"
#include "VrektAliveAI.inc"
 }
 m_isUpdate=false;
}
struct Hit:Unit {uint32 Apply(Creature* creature,uint32 damage,DamageEffectType damagetype){
#include "VrektScaledDamage.inc"
 return damage;}};
int main(){
 npc_vrekt factory;Unit passerby;Hit hit;WorldObject seer;
 for(uint32 entry:{115846u,116180u,116600u,116601u}){
  GameTime::now=100;store.have=true;store.rotation.ResetTime=200;
  Creature c;c.entry=entry;c.flags=32768|0x10000000;std::unique_ptr<CreatureAI> owner(factory.GetAI(&c));assert(owner);c.i_AI=owner.get();
  auto& ai=*static_cast<npc_vrekt_rotation*>(c.i_AI);ai.InitializeAI();assert(ai.initializes==1&&!c.IsNeverVisible(&seer));
  ai.AttackStart(&passerby);ai.EnterCombat(&passerby);ai.UpdateAI(1);ai.MoveInLineOfSight(&passerby);ai.JustDied(&passerby);
  assert(ai.attacks==1&&ai.enters==1&&ai.updates==1&&ai.los==1&&ai.deaths==1);
  c.casting=true;GameTime::now=200;assert(c.IsNeverVisible(&seer)&&hit.Apply(&c,0xffffffff,DIRECT_DAMAGE)==0);
  c.Update(1);assert(!c.combat&&!c.casting&&c.react==REACT_PASSIVE&&ai.evades==1);
  int refresh=c.visibility;c.Update(1);assert(refresh==c.visibility);
  ai.AttackStart(&passerby);ai.EnterCombat(&passerby);ai.UpdateAI(1);ai.MoveInLineOfSight(&passerby);ai.JustDied(&passerby);
  assert(ai.attacks==1&&ai.enters==1&&ai.updates==1&&ai.los==1&&ai.deaths==1);
  c.alive=false;c.Update(1);assert(c.IsNeverVisible(&seer));
  store.rotation.ResetTime=300;c.Update(1);assert(!c.IsNeverVisible(&seer)&&!c.alive&&c.react==REACT_AGGRESSIVE&&(c.flags&0x10000000)&&!(c.flags&ai.GateFlags));
  c.alive=true;c.evading=false;ai.JustRespawned();assert(ai.respawns==1&&hit.Apply(&c,100,DIRECT_DAMAGE)==100);
 }
 for(int kind=0;kind<4;++kind){Creature c;if(!kind)c.hasData=false;if(kind==1)c.map=1;if(kind==2)c.data.areaId=1;if(kind==3)c.entry=1;assert(!factory.GetAI(&c));}
 // Native application, AI replacement, aura removal/control cleanup and
 // restoration, rather than assuming a charmed AggressorAI stays current.
 for(uint32 entry:{115846u,116180u,116600u,116601u})for(int type:{SPELL_AURA_MOD_CHARM,SPELL_AURA_MOD_POSSESS,SPELL_AURA_AOE_CHARM,0})for(bool missingCaster:{false,true}){
  GameTime::now=100;store.have=true;store.rotation.ResetTime=200;Player player;Creature c,pet;c.entry=entry;player.guid={1};c.guid={2};pet.guid={3};
  registry={{1,&player},{2,&c},{3,&pet}};pet.owner=&player;pet.guardian=true;player.minionGuid=pet.guid;player.m_Controlled.insert(pet.guid);
  std::unique_ptr<CreatureAI> original(factory.GetAI(&c));c.i_AI=original.get();auto& ai=*static_cast<npc_vrekt_rotation*>(c.i_AI);ai.InitializeAI();
  AuraApplication app{&c};CharmType kind=type==SPELL_AURA_MOD_POSSESS?CHARM_TYPE_POSSESS:(type==SPELL_AURA_AOE_CHARM?CHARM_TYPE_CONVERT:CHARM_TYPE_CHARM);
  assert(c.SetCharmedBy(&player,kind,&app));assert(!c.IsAIEnabled&&c.NeedChangeAI);c.Update(1);
  assert(c.i_disabledAI==original.get()&&c.i_AI!=original.get()&&c.IsAIEnabled&&!c.NeedChangeAI);
  assert((dynamic_cast<PossessedAI*>(c.i_AI)!=nullptr)==(kind==CHARM_TYPE_POSSESS));
  auto temporary=static_cast<PetAI*>(c.i_AI);assert(temporary->updates==1&&!c.IsNeverVisible(&seer));
  if(type)auras.push_back({&c,type,{missingCaster?nullptr:&player}});auras.push_back({&c,999,{&player}});
  GameTime::now=200;
#ifdef VREKT_OLD_CORE
  assert(!c.IsNeverVisible(&seer)&&hit.Apply(&c,0xffffffff,DIRECT_DAMAGE)==0xffffffff);
  c.Update(1);assert(c.isCharmed()&&player.charmGuid==c.guid&&c.i_disabledAI==original.get());
  c.RemoveCharmAuras();if(c.isCharmed())c.RemoveCharmedBy(nullptr);c.UpdateCharmAI();c.NeedChangeAI=false;c.IsAIEnabled=true;
  auras.clear();registry.clear();continue;
#else
  assert(c.IsNeverVisible(&seer)&&hit.Apply(&c,0xffffffff,DIRECT_DAMAGE)==0&&hit.Apply(&c,100,DOT)==0);
  int destroyed=temporaryDestroyed;c.Update(1);
  assert(!c.isCharmed()&&!player.charmGuid&&player.m_Controlled.size()==1&&player.m_Controlled.count(pet.guid));
  assert(c.i_AI==original.get()&&!c.i_disabledAI&&c.IsAIEnabled&&!c.NeedChangeAI&&temporaryDestroyed==destroyed+1);
  assert(!c.LastCharmerGUID&&c.faction==2817&&c.react==REACT_PASSIVE&&player.mover==&player&&player.minionGuid==pet.guid&&player.GetFirstControlled()==&pet);
  assert(ai.attacks==0&&auras.size()==1&&auras[0].type==999);int bars=player.removedBars;c.Update(1);assert(player.removedBars==bars);
  store.rotation.ResetTime=300;c.Update(1);assert(!c.IsNeverVisible(&seer)&&c.react==REACT_AGGRESSIVE&&!(c.flags&ai.GateFlags));
  auras.clear();registry.clear();
#endif
 }
 #ifndef VREKT_OLD_CORE
 for(int type:{SPELL_AURA_MOD_CHARM,SPELL_AURA_MOD_POSSESS,SPELL_AURA_AOE_CHARM,0}){
  GameTime::now=100;store.rotation.ResetTime=200;Player player;Creature c;player.guid={1};c.guid={2};registry={{1,&player},{2,&c}};
  std::unique_ptr<CreatureAI> original(factory.GetAI(&c));c.i_AI=original.get();original->InitializeAI();AuraApplication app{&c};
  CharmType kind=type==SPELL_AURA_MOD_POSSESS?CHARM_TYPE_POSSESS:(type==SPELL_AURA_AOE_CHARM?CHARM_TYPE_CONVERT:CHARM_TYPE_CHARM);
  assert(c.SetCharmedBy(&player,kind,&app));if(type)auras.push_back({&c,type,{&player}});
  GameTime::now=200;int destroyed=temporaryDestroyed;
  assert(c.IsNeverVisible(&seer)&&hit.Apply(&c,100,DIRECT_DAMAGE)==0);c.Update(1);
  assert(!c.isCharmed()&&!c.i_disabledAI&&c.i_AI==original.get()&&temporaryDestroyed==destroyed&&c.IsAIEnabled&&!c.NeedChangeAI&&player.mover==&player);
  assert(!c.LastCharmerGUID);auras.clear();registry.clear();
 }
 #endif
 // The original policy also applies before a pending native AI swap and
 // while manually disabled; default AI hooks still leave ordinary NPCs alone.
 GameTime::now=200;store.rotation.ResetTime=200;Creature disabled;disabled.IsAIEnabled=false;std::unique_ptr<CreatureAI> original(factory.GetAI(&disabled));disabled.i_AI=original.get();
#ifdef VREKT_OLD_CORE
 assert(!disabled.IsNeverVisible(&seer)&&hit.Apply(&disabled,100,DIRECT_DAMAGE)==100);
 std::cout<<"PASS: pinned old core reproduces visible expired controlled actors, accepted damage and retained native charm in all32 AI-swap cases.\n";
 return 0;
#endif
 assert(disabled.IsNeverVisible(&seer)&&hit.Apply(&disabled,100,DIRECT_DAMAGE)==0);disabled.Update(1);assert(disabled.react==REACT_PASSIVE);
 Creature ordinary;CreatureAI ordinaryAI(&ordinary);ordinary.i_AI=&ordinaryAI;ordinary.IsAIEnabled=false;assert(!ordinary.IsNeverVisible(&seer)&&hit.Apply(&ordinary,100,DIRECT_DAMAGE)==100);
 store.have=false;GameTime::now=100;assert(disabled.IsNeverVisible(&seer)); // An unrelated Grey Shoals rotation cannot activate Vrekt.
 std::cout<<"PASS:4 Vrekt actors retain Aggressor delegates; exact expiry/corpse/respawn/scope/default hooks and32 native charm/possession/conversion/direct-control cycles with actual AI swap/restoration, primary pet relations and suspended content gates. World/packet/Aggressor combat services remain fixtures.\n";
}
