#include "NativeHost.inc"
enum{CRITERIA_TYPE_SCRIPT_EVENT=73,SPELL_EFFECT_HANDLE_HIT=1,SPELL_EFFECT_HANDLE_HIT_TARGET=2,
 TARGET_FLAG_UNIT_MASK=2,TARGET_FLAG_GAMEOBJECT_MASK=0x800,SPELL_EFFECT_SEND_EVENT=61,DIFFICULTY_NONE=0};
using SpellEffIndex=uint32;
enum SpellCastResult{SPELL_CAST_OK=0,SPELL_FAILED_BAD_TARGETS=13};
constexpr SpellEffIndex EFFECT_0=0;
inline uint32 clockNow=100;
uint32 getMSTime(){return clockNow;}
#include "TimerDiff.inc"
struct SpellEffectInfo{uint32 Effect=61;int32 MiscValue=49816;uint32 GetProvidedTargetMask()const{return TARGET_FLAG_UNIT_MASK;}};
struct SpellInfo{uint32 Id=208171;SpellEffectInfo effect;SpellEffectInfo const* GetEffect(uint32,uint32)const{return &effect;}};
inline uint32 sEventScripts=1;
#define TC_LOG_DEBUG(...) ((void)0)
struct Spell{
 Unit* m_caster=nullptr;SpellInfo const* m_spellInfo=nullptr;uint32 m_diffMode=0,effectHandleMode=SPELL_EFFECT_HANDLE_HIT_TARGET;
 Unit* unitTarget=nullptr;GameObject* gameObjTarget=nullptr;WorldObject* focusObject=nullptr;
 void EffectSendEvent(SpellEffIndex);
};
#include "SendEvent.inc"
struct Hook{template<class T>void operator+=(T){}};
struct SpellScript{
 virtual ~SpellScript()=default;virtual bool Load(){return true;}virtual bool Validate(SpellInfo const*){return true;}virtual void Register(){}
 Unit* caster=nullptr;Unit* original=nullptr;Unit* hit=nullptr;bool prevented=false;Hook OnCheckCast,OnEffectHitTarget,AfterHit;
 Unit* GetCaster(){return caster;}Unit* GetOriginalTarget(){return original;}Unit* GetExplTargetUnit(){return nullptr;}Unit* GetHitUnit(){return hit;}
 void PreventHitDefaultEffect(uint32 effect){assert(!effect);prevented=true;}
};
// Expose actual handlers to the host. Native registration/dispatch is checked in the PE.
#define PrepareSpellScript(name) public:
#define SpellCheckCastFn(...) 0
#define SpellEffectFn(...) 0
#define SpellHitFn(...) 0
#include "Policy.inc"
#include "Actor.inc"
#ifndef BASELINE
#include "Cast.inc"
#endif
struct Run{
 Player owner,member;Creature actor,grash;CreatureAI grashAI;
 std::unique_ptr<npc_adel_waterspeaker::npc_adel_waterspeakerAI> ai;
 SpellInfo info;Spell native;
 Run(){scenario=Scenario{};scenario.step=1;scenario.achievements.progress[47197].Counter=1;manager.active=&scenario;
  nativeMapEvents=nativeInstanceEvents=warnings=0;clockNow=100;
  owner.guid={1,1};member.guid={1,2};ObjectAccessor::players={&owner,&member};
  actor.entry=104853;actor.guid={2,104853};actor.flags=UNIT_NPC_FLAG_SPELLCLICK|0x10;actor.grash=&grash;
  grash.guid={2,105298};grash.entry=105298;grash.flags=0x10;grash.primary=&grashAI;
  ObjectAccessor::creatures={&actor,&grash};ai=std::make_unique<npc_adel_waterspeaker::npc_adel_waterspeakerAI>(&actor);
  native.m_caster=&owner;native.m_spellInfo=&info;native.unitTarget=&owner;
 }
 ~Run(){ObjectAccessor::players.clear();ObjectAccessor::creatures.clear();}
 uint64 Rescue()const{auto f=scenario.achievements.progress.find(47123);return f==scenario.achievements.progress.end()?0:f->second.Counter;}
 uint64 Guards()const{auto f=scenario.achievements.progress.find(47197);return f==scenario.achievements.progress.end()?0:f->second.Counter;}
 void Tick(uint32 diff){clockNow+=diff;actor.Tick(diff);}
#ifndef BASELINE
 void Bind(spell_sharasdal_breaking_bonds& cast,Player* player=nullptr){cast.caster=player?player:&owner;cast.original=&actor;cast.hit=cast.caster;assert(cast.Load());}
 bool Finish(spell_sharasdal_breaking_bonds& cast,bool check=true){if(check&&cast.Check()!=SPELL_CAST_OK)return false;cast.BeforeEvent(0);if(!cast.prevented){native.m_caster=cast.caster;native.unitTarget=cast.hit;native.EffectSendEvent(0);}cast.AfterEvent();return !cast.prevented;}
 void Complete(){actor.Tick(5000);assert(!actor.poses);Tick(1999);assert(!actor.poses);Tick(1);assert(actor.poses==1);Tick(0);assert(grashAI.speech==std::vector<uint32>{5});}
#endif
};
int main(){
#ifdef BASELINE
 {Run r;scenario.achievements.progress[47197].Counter=0;r.ai->OnSpellClick(&r.owner);r.actor.functions.Update(7000);assert(r.Rescue()==1&&r.Guards()==1&&scenario.step==2&&!nativeMapEvents&&r.actor.poses==1);}
 {Run r;scenario.achievements.progress[47197].Counter=0;r.ai->OnSpellClick(&r.owner);r.native.EffectSendEvent(0);assert(r.Rescue()==1&&!r.Guards()&&scenario.step==1);r.actor.functions.Update(7000);assert(r.Rescue()==2&&r.Guards()==1&&scenario.step==2);}
 std::cout<<"PASS COUNTERFACTUAL dd6965c51: actual click callback gives49816 AND49957 without any successful native spell; with guards incomplete it duplicates actual native5s rescue before inventing guard completion at7s. Normal already-completed steps may reject these redundant calls. Raw freed-pointer branch deliberately not executed.\n";
#else
 unsigned matrices=0;
 for(uint32 status:{1u,3u})for(int mode=0;mode<4;++mode){Run r;r.owner.quest.Status=status;CreatureAI temporary;
  if(mode==1){r.actor.primary=&temporary;r.actor.i_disabledAI=r.ai.get();}
  if(mode==2)r.actor.i_disabledAI=r.ai.get();
  if(mode==3){r.actor.primary=nullptr;r.actor.i_disabledAI=r.ai.get();r.actor.IsAIEnabled=false;}
  spell_sharasdal_breaking_bonds cast;r.Bind(cast);assert(!cast.GetExplTargetUnit()&&cast.Check()==SPELL_CAST_OK);
  r.ai->OnSpellClick(&r.owner);r.actor.functions.Update(7000);assert(!r.Rescue()&&!r.actor.poses);
  clockNow+=5000;assert(r.Finish(cast));assert(r.Rescue()==1&&r.Guards()==1&&scenario.step==2&&nativeMapEvents==1&&nativeInstanceEvents==1);
  assert(r.actor.flags==0x10&&!r.actor.poses);cast.AfterEvent();r.Complete();
  assert(r.actor.animations==1&&r.actor.emotes==1&&r.grash.flags==0x11&&r.owner.credits[49816]==1&&!r.owner.credits[49957]);
  r.Tick(10000);assert(r.actor.poses==1&&r.grashAI.speech.size()==1);++matrices;
 }
 for(int denied=0;denied<24;++denied){Run r;spell_sharasdal_breaking_bonds cast;
  switch(denied){case 0:r.owner.world=false;break;case 1:r.owner.contact=false;break;case 2:r.owner.alive=false;break;case 3:r.owner.gm=true;break;case 4:r.owner.teleport=true;break;case 5:r.owner.classId=5;break;case 6:r.actor.world=false;break;case 7:r.actor.preDelete=true;break;case 8:r.actor.alive=false;break;case 9:r.actor.entry=999;break;case 10:r.actor.mapId=1;break;case 11:r.owner.map=&otherMap;break;case 12:r.owner.phase=false;break;case 13:r.owner.detect=false;break;case 14:r.owner.los=false;break;case 15:r.owner.interact=false;break;case 16:r.owner.rewarded=true;break;case 17:r.owner.record=false;break;case 18:r.owner.quest.Status=0;break;case 19:scenario.id=99;break;case 20:scenario.step=0;break;case 21:r.actor.z=795;break;case 22:scenario.achievements.progress[47197].Counter=0;break;case 23:r.actor.flags=0x10;break;}
  r.Bind(cast);assert(cast.Check()==SPELL_FAILED_BAD_TARGETS);assert(!r.Finish(cast,false));r.Tick(9000);r.Tick(9000);
  assert(!r.Rescue()&&!r.actor.poses&&!r.owner.credits[49816]&&r.grashAI.speech.empty());++matrices;
 }
 for(int changed=0;changed<8;++changed){Run r;spell_sharasdal_breaking_bonds cast;r.Bind(cast);assert(cast.Check()==SPELL_CAST_OK);auto lease=r.owner.quest.ScriptLifetime;auto actorLife=r.ai->_rescueLife;
  switch(changed){case 0:r.owner.quest.ScriptLifetime=std::make_shared<uint8>();break;case 1:r.owner.record=false;break;case 2:r.owner.map=&otherMap;break;case 3:r.ai->Reset();break;case 4:r.ai=std::make_unique<npc_adel_waterspeaker::npc_adel_waterspeakerAI>(&r.actor);break;case 5:r.actor.world=false;break;case 6:r.actor.flags=0;break;case 7:r.owner.alive=false;break;}
  clockNow+=5000;assert(cast.Check()==SPELL_FAILED_BAD_TARGETS&&!r.Finish(cast,false));r.Tick(9000);assert(!r.Rescue()&&!r.actor.poses);++matrices;
 }
 {Run r;spell_sharasdal_breaking_bonds first,second;r.Bind(first);r.Bind(second,&r.member);assert(first.Check()==SPELL_CAST_OK&&second.Check()==SPELL_CAST_OK);clockNow+=5000;assert(r.Finish(first));assert(second.Check()==SPELL_FAILED_BAD_TARGETS&&!r.Finish(second,false));assert(r.Rescue()==1&&r.owner.credits[49816]==1&&r.member.credits.empty());r.Complete();}
 {Run r;spell_sharasdal_breaking_bonds cast;r.Bind(cast);clockNow+=5000;cast.prevented=true;assert(!r.Finish(cast));assert(!r.Rescue()&&!r.ai->_rescueAccepted&&r.actor.HasFlag(UNIT_FIELD_NPC_FLAGS,UNIT_NPC_FLAG_SPELLCLICK));}
 {Run r;spell_sharasdal_breaking_bonds cast;r.Bind(cast);r.ai->OnSpellClick(nullptr);r.ai->OnSpellClick(&r.grash);r.actor.functions.Update(9000);r.Tick(9000);assert(!r.Rescue()&&!r.actor.poses&&r.actor.flags==(UNIT_NPC_FLAG_SPELLCLICK|0x10));}
 {Run r;auto actor=std::make_unique<Creature>();actor->entry=104853;actor->guid={2,12345};actor->flags=UNIT_NPC_FLAG_SPELLCLICK;auto ai=std::make_unique<npc_adel_waterspeaker::npc_adel_waterspeakerAI>(actor.get());ObjectAccessor::creatures={actor.get()};spell_sharasdal_breaking_bonds cast;cast.caster=&r.owner;cast.original=actor.get();cast.hit=&r.owner;assert(cast.Load()&&cast.Check()==SPELL_CAST_OK);auto held=ai->_rescueLife;ai.reset();ObjectAccessor::creatures.clear();actor.reset();clockNow+=5000;assert(cast.Check()==SPELL_FAILED_BAD_TARGETS);cast.BeforeEvent(0);assert(cast.prevented);cast.AfterEvent();assert(!r.Rescue());}
 {Run r;spell_sharasdal_breaking_bonds cast;cast.caster=&r.grash;cast.original=&r.actor;cast.hit=&r.grash;assert(cast.Load()&&cast.Check()==SPELL_FAILED_BAD_TARGETS);cast.BeforeEvent(0);assert(cast.prevented);r.Tick(9000);assert(!r.Rescue());}
 {Run r;spell_sharasdal_breaking_bonds cast;r.Bind(cast);clockNow+=5000;assert(r.Finish(cast));r.owner.world=false;ObjectAccessor::players.clear();r.Complete();assert(r.Rescue()==1);}
 {Run r;spell_sharasdal_breaking_bonds cast;r.Bind(cast);clockNow+=5000;assert(r.Finish(cast));r.actor.Tick(5000);r.ai.reset();r.actor.primary=nullptr;r.actor.IsAIEnabled=false;r.actor.functions.Update(9000);assert(!r.actor.poses);r.actor.IsAIEnabled=true;r.ai=std::make_unique<npc_adel_waterspeaker::npc_adel_waterspeakerAI>(&r.actor);r.actor.Tick(0);r.Tick(1999);assert(!r.actor.poses);r.Tick(1);r.Tick(0);assert(r.actor.poses==1&&r.Rescue()==1);}
 {Run r;spell_sharasdal_breaking_bonds cast;r.Bind(cast);clockNow+=5000;assert(r.Finish(cast));r.actor.Tick(0);r.ai->Reset();r.actor.Tick(0);r.Tick(2000);r.Tick(0);assert(r.actor.poses==1&&r.grashAI.speech.size()==1);r.ai->Reset();r.Tick(5000);assert(r.actor.poses==1&&r.grashAI.speech.size()==1);}
 {Run r;spell_sharasdal_breaking_bonds cast;r.Bind(cast);clockNow+=5000;assert(r.Finish(cast));r.actor.onEmote=[&]{r.ai->Reset();};r.actor.Tick(0);r.Tick(2000);r.Tick(0);r.Tick(0);assert(r.actor.poses==1&&r.grashAI.speech.size()==1);}
 {Run r;spell_sharasdal_breaking_bonds cast;r.Bind(cast);clockNow+=5000;assert(r.Finish(cast));r.actor.grash=nullptr;r.actor.Tick(0);r.Tick(2000);for(int i=0;i<10;++i)r.Tick(1000);assert(!warnings&&r.grashAI.speech.empty());r.actor.grash=&r.grash;r.Tick(1000);assert(r.grashAI.speech.size()==1&&r.grash.flags==0x11);}
 {Run r;spell_sharasdal_breaking_bonds cast;r.Bind(cast);clockNow+=5000;assert(r.Finish(cast));r.actor.grash=nullptr;r.actor.Tick(0);r.Tick(2000);for(int i=0;i<35;++i)r.Tick(1000);assert(warnings==1&&r.ai->_rescueNotifyExhausted);r.actor.grash=&r.grash;r.ai->Reset();r.Tick(0);r.Tick(0);assert(r.actor.poses==1&&r.grashAI.speech.size()==1);}
 {Run r;clockNow=0xffffff00u;spell_sharasdal_breaking_bonds cast;r.Bind(cast);clockNow+=5000;assert(r.Finish(cast));r.actor.Tick(5000);r.Tick(2000);assert(!r.actor.poses);r.Tick(1);r.Tick(0);assert(r.actor.poses==1);}
 {Run r;spell_sharasdal_breaking_bonds cast;r.Bind(cast);clockNow+=9000;assert(r.Finish(cast));r.actor.Tick(9000);r.Tick(0);r.Tick(0);assert(r.actor.poses==1);}
 {Run r;r.grash.flags|=UNIT_NPC_FLAG_GOSSIP;spell_sharasdal_breaking_bonds cast;r.Bind(cast);clockNow+=5000;assert(r.Finish(cast));r.actor.Tick(0);r.Tick(2000);r.Tick(0);assert(r.actor.poses==1&&r.grashAI.speech.empty());}
 {Run r;struct Consumed:CreatureAI{uint32 GetData(uint32 menu)const override{return menu==19483?4:0;}}consumed;r.grash.i_disabledAI=&consumed;spell_sharasdal_breaking_bonds cast;r.Bind(cast);clockNow+=5000;assert(r.Finish(cast));r.actor.Tick(0);r.Tick(2000);r.Tick(0);assert(r.actor.poses==1&&r.grash.flags==0x10&&r.grashAI.speech.empty());}
 {Run r;spell_sharasdal_breaking_bonds cast;cast.caster=&r.owner;cast.original=&r.owner;cast.hit=&r.owner;assert(cast.Load()&&cast.Check()==SPELL_FAILED_BAD_TARGETS);r.owner.nativeMapId=1220;spell_sharasdal_breaking_bonds other;other.caster=&r.owner;other.original=&r.grash;other.hit=&r.owner;assert(other.Load()&&other.Check()==SPELL_CAST_OK);}
 {Run r;spell_sharasdal_breaking_bonds cast;assert(cast.Validate(&r.info));r.info.effect.MiscValue=1;assert(!cast.Validate(&r.info));r.info.effect.MiscValue=49816;r.info.effect.Effect=3;assert(!cast.Validate(&r.info));}
 {Run r;r.ai->wait_heal=true;r.actor.states=UNIT_STATE_STUNNED;r.actor.health=1;spell_sharasdal_breaking_bonds cast;r.Bind(cast);clockNow+=5000;assert(r.Finish(cast));r.Complete();assert(r.ai->wait_heal&&r.actor.states==UNIT_STATE_STUNNED&&!r.actor.removals);r.owner.DealHeal(&r.actor,399);r.Tick(0);assert(!r.ai->wait_heal&&!r.actor.states&&r.actor.removals==1&&r.actor.poses==1);}
 std::cout<<"PASS "<<matrices<<" native rescue contexts plus cast/cancellation/owner and AI lease/replacement/lifetime, two simultaneous casters, absent/rejected default,7s minimum/stale diff/wrap, native receipt recovery, owner leaving after success, missing Grash/retry bound/reset and pose/role dedup. Full production actor/cast handlers and native EffectSendEvent/DealHeal/Notify/lifecycle/quest/FunctionProcessor/timer arithmetic compiled; map, criteria, geometry, caster hook framework and clock services controlled. No client/actual castbar/voice/full-chain proof.\n";
#endif
}
