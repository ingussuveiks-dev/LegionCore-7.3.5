// Real native queues/GUID lookup/entry token lifetimes; payloads captured explicitly.
#define SLAGHAMMER_COMPANION_SUPPORT_ONLY
#include "SlaghammerCompanionNative.cpp"
#include <memory>
static std::vector<std::string> records;
static void Record(char const*s){records.push_back(s);}
struct NativeBase{Creature*me;InstanceScript*instance=nullptr;explicit NativeBase(Creature*u):me(u){}};
#include "FourteenthNativeScripts.inc"
struct QueueTag14{using Type=FunctionProcessor Unit::*;friend Type Access(QueueTag14);};template struct FixtureAccess<QueueTag14,&Unit::m_Functions>;
struct CombatQueueTag14{using Type=FunctionProcessor Unit::*;friend Type Access(CombatQueueTag14);};template struct FixtureAccess<CombatQueueTag14,&Unit::m_CombatFunctions>;
static void Pump14(Unit&u,uint32 n){(u.*Access(QueueTag14{})).Update(n);}
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1600,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1600,{}}});
 for(uint32 id:{249227u,239052u,218073u,116894u,240155u,243303u,240188u,243958u})Require(sSpellMgr->GetSpellInfo(id),"Matching native spell missing");
 for(uint32 id:{46110u,46199u,46182u,47430u})Require(sQuestDataStore->GetQuestTemplate(id),"Matching native quest missing");
 auto quest47430=const_cast<Quest*>(sQuestDataStore->GetQuestTemplate(47430));
 auto objectiveRows=WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged FROM quest_objectives WHERE QuestID=47430 ORDER BY StorageIndex");
 Require(bool(objectiveRows),"Matching Dramock quest objectives missing");
 do{quest47430->LoadQuestObjective(objectiveRows->Fetch());}while(objectiveRows->NextRow());
 bool dramockObjective=false;for(auto const&o:quest47430->GetObjectives())if(o.ObjectID==122390){Require(o.Amount==1,"Dramock objective amount changed");dramockObjective=true;}
 Require(dramockObjective,"Dramock native credit objective absent");
 MapInstanced parent(1600,300);InstanceMap map(1600,300,9999773,DIFFICULTY_N_SCENARIO,&parent);
 Actor boss(&map,122390,10411,{52,803,805,0}),cocoon(&map,120015,10412,{53,803,805,0}),spider(&map,120016,10413,{54,803,805,0});
 for(Creature*u:{static_cast<Creature*>(&boss),static_cast<Creature*>(&cocoon),static_cast<Creature*>(&spider)})Require(map.AddToMap(u),"Native actor insertion");
 auto session=Session();{NativePlayer p(session,map,996,{50,803,805,0});p.setFactionForRace(RACE_HUMAN);
 NMolten molten(&boss);molten.Start(&p,&cocoon,&spider,&cocoon,&p);(boss.*Access(CombatQueueTag14{})).Update(1800);Require(records.size()==1&&records.back()=="Molten","Native Molten actual combat queue and GUID lookup");
 NCocoon release(&boss);release.Start(&p,&cocoon,&spider,&cocoon,&p);Pump14(spider,10000);Require(records.size()==4&&records.back()=="CocoonJump","Native cocoon/caster resolves and complete release endpoints");
 NBrawler route(&boss);route.Start(&p,&cocoon,&spider,&cocoon,&p);Pump14(cocoon,47000);Require(records.back()=="Alliance","Native faction-value capture");
 std::cout<<"CASE native Molten combat queue/summon lookup,Cathedral two actor lookups and Brawlers immutable faction\n";
 auto exercise=[&](auto&f){auto before=records.size();f.OnPlayerEnter(&p);f.OnPlayerLeave(&p);f.OnPlayerEnter(&p);Pump14(p,5000);Require(records.size()==before+1,"Old entry invalidated,new entry admitted");f.OnPlayerEnter(&p);f.OnPlayerLeave(&p);Pump14(p,5000);Require(records.size()==before+1,"Leave invalidates callback");};
 NStormEntry storm(&boss);NAzsunaEntry azsuna(&boss);NHMEntry hm(&boss);NAssaultEntry assault(&boss);NNightholdEntry night(&boss);exercise(storm);exercise(azsuna);exercise(hm);exercise(assault);exercise(night);
 {NStormEntry transient(&boss);transient.OnPlayerEnter(&p);}auto before=records.size();Pump14(p,5000);Require(records.size()==before,"Destroyed controller expires weak token");
 std::cout<<"CASE native five entry controllers:real Player queue/map IDs,leave/reentry and instance token destruction;quest/scene/floor payloads source-only\n";
 p.SetQuestStatus(47430,QUEST_STATUS_INCOMPLETE);NDramock dramock(&boss);dramock.playerGUIDs.push_back(p.GetGUID());dramock.Finish();Require(records.size()==before+1&&records.back()=="Dramock"&&dramock.playerGUIDs.empty(),"Native eligible current quest/GUID exactly-once snapshot");dramock.Finish();Require(records.size()==before+1,"Repeated finish consumed list");p.SetQuestStatus(47430,QUEST_STATUS_NONE);dramock.playerGUIDs.push_back(p.GetGUID());dramock.Finish();Require(records.size()==before+1,"Abandoned native quest no credit endpoint");
 std::cout<<"CASE native Dramock real current quest/objective eligibility and GUID snapshot consumption;credit endpoint captured\n";
 NStormEntry departure(&boss);departure.OnPlayerEnter(&p);molten.Start(&p,&cocoon,&spider,&cocoon,&p);release.Start(&p,&cocoon,&spider,&cocoon,&p);map.RemoveFromMap(static_cast<Creature*>(&cocoon),false);p.Unit::RemoveFromWorld();sObjectAccessor->RemoveObject(static_cast<Player*>(&p));before=records.size();Pump14(p,5000);Pump14(spider,10000);(boss.*Access(CombatQueueTag14{})).Update(1800);Require(records.size()==before,"Departed player/missing cocoon/missing Molten target skipped");
 std::cout<<"CASE native actual map removal rejects pending lookups and live-world guard\n";
 }session->SetPlayer(nullptr);delete session;CloseAltheaNative();
 std::cout<<"PASS native nine selected lifecycle cases;real queues/ObjectAccessor/quest/map/token lifetime,payload endpoints captured. Telarn creature ownership and full entry/quest/scene/credit payloads remain source-only;no character saves or full-core ASAN/client claim\n";return 0;
 }catch(std::exception const&e){std::cerr<<e.what()<<std::endl;return 1;}}
