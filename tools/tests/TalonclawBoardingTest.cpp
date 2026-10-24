#define TALONCLAW_BOARDING_TEST
#include "HueyReturnAdmissionTest.cpp"
struct PhaseUpdateData {void AddQuestUpdate(uint32 identity){assert(identity==40385||identity==40604||identity==40835||identity==40932||identity==40606||identity==1);}};
struct Phases {void NotifyConditionChanged(PhaseUpdateData const&) {}} phases;
struct BoardingPlayer:Player {
    Creature* bird=nullptr;
    bool visible=false;
    Phases& GetPhaseMgr(){return phases;}
    void UpdateForQuestWorldObjects(){visible=canSeeSpellClickOn(bird);}
    void Removed(uint32 quest_id){
#include "Removal.inc"
    }
    void OldRemoved(uint32 quest_id){
#include "OldRemoval.inc"
    }
};
int main(int argc,char** argv)
{
    _set_abort_behavior(0,_CALL_REPORTFAULT);
    assert(argc==5);
    std::vector<Condition> clickRows,rootRows,castRows;
    ReadRows(argv[1],clickRows);ReadRows(argv[2],rootRows);ReadRows(argv[3],castRows);
    bool baseline=std::string(argv[4])=="baseline";
    assert(clickRows.size()==(baseline?0u:8u)&&rootRows.size()==(baseline?1u:8u)&&castRows.size()==(baseline?0u:5u));
    for(auto& row:clickRows)conditionManager.SpellClickEventConditionStore[100965][198941].push_back(&row);
    for(auto& row:rootRows)conditionManager.SmartEventConditionStore[{100965,0}][1].push_back(&row);
    templates.clicks.clear();templates.clicks.emplace(100965,SpellClickInfo{198941,1});spells.identity=198941;
    outdoorMap.instanced=false;
    BoardingPlayer owner;Creature bird;bird.entry=100965;bird.mapId=owner.mapId=1220;bird.map=owner.map=&outdoorMap;owner.bird=&bird;owner.quests.clear();
    assert(!owner.canSeeSpellClickOn(&bird));
    for(uint32 status:{0u,1u,3u,5u,6u}){
        owner.quests[40385]=status;owner.casts=0;bird.ai.calls=0;
        bool eligible=status==3;
        assert(owner.canSeeSpellClickOn(&bird)==eligible);
        assert(bird.HandleSpellClick(&owner)==eligible&&owner.casts==uint32(eligible)&&bird.ai.calls==1);
        assert(conditionManager.IsObjectMeetingSmartEventConditions(100965,0,0,&owner,&bird)==eligible);
        ConditionSourceInfo cast(&owner);
        assert(conditionManager.IsObjectMeetToConditions(cast,[&]{ConditionList rows;for(auto& row:castRows)rows.push_back(&row);return rows;}())==eligible);
    }
    owner.quests[40385]=3;
    for(uint32 failure=0;failure<5;++failure){
        owner.classMask=4;owner.mapId=bird.mapId=1220;owner.alive=bird.alive=true;bird.entry=100965;
        if(failure==0)owner.classMask=1;if(failure==1)owner.mapId=1;if(failure==2)owner.alive=false;if(failure==3)bird.mapId=1511;if(failure==4)bird.alive=false;
        owner.casts=0;assert(!owner.canSeeSpellClickOn(&bird)&&!bird.HandleSpellClick(&owner)&&!owner.casts);
        assert(!conditionManager.IsObjectMeetingSmartEventConditions(100965,0,0,&owner,&bird));
    }
    owner.classMask=4;owner.mapId=bird.mapId=1220;owner.alive=bird.alive=true;
    Unit pet;pet.classMask=4;pet.mapId=1220;
    assert(!conditionManager.IsObjectMeetingSmartEventConditions(100965,0,0,&pet,&bird));
    assert(!conditionManager.IsObjectMeetingSmartEventConditions(100965,0,0,nullptr,&bird));
    assert(!conditionManager.IsObjectMeetingSmartEventConditions(100965,0,0,&owner,nullptr));
    owner.UpdateForQuestWorldObjects();assert(owner.visible);
    owner.quests.erase(40385);owner.OldRemoved(40385);assert(owner.visible&&!owner.canSeeSpellClickOn(&bird));
    owner.Removed(40385);assert(!owner.visible);
    owner.quests[40385]=3;owner.UpdateForQuestWorldObjects();assert(owner.visible);
    owner.quests[40385]=6;owner.Removed(40385);assert(!owner.visible);
    owner.quests[40385]=3;owner.UpdateForQuestWorldObjects();assert(owner.visible);
    owner.quests.clear();owner.Removed(1);assert(owner.visible);
    for(uint32 identity:{40604u,40835u,40932u,40606u}){owner.visible=true;owner.Removed(identity);assert(!owner.visible);}
    std::cout<<"PASS: actual source18 visibility/click, source22 independent AI admission and Player-only source17 predicates reject early/dead/abandoned/rewarded/wrong class/map/pet/null; actual removal callback refresh closes stale cached boarding flag while preserving prior camp quests. Controlled cast/packet/phase sinks, not client traversal.\n";
}
