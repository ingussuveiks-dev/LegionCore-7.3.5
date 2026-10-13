#include "HighmountainUnderkingTestSupport.h"
#include "HighmountainUnderkingEscape.inc"
int main()
{
 Creature boss;boss.entry=97266;boss.alive=false;static_cast<Position&>(boss)=UnderkingEscape::Start;
 StartHighmountainUnderkingEscape(&boss);assert(boss.summons==1);
 for(int reason=0;reason<5;++reason){Creature bad=boss;bad.summons=0;
  if(reason==0)bad.alive=true;if(reason==1)bad.entry=1;if(reason==2)bad.map=1;if(reason==3)bad.phase=false;if(reason==4)bad.z+=100;
  StartHighmountainUnderkingEscape(&bad);assert(!bad.summons);
  Creature actor;npc_highmountain_underking_escape ai(&actor);ai.IsSummonedBy(&bad);assert(actor.despawn&&!ai.active);
 }
 for(int failure=0;failure<4;++failure){Creature actor;actor.entry=97442;static_cast<Position&>(actor)=UnderkingEscape::Start;actor.failProp=failure==3;
  npc_highmountain_underking_escape ai(&actor);ai.IsSummonedBy(&boss);ai.IsSummonedBy(&boss);
  assert(ai.active&&!actor.walk&&actor.flags==14&&actor.phase);
  ai.UpdateAI(1999);assert(ai.talks.empty());ai.UpdateAI(1);assert(ai.talks==std::vector<uint32>{50});
  ai.UpdateAI(6005);assert(actor.casts.empty());ai.UpdateAI(1);assert(actor.casts==std::vector<uint32>{192468});
  SpellInfo unrelated{192269},escape{192468};ai.SpellFinishCast(&unrelated);assert(!actor.props);
  if(failure==1){ai.UpdateAI(5000);assert(actor.despawn&&!actor.props);continue;}
  ai.SpellFinishCast(&escape);ai.SpellFinishCast(&escape);assert(actor.props==1&&actor.motion.moves==1);
  if(!actor.failProp)assert(!actor.rubble.collision);
  ai.MovementInform(POINT_MOTION_TYPE,2);ai.MovementInform(POINT_MOTION_TYPE,1);assert(!actor.despawn);
  if(failure==2){ai.UpdateAI(10000);assert(actor.despawn);continue;}
  static_cast<Position&>(actor)=UnderkingEscape::Exit;ai.MovementInform(0,1);assert(!actor.despawn);
  ai.MovementInform(POINT_MOTION_TYPE,1);assert(actor.despawn&&ai.stage==ai.Finished);
 }
 Creature caster;caster.entry=97442;spell_highmountain_underking_escape spell;spell.caster=&caster;
 spell.Credit(0);assert(spell.suppressed.count(0));spell.suppressed.clear();caster.entry=1;spell.Credit(0);assert(spell.suppressed.empty());
 std::cout<<"PASS: death-only cave escape, native dialogue/cast before movement, verified arrival, duplicate callback guards, cast/path/prop failures and zero extra quest credit.\n";
}
