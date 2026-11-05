// Whole original AI/native owner/Kill/progression/doors/Conversation payload.
// Earlier quest/stage/positions and playback clock endpoints are seeded.
#define ALTHEA_INTRO_NATIVE_SUPPORT_ONLY
#include "AltheaIntroNative.cpp"
#include "GameObjectModel.h"
#include "DevourerInstance.inc"
#include "Zirus.inc"
struct ZirusDoor:GameObject{~ZirusDoor(){if(IsInWorld())GetMap()->RemoveFromMap(static_cast<GameObject*>(this),false);ResetMap();}};
struct ZirusPlayingTag{using Type=std::map<ObjectGuid,uint32> Conversation::*;friend Type Access(ZirusPlayingTag);};
template struct FixtureAccess<ZirusPlayingTag,&Conversation::playing>;
struct ZirusCreatedTag{using Type=uint32 Conversation::*;friend Type Access(ZirusCreatedTag);};
template struct FixtureAccess<ZirusCreatedTag,&Conversation::_createdAt>;
static Conversation* DeathStory(Map& map){Conversation* found=nullptr;for(auto story:*Access(StoriesTag{}))if(story&&story->GetMap()==&map&&story->GetEntry()==1890){Require(!found,"Duplicate1890");found=story;}return found;}
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1620,{}},{1607,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1620,{}},{1607,{}}});sObjectMgr->LoadTrinityStrings();sObjectMgr->LoadPageTexts();sObjectMgr->LoadGameObjectTemplate();LoadGameObjectModelList(sWorld->GetDataPath());HashMapHolder<GameObject>::SetSize(20000);
 sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new instance_devourer;sScriptMgr->SwapScriptContext(true);
 auto& lootName=LootTemplates_Creature.*Access(LootNameTag{});auto oldName=lootName;lootName+=" WHERE Entry IN(108218,105542) ORDER BY Entry,Item";auto loaded=(LootTemplates_Creature.*Access(LootLoadTag{}))();lootName=oldName;Require(loaded&&LootTemplates_Creature.HaveLootFor(105542),"Original Zirus loot unavailable");
 auto actors=const_cast<std::vector<ConversationActor>*>(sConversationDataStore->GetConversationActor(1890));auto lines=const_cast<std::vector<ConversationData>*>(sConversationDataStore->GetConversationData(uint32(1890)));Require(actors&&actors->size()==2&&lines&&lines->size()==2,"Actual1890 definitions differ");
#ifdef ZIRUS_DIALOGUE_ORIGINAL
 bool original=true;
#else
 bool original=false;
#endif
#if defined(ZIRUS_DIALOGUE_ORIGINAL)||defined(ZIRUS_DIALOGUE_SHADOW)
 for(auto& a:*actors)if(a.id==1){Require(a.duration==2600||a.duration==12557,"Custom1890 duration; refuse native shadow");a.duration=original?2600:12557;}
 for(auto& l:*lines)if(l.id==4018){Require(l.textId==8600||l.textId==11974,"Custom1890 start; refuse native shadow");l.textId=original?8600:11974;}
