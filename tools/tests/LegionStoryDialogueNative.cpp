// Actual event/condition/conversation publication for single-action story roots.
// Quest states are controlled inputs;no quest acceptance,items,rewards or saves.
#define AREA_DIALOGUE_SUPPORT_ONLY
#include "LegionAreaDialogueNative.cpp"
#include <memory>
static char const* StoryEntries="127,237,238,305,321,366,515,530,532";
struct StoryIntro{uint32 Entry,Quest,Other,Conversation;};
static StoryIntro const Intros[]={{127,42166,0,2203},{237,40747,0,1048},{238,40956,0,1200},{305,44084,0,2768},{321,45063,0,4146},{366,44789,0,3941},{515,46000,0,4561},{530,45629,45628,4270},{532,46278,0,4551}};
// Creature-backed conversations retain the native nearby-actor receipt. Publish
// the matching real template under controlled NullAI;NPC story behavior is not
// part of this dialogue-gate fixture.
struct StoryActor:Creature{
 StoryActor(Map& map,Player& player,uint32 entry,uint32 low){auto pos=player.GetPosition();Require(Create(low,&map,1,entry,0,player.GetTeam(),pos.GetPositionX()+1,pos.GetPositionY(),pos.GetPositionZ(),0),"Actual story conversation actor creation failed");SetHomePosition(*this);Require(map.AddToMap(static_cast<Creature*>(this)),"Native story actor publication failed");}
 void AddToWorld()override{sObjectAccessor->AddObject(static_cast<Creature*>(this));Unit::AddToWorld();Require(AIM_Initialize(new NullCreatureAI(this)),"Controlled story actor AI failed");}
 ~StoryActor(){CombatStop(true);CastStop();RemoveAllAuras();m_Events.KillAllEvents(true);if(IsInWorld())GetMap()->RemoveFromMap(static_cast<Creature*>(this),false);ResetMap();}
};
int main(int argc,char**argv){try{
 InitializeDungeon(argc,argv,true);DisableMgr::LoadDisables();sObjectMgr->LoadItemTemplates();sObjectMgr->LoadItemTemplateAddon();sQuestDataStore->LoadQuests();
 std::unordered_map<uint32,std::vector<uint32>> outdoor{{1220,{}}};dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe(outdoor);MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe(outdoor);
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
 ShadowDialogue("smart_scripts",std::string("source_type IN(10,13) AND entryorguid IN(")+StoryEntries+")");ShadowDialogue("conditions",std::string("SourceTypeOrReferenceId=22 AND SourceId IN(10,13) AND SourceEntry IN(")+StoryEntries+")");
 auto graph=WorldDatabase.Query("SELECT COUNT(*),SUM(source_type<>13 OR id<>0 OR link<>0 OR event_type<>89 OR event_chance<>100 OR event_flags<>0 OR action_type<>206 OR target_type<>7 OR action_param2<>0 OR action_param3<>0 OR action_param4<>0) FROM smart_scripts");Require(graph&&graph->Fetch()[0].GetUInt32()==9&&!graph->Fetch()[1].GetUInt32(),"Only the original9 single-conversation EventObject roots are supported");
#ifdef STORY_AREA_DIALOGUE_BASELINE
 WorldDatabase.DirectExecute("UPDATE conditions SET SourceId=10 WHERE SourceId=13 AND SourceGroup=1");
#else
 auto migration=std::getenv("LEGION_AREA_DIALOGUE_MIGRATION");Require(migration,"Explicit story dialogue migration required");
 for(unsigned replay=0;replay<2;++replay){std::ifstream input(migration);Require(bool(input),"Story migration unavailable");std::string sql,line;while(std::getline(input,line))if(line.rfind("--",0)!=0)sql+=line+'\n';WorldDatabase.DirectExecute(sql.c_str());}
#endif
 auto rows=WorldDatabase.Query("SELECT COUNT(*),MIN(SourceId),MAX(SourceId),SUM(SourceGroup<>1 OR ConditionTarget<>0 OR ConditionTypeOrReference NOT IN(9,14,28) OR NegativeCondition<>0) FROM conditions");Require(rows&&rows->Fetch()[0].GetUInt32()==10&&!rows->Fetch()[3].GetUInt32(),"Actual story condition cohort changed");
#ifdef STORY_AREA_DIALOGUE_BASELINE
 Require(rows->Fetch()[1].GetUInt32()==10&&rows->Fetch()[2].GetUInt32()==10,"Counterfactual source changed");
#else
 Require(rows->Fetch()[1].GetUInt32()==13&&rows->Fetch()[2].GetUInt32()==13,"Two migration replays did not bind all original gates");
#endif
 sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());AddSC_SmartScripts();sScriptMgr->SwapScriptContext(true);sEventObjectDataStore->LoadEventObjectTemplates();sSmartScriptMgr->LoadSmartAIFromDB();sConditionMgr->LoadConditions();sConversationDataStore->LoadConversationData();HashMapHolder<EventObject>::SetSize(1000);HashMapHolder<Conversation>::SetSize(20000);
 unsigned cases=0;std::vector<QuestStatus> states{QUEST_STATUS_NONE,QUEST_STATUS_INCOMPLETE,QUEST_STATUS_COMPLETE,QUEST_STATUS_REWARDED,QUEST_STATUS_FAILED};
 for(auto const& intro:Intros){
  auto q=sQuestDataStore->GetQuestTemplate(intro.Quest);Require(q&&q->Type==QUEST_TYPE_NORMAL&&!q->IsWorld(),"Selected story quest classification changed");
  auto root=WorldDatabase.PQuery("SELECT action_param1 FROM smart_scripts WHERE entryorguid=%u",intro.Entry);Require(root&&root->Fetch()[0].GetUInt32()==intro.Conversation,"Original native conversation changed");
  auto gate=WorldDatabase.PQuery("SELECT COUNT(*) FROM conditions WHERE SourceEntry=%u AND ConditionValue1=%u AND ConditionTypeOrReference=%u",intro.Entry,intro.Quest,intro.Entry==238?28:intro.Entry==532?14:9);Require(gate&&gate->Fetch()[0].GetUInt32()==1,"Original quest-state gate changed");
  std::cout<<"INTRO event="<<intro.Entry<<" quest="<<intro.Quest<<" alternative="<<intro.Other<<" conversation="<<intro.Conversation<<" mode="<<(intro.Entry==238?"COMPLETE":intro.Entry==532?"NONE":"INCOMPLETE")<<std::endl;
  for(uint8 race:{uint8(RACE_HUMAN),uint8(RACE_ORC)})for(auto status:states)for(auto other:states){
   if(!intro.Other&&other!=QUEST_STATUS_NONE)continue;
   Map map(1220,300,0,DIFFICULTY_NONE);DungeonPlayer player(map,race);player.SetQuestStatus(intro.Quest,status);if(intro.Other)player.SetQuestStatus(intro.Other,other);
   std::vector<std::unique_ptr<StoryActor>> actors;if(auto data=sConversationDataStore->GetConversationCreature(intro.Conversation))for(auto const& actor:*data)if(actor.creatureId)actors.emplace_back(std::make_unique<StoryActor>(map,player,actor.creatureId,180500+unsigned(actors.size())));
   AreaDialogueEvent event(map,player,intro.Entry);auto origin=player.GetPosition();
   Require(event.eventTemplate->ScriptID==sObjectMgr->GetScriptId("SmartEventObject")&&!event.eventTemplate->Flags&&!event.eventTemplate->SpellID&&!event.eventTemplate->WorldSafeLocID,"Review changed event template side effects/dispatcher");
   auto pulse=[&]{event.Update(1);for(auto live:LiveConversations())Require(live->IsInWorld()&&live->GetEntry()==intro.Conversation&&live->GetCasterGUID()==player.GetGUID()&&ObjectAccessor::GetConversation(player,live->GetGUID())==live,"Native story conversation lost actual player publication/accessor");};
   player.SetPhaseMask(2,false);pulse();Require(LiveConversations().empty()&&!event.ActivatedForPlayer(&player),"Foreign phase triggered story dialogue");player.SetPhaseMask(1,false);player.SetHealth(0);player.setDeathState(CORPSE);pulse();Require(LiveConversations().empty()&&!event.ActivatedForPlayer(&player),"Dead player triggered story dialogue");player.setDeathState(ALIVE);player.SetHealth(player.GetMaxHealth());pulse();
#ifdef STORY_AREA_DIALOGUE_BASELINE
   bool expected=true;
#else
   bool expected=intro.Entry==238?status==QUEST_STATUS_COMPLETE:intro.Entry==532?status==QUEST_STATUS_NONE:status==QUEST_STATUS_INCOMPLETE||(intro.Other&&other==QUEST_STATUS_INCOMPLETE);
#endif
   std::cout<<"OBSERVE event="<<intro.Entry<<" race="<<unsigned(race)<<" status="<<unsigned(status)<<" other="<<unsigned(other)<<" actual="<<LiveConversations().size()<<" expected="<<expected<<std::endl;
   Require(LiveConversations().size()==unsigned(expected),"Actual story-state dialogue admission differs");pulse();Require(LiveConversations().size()==unsigned(expected),"Inside-zone pulse duplicated story dialogue");ClearConversations(map);
   player.Relocate(origin.GetPositionX()+event.eventTemplate->radius+10,origin.GetPositionY(),origin.GetPositionZ(),0);pulse();Require(!event.ActivatedForPlayer(&player),"Native story exit retained entry suppression");player.Relocate(origin);pulse();Require(LiveConversations().size()==unsigned(expected),"Native story re-entry lost its gate");ClearConversations(map);
   Require(player.GetQuestStatus(intro.Quest)==status&&(!intro.Other||player.GetQuestStatus(intro.Other)==other)&&player.getRewardedQuests().empty(),"Story dialogue changed quest progression/rewards");
   if(q->SourceItemId)Require(!player.HasItemCount(q->SourceItemId,1,true),"Story dialogue granted a quest source item");for(auto const& objective:q->Objectives)Require(player.GetQuestObjectiveData(q,objective.StorageIndex)==0,"Story dialogue granted objective progress");++cases;
  }
 }
#ifndef STORY_AREA_DIALOGUE_BASELINE
 // A legacy COMPLETE log must still respect the original reward-history
 // predicate of CONDITION_QUEST_COMPLETE (EO238),without granting/rewarding.
 for(uint8 race:{uint8(RACE_HUMAN),uint8(RACE_ORC)}){
  Map map(1220,300,0,DIFFICULTY_NONE);DungeonPlayer player(map,race);player.SetQuestStatus(40956,QUEST_STATUS_COMPLETE);player.SetRewardedQuest(40956);AreaDialogueEvent event(map,player,238);event.Update(1);Require(LiveConversations().empty()&&player.GetQuestStatus(40956)==QUEST_STATUS_COMPLETE,"Complete-story dialogue ignored original rewarded-history guard");++cases;
 }
#endif
 unsigned actorCases=0;
 for(auto const& intro:Intros)if(auto data=sConversationDataStore->GetConversationCreature(intro.Conversation)){
  Require(data->size()==1&&(*data)[0].creatureId,"Review changed nearby actor cohort");
  for(uint8 race:{uint8(RACE_HUMAN),uint8(RACE_ORC)}){
   Map map(1220,300,0,DIFFICULTY_NONE);DungeonPlayer player(map,race);player.SetQuestStatus(intro.Quest,intro.Entry==532?QUEST_STATUS_NONE:QUEST_STATUS_INCOMPLETE);AreaDialogueEvent event(map,player,intro.Entry);auto origin=player.GetPosition();
   event.Update(1);Require(LiveConversations().empty()&&event.ActivatedForPlayer(&player),"Native missing-actor refusal/entry receipt changed");
   StoryActor actor(map,player,(*data)[0].creatureId,180550);event.Update(1);Require(LiveConversations().empty(),"Actor arrival incorrectly replayed the same suppressed entry");
   player.Relocate(origin.GetPositionX()+event.eventTemplate->radius+10,origin.GetPositionY(),origin.GetPositionZ(),0);event.Update(1);player.Relocate(origin);event.Update(1);Require(LiveConversations().size()==1,"Native actor/re-entry recovery failed");
   auto conversation=LiveConversations()[0];auto nativeActor=conversation->GetCreature((*data)[0].id);Require(nativeActor&&nativeActor->GuidLow==actor.GetGUID().GetLowPart()&&nativeActor->GuidHight==actor.GetGUID().GetHighPart(),"Native conversation did not bind the actual required actor");ClearConversations(map);++actorCases;
  }
 }
 Require(actorCases==6,"Review changed creature-backed story introduction count");
 std::cout<<"PASS "<<actorCases<<" native missing-actor/refusal/entry-suppression/re-entry cases with actual actor GUID binding;no synthetic production actor fallback\n";
#ifdef STORY_AREA_DIALOGUE_BASELINE
 std::cout<<"COUNTERFACTUAL "<<cases<<" native story intro cases;all9 roots ignore their original quest-state/alternative gates under wrong source10;expected broken,not ordinary success\n";
#else
 std::cout<<"PASS "<<cases<<" native story intro cases across9 roots/10 original conditions:5 states,2 races,all25 alternative pairs,phase/death/inside/exit/reentry,actual conversation publication,no quest/item/reward grants;client voice/full quests separate\n";
#endif
 CharacterDatabase.Close();LoginDatabase.Close();WorldDatabase.Close();HotfixDatabase.Close();MySQL::Library_End();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
