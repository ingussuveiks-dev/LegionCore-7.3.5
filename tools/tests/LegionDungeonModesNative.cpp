// Actual Legion loader and InstanceMap constructors, selected by matching DB2.
// This checks fresh instance initialization, not boss combat or client traversal.
#define HARBARON_NATIVE_SUPPORT_ONLY
#include "HarbaronFragmentNative.cpp"
#include <set>
#include "ChallengeMgr.h"
int main(int argc,char**argv){try{
    InitializeDungeon(argc,argv,true);
    std::set<uint32> selected;
    for(auto lfg:sLfgDungeonsStore){auto map=sMapStore.LookupEntry(lfg->MapID);if(map&&map->ExpansionID==6&&map->InstanceType==MAP_INSTANCE&&lfg->MinLevel<=110&&lfg->MaxLevel>=110)selected.insert(map->ID);}
    Check(selected.size()==13,"Level110 native dungeon scope changed; review catalog");
    // Bootstrap prepared connection-local shadows of this scope's actual66 rows.
    sObjectMgr->LoadInstanceEncounters();
    auto modes=sDB2Manager.GetAllMapsDifficultyes();unsigned pairs=0;
    for(auto id:selected)for(auto const& pair:modes[id]){
        auto difficulty=Difficulty(pair.first);Check(difficulty==1||difficulty==2||difficulty==8||difficulty==23,"Unexpected native dungeon mode");
        MapInstanced parent(id,300);InstanceMap map(id,300,9986000+pairs++,difficulty,&parent);map.CreateInstanceData(nullptr);
        auto script=map.GetInstanceScript();Check(script,"Native registered instance factory missing");
        unsigned bosses=0;while(bosses<64&&script->GetBossState(bosses)!=TO_BE_DECIDED){Check(script->GetBossState(bosses)==NOT_STARTED,"Fresh native boss state differs");++bosses;}
        Check(bosses>0&&bosses<64,"Missing or unbounded native encounter state vector");
        auto save=script->GetSaveData();Check(!save.empty(),"Native instance save serialization missing");
        auto encounters=sObjectMgr->GetDungeonEncounterList(id,difficulty);Check(encounters&&!encounters->empty(),"Native encounter loader produced no credit bindings");
        for(auto row:sDungeonEncounterStore)if(row->MapID==id&&(row->DifficultyID==0||row->DifficultyID==difficulty)){
            Check(std::any_of(encounters->begin(),encounters->end(),[&](DungeonEncounter const* encounter){return encounter->dbcEntry->ID==row->ID;}),"Native boss credit binding missing");
        }
        std::cout<<"INSTANCE\t"<<id<<'\t'<<unsigned(difficulty)<<'\t'<<map.GetScriptName()<<'\t'<<bosses<<'\t'<<save.size()<<'\n';
    }
    Check(pairs==46,"Native map/mode scope changed; review catalog");
    // The native owner lookup selects Lower/Upper Karazhan from the actual key.
    {MapInstanced parent(1492,300);InstanceMap map(1492,300,9987000,DIFFICULTY_NORMAL,&parent);map.CreateInstanceData(nullptr);DungeonPlayer owner(map,RACE_HUMAN);unsigned routes=0;
    for(auto route:sMapChallengeModeStore)if(selected.count(route->MapID)){
        Check(modes[route->MapID].count(DIFFICULTY_MYTHIC_KEYSTONE),"Challenge route lacks actual keystone mode");
        owner.m_challengeKeyInfo.ID=route->ID;float x=0,y=0,z=0,o=0;
        Check(ChallengeMgr::GetStartPosition(route->MapID,x,y,z,o,owner.GetGUID()),"Native challenge start lookup failed");
        Check(std::isfinite(x)&&std::isfinite(y)&&std::isfinite(z)&&std::isfinite(o),"Native challenge start not finite");
        auto location=std::find_if(sWorldSafeLocsStore.begin(),sWorldSafeLocsStore.end(),[&](WorldSafeLocsEntry const* row){return row->MapID==route->MapID&&std::fabs(row->Loc.X-x)<.01f&&std::fabs(row->Loc.Y-y)<.01f&&std::fabs(row->Loc.Z-z)<.01f;});
        Check(location!=sWorldSafeLocsStore.end(),"Native challenge start does not match a safe location on its map");
        Check(route->CriteriaCount[0]>route->CriteriaCount[1]&&route->CriteriaCount[1]>route->CriteriaCount[2]&&route->CriteriaCount[2]>0,"Native challenge timer thresholds invalid");
        std::cout<<"START\t"<<route->MapID<<'\t'<<route->ID<<'\t'<<x<<'\t'<<y<<'\t'<<z<<'\n';++routes;
    }Check(routes==13,"Native challenge route scope changed");}
    std::cout<<"PASS "<<checked<<" native instance checks across13 maps/46 native mode pairs;actual Legion script registration,factory,fresh encounter states,save serialization,actual scoped encounter loader/credit bindings and13 challenge starts. No combat/loot/affix/LFG/client traversal claim\n";
    CharacterDatabase.Close();LoginDatabase.Close();WorldDatabase.Close();HotfixDatabase.Close();MySQL::Library_End();return 0;
}catch(std::exception const& error){std::cerr<<error.what()<<'\n';return 1;}}
