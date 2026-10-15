#include <array>
#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <vector>
#include <sstream>
#include <string>
using uint32=uint32_t;using uint8=uint8_t;using int32=int32_t;using uint64=uint64_t;
constexpr int INVALIDREASON_DONT_HAVE_REQ=1,INVALIDREASON_QUEST_FAILED_WRONG_RACE=2;
enum {CLASS_WARRIOR=1,CLASS_PALADIN,CLASS_HUNTER,CLASS_ROGUE,CLASS_PRIEST,CLASS_DEATH_KNIGHT,CLASS_SHAMAN,CLASS_MAGE,CLASS_WARLOCK,CLASS_MONK,CLASS_DRUID,CLASS_DEMON_HUNTER,QUEST_STATUS_NONE=0};
struct Quest{uint32 id=0;int32 AllowableClasses=0;uint64 AllowableRaces=uint64(-1);};
struct Store{std::map<uint32,Quest> quests;bool missing=false;Quest const* GetQuestTemplate(uint32 q){if(missing)return nullptr;return &(quests[q]=Quest{q});}}store;
auto sQuestDataStore=&store;
struct Player
{
 uint32 cls=9,level=103,race=2;bool canTake=true,bagSpace=true,shopOwned=false;
 std::map<uint32,uint32> statuses;std::set<uint32> chosen;std::vector<uint32> credits,added;
 uint32 getClass(){return cls;}uint32 getLevel(){return level;}
 bool IsQuestRewarded(uint32 q){return chosen.count(q)!=0;}
 uint32 GetQuestStatus(uint32 q){return statuses[q];}
 void KilledMonsterCredit(uint32 q){credits.push_back(q);}
 uint32 getClassMask()const{return 1u<<(cls-1);}uint64 getRaceMask()const{return uint64(1)<<(race-1);}
 template<typename... T>void SendCanTakeQuestResponse(T...)const{}
#include "QuestMasks.inc"
 bool CanTakeQuest(Quest const* q,bool){return canTake&&SatisfyQuestClass(q,false)&&SatisfyQuestRace(q,false);}bool CanAddQuest(Quest const*,bool){return bagSpace;}
 void AddQuest(Quest const* q,void*){added.push_back(q->id);statuses[q->id]=3;}
};
struct PlayerScript{explicit PlayerScript(char const*){}virtual void OnLogin(Player*){}virtual void OnPlayerChoiceResponse(Player*,uint32,uint32){}};
#include "Onboarding.inc"
int main(int argc,char**argv)
{
 assert(argc==3);std::ifstream file(argv[1]);assert(file);std::vector<std::pair<uint32,uint32>> pairs;
 uint32 race,cls;while(file>>race>>cls)pairs.emplace_back(race,cls);assert(pairs.size()==149);
 std::ifstream masks(argv[2]);assert(masks);std::string line;uint32 maskCases=0;
 while(std::getline(masks,line)){
  std::istringstream row(line);Quest q;uint32 routeClass;std::string eligible;
  row>>q.id>>routeClass>>q.AllowableClasses>>q.AllowableRaces>>eligible;assert(row);
  for(auto pair:pairs)if(pair.second==routeClass){Player p;p.cls=routeClass;p.race=pair.first;
   bool expected=(","+eligible+",").find(","+std::to_string(p.race)+",")!=std::string::npos;
   assert((p.SatisfyQuestClass(&q,false)&&p.SatisfyQuestRace(&q,false))==expected);++maskCases;
  }
 }
 class_hall_artifact_onboarding script;uint32 cases=0;
 for(auto const& route:ArtifactRoutes)
 {
  for(auto pair:pairs)if(pair.second==route.ClassId)
  {
   for(bool owned:{false,true})
   {
    Player p;p.cls=route.ClassId;p.race=pair.first;p.shopOwned=owned;
    script.OnPlayerChoiceResponse(&p,route.ChoiceId,route.ResponseId);
    assert(p.added==std::vector<uint32>{route.QuestId}&&!p.credits.empty());
    script.OnPlayerChoiceResponse(&p,route.ChoiceId,route.ResponseId);assert(p.added.size()==1);
    p.chosen.insert(route.ChosenQuestId);script.OnLogin(&p);assert(p.added.size()==1);
    p.statuses[route.QuestId]=0;script.OnLogin(&p);assert(p.added.size()==2); // Abandon/retry.
    ++cases;
   }
  }
  Player login;login.cls=route.ClassId;login.chosen.insert(route.ChosenQuestId);
  script.OnLogin(&login);assert(login.added==std::vector<uint32>{route.QuestId});
  Player shopOnly;shopOnly.cls=route.ClassId;shopOnly.shopOwned=true;
  script.OnLogin(&shopOnly);assert(shopOnly.added.empty()&&shopOnly.credits.empty());
  Player full;full.cls=route.ClassId;full.bagSpace=false;full.chosen.insert(route.ChosenQuestId);
  script.OnLogin(&full);assert(full.added.empty());full.bagSpace=true;
  script.OnLogin(&full);assert(full.added==std::vector<uint32>{route.QuestId});
  Player foreign;foreign.cls=route.ClassId==1?2:1;foreign.chosen.insert(route.ChosenQuestId);
  script.OnLogin(&foreign);assert(foreign.added.empty()&&foreign.credits.empty());
  for(int i=0;i<7;++i)
  {
   Player p;p.cls=route.ClassId;store.missing=false;
   if(i==0)p.level=97;if(i==1)p.canTake=false;if(i==2)p.bagSpace=false;
   if(i==3)p.statuses[route.QuestId]=1;if(i==4)p.statuses[route.QuestId]=6;
   if(i==5)store.missing=true;if(i==6)p.cls=route.ClassId==1?2:1;
   script.OnPlayerChoiceResponse(&p,route.ChoiceId,route.ResponseId);assert(p.added.empty());
  }
  store.missing=false;Player wrong;wrong.cls=route.ClassId;
  script.OnPlayerChoiceResponse(&wrong,route.ChoiceId,999999);assert(wrong.added.empty()&&wrong.credits.empty());
 }
 std::cout<<"PASS: all36 production artifact choices, "<<cases<<" native race/class and owned/unowned cases, "<<maskCases<<" native quest race/class admission cases; login/retry, class/level/admission/quest-log and repeat guards\n";
}
