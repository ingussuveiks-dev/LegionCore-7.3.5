// Native TempSummon/SmartAI loader, target selection, SetFollow and scenario credit.
// Earlier quest/stage and spawn poses are controlled; no character saves.
#define ALTHEA_INTRO_NATIVE_SUPPORT_ONLY
#include "AltheaIntroNative.cpp"
#include "SmartAI.h"
#include "SmartScriptMgr.h"
#include "CreatureTextMgr.h"

static void Shadow(char const* table,char const* filter) {
    auto seed=std::string("slaghammer_seed_")+table;
    WorldDatabase.DirectExecute(("CREATE TEMPORARY TABLE "+seed+" LIKE "+table).c_str());
    WorldDatabase.DirectExecute(("INSERT INTO "+seed+" SELECT * FROM "+table+" WHERE "+filter).c_str());
    WorldDatabase.DirectExecute(("CREATE TEMPORARY TABLE "+std::string(table)+" LIKE "+seed).c_str());
    auto check=WorldDatabase.Query(("SHOW CREATE TABLE "+std::string(table)).c_str());
    Require(check&&check->Fetch()[1].GetString().find("CREATE TEMPORARY TABLE")!=std::string::npos,"Native shadow must be TEMP before writing");
    WorldDatabase.DirectExecute(("INSERT INTO "+std::string(table)+" SELECT * FROM "+seed).c_str());
}
static uint64 FollowCredit(Scenario* scenario) {
    uint64 count=0;
    for(auto const& row:*scenario->GetAchievementMgr().GetCriteriaProgressMap()) {
        auto tree=sCriteriaTreeStore.LookupEntry(row.first);
        auto criterion=tree?sCriteriaStore.LookupEntry(tree->CriteriaID):nullptr;
        if(criterion&&criterion->Type==92&&criterion->Asset==48306)count+=row.second.Counter;
    }
    return count;
}
int main(int argc,char**argv) {try {
    InitializeAltheaGuardsNative(argc,argv);
    sObjectMgr->LoadPageTexts();sObjectMgr->LoadGameObjectTemplate();
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    Shadow("smart_scripts","entryorguid=101899 AND source_type=0");
    Shadow("conditions","SourceTypeOrReferenceId=22 AND SourceEntry=101899");
    Shadow("creature_text","CreatureID=101899");
    auto initial=WorldDatabase.Query("SELECT target_type,target_param1 FROM smart_scripts WHERE id=1");
    Require(bool(initial),"Native companion follow row missing");
#ifdef SLAGHAMMER_COMPANION_LIVE
    Require(initial->Fetch()[0].GetUInt32()==23&&initial->Fetch()[1].GetUInt32()==0,"Live companion follow not repaired");
#elif defined(SLAGHAMMER_COMPANION_BASELINE)
    WorldDatabase.DirectExecute("UPDATE smart_scripts SET target_type=21,target_param1=100 WHERE id=1");
#else
    Require((initial->Fetch()[0].GetUInt32()==21&&initial->Fetch()[1].GetUInt32()==100)||(initial->Fetch()[0].GetUInt32()==23&&initial->Fetch()[1].GetUInt32()==0),"Reinspect changed live companion follow");
    WorldDatabase.DirectExecute("UPDATE smart_scripts SET target_type=23,target_param1=0 WHERE id=1 AND target_type=21 AND target_param1=100");
#endif
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    auto readonly=WorldDatabase.Query("SELECT @@session.transaction_read_only");Require(readonly&&readonly->Fetch()[0].GetBool(),"World is not readonly after TEMP preparation");
    sSmartScriptMgr->LoadSmartAIFromDB();sConditionMgr->LoadConditions();sCreatureTextMgr->LoadCreatureTexts();
    dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1539,{}}});
    MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1539,{}}});
    auto properties=sSummonPropertiesStore.LookupEntry(3747);auto spell=sSpellMgr->GetSpellInfo(201004);
    Require(properties&&spell&&spell->GetEffect(EFFECT_0)->MiscValue==101899&&spell->GetEffect(EFFECT_0)->MiscValueB==3747,"Native companion producer changed");
    uint32 cases=0;
    for(uint32 race:{uint32(RACE_HUMAN),uint32(RACE_BLOODELF)})for(uint32 mode=0;mode<7;++mode) {
        MapInstanced parent(1539,300);InstanceMap map(1539,300,9926000+cases,DIFFICULTY_N_SCENARIO,&parent);
        auto scenario=sScenarioMgr->AddScenario(&map,991);scenario->SetCurrentStep(3);auto first=Session(),second=Session();
        {Position pose{2020.16f,2337.81f,75.2808f,0};Position ownerPose=pose;ownerPose.m_positionX+=10;
            NativePlayer owner(first,map,101,ownerPose),visitor(second,map,102,pose);
            for(auto p:{&owner,&visitor}){p->SetByteValue(UNIT_FIELD_BYTES_0,0,race);p->SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_PRIEST);p->setFactionForRace(race);p->SetQuestStatus(40710,QUEST_STATUS_INCOMPLETE);p->SetQuestSlot(0,40710);}
            if(mode==1){visitor.Relocate(pose.GetPositionX()+30,pose.GetPositionY(),pose.GetPositionZ(),0);}
            if(mode==2)visitor.SetGameMaster(true);
            if(mode==3)visitor.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_WARRIOR);
            if(mode==4){visitor.SetQuestStatus(40710,QUEST_STATUS_NONE);visitor.SetQuestSlot(0,0);}
            std::unique_ptr<CreditParty> party;if(mode==5)party=std::make_unique<CreditParty>(&owner,&visitor);
            // Same native factory/properties/creator as201004; separated pose is
            // a controlled witness for a nearer unrelated/group Player.
            TempSummon* companion=nullptr;
            if(mode==6) {
                owner.CastSpell(&owner,201004,true);
                auto guids=owner.GetSummonList(101899);Require(guids&&guids->size()==1,"Actual201004 did not create exactly one owned companion");
                auto actor=ObjectAccessor::GetCreature(owner,guids->front());companion=actor?actor->ToTempSummon():nullptr;
            }else companion=map.SummonCreature(101899,pose,properties,30000,&owner,{},201004);
            Require(companion&&companion->GetSummoner()==&owner&&companion->GetAnyOwner()==&owner,"Native companion summoner missing");
            auto ai=dynamic_cast<SmartAI*>(companion->AI());Require(ai,"Actual native SmartAI not selected");
