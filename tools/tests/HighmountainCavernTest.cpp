#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <set>
#include <vector>
using uint8=uint8_t;using uint32=uint32_t;using int32=int32_t;
using PrevQuests=std::vector<int32>;using ExclusiveQuestGroups=std::multimap<int32,uint32>;
enum {QUEST_STATUS_NONE=0,INVALIDREASON_DONT_HAVE_REQ=1,MAX_KILL_CREDIT=2,SMART_EVENT_FLAG_ALLOW_EVENT_IN_COMBAT=32};
#define ASSERT assert
struct Quest {uint32 id;int32 ExclusiveGroup=0;PrevQuests prevQuests;};
struct QuestStore {
 std::map<uint32,Quest> quests;ExclusiveQuestGroups mExclusiveQuestGroups;
 Quest const* GetQuestTemplate(uint32 id){auto i=quests.find(id);return i==quests.end()?nullptr:&i->second;}
} store;
auto sQuestDataStore=&store;
struct ObjectGuid {};
struct CreatureTemplate {uint32 Entry;uint32 KillCredit[2];};
struct Player {
 std::set<uint32> m_RewardedQuests;std::vector<uint32> credits;unsigned errors=0;
 bool SatisfyQuestPreviousQuest(Quest const*,bool);
 bool SatisfyQuestRace(Quest const*,bool){return true;}
 int GetQuestStatus(uint32){return QUEST_STATUS_NONE;}
 void SendCanTakeQuestResponse(int,Quest const*,char const*){++errors;}
 void KilledMonster(CreatureTemplate const*,ObjectGuid);
 void KilledMonsterCredit(uint32 id,ObjectGuid={}){credits.push_back(id);}
};
#include "CavernQuestHandlers.inc"
struct Creature {bool combat=true;unsigned maxHealth=100;float health=100;bool isInCombat(){return combat;}unsigned GetMaxHealth(){return maxHealth;}float GetHealthPct(){return health;}};
struct Event {struct {unsigned event_flags=1;struct {unsigned min=0,max=50,repeatMin=0,repeatMax=0;} minMaxRepeat;} event;};
struct SmartScript {
 Creature* me;unsigned actions=0;
 void RecalcTimer(Event&,unsigned,unsigned){}
 void ProcessAction(Event&){++actions;}
 void HealthEvent(Event& e){switch(0){case 0:
#include "CavernHealthHandler.inc"
 }}
};
int main()
{
 store.quests.emplace(39277,Quest{39277,0,{}});
 store.quests.emplace(39661,Quest{39661,0,{39277}});
 store.quests.emplace(39488,Quest{39488,-39488,{39661,39661}});
 store.quests.emplace(39489,Quest{39489,-39488,{39661}});
 store.quests.emplace(39487,Quest{39487,0,{39489,39488,39489}});
 store.quests.emplace(39498,Quest{39498,0,{39487,39487}});
 store.mExclusiveQuestGroups.emplace(-39488,39488);store.mExclusiveQuestGroups.emplace(-39488,39489);
 Player player;auto allowed=[&](unsigned q){return player.SatisfyQuestPreviousQuest(store.GetQuestTemplate(q),false);};
 assert(!allowed(39661)&&!allowed(39488)&&!allowed(39489)&&!allowed(39487)&&!allowed(39498));
 player.m_RewardedQuests.insert(39277);assert(allowed(39661)&&!allowed(39487));
 player.m_RewardedQuests.insert(39661);assert(allowed(39488)&&allowed(39489)&&!allowed(39487));
 for(auto first:{39488u,39489u}){player.m_RewardedQuests.insert(first);assert(!allowed(39487));player.m_RewardedQuests.erase(first);}
 player.m_RewardedQuests.insert(39488);player.m_RewardedQuests.insert(39489);assert(allowed(39487)&&!allowed(39498));
 player.m_RewardedQuests.insert(39487);assert(allowed(39498));
 // Actual core death-credit dispatch: crageaters count as the native target;
 // the retained transformed Gelmogg alias still credits Crystal Fury.
 CreatureTemplate crageater{95916,{95866,0}},gelmogg{95882,{100261,95881}};
 player.KilledMonster(&crageater,{});assert(player.credits==std::vector<uint32>({95916,95866}));
 player.credits.clear();player.KilledMonster(&gelmogg,{});assert(player.credits==std::vector<uint32>({95882,100261,95881}));
 Creature boss;SmartScript smart{&boss};Event e;
 for(float hp:{100.f,51.f}){boss.health=hp;smart.HealthEvent(e);assert(!smart.actions);}
 for(float hp:{50.f,39.f,1.f}){boss.health=hp;auto old=smart.actions;smart.HealthEvent(e);assert(smart.actions==old+1);}
 auto old=smart.actions;boss.combat=false;smart.HealthEvent(e);assert(smart.actions==old);boss.combat=true;boss.maxHealth=0;smart.HealthEvent(e);assert(smart.actions==old);
 std::cout<<"PASS: extracted production prerequisite checks require both cavern turn-ins in either order, no Spray shortcut; actual alternate death-credit dispatch; Gelmogg health event handles hits below 40%.\n";
}
