#include "HighmountainUnderkingTestSupport.h"
unsigned escapeStarts=0;
void StartHighmountainUnderkingEscape(Creature*){++escapeStarts;}
#include "HighmountainUnderking.inc"
int main()
{
 Creature mayla;static_cast<Position&>(mayla)=Underking::CaveMayla;mayla.entry=93846;
 Player base;static_cast<Position&>(base)=Underking::CaveMayla;base.mayla=&mayla;
 player_highmountain_underking meeting;
 Player p=base;meeting.OnUpdate(&p,1);meeting.OnUpdate(&p,1);assert(p.credits==1);
 Player second=base;meeting.OnUpdate(&second,1);assert(second.credits==1);
 for(int reason=0;reason<17;++reason){p=base;
  switch(reason){case 0:p.world=false;break;case 1:p.alive=false;break;case 2:p.map=1;break;case 3:p.status=0;break;case 4:p.status=2;break;case 5:p.teleport=true;break;case 6:p.vehicle=true;break;case 7:p.transport=true;break;case 8:p.flight=true;break;case 9:p.flying=true;break;case 10:p.x+=15;break;case 11:p.z+=80;break;case 12:p.mayla=nullptr;break;case 13:p.visible=false;break;case 14:p.los=false;break;case 15:p.phase=false;break;case 16:p.credits=1;break;}
  auto before=p.credits;meeting.OnUpdate(&p,1);assert(p.credits==before);
 }
 p=base;mayla.alive=false;meeting.OnUpdate(&p,1);assert(!p.credits);mayla.alive=true;
 mayla.x+=4;meeting.OnUpdate(&p,1);assert(!p.credits);mayla.x-=4;
 Creature boss;boss.entry=97266;boss.guid=20;Unit victim;victim.guid=30;boss.victim=&victim;
 npc_highmountain_underking ai(&boss);ai.EnterCombat(&victim);ai.UpdateAI(3999);assert(boss.casts.empty());
 boss.casting=true;ai.UpdateAI(1);assert(boss.casts.empty());boss.casting=false;ai.UpdateAI(0);assert(boss.casts==std::vector<uint32>{192269}&&boss.faces==1);
 ai.UpdateAI(8000);assert(boss.casts.size()==2);ai.UpdateAI(3000);assert(boss.casts.back()==213577);
 ai.Reset();boss.casts.clear();ai.UpdateAI(60000);assert(boss.casts.empty());
 spell_highmountain_underking_breath breath;breath.caster=&boss;breath.hit=&victim;
 breath.SuppressTrigger(1);breath.SuppressTrigger(2);assert(breath.suppressed==std::set<int>({1,2}));
 for(uint32 n=0;n<10;++n){victim.auras[{192270,boss.guid}]={n};breath.After();assert(victim.casts.empty());}
 victim.auras[{192270,boss.guid}]={10};breath.After();assert(victim.casts==std::vector<uint32>{192322}&&!victim.HasAura(192270,boss.guid));
 victim.auras[{192270,boss.guid}]={10};breath.After();assert(victim.casts.size()==1);
 Unit tauren;tauren.entry=97498;tauren.guid=31;tauren.auras[{192270,boss.guid}]={10};breath.hit=&tauren;breath.After();assert(tauren.HasAura(216101,boss.guid));
 for(int reason=0;reason<4;++reason){Unit target;target.auras[{192270,reason==0?999:boss.guid}]={10};breath.hit=reason==1?nullptr:&target;
  if(reason==2)target.alive=false;if(reason==3)boss.entry=1;breath.After();assert(target.casts.empty());boss.entry=97266;
 }
 boss.entry=1;breath.suppressed.clear();breath.SuppressTrigger(1);assert(breath.suppressed.empty());boss.entry=97266;
 spell_highmountain_underking_smash smash;smash.caster=&boss;smash.hit=&tauren;smash.After();assert(!tauren.alive&&boss.kills==1);
 for(int reason=0;reason<7;++reason){tauren.alive=true;tauren.entry=97498;tauren.phase=true;tauren.map=1220;tauren.auras={{{216101,boss.guid},{1}}};
  if(reason==0)tauren.entry=0;if(reason==1)tauren.entry=93846;if(reason==2)tauren.entry=97344;if(reason==3)tauren.phase=false;if(reason==4)tauren.map=1;if(reason==5)tauren.auras={{{216101,999},{1}}};if(reason==6)boss.entry=1;
  smash.After();assert(tauren.alive&&boss.kills==1);boss.entry=97266;
 }
 for(bool death:{false,true}){units={{30,&victim},{31,&tauren}};SpellInfo spell{192270};
  for(Unit* target:{&victim,&tauren}){target->auras.clear();for(uint32 id:{192270u,192322u,216101u}){target->auras[{id,boss.guid}]={};target->auras[{id,999}]={};}ai.SpellHitTarget(target,&spell);}
  ai.affected.insert(9999);ai.EnterCombat(&victim);if(death)ai.JustDied(&victim);else ai.Reset();assert(ai.affected.empty()&&ai.attacks.scheduled.empty());
  for(Unit* target:{&victim,&tauren})for(uint32 id:{192270u,192322u,216101u})assert(!target->HasAura(id,boss.guid)&&target->HasAura(id,999));
 }
 assert(escapeStarts==1);ai.JustDied(&victim);assert(escapeStarts==1);
 std::cout<<"PASS: cave-only one-time meeting; casting/timer guards; stack-ten player/NPC petrification; protected players/quest givers; own-caster cleanup on reset/death.\n";
}