#ifdef SLAGHAMMER_COMPANION_BASELINE
            Require(ai->GetFollowGuid()==(mode==6?companion->SelectNearestPlayer(100)->GetGUID():mode==1?owner.GetGUID():visitor.GetGUID()),"Pinned nearest follow counterfactual differs");
#else
            Require(ai->GetFollowGuid()==owner.GetGUID(),"Companion follows a foreign Player");
#endif
            Require(FollowCredit(scenario)==1,"Unchanged native companion credit missing or duplicated");
            ai->IsSummonedBy(&owner);Require(FollowCredit(scenario)==1,"Repeated JS duplicated original credit");
            std::cout<<"CASE race="<<race<<" mode="<<mode<<" native follow="<<ai->GetFollowGuid().GetCounter()<<" owner="<<owner.GetGUID().GetCounter()<<" credit="<<FollowCredit(scenario)<<'\n';
            companion->UnSummon();map.RemoveAllObjectsInRemoveList();party.reset();CleanStories(map);++cases;
        }
        first->SetPlayer(nullptr);second->SetPlayer(nullptr);delete first;delete second;sScenarioMgr->RemoveScenario(map.GetInstanceId());
    }
#ifdef SLAGHAMMER_COMPANION_BASELINE
    std::cout<<"COUNTERFACTUAL "<<cases<<" native companion contexts:10 controlled nearer visitors steal follow, including GM/wrong-class/questless/group visitors;2 actual201004 self-casts also exercised. Earlier quest/stage/spawn controlled; not successful ordinary traversal\n";
#else
    std::cout<<"PASS "<<cases<<" native companion contexts: actual summoner remains follow target across nearer GM/wrong-class/questless/group visitors; original SmartAI/dialogue/credit dispatch retained. Earlier quest/stage/spawn controlled, not full client traversal\n";
#endif
    CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
