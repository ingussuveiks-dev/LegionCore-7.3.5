// Actual native normal/Teeming route selection and scenario progression.
// Completed leaf counters are controlled checkpoints, not earned combat.
#define CHALLENGE_DEATH_SUPPORT_ONLY
#include "LegionChallengeDeathAffixesNative.cpp"
#ifdef TEEMING_CRITERIA_BASELINE
#include "ScenarioActual.inc"
#endif

struct TeemingCriteriaScenario : Scenario
{
    using Scenario::Scenario;
    uint32 Tree() const { return currentTree; }
    bool Rewarded() const { return rewarded; }
    ScenarioSteps const& Steps() const { return steps; }
    void ControlledSteps(ScenarioSteps value) { steps=std::move(value); }
};
struct TeemingProgressTag { using Type=CriteriaProgressMap AchievementMgr<Scenario>::*;friend Type Access(TeemingProgressTag); };
template struct FixtureAccess<TeemingProgressTag,&AchievementMgr<Scenario>::_criteriaProgress>;
struct TeemingProgressArrayTag { using Type=std::vector<CriteriaProgress*> AchievementMgr<Scenario>::*;friend Type Access(TeemingProgressArrayTag); };
template struct FixtureAccess<TeemingProgressArrayTag,&AchievementMgr<Scenario>::_criteriaProgressArr>;

static void SeedLeaves(TeemingCriteriaScenario& scenario,CriteriaTree const* tree)
{
    if(tree->Criteria)
    {
        auto& progress=(scenario.GetAchievementMgr().*Access(TeemingProgressTag{}))[tree->ID];
        progress.Counter=1000000;
        (scenario.GetAchievementMgr().*Access(TeemingProgressArrayTag{}))[tree->ID]=&progress;
    }
    for(auto child:tree->Children)SeedLeaves(scenario,child);
}

