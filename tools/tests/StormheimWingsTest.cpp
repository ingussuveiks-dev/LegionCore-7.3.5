#include "WingsFixture.inc"
uint64 nextGuid=90000;
std::array<GameObject,4> circles;
std::array<Creature,4> captives;
std::array<std::unique_ptr<go_stormheim_wings_ai>,4> ais;
struct Run {
 Player p;player_stormheim_wings script;bool failSummon=false;std::vector<std::unique_ptr<Creature>> clones;
 Run(uint32 team=ALLIANCE){p.guid={++nextGuid};p.team=team;p.race=team==ALLIANCE?1:2;p.Accept(38413);players[p.guid]=&p;
  p.judgmentSummon=[&](uint32 entry,Position const& point,ObjectGuid viewer)->Creature*{assert(viewer==p.guid);if(failSummon)return nullptr;auto c=std::make_unique<Creature>();c->entry=entry;c->guid={++nextGuid};static_cast<Position&>(*c)=point;c->personal=viewer;creatures[c->guid]=c.get();clones.push_back(std::move(c));return clones.back().get();};
 }
 ~Run(){script.OnLogout(&p);for(auto& c:clones)creatures.erase(c->guid);players.erase(p.guid);}
 void Move(int i){static_cast<Position&>(p)=StormheimWings::Sites[i].point;}
 void Open(int i){Move(i);Spell cast(&p,&circles[i]);cast.Complete();assert(!cast.sends);}
 void Tick(uint32 diff){script.OnUpdate(&p,diff);}
};
int main()
{
 auto& q=store.quests[38413];q.id=38413;q.NextQuestID=39652;q.NextQuestIdChain=39652;q.special=QUEST_SPECIAL_FLAGS_KILL|QUEST_SPECIAL_FLAGS_CAST;
 for(int i=0;i<4;++i){auto const& s=StormheimWings::Sites[i];q.objectives.push_back({2,s.circle,1,0,int8(i+1)});templates.objects[s.circle]={};
  auto& go=circles[i];go.guid={uint64(80000+i)};go.entry=s.circle;go.dbGuid=s.spawn;static_cast<Position&>(go)=s.point;objects[go.guid]=&go;ais[i]=std::make_unique<go_stormheim_wings_ai>(&go);
  auto& c=captives[i];c.guid={uint64(81000+i)};c.entry=s.actor;c.dbGuid=s.actorSpawn;static_cast<Position&>(c)=s.actorPoint;c.stand=3;creatures[c.guid]=&c;
 }
 store.quests[39652].id=39652;store.starters.emplace(96465,39652);Creature vethir;vethir.guid={99999};vethir.entry=96465;creatures[vethir.guid]=&vethir;
 for(auto team:{ALLIANCE,HORDE}){std::array<int,4> order{0,1,2,3};unsigned orders=0;do{Run r(team);for(int i:order){r.Move(i);go_stormheim_wings script;assert(script.OnGossipHello(&r.p,&circles[i]));ais[i]->GossipHello(&r.p,false);assert(r.clones.empty()||r.clones.size()==r.p.packets);
   auto before=r.clones.size();r.Open(i);assert(r.clones.size()==before+1&&r.p.GetQuestObjectiveData(q.id,StormheimWings::Sites[i].circle)==1);
   auto clone=r.clones.back().get();assert(clone->stand==0&&clone->personal==r.p.guid&&clone->bound&&captives[i].stand==3);
   assert(circles[i].ShouldHideFor(r.p.guid)&&captives[i].ShouldHideFor(r.p.guid));r.Open(i);assert(r.clones.size()==before+1);
   r.Tick(1999);assert(!clone->fly&&!clone->spline.launches);r.Tick(1);assert(clone->fly&&!clone->gravity&&clone->spline.launches==1);
   clone->spline.finished=true;r.Tick(1000);assert(!clone->world);
  }assert(r.p.completions==1&&r.p.CanRewardQuest(&q,false));r.p.m_RewardedQuests.insert(q.id);r.p.quests.erase(q.id);r.Tick(500);assert(r.p.GetNextQuest(vethir.guid,&q)==&store.quests[39652]);++orders;
 }while(std::next_permutation(order.begin(),order.end()));assert(orders==24);}
 {Run a,b;a.Open(0);assert(captives[0].ShouldHideFor(a.p.guid)&&!captives[0].ShouldHideFor(b.p.guid));b.Open(0);assert(a.clones.back()!=b.clones.back());a.p.quests.clear();a.p.data.clear();a.Tick(1);assert(!captives[0].ShouldHideFor(a.p.guid)&&captives[0].ShouldHideFor(b.p.guid)&&!a.clones.back()->world);a.p.Accept(q.id);a.Open(0);assert(a.p.GetQuestObjectiveData(q.id,240650)==1);}
 for(int failure=0;failure<10;++failure){Run r;r.Move(0);if(failure==0)r.failSummon=true;if(failure==1)heightMap.clear=false;if(failure==2)r.p.alive=false;if(failure==3)r.p.phase=false;if(failure==4)r.p.visible=false;if(failure==5)r.p.vehicle=true;if(failure==6)r.p.transport=true;if(failure==7)r.p.teleport=true;if(failure==8)captives[0].dbGuid=1;if(failure==9)circles[0].spawned=false;
  r.Open(0);assert(!r.p.packets&&r.clones.empty()&&!circles[0].ShouldHideFor(r.p.guid));heightMap.clear=true;captives[0].dbGuid=StormheimWings::Sites[0].actorSpawn;circles[0].spawned=true;
 }
 {Run r;r.failSummon=true;r.Open(0);r.failSummon=false;r.Open(0);assert(r.p.packets==1);r.script.OnMapChanged(&r.p);assert(!r.clones.back()->world&&!captives[0].ShouldHideFor(r.p.guid));r.Tick(500);assert(captives[0].ShouldHideFor(r.p.guid)&&r.p.packets==1);}
 {Run r;r.Move(0);Spell cast(&r.p,&circles[0]);assert(cast.Start());r.p.Accept(q.id);cast.Complete();assert(r.clones.empty()&&!r.p.packets);}
 {Run r;r.Move(0);Spell cast(&r.p,&circles[0]);assert(cast.Start());cast.gameObjTarget=&circles[1];cast.Complete();assert(r.clones.empty());}
 {Run r;r.Move(0);Spell cast(&r.p,&circles[0]);assert(cast.Start());r.p.x+=50;cast.Complete();assert(r.clones.empty());}
 {Run r;r.Move(0);Spell cast(&r.p,&circles[0]);cast.info.effect.MiscValue=13;cast.Complete();assert(cast.failed&&!r.p.packets);}
 {Run r;r.p.SetQuestObjectiveData(&q,&q.objectives[0],1);r.Move(0);r.Tick(1);assert(captives[0].ShouldHideFor(r.p.guid)&&r.clones.empty()&&!r.p.packets);}
 {Run r;r.Open(1);r.Tick(2000);auto c=r.clones.back().get();r.Tick(30000);assert(!c->world);}
 {Run r;r.Open(2);auto c=r.clones.back().get();r.p.alive=false;r.Tick(1);assert(!c->world&&!captives[2].ShouldHideFor(r.p.guid));r.p.alive=true;r.Tick(500);assert(captives[2].ShouldHideFor(r.p.guid)&&r.clones.size()==1&&r.p.packets==1);}
 {Run r;r.Move(0);auto old=circles[0].map;circles[0].map=1;go_stormheim_wings script;assert(!script.OnGossipHello(&r.p,&circles[0]));Spell cast(&r.p,&circles[0]);cast.Complete();assert(cast.sends==1&&r.clones.empty()&&!r.p.packets);circles[0].map=old;}
 {Run r;r.Open(0);auto c=r.clones.back().get();heightMap.clear=false;r.Tick(2000);assert(!c->world&&!c->spline.launches&&r.p.packets==1);heightMap.clear=true;}
 {Run r;r.Open(1);auto c=r.clones.back().get();r.p.phase=false;r.Tick(1);assert(!c->world&&r.p.packets==1);}
 std::cout<<"PASS: extracted opening/lock dispatch and actual rescue handlers, both factions/all24 orders, four private awake departures, original actors unchanged, per-owner hiding, native objective/completion/reward/GetNextQuest; interrupted/stale/retargeted casts, failed summon/collision retry, phase/death/map/vehicle gates, concurrent owners, abandon/reaccept/re-entry visibility and flight timeout. World construction, spline execution and client rendering remain fixtures.\n";
}
