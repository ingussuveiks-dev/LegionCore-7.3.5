#include <mutex>
constexpr int QUEST_SPECIAL_FLAGS_SPEAKTO=128;
#include "FinalJudgmentFixture.inc"
#include "Talked.inc"
struct Templates {
 CreatureTemplate data;
 CreatureTemplate* GetCreatureTemplate(uint32 id){return id==93093?&data:nullptr;}
 std::string GetScriptName(uint32){return "npc_stormheim_final_judgment";}
} templates;auto sObjectMgr=&templates;
struct PlayerScript {explicit PlayerScript(char const*){}virtual void OnUpdate(Player*,uint32){}virtual void OnMapChanged(Player*){}virtual void OnLogout(Player*){}};
struct InstanceScript {void OnCreatureDamageTaken(Creature*,Unit*,uint32&){}};
uint32 Unit::DealFinalDamage(Creature* victim,uint32 damage)
{
 DamageEffectType damagetype=0;
 if(victim->IsAIEnabled)victim->AI()->DamageTaken(this,damage,damagetype);
 uint64 health=victim->GetHealth(this);
#include "NativeScaledHook.inc"
 if(health<=damage)victim->alive=false;else victim->health-=damage;
 return damage;
}
#include "FinalJudgment.inc"
uint64 nextGuid=500000;
Creature original,originalSpirit;
struct Run {
 Player p;player_stormheim_final_judgment script;
 std::vector<std::unique_ptr<Creature>> actors;std::vector<std::unique_ptr<SmartAI>> ais;
 bool failBoss=false,failSpirit=false,acceptPort=true,synchronous=false;Position destination;uint32 destinationMap=0;
 Run(uint32 team=ALLIANCE){
  p.guid={++nextGuid};p.team=team;p.race=team==ALLIANCE?1:2;p.Accept(38818);players[p.guid]=&p;static_cast<Position&>(p)=StormheimFinalJudgment::FaljarPoint;
  p.judgmentSummon=[&](uint32 entry,Position const& point,ObjectGuid viewer)->Creature*{
   assert(viewer==p.guid);if((entry==93093&&failBoss)||(entry==93428&&failSpirit))return nullptr;
   auto c=std::make_unique<Creature>();c->guid={++nextGuid};c->entry=entry;c->personal=viewer;static_cast<Position&>(*c)=point;c->fields[UNIT_FIELD_FLAGS]=UNIT_FLAG_IMMUNE_TO_PC;
   SmartAI* ai=entry==93093?static_cast<SmartAI*>(new npc_stormheim_final_judgment(c.get())):new SmartAI(c.get());
   creatures[c->guid]=c.get();actors.push_back(std::move(c));ais.emplace_back(ai);ai->IsSummonedBy(&p);return actors.back().get();
  };
  p.finalTeleport=[&](uint32 map,Position const& point){destinationMap=map;destination=point;if(!acceptPort)return false;if(synchronous)Arrive();else p.teleport=true;return true;};
 }
 ~Run(){script.OnLogout(&p);for(auto& c:actors)creatures.erase(c->guid);players.erase(p.guid);}
 void Tick(uint32 diff=500){script.OnUpdate(&p,diff);}
 Creature* Boss(){auto i=StormheimFinalJudgment::runs.find(p.guid);return i==StormheimFinalJudgment::runs.end()?nullptr:p.GetMap()->GetCreature(i->second.boss);}
 npc_stormheim_final_judgment* AI(){return static_cast<npc_stormheim_final_judgment*>(Boss()->AI());}
 void Begin(){Tick();uint32 live=0;for(auto const& c:actors)if(c->world)++live;assert(Boss()&&live==2&&original.hidden.count(p.guid)&&originalSpirit.hidden.count(p.guid));if(p.GetQuestStatus(38818)==QUEST_STATUS_INCOMPLETE)assert(!Boss()->HasFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_IMMUNE_TO_PC));}
 void Win(Unit* attacker=nullptr,float scale=1){Begin();auto c=Boss();assert(!c->removedAuras.count(209164));c->multiplier=scale;Unit* source=attacker?attacker:static_cast<Unit*>(&p);assert(source->DealFinalDamage(c,1000000000)==0);assert(c->alive&&c->health==1&&p.GetQuestStatus(38818)==QUEST_STATUS_COMPLETE&&p.completions==1);}
 void Arrive(){p.map=1463;static_cast<Position&>(p)=destination;p.teleport=false;script.OnMapChanged(&p);}
};
int main()
{
 auto& q=store.quests[38818];q.id=38818;q.special=QUEST_SPECIAL_FLAGS_KILL;q.PrevQuestID=38815;q.objectives={{QUEST_OBJECTIVE_MONSTER,93442,1,0,0}};
 auto& previous=store.quests[38815];previous.id=38815;previous.NextQuestID=38818;
 auto& next=store.quests[39837];next.id=39837;next.PrevQuestID=38818;next.NextQuestID=38324;
 QuestMap _questTemplates;for(auto& e:store.quests)_questTemplates[e.first]=&e.second;for(auto const& e:_questTemplates){auto qinfo=e.second;
#include "QuestPrevLoader.inc"
 }
 store.starters.emplace(97319,39837);
 original.guid={377348};original.dbGuid=377348;original.entry=93093;static_cast<Position&>(original)=StormheimFinalJudgment::FaljarPoint;creatures[original.guid]=&original;
 originalSpirit.guid={292001};originalSpirit.dbGuid=292001;originalSpirit.entry=93428;static_cast<Position&>(originalSpirit)=StormheimFinalJudgment::AshildirPoint;creatures[originalSpirit.guid]=&originalSpirit;
 for(auto team:{ALLIANCE,HORDE})for(bool petBlow:{false,true})for(float scale:{0.001f,1.f,100.f}){
  Run r(team);assert(!r.p.CanTakeQuest(&q,false));r.p.m_RewardedQuests.insert(38815);assert(r.p.CanTakeQuest(&q,false));
  Creature pet;pet.ownerPlayer=&r.p;r.Win(petBlow?static_cast<Unit*>(&pet):static_cast<Unit*>(&r.p),scale);
  auto boss=r.Boss();assert(boss->personal==r.p.guid&&boss->bound&&boss->removedAuras.count(89319)&&boss->removedAuras.count(209363)&&boss->removedAuras.count(209164));
  boss->AI()->EnterEvadeMode();assert(r.Boss()==boss&&r.p.completions==1);
  uint32 damage=99;boss->AI()->DamageTakenAfterScaling(&r.p,damage,0);assert(!damage&&r.p.completions==1&&r.p.packets==1);
  r.Tick(4999);assert(!r.p.portCalls);r.Tick(1);assert(!r.p.portCalls);r.Tick(11999);assert(!r.p.portCalls);r.Tick(1);
  assert(r.p.portCalls==1&&r.destinationMap==1463&&r.destination.x==343.62f&&r.destination.y==354.5f&&r.destination.z==30.54f&&r.destination.o==4.57f);
  r.Tick(10000);assert(r.p.portCalls==1);r.Arrive();assert(!r.Boss()&&!original.hidden.count(r.p.guid)&&!originalSpirit.hidden.count(r.p.guid));
  r.p.m_RewardedQuests.insert(38818);r.p.quests.erase(38818);assert(r.p.CanTakeQuest(&next,false));Creature ash;ash.entry=97319;ash.guid={11546745};creatures[ash.guid]=&ash;
  // Native38818 RewardNextQuest is0:39837 is a normal quest-menu offer.
  assert(!r.p.GetNextQuest(ash.GetGUID(),&q)&&store.starters.count(97319)==1);creatures.erase(ash.guid);
 }
 {Run owner,other;owner.Begin();other.Begin();assert(owner.Boss()!=other.Boss());auto boss=owner.Boss();assert(!boss->AI()->CanAIAttack(&other.p));assert(other.p.DealFinalDamage(boss,1000000000)==0&&boss->health==1000&&!owner.p.completions&&!other.p.completions);owner.p.DealFinalDamage(boss,1000000000);assert(owner.p.completions==1&&!other.p.completions);}
 {Run r;r.Begin();auto c=r.Boss();assert(r.p.DealFinalDamage(c,100)==100&&c->health==900&&!r.p.completions);c->health=11;assert(r.p.DealFinalDamage(c,1)==0&&r.p.completions==1);}
 {Run r;r.acceptPort=false;r.Win();r.Tick(5000);r.Tick(12000);assert(r.p.portCalls==1&&r.Boss());r.Tick(2999);assert(r.p.portCalls==1);r.acceptPort=true;r.Tick(1);assert(r.p.portCalls==2&&r.p.completions==1);r.Arrive();}
 {Run r;r.synchronous=true;r.Win();r.Tick(5000);r.Tick(12000);assert(r.p.map==1463&&!r.Boss()&&r.p.portCalls==1);}
 {Run r;r.Win();r.script.OnLogout(&r.p);assert(!r.Boss());r.p.teleport=false;r.Begin();assert(StormheimFinalJudgment::runs.at(r.p.guid).stage==1&&r.p.completions==1);r.Tick(5000);r.Tick(12000);assert(r.p.portCalls==1);}
 for(int failure=0;failure<11;++failure){Run r;r.Begin();auto old=r.Boss();if(failure==0)r.p.quests.erase(38818);if(failure==1)r.p.Accept(38818);if(failure==2)r.p.alive=false;if(failure==3)r.p.map=1;if(failure==4)r.p.x+=81;if(failure==5)r.p.vehicle=true;if(failure==6)r.p.transport=true;if(failure==7)old->alive=false;if(failure==8)r.actors.back()->alive=false;if(failure==9)r.p.phase=false;if(failure==10)r.p.visible=false;r.Tick();assert(!r.Boss()&&!old->world&&!original.hidden.count(r.p.guid)&&!r.p.completions);}
 for(int fail=0;fail<2;++fail){Run r;r.failBoss=fail==0;r.failSpirit=fail==1;r.Tick();assert(!r.Boss()&&!original.hidden.count(r.p.guid));r.failBoss=r.failSpirit=false;r.Tick();assert(r.Boss());}
 {Run r;original.world=false;r.Tick();assert(!r.Boss());original.world=true;r.Tick();assert(r.Boss());r.AI()->EnterEvadeMode();assert(!r.Boss()&&!r.p.completions);r.Begin();r.Tick(180000);assert(!r.Boss());}
 {Run r;r.Begin();for(uint32 id:{93442u,93093u}){r.p.KilledMonsterCredit(id);r.p.TalkedToCreature(id,{1});assert(!r.p.GetQuestObjectiveData(38818,93442));}r.p.map=1463;r.script.OnMapChanged(&r.p);r.p.KilledMonsterCredit(93442);assert(!r.p.completions);}
 {Run r;auto& ordinary=store.quests[777];ordinary.id=777;ordinary.special=QUEST_SPECIAL_FLAGS_KILL;ordinary.objectives={{0,93442,1,0,0}};r.p.Accept(777);r.p.KilledMonsterCredit(93442);assert(r.p.GetQuestObjectiveData(777,93442)==1&&!r.p.GetQuestObjectiveData(38818,93442));}
 // The new production default hook leaves unrelated creatures unchanged.
 {Creature c;SmartAI ai(&c);Player p;c.multiplier=0.001f;assert(p.DealFinalDamage(&c,10)==10000&&!c.alive);}
 {Creature c;c.entry=117199;SmartAI ai(&c);Player p;uint32 d=123;ai.DamageTakenAfterScaling(&p,d,0);assert(d==123);}
 std::cout<<"PASS: production personal Faljar lifecycle and full native post-scaling damage hook; owner/pet scaled one-shots surrender once; both factions, prerequisite/Helheim next handoff, exact5s+12s delays, rejected/synchronous transfer, completed-ending reload, abandonment/reacceptance/death/range/vehicle/evade/timeout/summon retry, concurrent players, no ambient/group/talk shortcut and unchanged default damage. World, movement, packets and client remain fixtures.\n";
}
