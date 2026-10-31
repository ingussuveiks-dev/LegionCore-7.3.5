#include "NativeHost.inc"
static void Prepare(Spell& spell){spell.info.Id=209683;spell.info.effect.TriggerSpell=209684;spell.info.effect.MiscValue=125;spell.location.Relocate(50.5781f,802.392f,805.731f);}
int main(){
 _set_abort_behavior(0,_CALL_REPORTFAULT);
#ifdef BASELINE
 {Player player;player.map=1600;Spell spell(player);Prepare(spell);spell.EffectJumpDest(0);assert(player.movement.current&&player.movement.current->_arrivalSpellTargetGuid.IsEmpty());player.movement.Complete();assert(!player.casts);}
 {Player player;player.map=1600;player.launchResult=0;Spell spell(player);Prepare(spell);spell.EffectJumpDest(0);assert(player.movement.current&&(player.state&UNIT_STATE_LONG_JUMP));}
 // Counterfactual SELF-only change: native old arrival would then fire after
 // interruption or on another map. This is not a successful ordinary run.
 for(bool transfer:{false,true}){Player p;p.map=1600;Spell spell(p);Prepare(spell);spell.EffectJumpDest(0);p.movement.current->_arrivalSpellTargetGuid=p.GetGUID();
  if(transfer){p.map=1220;p.movement.Complete();}else{auto interrupted=std::move(p.movement.current);interrupted->Finalize(p);}assert(p.casts==1);
 }
 std::cout<<"PASS COUNTERFACTUAL71f30c267: actual destination jump leaves209684 self target empty; zero launch installs false movement. Controlled SELF-only counterfactual fires on interruption/map transfer, proving target normalization must retain actual arrival/origin-map gates. No client/DB destination/quest claim.\n";
#else
 unsigned cases=0;
 for(int state=0;state<9;++state){Player p;p.map=1600;Spell spell(p);Prepare(spell);
  if(state==1)p.launchResult=0;if(state==2)p.launchResult=-1;
  spell.EffectJumpDest(0);assert(!p.casts);
  if(state==1||state==2){assert(!p.movement.current&&!(p.state&UNIT_STATE_LONG_JUMP));++cases;continue;}
  assert(p.movement.current&&p.movement.current->_arrivalSpellTargetGuid.value==p.GetGUID().value&&p.movement.current->_arrivalMapId==1600);
  if(state==3){auto interrupted=std::move(p.movement.current);interrupted->Finalize(p);assert(!p.casts);++cases;continue;}
  if(state==4)p.alive=false;if(state==5)p.map=1220;if(state==6)p.m_cleanupDone=true;if(state==7)p.failCast=true;
  p.movement.Complete();p.movement.Complete();assert(p.casts==(state==0||state==7||state==8?1u:0u));if(p.casts)assert(p.castSpell==209684);++cases;
 }
 for(int bad=0;bad<4;++bad){Player p;p.map=bad==0?1220:1600;Spell spell(p);Prepare(spell);
  if(bad==1)spell.info.Id=209682;if(bad==2)spell.info.effect.TriggerSpell=209685;if(bad==3)spell.info.effect.TriggerSpell=0;
  spell.EffectJumpDest(0);assert(p.movement.current&&p.movement.current->_arrivalSpellTargetGuid.IsEmpty()&&p.movement.current->_arrivalMapId==uint32(-1));p.movement.Complete();assert(!p.casts);++cases;
 }
 {Creature npc;npc.map=1600;Spell spell(npc);Prepare(spell);spell.EffectJumpDest(0);assert(npc.movement.current&&npc.movement.current->_arrivalSpellTargetGuid.IsEmpty()&&npc.movement.current->_arrivalMapId==uint32(-1));npc.movement.Complete();assert(!npc.casts);++cases;}
 for(int guard=0;guard<4;++guard){Player p;p.map=1600;Spell spell(p);Prepare(spell);
  if(guard==0)spell.effectHandleMode=0;if(guard==1)p.flying=true;if(guard==2)spell.m_targets.destination=false;
  if(guard==3)spell.location=p.pose;
  spell.EffectJumpDest(0);assert(!p.jumps&&!p.casts&&!p.movement.current);++cases;
 }
 for(uint32 spellId:{191473u,203921u,206455u,206360u}){Player p;p.map=(spellId==206455||spellId==206360)?1539:1479;Spell spell(p);
  spell.info.Id=spellId;spell.info.effect.TriggerSpell=spellId==206455?206456:spellId==206360?206361:SkyholdDeparture::Arrival(spellId);spell.info.effect.MiscValue=300;spell.location.Relocate(1000,2000,300);
  spell.EffectJumpDest(0);assert(p.movement.current&&p.movement.current->_arrivalSpellTargetGuid.value==p.GetGUID().value);p.movement.Complete();assert(p.casts==1);++cases;
 }
 std::cout<<"PASS "<<cases<<" actual Spell::EffectJumpDest/CalculateJumpSpeeds/NativeDelay, Unit::CastSpell, MotionMaster and EffectMovementGenerator contexts: only map1600 Player209683->209684 self/origin map, positive actual launch/completion, cancellation/death/transfer/cleanup/failure/repeat and retained other native branches. Existing zero-distance no-launch remains; endpoint/spline/Spell services controlled. World209683 target and actual GO claim/item/equip/scene are covered separately by acquisition suites; actual client remains separate.\n";
#endif
}