#endif
 auto savedLines=*lines;uint32 cases=0;
 for(uint32 race:{uint32(RACE_HUMAN),uint32(RACE_ORC)})for(uint32 kind=0;kind<5;++kind){
  MapInstanced parent(1607,300);InstanceMap map(1607,300,9930000+cases,DIFFICULTY_N_SCENARIO,&parent);map.CreateInstanceData(nullptr);auto scenario=sScenarioMgr->AddScenario(&map,1078);scenario->SetCurrentStep(3);auto first=Session(),second=Session(),third=Session();
  {Position pose{1854.8f,1327.93f,66.22f,0};NativePlayer player(first,map,101,pose),member(second,map,102,pose),outsider(third,map,103,pose);player.SetByteValue(UNIT_FIELD_BYTES_0,0,race);player.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_ROGUE);player.setFactionForRace(race);player.SetQuestStatus(41924,QUEST_STATUS_INCOMPLETE);player.SetQuestSlot(0,41924);std::unique_ptr<CreditParty> group;if(kind==2)group=std::make_unique<CreditParty>(&player,&member);
   Actor boss(&map,105542,36001,pose);Require(map.AddToMap(static_cast<Creature*>(&boss)),"Native Zirus admission failed");auto ai=new npc_zirus_probe::npc_devourer_zirusAI(&boss);Require(boss.AIM_Initialize(ai),"Whole original Zirus AI unavailable");
   std::vector<std::unique_ptr<ZirusDoor>> doors;auto rows=WorldDatabase.Query("SELECT id,position_x,position_y,position_z,orientation FROM gameobject WHERE map=1607 AND id IN(248983,248984,248910,249028) ORDER BY id,guid");Require(bool(rows),"Original door data unavailable");do{auto f=rows->Fetch();Position p{f[1].GetFloat(),f[2].GetFloat(),f[3].GetFloat(),f[4].GetFloat()};auto door=std::make_unique<ZirusDoor>();Require(door->Create(6000+doors.size(),f[0].GetUInt32(),&map,1,p,G3D::Quat(0,0,std::sin(p.GetOrientation()/2),std::cos(p.GetOrientation()/2)),255,GO_STATE_READY)&&map.AddToMap(static_cast<GameObject*>(door.get())),"Native original door create/admission failed");doors.push_back(std::move(door));}while(rows->NextRow());Require(doors.size()==7,"Original seven-door set differs");
   std::unique_ptr<CreditPet> pet;std::unique_ptr<CreditGuardian> guardian;if(kind==1)pet=std::make_unique<CreditPet>(player,map);if(kind==2)guardian=std::make_unique<CreditGuardian>(player,map);Unit* killer=kind==3?nullptr:kind==4?static_cast<Unit*>(&boss):pet?static_cast<Unit*>(pet.get()):guardian?static_cast<Unit*>(guardian.get()):static_cast<Unit*>(&player);
#ifdef ZIRUS_DIALOGUE_MISSING
   lines->clear();
#endif
   boss.SetLootRecipient(&player);boss.LowerPlayerDamageReq(uint32(boss.GetMaxHealth()));
   if(kind==3){boss.Unit::setDeathState(CORPSE);ai->JustDied(nullptr);} // Explicit null callback endpoint, not a native Kill claim.
   else if(!kind)killer->DealDamage(&boss,UINT32_MAX-4096,nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false);
   else killer->Kill(&boss,false);
   auto story=DeathStory(map);bool credited=kind<3;Require(!boss.IsAlive()&&scenario->GetCurrentStep()==(credited?4:3),"Native50298 progression differs");for(auto const& door:doors)Require(door->GetGoState()==GO_STATE_ACTIVE&&door->GetPhaseMask()==4,"Original seven-door cleanup changed");
#ifdef ZIRUS_DIALOGUE_MISSING
   Require(!story,"Missing native data still created1890");*lines=savedLines;
#else
   std::cout<<"PAYLOAD race="<<race<<" kind="<<kind<<" present="<<bool(story);if(story)std::cout<<" world="<<story->IsInWorld()<<" duration="<<story->GetDuration()<<" actors="<<story->GetActors().size()<<" creatures="<<story->GetCreatures().size()<<" lines="<<story->GetLines().size();std::cout<<std::endl;
   Require(story&&story->IsInWorld()&&story->GetDuration()==(original?10200:20157)&&story->GetActors().size()==2&&story->GetLines().size()==2,"Native1890 virtual payload/lifetime differs");for(auto const& a:*actors){auto field=story->GetActors()[a.id];Require(field->actorId==a.actorId&&field->creatureId==a.creatureId&&field->displayId==a.displayId&&field->unk1==a.unk1&&field->unk2==a.unk2&&field->unk3==a.unk3,"Original virtual actor fields changed");}for(auto const& l:*lines){auto field=story->GetLines()[l.idx];Require(field->id==l.id&&field->textId==(l.idx?(original?8600:11974):0)&&field->unk1==l.unk1&&field->unk2==l.unk2,"Native line/start/flags changed");}
   Require(story->GetCasterGUID()==(credited?player.GetGUID():boss.GetGUID())&&story->IsInPersonnalVisibilityList(player.GetGUID())==credited&&story->IsInPersonnalVisibilityList(member.GetGUID())==bool(group)&&!story->IsInPersonnalVisibilityList(outsider.GetGUID()),"Original native owner/group or NPC fallback changed");
   auto id=story->GetGUID();auto& playing=story->*Access(ZirusPlayingTag{});playing.clear();playing[player.GetGUID()]=getMSTime()-(original?10201:15157);if(!credited)story->*Access(ZirusCreatedTag{})=getMSTime()-(original?10201:15157);story->Update(0);Require(story->IsInWorld()!=original,"Actual native playback clock failed lifetime repair");if(!original){playing[player.GetGUID()]=getMSTime()-20158;if(!credited)story->*Access(ZirusCreatedTag{})=getMSTime()-20158;story->Update(0);Require(!story->IsInWorld(),"Native repaired story did not expire");}map.RemoveAllObjectsInRemoveList();Require(!ObjectAccessor::GetConversation(boss,id),"Native story retained after expiry");
#endif
   boss.Kill(&boss,false);Require(!DeathStory(map),"Repeated native death replayed1890");auto progress=scenario->GetAchievementMgr().GetCriteriaProgressMap();auto earned=progress->find(47365);Require((earned==progress->end()?0:earned->second.Counter)==(credited?1u:0u),"Native death credit missing or repeated");Require(player.GetQuestStatus(41924)==QUEST_STATUS_INCOMPLETE&&!player.HasItemCount(128476,1),"Narration granted quest/artifact");CleanStories(map);
  }for(auto s:{first,second,third}){s->SetPlayer(nullptr);delete s;}sScenarioMgr->RemoveScenario(map.GetInstanceId());++cases;
 }
#ifdef ZIRUS_DIALOGUE_MISSING
 std::cout<<"PASS "<<cases<<" native optional1890 factory-refusal contexts: direct/pet/guardian credit50298 and all seven doors survive failed presentation; null callback/self-kill remain unowned, no artifact grant\n";
#else
 std::cout<<(original?"COUNTERFACTUAL ":"PASS ")<<cases<<" whole original Zirus AI/native kills/owner50298/seven-door cleanup and actual1890 virtual actors/line clocks; original early expiry paired repaired20157/native pauses. Earlier state/poses/clock endpoints controlled, full combat/client separate\n";
#endif
 CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
