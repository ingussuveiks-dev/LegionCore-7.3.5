// Alias only the extracted outer class to prevent COMDAT selection of the
// current scripts.lib class when compiling the pinned old-source control.
#define go_mystic_bonfire RetirementBonfire
#define main RetainedBonfireRetirementSupport
#include "WarswordsBonfireNative.cpp"
#undef main
#undef go_mystic_bonfire
static_assert(GO_FLAG_NOT_SELECTABLE==0x10,"Wrong client interaction flag");
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);sObjectMgr->LoadPageTexts();sObjectMgr->LoadGameObjectTemplate();
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1511,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1511,{}}});
 sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new BonfireInstanceFactory;new RetirementBonfire;sScriptMgr->SwapScriptContext(true);
 uint32 cases=0;
 for(uint32 race:{RACE_HUMAN,RACE_ORC})for(uint32 mode=0;mode<8;++mode){
  MapInstanced parent(1511,300);InstanceMap map(1511,300,9987900+race+100*mode,DIFFICULTY_N_SCENARIO,&parent);map.CreateInstanceData(nullptr);auto scenario=sScenarioMgr->AddScenario(&map,944);scenario->SetCurrentStep(0);auto session=Session();
  {NativePlayer player(session,map,901,{3402.01f,1475.4f,58.5142f,0});player.SetByteValue(UNIT_FIELD_BYTES_0,0,race);player.SetQuestStatus(40043,QUEST_STATUS_INCOMPLETE);player.SetQuestSlot(0,40043);
   if(mode==1)scenario->SetCurrentStep(1);if(mode==2)scenario->SetCurrentStep(6);
   auto fire=new GameObject;Require(fire->Create(sObjectMgr->GetGenerator<HighGuid::GameObject>()->Generate(),245188,&map,1,{3402.01f,1477.4f,58.5142f,0},G3D::Quat(),0,GO_STATE_READY),"Native fire creation failed");fire->*Access(BonfireDbTag{})=mode==5?134118:134117;Require(map.AddToMap(fire),"Native fire insertion failed");Require(dynamic_cast<RetirementBonfire::go_mystic_bonfireAI*>(fire->AI()),"Extracted native script binding failed");
   auto count=[&]{auto rows=scenario->GetAchievementMgr().GetCriteriaProgressMap();auto found=rows->find(45849);return found==rows->end()?uint64(0):found->second.Counter;};
   if(mode==0||mode==6){if(mode==6)fire->SetFlag(GAMEOBJECT_FIELD_FLAGS,0x80);Require(player.CastSpell(fire,198513,true)==SPELL_CAST_OK,"Actual native ignition rejected");for(uint32 i=0;i<4;++i)player.m_Events.Update(500);Require(count()==1&&scenario->GetCurrentStep()==1&&fire->GetGoState()==GO_STATE_ACTIVE,"Actual ignition progress/lighting failed");
#ifdef BONFIRE_RETIREMENT_BASELINE
    Require(!fire->HasFlag(GAMEOBJECT_FIELD_FLAGS,GO_FLAG_NOT_SELECTABLE),"Pinned old source unexpectedly retires fire");
#else
    Require(fire->HasFlag(GAMEOBJECT_FIELD_FLAGS,GO_FLAG_NOT_SELECTABLE),"Acknowledged fire still selectable");auto before=count();fire->Use(&player);fire->AI()->GossipHello(&player,true);Require(count()==before&&!player.IsNonMeleeSpellCast(false),"Consumed native Use recast or granted duplicate credit");
#endif
    if(mode==6){scenario->SetCurrentStep(0);fire->AI()->Reset();Require(fire->GetUInt32Value(GAMEOBJECT_FIELD_FLAGS)==0x80,"Reset removed unrelated flags or retained owned inactive flag");
#ifdef BONFIRE_RETIREMENT_BASELINE
     Require(fire->GetGoState()==GO_STATE_ACTIVE,"Pinned old reset unexpectedly restores READY");
#else
     Require(fire->GetGoState()==GO_STATE_READY,"Native first-step reset did not restore authored READY");
#endif
    }
   }
   if(mode==1||mode==2){fire->Update(1);fire->AI()->Reset();
#ifdef BONFIRE_RETIREMENT_BASELINE
    Require(!fire->HasFlag(GAMEOBJECT_FIELD_FLAGS,GO_FLAG_NOT_SELECTABLE),"Pinned old late-load fire unexpectedly inactive");
#else
    Require(fire->HasFlag(GAMEOBJECT_FIELD_FLAGS,GO_FLAG_NOT_SELECTABLE),"Native update/reset lost consumed late-load state");
#endif
    Require(!count(),"Synchronization fabricated criterion credit");}
   if(mode==3){Require(player.CastSpell(fire,198513,false)==SPELL_CAST_OK,"Actual interruptible ignition rejected");player.CastStop();player.m_Events.Update(5000);fire->Update(1);Require(!count()&&scenario->GetCurrentStep()==0&&!fire->HasFlag(GAMEOBJECT_FIELD_FLAGS,GO_FLAG_NOT_SELECTABLE),"Interrupted cast consumed or retired fire");}
   if(mode==4){fire->SetFlag(GAMEOBJECT_FIELD_FLAGS,GO_FLAG_NOT_SELECTABLE|0x80);fire->AI()->Reset();scenario->SetCurrentStep(1);fire->AI()->UpdateAI(1);scenario->SetCurrentStep(0);fire->AI()->Reset();Require(fire->GetUInt32Value(GAMEOBJECT_FIELD_FLAGS)==(GO_FLAG_NOT_SELECTABLE|0x80),"Reset stripped authored inactive flag");}
   if(mode==5){scenario->SetCurrentStep(1);fire->AI()->UpdateAI(1);fire->AI()->Reset();Require(!fire->HasFlag(GAMEOBJECT_FIELD_FLAGS,GO_FLAG_NOT_SELECTABLE),"Foreign spawn was retired");}
   if(mode==7){sScenarioMgr->RemoveScenario(map.GetInstanceId());fire->AI()->UpdateAI(1);fire->AI()->Reset();Require(!fire->HasFlag(GAMEOBJECT_FIELD_FLAGS,GO_FLAG_NOT_SELECTABLE),"Missing scenario changed fire");}
   fire->RemoveFromWorld();fire->RemoveFromGrid();fire->ResetMap();delete fire;map.RemoveAllObjectsInRemoveList();CleanStories(map);
  }session->SetPlayer(nullptr);delete session;sScenarioMgr->RemoveScenario(map.GetInstanceId());std::cout<<"CASE retirement race="<<race<<" mode="<<mode<<" PASS\n";++cases;
 }
 Require(cases==16,"Native retirement cases incomplete");CloseAltheaNative();
#ifdef BONFIRE_RETIREMENT_BASELINE
 std::cout<<"COUNTERFACTUAL16 pinned old bonfire:real acknowledged and late-loaded fire remains selectable; native interrupted/authored/foreign/missing controls preserved,not ordinary successful repaired gameplay\n";
#else
 std::cout<<"PASS16 actual native bonfire retirement:Player/Spell/GO/Scenario/flag/update/reset,post-ack and late-load inactive,interrupt/authored/foreign/missing guards,owned reset/repeat without new credit;Human+Orc,no Character writes;hand cursor requires client retest\n";
#endif
 return 0;}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
