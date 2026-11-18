// Native completion endpoint and async prepared writes, isolated in empty
// connection-local tables. This deliberately does not manufacture boss kills.
#define CHALLENGE_LIFECYCLE_SUPPORT_ONLY
#include "LegionChallengeLifecycleNative.cpp"
#include "GroupMgr.h"
#ifdef CHALLENGE_COMPLETION_BASELINE
#include "ScenarioActual.inc"
#include "ChallengeMgrActual.inc"
#include "CharacterDatabaseActual.inc"
#endif
#include <thread>
struct CompletionGroupGuidTag{using Type=ObjectGuid Group::*;friend Type Access(CompletionGroupGuidTag);};
template struct FixtureAccess<CompletionGroupGuidTag,&Group::m_guid>;
struct RegisteredCompletionGroup{
 Group* group;
 RegisteredCompletionGroup(Group& value,bool active,uint32 id):group(active?&value:nullptr){if(group){group->*Access(CompletionGroupGuidTag{})=ObjectGuid::Create<HighGuid::Party>(id,0);sGroupMgr->AddGroup(group);}}
 ~RegisteredCompletionGroup(){if(group)sGroupMgr->RemoveGroup(group);}
};
static void ShadowChallengeWrites(){
 CharacterDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");CharacterDatabase.Execute("SET SESSION TRANSACTION READ WRITE");
 for(auto table:{"challenge","challenge_member","challenge_key","groups","challenge_oplote_loot","worldstates"}){
  std::string seed=std::string("CREATE TEMPORARY TABLE completion_seed_")+table+" LIKE `"+table+"`";
  std::string target=std::string("CREATE TEMPORARY TABLE `")+table+"` LIKE completion_seed_"+table;
  CharacterDatabase.DirectExecute(seed.c_str());CharacterDatabase.Execute(seed.c_str());CharacterDatabase.DirectExecute(target.c_str());CharacterDatabase.Execute(target.c_str());
 }
 CharacterDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");CharacterDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
}
struct SavedCompletion{uint32 count=0,map=0,route=0,time=0,level=0,tier=0,members=0;};
static SavedCompletion Saved(){
 SavedCompletion saved;bool ready=false;
 auto callback=CharacterDatabase.AsyncQuery("SELECT (SELECT COUNT(*) FROM challenge),MapID,ChallengeID,RecordTime,ChallengeLevel,TimerLevel,(SELECT COUNT(*) FROM challenge_member) FROM challenge ORDER BY ID DESC LIMIT 1").WithCallback([&](QueryResult result){if(result){auto f=result->Fetch();saved={f[0].GetUInt32(),f[1].GetUInt32(),f[2].GetUInt32(),f[3].GetUInt32(),f[4].GetUInt32(),f[5].GetUInt32(),f[6].GetUInt32()};}ready=true;});
 for(unsigned i=0;!ready&&i<10000;++i){callback.InvokeIfReady();if(!ready)std::this_thread::sleep_for(std::chrono::milliseconds(1));}Require(ready,"Native completion async shadow read did not drain");return saved;
}
static QueryResult ReadAsync(char const* sql){QueryResult result;bool ready=false;auto callback=CharacterDatabase.AsyncQuery(sql).WithCallback([&](QueryResult rows){result=rows;ready=true;});for(unsigned i=0;!ready&&i<10000;++i){callback.InvokeIfReady();if(!ready)std::this_thread::sleep_for(std::chrono::milliseconds(1));}Require(ready,"Native public record replay read did not drain");return result;}
#define COMPLETION_CACHE(tag,type,field) struct tag{using Type=type ChallengeMgr::*;friend Type Access(tag);};template struct FixtureAccess<tag,&ChallengeMgr::field>;
COMPLETION_CACHE(CompletionRecordsTag,ChallengeMap,_challengeMap)
COMPLETION_CACHE(CompletionLastTag,ChallengesOfMember,_lastForMember)
COMPLETION_CACHE(CompletionMembersTag,ChallengesOfMember,_challengesOfMember)
COMPLETION_CACHE(CompletionBestTag,ChallengeByMap,_bestForMap)
COMPLETION_CACHE(CompletionGuildTag,GuildBestRecord,m_GuildBest)
COMPLETION_CACHE(CompletionWeekTag,ChallengeWeekListMap,_challengeWeekList)
#undef COMPLETION_CACHE
struct ExpectedHistory{ObjectGuid player;uint32 route,time;};
static void ReplayPublicRecords(std::vector<ExpectedHistory>const& expected,unsigned count,unsigned memberCount){
 // The native async and sync pools have different TEMP tables. Copy every
 // saved field between those empty tables, then rebuild the actual loader's
 // caches. No persistent DB or server restart is represented by this replay.
 auto rows=ReadAsync("SELECT ID,GuildID,MapID,RecordTime,Date,ChallengeLevel,TimerLevel,Affixes,ChestID,ChallengeID FROM challenge ORDER BY ID");unsigned copied=0;
 if(rows)do{auto f=rows->Fetch();auto affixes=f[7].GetString();Require(std::all_of(affixes.begin(),affixes.end(),[](unsigned char c){return std::isdigit(c)||c==' ';}),"Unexpected public fixture affix serialization");CharacterDatabase.DirectPExecute("INSERT INTO challenge(ID,GuildID,MapID,RecordTime,Date,ChallengeLevel,TimerLevel,Affixes,ChestID,ChallengeID) VALUES(%u," UI64FMTD ",%u,%u,%u,%u,%u,'%s',%u,%u)",f[0].GetUInt32(),f[1].GetUInt64(),f[2].GetUInt32(),f[3].GetUInt32(),f[4].GetUInt32(),f[5].GetUInt32(),f[6].GetUInt32(),affixes.c_str(),f[8].GetUInt32(),f[9].GetUInt32());++copied;}while(rows->NextRow());Check(copied==count,"Native replay record count changed");
 rows=ReadAsync("SELECT id,member,specID,ChallengeLevel,Date,ChestID FROM challenge_member ORDER BY id,member");copied=0;if(rows)do{auto f=rows->Fetch();CharacterDatabase.DirectPExecute("INSERT INTO challenge_member(id,member,specID,ChallengeLevel,Date,ChestID) VALUES(%u," UI64FMTD ",%u,%u,%u,%u)",f[0].GetUInt32(),f[1].GetUInt64(),f[2].GetUInt32(),f[3].GetUInt32(),f[4].GetUInt32(),f[5].GetUInt32());++copied;}while(rows->NextRow());Check(copied==memberCount,"Native replay member count changed");
 auto mgr=sChallengeMgr;auto owned=std::move(mgr->*Access(CompletionRecordsTag{}));(mgr->*Access(CompletionLastTag{})).clear();(mgr->*Access(CompletionMembersTag{})).clear();(mgr->*Access(CompletionBestTag{})).clear();(mgr->*Access(CompletionGuildTag{})).clear();(mgr->*Access(CompletionWeekTag{})).clear();for(auto const&row:owned)delete row.second;
 sWorld->setWorldState(WS_CHALLENGE_AFFIXE1_RESET_TIME,uint32(Affixes::Quaking));mgr->LoadFromDB();
 for(auto const&row:expected){auto loaded=mgr->BestForMemberMap(row.player,row.route);Check(loaded&&loaded->ChallengeID==row.route&&loaded->RecordTime==row.time&&loaded->ChallengeLevel==10,"Native DB reload lost a saved member's route/record");}
 std::set<uint32> routes;for(auto const&row:expected)routes.insert(row.route);for(auto route:routes)Check(mgr->BestServerChallenge(route)&&mgr->BestServerChallenge(route)->ChallengeID==route,"Native reload lost a split-route best record bucket");Check(routes.size()==13,"Native reload route count changed");std::cout<<"RELOAD records="<<count<<" members="<<memberCount<<" nativeRouteBuckets="<<routes.size()<<" including227/234 on1651;TEMP field replay and rebuilt native caches,not a server/client restart\n";
}
struct CompletionChest:GameObject{
 CompletionChest(InstanceMap&map,uint32 route){
  auto row=WorldDatabase.PQuery("SELECT id,position_x,position_y,position_z,orientation,rotation0,rotation1,rotation2,rotation3 FROM gameobject WHERE map=%u AND id IN(252674,252677,252686,252668,252665,252056,252680,252671,252683,269852,269871,269843,272689) AND (%u<>1651 OR id=%u) ORDER BY guid LIMIT 1",map.GetId(),map.GetId(),route==227?269852:269871);Require(bool(row),"Native challenge chest placement missing");auto f=row->Fetch();Position pose{f[1].GetFloat(),f[2].GetFloat(),f[3].GetFloat(),f[4].GetFloat()};Require(Create(161000+route,f[0].GetUInt32(),&map,1,pose,G3D::Quat(f[5].GetFloat(),f[6].GetFloat(),f[7].GetFloat(),f[8].GetFloat()),255,GO_STATE_READY),"Native reward chest create failed");map.LoadGrid(pose.GetPositionX(),pose.GetPositionY());Require(map.AddToMap(static_cast<GameObject*>(this)),"Native reward chest publish failed");
 }
 ~CompletionChest(){if(IsInWorld())GetMap()->RemoveFromMap(static_cast<GameObject*>(this),false);ResetMap();}
};
int main(int argc,char**argv){try{
 InitializeDungeon(argc,argv,true);sObjectMgr->LoadItemTemplates();sObjectMgr->LoadItemTemplateAddon();sObjectMgr->LoadScenarioData();(sQuestDataStore->*Access(QuestVectorTag{})).resize(1,nullptr);sQuestDataStore->*Access(QuestMaxTag{})=0;auto live=CharacterDatabase.Query("SELECT COUNT(*),COALESCE(SUM(ChallengeID=0),0) FROM challenge");Require(bool(live),"Existing challenge history aggregate unavailable");std::cout<<"LIVE_HISTORY rows="<<live->Fetch()[0].GetUInt32()<<" missingRoute="<<live->Fetch()[1].GetUInt32()<<";read-only aggregate,no character rows copied\n";ShadowChallengeWrites();unsigned contexts=0,records=0,members=0;std::vector<ExpectedHistory> expected;
 for(auto route:sMapChallengeModeStore)if(route->ID==197||route->ID==198||route->ID==199||route->ID==200||route->ID==206||route->ID==207||route->ID==208||route->ID==209||route->ID==210||route->ID==227||route->ID==233||route->ID==234||route->ID==239)for(bool grouped:{false,true})for(unsigned tier=0;tier<4;++tier){
  MapInstanced parent(route->MapID,300);InstanceMap map(route->MapID,300,9956000+contexts,DIFFICULTY_MYTHIC_KEYSTONE,&parent);map.CreateInstanceData(nullptr);Position pose{2934.49f,711.757f,554.286f,1.55255f};LifecyclePlayer owner(map,301+contexts*2,pose);auto key=owner.Key(*route);auto member=grouped?std::make_unique<LifecyclePlayer>(map,302+contexts*2,pose):nullptr;LifecycleGroup group(owner,member.get(),*route,*key);RegisteredCompletionGroup registered(group.group,grouped,9956000+contexts);
  Scenario scenario(&map,1175);scenario.CreateChallenge(&owner);auto challenge=scenario.GetChallenge();Require(challenge&&challenge->_canRun,"Native completion challenge unavailable");auto steps=sScenarioMgr->GetScenarioSteps(scenario.GetScenarioId());Require(steps&&!steps->empty()&&!steps->front()->RewardQuestID,"Native challenge completion has an actual quest reward;load it instead of using the empty quest fixture");CompletionChest chest(map,route->ID);Check(map.GetInstanceScript()->_challengeChest==chest.GetGUID(),"Real reward chest not attached to instance");challenge->Update(100);challenge->Update(10000);Require(challenge->_run,"Native completion did not start");
  challenge->_challengeTimer=(tier==0?route->CriteriaCount[0]:route->CriteriaCount[tier-1]-1)*1000;
  if(!tier)challenge->Update(1000);uint32 elapsed=challenge->_challengeTimer;
  // Exercise the actual callback reached after criteria completion. Upstream
  // combat/forces receipts are separate and are not seeded by this fixture.
  scenario.Reward(false,0);
#ifdef CHALLENGE_COMPLETION_BASELINE
  if(!grouped){Check(!challenge->_complete&&key->GetModifier(ITEM_MODIFIER_CHALLENGE_KEYSTONE_LEVEL)==(tier?10:9),"Expected original solo completion loss changed");Check(Saved().count==records,"Original solo completion unexpectedly persisted a record");std::cout<<"COUNTERFACTUAL solo route="<<route->ID<<" tier="<<tier<<" native reward callback consumed without completing run\n";++contexts;continue;}
#endif
  Check(challenge->_complete,"Scenario completion endpoint did not complete native run");Check(key->GetModifier(ITEM_MODIFIER_CHALLENGE_KEYSTONE_LEVEL)==(tier?std::min(10u+tier,sWorld->getIntConfig(CONFIG_CHALLENGE_LEVEL_LIMIT)):9u),"Native timed upgrade/depleted key outcome differs");
  unsigned items=challenge->GetItemCount(owner.GetGUID())+(member?challenge->GetItemCount(member->GetGUID()):0);Check(items==(tier?3u:2u),"Native level10 chest entitlements differ from timed/late policy");
  ++records;members+=grouped?2:1;auto saved=Saved();Check(saved.count==records&&saved.members==members&&saved.map==route->MapID&&saved.time==elapsed&&saved.level==10&&saved.tier==tier,"Native async completion record/member fields differ");
#ifdef CHALLENGE_COMPLETION_BASELINE
  Check(!saved.route,"Expected original omission of saved ChallengeID changed");
#else
  Check(saved.route==route->ID,"Native prepared save lost ChallengeID,including split Karazhan identity");
  expected.push_back({owner.GetGUID(),uint32(route->ID),elapsed});if(member)expected.push_back({member->GetGUID(),uint32(route->ID),elapsed});
#endif
  auto after=key->GetModifier(ITEM_MODIFIER_CHALLENGE_KEYSTONE_LEVEL);scenario.Reward(false,0);challenge->Complete();Check(Saved().count==records&&key->GetModifier(ITEM_MODIFIER_CHALLENGE_KEYSTONE_LEVEL)==after,"Repeated native completion duplicated record/key upgrade");Check(!grouped||(!group.group.m_challengeEntry&&group.group.m_challengeLevel==0),"Native completed group retained pending challenge metadata");
  std::cout<<"COMPLETE route="<<route->ID<<" group="<<grouped<<" tier="<<tier<<" key="<<after<<" items="<<items<<" savedRoute="<<saved.route<<'\n';++contexts;
 }
 Check(contexts==104,"Completion route/group/tier scope changed");
#ifndef CHALLENGE_COMPLETION_BASELINE
 ReplayPublicRecords(expected,records,members);
#endif
#ifdef CHALLENGE_COMPLETION_BASELINE
 std::cout<<"COUNTERFACTUAL "<<checked<<" native endpoint/async-shadow checks:52 solo finishes lost and52 party records omit route ID;not ordinary successful runs\n";
#else
 std::cout<<"PASS "<<checked<<" native completion endpoint checks across104 contexts/13 routes/solo+party/all4 timer tiers:actual stored key upgrade/depletion,real placed chest and2/3 entitlement allocation,async prepared record/member persistence and idempotency. No real earned combat/forces,loot opening or client traversal claim\n";
#endif
 CharacterDatabase.Close();LoginDatabase.Close();WorldDatabase.Close();HotfixDatabase.Close();MySQL::Library_End();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
