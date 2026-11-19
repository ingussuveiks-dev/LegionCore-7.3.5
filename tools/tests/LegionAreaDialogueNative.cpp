// Actual outdoor EventObject -> SmartEvent/Condition -> Conversation dispatch.
// Scoped temporary World tables; synthetic accepted quest statuses; no saves.
#define HARBARON_NATIVE_SUPPORT_ONLY
#include "HarbaronFragmentNative.cpp"
#include "Conversation.h"
#include "ConversationData.h"
#include "EventObject.h"
#include "EventObjectData.h"
#include "SmartScriptMgr.h"
#include "DisableMgr.h"
#include <fstream>

static char const* Entries="200,203,204,205,208,209,210,211,212,213,214,215,217,218,219,220,221,222,223,224,225,226,227,229,230,231,232,233,307,308,331,332";
void AddSC_SmartScripts();
static void ShadowDialogue(char const* table,std::string const& filter){
 std::string name=table;
 WorldDatabase.DirectExecute(("CREATE TEMPORARY TABLE area_seed_"+name+" AS SELECT * FROM "+name+" WHERE "+filter).c_str());
 WorldDatabase.DirectExecute(("CREATE TEMPORARY TABLE "+name+" LIKE area_seed_"+name).c_str());
 WorldDatabase.DirectExecute(("INSERT INTO "+name+" SELECT * FROM area_seed_"+name).c_str());
}
struct AreaDialogueEvent:EventObject{
 AreaDialogueEvent(Map& map,Player& player,uint32 entry){Require(Create(990000+entry,&map,1,entry,player.GetPositionX(),player.GetPositionY(),player.GetPositionZ(),0,1,0,0),"Native area event create failed");map.AddToGrid(static_cast<EventObject*>(this),Cell(Trinity::ComputeCellCoord(GetPositionX(),GetPositionY())));AddToWorld();}
 ~AreaDialogueEvent(){RemoveFromWorld();RemoveFromGrid();ResetMap();}
};
// This core's hash-map container stores Players only. Other native accessors
// use the GUID vector; inspect that actual vector through its public lookup.
static std::vector<Conversation*> LiveConversations(){std::vector<Conversation*> result;for(uint32 low=0;low<20000;++low)if(auto conv=HashMapHolder<Conversation>::FindLow(low))result.push_back(conv);Require(result.size()==objectCountInWorld[uint8(HighGuid::Conversation)],"Review native conversation GUID scope/allocation");return result;}
static void ClearConversations(Map& map){for(auto conv:LiveConversations())conv->Remove();map.RemoveAllObjectsInRemoveList();Require(LiveConversations().empty(),"Native conversation cleanup failed");}
#ifndef AREA_DIALOGUE_SUPPORT_ONLY
int main(int argc,char**argv){try{
 InitializeDungeon(argc,argv,true);DisableMgr::LoadDisables();sObjectMgr->LoadItemTemplates();sObjectMgr->LoadItemTemplateAddon();sQuestDataStore->LoadQuests();
 std::unordered_map<uint32,std::vector<uint32>> outdoor{{1220,{}}};dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe(outdoor);MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe(outdoor);
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
 ShadowDialogue("smart_scripts",std::string("source_type IN(10,13) AND entryorguid IN(")+Entries+")");
 ShadowDialogue("conditions",std::string("SourceTypeOrReferenceId=22 AND SourceId IN(10,13) AND SourceEntry IN(")+Entries+")");
 auto scene=WorldDatabase.Query("SELECT COUNT(*) FROM smart_scripts WHERE source_type=10");Require(scene&&!scene->Fetch()[0].GetUInt32(),"Scoped IDs contain a real scene script;review collision before migration");
#ifdef AREA_DIALOGUE_BASELINE
 // Exact original cohort captured before promotion; keep baseline reproducible
 // after deployment by restoring only these original temporary-table fields.
 WorldDatabase.DirectExecute("UPDATE conditions SET SourceId=10 WHERE SourceId=13 AND SourceGroup=1");
#else
 auto migration=std::getenv("LEGION_AREA_DIALOGUE_MIGRATION");Require(migration,"Explicit area-dialogue migration required");
 for(unsigned replay=0;replay<2;++replay){
  std::ifstream input(migration);Require(bool(input),"Migration file unavailable");std::string text,line;while(std::getline(input,line))if(line.rfind("--",0)!=0)text+=line+'\n';
  std::string sql;for(char c:text){if(c==';'){WorldDatabase.DirectExecute(sql.c_str());sql.clear();}else sql+=c;}
 }
#endif
 auto rows=WorldDatabase.Query("SELECT SourceEntry,ConditionValue1,SourceId FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=1 AND ConditionTypeOrReference=9 ORDER BY SourceEntry");
 Require(rows&&rows->GetRowCount()==32,"Actual world-quest intro cohort changed");
 std::map<uint32,uint32> quests;do{auto f=rows->Fetch();quests[f[0].GetUInt32()]=f[1].GetUInt32();
#ifdef AREA_DIALOGUE_BASELINE
  Require(f[2].GetUInt32()==10,"Original wrong-source fixture differs");
#else
  Require(f[2].GetUInt32()==13,"Replayed migration missed native EventObject source");
#endif
 }while(rows->NextRow());
 sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());AddSC_SmartScripts();sScriptMgr->SwapScriptContext(true);
 sEventObjectDataStore->LoadEventObjectTemplates();sSmartScriptMgr->LoadSmartAIFromDB();sConditionMgr->LoadConditions();sConversationDataStore->LoadConversationData();
 HashMapHolder<EventObject>::SetSize(1000);HashMapHolder<Conversation>::SetSize(20000);unsigned cases=0;
 for(auto const& row:quests){
  uint32 entry=row.first,questId=row.second;auto quest=sQuestDataStore->GetQuestTemplate(questId);Require(quest&&quest->IsWorld(),"Actual gated quest is not a world task");
  auto root=WorldDatabase.PQuery("SELECT action_param1 FROM smart_scripts WHERE source_type=13 AND entryorguid=%u AND id=0 AND event_type=89 AND action_type=206",entry);Require(bool(root),"Actual native introduction root changed");uint32 conversation=root->Fetch()[0].GetUInt32();
  auto cli=sQuestV2CliTaskStore.LookupEntry(questId);std::cout<<"INTRO event="<<entry<<" quest="<<questId<<" conversation="<<conversation<<" title="<<(cli?cli->QuestTitle->Str[0]:"")<<'\n';
  for(auto status:{QUEST_STATUS_NONE,QUEST_STATUS_INCOMPLETE,QUEST_STATUS_COMPLETE,QUEST_STATUS_REWARDED}){
   Map map(1220,300,0,DIFFICULTY_NONE);DungeonPlayer player(map,RACE_HUMAN);player.SetQuestStatus(questId,status);AreaDialogueEvent event(map,player,entry);auto origin=player.GetPosition();
   auto pulse=[&]{event.Update(1);for(auto live:LiveConversations())Require(live->IsInWorld()&&live->GetEntry()==conversation&&live->GetCasterGUID()==player.GetGUID()&&ObjectAccessor::GetConversation(player,live->GetGUID())==live,"Native conversation lost its actual entering player/world admission");};
   pulse();
#ifdef AREA_DIALOGUE_BASELINE
   bool expected=true;
#else
   bool expected=status==QUEST_STATUS_INCOMPLETE;
#endif
   std::cout<<"OBSERVE event="<<entry<<" quest="<<questId<<" status="<<unsigned(status)<<" activated="<<event.ActivatedForPlayer(&player)<<" conversations="<<LiveConversations().size()<<" expected="<<expected<<std::endl;
   Require(LiveConversations().size()==unsigned(expected),"Actual native quest/dialogue admission differs");
   pulse();Require(LiveConversations().size()==unsigned(expected),"Repeated inside-zone pulse duplicated introduction");
   ClearConversations(map);player.Relocate(origin.GetPositionX()+event.eventTemplate->radius+10,origin.GetPositionY(),origin.GetPositionZ(),0);pulse();Require(!event.ActivatedForPlayer(&player),"Actual area exit retained entry suppression");
   player.Relocate(origin);pulse();Require(LiveConversations().size()==unsigned(expected),"Actual area re-entry lost dialogue admission");ClearConversations(map);
   Require(player.GetQuestStatus(questId)==status&&player.getRewardedQuests().empty(),"Introduction granted or changed quest progression");
   player.RemoveAllAuras();++cases;
  }
 }
#ifdef AREA_DIALOGUE_BASELINE
 std::cout<<"COUNTERFACTUAL "<<cases<<" actual native outdoor intro cases:32 wrong-source roots play without quest;native EO224 target0 also correctly resolves the player;expected broken,not passing gameplay\n";
#else
 std::cout<<"PASS "<<cases<<" actual native outdoor intro cases across32 roots:quest states,inside suppression,exit/reentry,real player conversation,no quest/reward grants;two temporary migration replays;client audio/visual/traversal separate\n";
#endif
 CharacterDatabase.Close();LoginDatabase.Close();WorldDatabase.Close();HotfixDatabase.Close();MySQL::Library_End();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
#endif
