#define main PreservedLootMain
#include "TuurePickupConditionsTest.cpp"
#undef main
int main(int argc,char**argv)
{
 assert(argc==3);expectedQuest=42504;expectedMap=1620;expectedScenario=1123;expectedItem=128870;expectedLoot=251107;expectedClassMask=8;firstStage=8;lastStage=9;
 auto load=[](char*path){std::ifstream input(path);assert(input);std::vector<Condition> rows;Condition c;uint32 target;while(input>>c.ElseGroup>>c.ConditionType>>target>>c.ConditionValue1>>c.ConditionValue2>>c.ConditionValue3>>c.NegativeCondition){assert(!target);rows.push_back(c);}return rows;};
 auto loot=load(argv[1]),cast=load(argv[2]);assert(loot.size()==28&&cast.size()==56);ConditionList castList;for(auto&c:cast)castList.push_back(&c);LootItem item;for(auto&c:loot)item.conditions.push_back(&c);CachedLoot cached;uint32 cases=0;
 for(uint32 quest:{42504u,42627u})for(uint32 stage=0;stage<11;++stage)for(uint32 status:{0u,1u,3u,5u,6u})for(uint32 owner=0;owner<3;++owner)for(uint32 cls:{1u,8u,16u}){
  scenarioManager={};scenarioManager.scenario.step=stage;Player p;p.quest=quest;p.status=status;p.classMask=cls;p.owned=owner==1;p.bank=owner==2;
  bool valid=(status==1||status==3)&&cls==8;
  assert(item.AllowedForPlayer(&p)==(valid&&stage==8&&owner==0));assert((cached.Read(&p,&item)!=nullptr)==(valid&&stage==8&&owner==0));
  assert(conditionManager.IsObjectMeetToConditions(&p,castList)==(valid&&(stage==8||stage==9)&&owner==1));++cases;
 }
 scenarioManager={};Player p;assert(cached.Read(&p,&item));p.owned=true;assert(!cached.Read(&p,&item));p.owned=false;p.bank=true;assert(!cached.Read(&p,&item));
 for(uint32 bad=0;bad<7;++bad){scenarioManager={};Player p;p.owned=true;if(bad==0)p.alive=false;if(bad==1){p.rewarded=true;p.status=QUEST_STATUS_COMPLETE;}if(bad==2)p.mapId=1;if(bad==3)p.quest=41924;if(bad==4)scenarioManager.present=false;if(bad==5)scenarioManager.scenario.id=1078;if(bad==6)worldMap.instanced=false;assert(!conditionManager.IsObjectMeetToConditions(&p,castList));worldMap.instanced=true;}
 std::cout<<"PASS "<<cases<<" actual native condition/ElseGroup/LootItem/cached-slot cases: both quests,stage8 loot,stage8/9 owned native casts,all status/ownership/class combinations; death/history/wrong map/scenario/quest and cached ownership changes.\n";
}