int main(int argc,char**argv)
{
    try
    {
        InitializeDungeon(argc,argv,true);sObjectMgr->LoadItemTemplates();sObjectMgr->LoadItemTemplateAddon();sObjectMgr->LoadScenarioData();sAchievementMgr->LoadCriteriaList();
        (sQuestDataStore->*Access(QuestVectorTag{})).resize(1,nullptr);sQuestDataStore->*Access(QuestMaxTag{})=0;
        unsigned routes=0,brokenOrders=0;
        for(auto route:sMapChallengeModeStore)
        if(route->MapID==1456||route->MapID==1458||route->MapID==1466||route->MapID==1477||route->MapID==1492||route->MapID==1493||route->MapID==1501||route->MapID==1516||route->MapID==1571||route->MapID==1651||route->MapID==1677||route->MapID==1753)
        {
            for(bool teeming:{false,true})
            {
                MapInstanced parent(route->MapID,300);InstanceMap map(route->MapID,300,9945000+routes*2+teeming,DIFFICULTY_MYTHIC_KEYSTONE,&parent);map.CreateInstanceData(nullptr);
                LifecyclePlayer owner(map,802,Position{2934.49f,715.757f,554.286f,1.55255f});auto key=owner.Key(*route);owner.m_challengeKeyInfo.Affix=teeming?Affixes::Teeming:0;
                TeemingCriteriaScenario scenario(&map,1175);scenario.CreateChallenge(&owner);auto challenge=scenario.GetChallenge();
                Check(challenge&&challenge->_canRun&&challenge->HasAffix(Affixes::Teeming)==teeming,"Native route lost selected key affix");
                auto normal=sScenarioMgr->GetScenarioSteps(scenario.GetScenarioId(),false);auto extra=sScenarioMgr->GetScenarioSteps(scenario.GetScenarioId(),true);
                Require(normal&&extra&&!normal->empty()&&!extra->empty(),"Matching native normal/Teeming steps absent");
                Check(extra->size()==1&&sScenarioMgr->HasAffixesTeeming(extra->front()->Criteriatreeid),"Native route lacks its single Teeming variant");
                Check(!sScenarioMgr->HasAffixesTeeming(normal->front()->Criteriatreeid),"Ordinary route selected Teeming criterion variant");
                auto selected=(teeming?extra:normal)->front();auto originalOrder=selected->OrderIndex;
                Check(selected==sScenarioStepStore.LookupEntry(selected->ID)&&scenario.Steps().front()==selected,"Native immutable DB2 step identity changed");
                bool broken=teeming&&originalOrder!=0;
                std::cout<<"ROUTE id="<<route->ID<<" map="<<route->MapID<<" teeming="<<teeming<<" scenario="<<scenario.GetScenarioId()<<" step="<<selected->ID<<" originalOrder="<<unsigned(originalOrder)<<" expected="<<selected->Criteriatreeid<<" actual="<<scenario.Tree()<<'\n';
#ifdef TEEMING_CRITERIA_BASELINE
                Check(scenario.GetScenarioCriteriaByStep(0)==(broken?0:selected->Criteriatreeid)&&scenario.Tree()==(broken?0:selected->Criteriatreeid),"Pinned criterion-order failure changed");
#else
                Check(scenario.GetScenarioCriteriaByStep(0)==selected->Criteriatreeid&&scenario.Tree()==selected->Criteriatreeid,"Native selected criterion tree lost effective step zero");
#endif
                Check(!scenario.IsCompleted(false)&&!scenario.Rewarded(),"Fresh native scenario was already completed");
                scenario.UpdateCurrentStep(false);Check(scenario.GetCurrentStep()==0&&!scenario.Rewarded(),"Empty real criterion graph advanced progression");
                if(teeming)
                {
                    auto tree=sAchievementMgr->GetCriteriaTree(selected->Criteriatreeid);Require(tree&&!tree->Children.empty(),"Native selected criterion graph absent");
                    auto children=sDB2Manager.GetCriteriaTreeList(tree->ID);Require(children&&!children->empty(),"Matching DB2 criterion children absent");
                    auto leaf=std::find_if(children->begin(),children->end(),[](CriteriaTreeEntry const* row){return row->CriteriaID!=0;});Require(leaf!=children->end(),"Matching selected tree has no immediate criterion receipt");
#ifdef TEEMING_CRITERIA_BASELINE
                    Check(scenario.CanUpdateCriteria((*leaf)->ID)==!broken,"Pinned currentTree-zero receipt denial changed");
#else
                    Check(scenario.CanUpdateCriteria((*leaf)->ID),"Selected native criterion receipt denied");
#endif
                    // Native tree evaluation and UpdateCurrentStep execute; only
                    // the earned leaf counters and inactive run are controlled.
                    SeedLeaves(scenario,tree);Check(scenario.GetAchievementMgr().IsCompletedScenarioTree(tree->Entry),"Controlled native leaf checkpoint did not satisfy real tree");
                    scenario.UpdateCurrentStep(true);
#ifdef TEEMING_CRITERIA_BASELINE
                    Check(scenario.GetCurrentStep()==originalOrder+1&&scenario.IsCompleted(false)==!broken,"Pinned Teeming completion overshoot changed");
#else
                    Check(scenario.GetCurrentStep()==1&&scenario.IsCompleted(false)&&scenario.Tree()==0,"Completed Teeming graph did not finish its single effective step");
#endif
                    auto finished=scenario.GetCurrentStep();scenario.UpdateCurrentStep(true);Check(scenario.GetCurrentStep()==finished,"Repeated native tree completion changed finished step");
                    Check(!challenge->_run&&!challenge->_complete&&!scenario.Rewarded(),"Loading-only checkpoint manufactured run reward");
                    if(broken)++brokenOrders;
                }
                Check(selected->OrderIndex==originalOrder,"Native global DB2 order was mutated");
            }
            ++routes;
        }
        Check(routes==13&&brokenOrders>0,"Native route/nonzero-order scope changed");
        auto routeChecks=checked;
        // Check the exact getter's fallback with every immutable DB2 step,
        // including nonzero orders. Publication of this one-element vector is
        // controlled; no ordinary quest/scenario traversal is claimed.
        {
            MapInstanced parent(1492,300);InstanceMap map(1492,300,9946000,DIFFICULTY_MYTHIC_KEYSTONE,&parent);map.CreateInstanceData(nullptr);
            LifecyclePlayer owner(map,804,Position{2934.49f,715.757f,554.286f,1.55255f});auto key=owner.Key(*sMapChallengeModeStore.LookupEntry(208));owner.m_challengeKeyInfo.Affix=Affixes::Teeming;
            TeemingCriteriaScenario scenario(&map,1175);
            unsigned ordinary=0,nonTeeming=0;
            for(auto row:sScenarioStepStore)
            {
                scenario.ControlledSteps({row});
                Check(scenario.GetScenarioCriteriaByStep(row->OrderIndex)==row->Criteriatreeid,"No-challenge native DB2 order fallback changed");
                Check(scenario.GetScenarioCriteriaByStep(uint8(row->OrderIndex+1))==0,"No-challenge native getter accepted unrelated order");++ordinary;
            }
            scenario.CreateChallenge(&owner);
            for(auto row:sScenarioStepStore)if(!sScenarioMgr->HasAffixesTeeming(row->Criteriatreeid))
            {
                scenario.ControlledSteps({row});
                Check(scenario.GetScenarioCriteriaByStep(row->OrderIndex)==row->Criteriatreeid,"Teeming key changed an ordinary native DB2 step order");++nonTeeming;
            }
            scenario.ControlledSteps({});Check(scenario.GetScenarioCriteriaByStep(0)==0,"Empty native scenario getter changed");
            std::cout<<"META ordinaryGetterRows="<<ordinary<<" teemingNonVariantRows="<<nonTeeming<<" routeAssertions="<<routeChecks<<" fallbackAssertions="<<checked-routeChecks<<'\n';
        }
#ifdef TEEMING_CRITERIA_BASELINE
        std::cout<<"COUNTERFACTUAL "<<checked<<" native checks across13 routes:nonzeroOrder="<<brokenOrders<<" actual Teeming variants lose currentTree and overshoot single-step completion;not ordinary successful runs\n";
#else
        std::cout<<"PASS "<<checked<<" native Teeming criterion checks across13 routes:nonzeroOrder="<<brokenOrders<<" normal+Teeming selection,real graph evaluation,controlled completed leaves and native loading-only progression;no earned combat/client claim\n";
#endif
        CharacterDatabase.Close();LoginDatabase.Close();WorldDatabase.Close();HotfixDatabase.Close();MySQL::Library_End();return 0;
    }catch(std::exception const&error){std::cerr<<error.what()<<'\n';return 1;}
}
