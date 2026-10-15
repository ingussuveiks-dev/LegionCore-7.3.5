#include <algorithm>
#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <vector>
using uint8=uint8_t;using uint16=uint16_t;using uint32=uint32_t;using uint64=uint64_t;
constexpr int QUEST_STATUS_REWARDED=6,QUEST_STATUS_NONE=0;
#define TC_LOG_DEBUG(...) ((void)0)
#include "Enums.inc"
struct GossipMenu{static uint64 GetRequiredNpcFlagForOption(GossipOptionNpc);};
#include "RequiredFlags.inc"
bool Admit(GossipOptionNpc optionNpc,uint32 npcflags){for(int i=0;i<1;++i){
#include "Admission.inc"
 return true;}return false;}
namespace WorldPackets{namespace NPC{struct ShowAdventureMap{int Guid=0;int Write(){return Guid;}};}namespace Quest{struct AdventureJournalStartQuest{uint32 QuestID;};}}
struct Quest{uint32 Id;};
struct QuestStore{std::map<int,Quest> rows;Quest const* GetQuestTemplate(uint32 id){auto i=rows.find(id);return i==rows.end()?nullptr:&i->second;}} quests;
auto sQuestDataStore=&quests;
struct AdventureMapPOIEntry{uint32 ID,Type,QuestID,PlayerConditionID;};
struct PoiStore:std::vector<AdventureMapPOIEntry*>{AdventureMapPOIEntry const* LookupEntry(uint32 id){for(auto p:*this)if(p->ID==id)return p;return nullptr;}} sAdventureMapPOIStore;
struct Player{
 bool take=true,add=true,condition=true,complete=false;uint16 m_adventure_questID=0;bool m_AdventureQuestChanged=false;
 std::map<int,int> status;int credits=0,starts=0,packets=0,lastGuid=0;
 uint16 getAdventureQuestID();int GetQuestStatus(uint32 id){return status[id];}
 bool CanTakeQuest(Quest const* q,bool){return take&&status[q->Id]==0;}bool CanAddQuest(Quest const*,bool){return add;}
 void AddQuest(Quest const* q,Player*){status[q->Id]=3;++starts;}bool CanCompleteQuest(uint32){return complete;}void CompleteQuest(uint32 id){status[id]=1;}
 void KilledMonsterCredit(int id){assert(id==97067);++credits;}
 void setAdventureQuestID(uint16 questID);
 void SendDirectMessage(int guid){++packets;lastGuid=guid;}
 void Select(GossipOptionNpc option,int guid){switch(option){
#include "Select.inc"
 default:break;}}
};
#include "Credit.inc"
struct Conditions{bool IsPlayerMeetingCondition(Player* p,int id){return id==1&&p->condition;}} conditions;
auto sConditionMgr=&conditions;
struct WorldSession{Player* _player;Player* GetPlayer(){return _player;}
 void HandleAdventureJournalStartQuest(WorldPackets::Quest::AdventureJournalStartQuest&);bool AdventureMapPOIAvailable(uint32);};
#include "Adventure.inc"
int main(int argc,char**argv){
 assert(argc==2);std::ifstream f(argv[1]);int entry;uint32 flags,flags2;int count=0;
 while(f>>entry>>flags>>flags2){assert(Admit(GossipOptionNpc::AdventureMap,flags));++count;}
 assert(count>0);assert(!Admit(GossipOptionNpc::AdventureMap,0));
 assert(!Admit(GossipOptionNpc::AdventureMap,uint32(UNIT_NPC_FLAG_VENDOR)));
 assert(Admit(GossipOptionNpc::Vendor,uint32(UNIT_NPC_FLAG_VENDOR))&&!Admit(GossipOptionNpc::Vendor,1));
 assert(Admit(GossipOptionNpc::Banker,uint32(UNIT_NPC_FLAG_BANKER))&&!Admit(GossipOptionNpc::Banker,1));
 assert(GossipMenu::GetRequiredNpcFlagForOption(GossipOptionNpc::GarrisonTalent)==UNIT_NPC_FLAG2_CLASS_HALL_UPGRADE);
 Player p;p.Select(GossipOptionNpc::AdventureMap,339088);assert(p.packets==1&&p.lastGuid==339088&&!p.credits);
 std::vector<AdventureMapPOIEntry> pois{{3,1,39718,1},{4,1,39731,1},{6,1,39735,1},{7,1,39864,1},{8,1,39733,1},{9,2,123,1}};
 for(auto& poi:pois){sAdventureMapPOIStore.push_back(&poi);quests.rows[poi.QuestID]={poi.QuestID};}
 WorldSession session{&p};assert(session.AdventureMapPOIAvailable(3)&&!session.AdventureMapPOIAvailable(999)&&!session.AdventureMapPOIAvailable(9));
 for(int mode=0;mode<5;++mode){Player q;WorldSession s{&q};if(mode==0)q.take=false;if(mode==1)q.add=false;if(mode==2){q.m_adventure_questID=39731;q.status[39731]=3;}if(mode==3)q.status[39718]=6;
 WorldPackets::Quest::AdventureJournalStartQuest request{uint32(mode==4?123:39718)};s.HandleAdventureJournalStartQuest(request);assert(!q.credits&&!q.starts);}
 p.condition=false;assert(!session.AdventureMapPOIAvailable(3));p.condition=true;
 WorldPackets::Quest::AdventureJournalStartQuest choose{39718};session.HandleAdventureJournalStartQuest(choose);assert(p.starts==1&&p.credits==1&&!session.AdventureMapPOIAvailable(4));
 for(int i=0;i<20;++i)session.HandleAdventureJournalStartQuest(choose);assert(p.starts==1&&p.credits==1);
 p.status[39718]=0;assert(session.AdventureMapPOIAvailable(4));choose.QuestID=39731;session.HandleAdventureJournalStartQuest(choose);assert(p.starts==2&&p.credits==2);
 std::cout<<"PASS: "<<count<<" real board templates through native flag filter, ShowAdventureMap packet and native selection handlers\n";
}
