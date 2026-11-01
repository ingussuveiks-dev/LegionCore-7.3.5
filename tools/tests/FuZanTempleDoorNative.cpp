// Actual GameObject::Use -> SmartGameObjectAI -> native205/49141 and door
// state machine. Scenario/quest preconditions are controlled in memory.
#define FUZAN_TEMPLE_NATIVE_SUPPORT_ONLY
#include "FuZanTempleIntroNative.cpp"
#include "GameObjectAI.h"
#include "../../src/server/scripts/Scenario/Artifacts/Monk/TempleOfTheJadeSerpent/fuzan_temple_sanctuary_door.h"

struct DoorDbTag {using Type=uint64 GameObject::*;friend Type Access(DoorDbTag);};
template struct FixtureAccess<DoorDbTag,&GameObject::m_DBTableGuid>;
struct DoorTreeTag {using Type=uint32 Scenario::*;friend Type Access(DoorTreeTag);};
template struct FixtureAccess<DoorTreeTag,&Scenario::currentTree>;
struct DoorObject : GameObject
{
    ~DoorObject(){if(IsInWorld())GetMap()->RemoveFromMap(static_cast<GameObject*>(this),false);ResetMap();}
};
struct DoorInstance : InstanceScript
{
    using InstanceScript::InstanceScript;
    DoorObject* door=nullptr;Player* player=nullptr;bool nested=false;
    void onScenarionNextStep(uint32 step) override
    {
        if(step!=2||!door||nested)return;
        nested=true;door->Use(player);
        Require(door->getLootState()==GO_READY&&!door->HasFlag(GAMEOBJECT_FIELD_FLAGS,GO_FLAG_IN_USE),"Nested native callback toggled door before outer activation");
    }
};
static void CheckDoor(uint8 race,uint32 number,uint32 mode)
{
    MapInstanced parent(1625,300);InstanceMap map(1625,300,9999600+number,DIFFICULTY_N_SCENARIO,&parent);
    auto instance=new DoorInstance(&map);map.*Access(IntroInstanceTag{})=instance;
    Scenario* scenario=sScenarioMgr->AddScenario(&map,1137);Require(scenario&&scenario->GetCurrentStep()==0,"Native door scenario missing");
    auto session=new WorldSession(0,"Fu Zan native sanctuary door fixture",{},SEC_PLAYER,6,0,"Win",LOCALE_enUS,0,false,AT_AUTH_FLAG_NONE,{});
    {
        LootPlayer player(session,map,1900+number,{917.288f,-2496.43f,180.524f,0.f});
        player.SetByteValue(UNIT_FIELD_BYTES_0,0,race);player.setFactionForRace(race);player.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_MONK);
        player.SetQuestStatus(42765,QUEST_STATUS_INCOMPLETE);player.SetQuestSlot(0,42765);
        DoorObject door;
        Require(door.Create(2000+number,251591,&map,1,player.GetPosition(),G3D::Quat(0,0,0,1),255,GO_STATE_READY),"Native door creation failed");
        door.*Access(DoorDbTag{})=138378;
        Require(map.AddToMap(static_cast<GameObject*>(&door)),"Native door map admission failed");
        door.Update(5000); // Actual native rate-limited initial GO_READY.
        auto ai=dynamic_cast<SmartGameObjectAI*>(door.AI());Require(ai,"Actual native door SmartGameObjectAI missing");
#ifdef FUZAN_DOOR_BASELINE
        Require(!dynamic_cast<go_fuzan_temple_sanctuary_door*>(door.AI()),"Counterfactual still uses repaired factory");
#else
        Require(dynamic_cast<go_fuzan_temple_sanctuary_door*>(door.AI()),"Actual derived door factory missing");
#endif
        Require(door.isSpawned()&&door.getLootState()==GO_READY&&!door.HasFlag(GAMEOBJECT_FIELD_FLAGS,GO_FLAG_IN_USE),"Native door initially unusable");
        Require(door.GetGoType()==GAMEOBJECT_TYPE_DOOR&&door.GetGOInfo()->GetAutoCloseTime()==0,"Native no-close door metadata changed");
        Require(door.GetGOInfo()->door.open==2570,"Native Ox Kick lock changed");
        Require(door.GetGOInfo()->displayId==11896&&door.GetGOInfo()->visualData[GO_VISUAL_BEFORE_COMPLETE_QUEST].SpellVisualID==58002,"Native door display/visual changed");
        if(mode==18)
        {
            door.*Access(DoorDbTag{})=138379; // Explicit other-source delegation control.
            door.Use(&player);
            Require(ai->GetScript()->HasRunEvent(0,SMART_EVENT_GOSSIP_HELLO)&&door.HasFlag(GAMEOBJECT_FIELD_FLAGS,GO_FLAG_IN_USE)&&
                !FuZanTempleDoor::Opened(scenario),"Shared other-source native behavior changed");
            std::cout<<"CASE other DB source preserves original native SmartAI/default door behavior PASS\n";
        }
        else
        {
        if(mode==1||mode==2||mode==19||mode==22)
        {
            if(mode==2){LootPet pet(player,map);door.Use(&pet);}
            else if(mode==19)door.AI()->GossipHello(&player,false); // Native report-use hook shape.
            else if(mode==22)
            {
                Require(player.CastSpell(&door,218174,false)==SPELL_CAST_OK,"Native early Ox Kick cast rejected");
                for(uint32 elapsed=0;elapsed<1000;elapsed+=10)player.Unit::Update(10);
            }
            else door.Use(&player);
            Require(!FuZanTempleDoor::Opened(scenario),"Future door criterion granted before active stage");
#ifdef FUZAN_DOOR_BASELINE
            Require(ai->GetScript()->HasRunEvent(0,SMART_EVENT_GOSSIP_HELLO)==(mode==1),"Original player/pet dispatch differs");
            Require(door.HasFlag(GAMEOBJECT_FIELD_FLAGS,GO_FLAG_IN_USE)&&door.getLootState()==GO_ACTIVATED,"Original early use did not latch native door");
#else
            Require(!ai->GetScript()->HasRunEvent(0,SMART_EVENT_GOSSIP_HELLO)&&!door.HasFlag(GAMEOBJECT_FIELD_FLAGS,GO_FLAG_IN_USE),"Repaired early use consumed root/door");
#endif
        }
        scenario->SetCurrentStep(1); // Explicit active-stage fixture, no granted door criterion.
        door.m_isUpdate=false;door.Update(300000); // Native map clears this per-frame guard.
#ifdef FUZAN_DOOR_BASELINE
        door.Use(&player);
        if(mode==1||mode==2)
        {
            Require(!FuZanTempleDoor::Opened(scenario)&&door.HasFlag(GAMEOBJECT_FIELD_FLAGS,GO_FLAG_IN_USE),"Expected persistent early door lock not reproduced");
            std::cout<<"EXPECTED_BROKEN native sanctuary door race="<<unsigned(race)<<" mode="<<mode<<" early player/pet Use latches no-close door without49141; active-stage retry remains blocked\n";
        }
        else Require(FuZanTempleDoor::Opened(scenario),"Native fresh stage1 positive control failed");
#else
        Position start=player.GetPosition();
        if(mode==9)player.SetQuestStatus(42765,QUEST_STATUS_COMPLETE);
        if(mode==3||mode==10)player.SetQuestStatus(42765,QUEST_STATUS_NONE);
        if(mode==4)player.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_WARRIOR);
        if(mode==5)player.Unit::setDeathState(JUST_DIED);
        if(mode==6)player.SetPhaseMask(2,false);
        if(mode==7)player.Relocate(start.GetPositionX()+40.f,start.GetPositionY(),start.GetPositionZ(),0.f);
        if(mode==8)player.SetGameMaster(true);
        if(mode==13)scenario->SetCurrentStep(2);
        if(mode==14)door.SetFlag(GAMEOBJECT_FIELD_FLAGS,GO_FLAG_IN_USE);
        if(mode==15)door.SetFlag(GAMEOBJECT_FIELD_FLAGS,GO_FLAG_NOT_SELECTABLE);
        if((mode>=3&&mode<=8)||mode==10||(mode>=13&&mode<=15))
        {
            uint32 flags=door.GetUInt32Value(GAMEOBJECT_FIELD_FLAGS);door.Use(&player);
            Require(!FuZanTempleDoor::Opened(scenario)&&!ai->GetScript()->HasRunEvent(0,SMART_EVENT_GOSSIP_HELLO)&&
                door.getLootState()==GO_READY&&door.GetUInt32Value(GAMEOBJECT_FIELD_FLAGS)==flags,"Rejected native use changed progress/root/door state or unrelated flags");
            player.SetQuestStatus(42765,QUEST_STATUS_INCOMPLETE);player.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_MONK);
            if(mode==5){player.Unit::setDeathState(ALIVE);player.SetHealth(player.GetMaxHealth());}
            if(mode==6)player.SetPhaseMask(1,false);
            if(mode==7)player.Relocate(start);
            if(mode==8)player.SetGameMaster(false);
            if(mode==13)scenario->SetCurrentStep(1);
            if(mode==14)door.RemoveFlag(GAMEOBJECT_FIELD_FLAGS,GO_FLAG_IN_USE);
            if(mode==15)door.RemoveFlag(GAMEOBJECT_FIELD_FLAGS,GO_FLAG_NOT_SELECTABLE);
        }
        if(mode==11)
        {
            auto peerSession=new WorldSession(0,"Fu Zan door group fixture",{},SEC_PLAYER,6,0,"Win",LOCALE_enUS,0,false,AT_AUTH_FLAG_NONE,{});
            {
                LootPlayer peer(peerSession,map,2400+number,start);peer.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_MONK);
                ProbeParty group(player,peer);door.Use(&peer);
                Require(!FuZanTempleDoor::Opened(scenario)&&door.getLootState()==GO_READY,"Ineligible group viewer latched door or granted progression");
            }
            peerSession->SetPlayer(nullptr);
        }
        if(mode==12)
        {
            // Native events seed only the sibling rescue prerequisite; this
            // does not claim traversal of the actual three guard encounters.
            for(uint32 n=0;n<3;++n)scenario->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,52435,0,0,nullptr,&player);
            instance->door=&door;instance->player=&player;
        }
        if(mode==16)
        {
            auto tree=scenario->*Access(DoorTreeTag{});scenario->*Access(DoorTreeTag{})=0;
            door.Use(&player); // Explicit missing native criteria-routing fault.
            Require(!FuZanTempleDoor::Opened(scenario)&&!ai->GetScript()->HasRunEvent(0,SMART_EVENT_GOSSIP_HELLO)&&
                door.getLootState()==GO_READY&&!door.HasFlag(GAMEOBJECT_FIELD_FLAGS,GO_FLAG_IN_USE),"Failed native credit did not preserve actual retry");
            scenario->*Access(DoorTreeTag{})=tree;
        }
        if(mode==19)
        {
            door.AI()->GossipHello(&player,false);
            Require(FuZanTempleDoor::Opened(scenario)&&door.getLootState()==GO_READY&&!door.HasFlag(GAMEOBJECT_FIELD_FLAGS,GO_FLAG_IN_USE),
                "Report-first native hook did not retain separate physical Use");
        }
        if(mode==23)
        {
            Require(player.CastSpell(&door,3365,false)==SPELL_FAILED_BAD_TARGETS,"Actual wrong native key was not rejected");
            for(uint32 elapsed=0;elapsed<1000;elapsed+=10)player.Unit::Update(10);
            Require(!FuZanTempleDoor::Opened(scenario)&&!ai->GetScript()->HasRunEvent(0,SMART_EVENT_GOSSIP_HELLO)&&
                door.getLootState()==GO_READY&&!door.HasFlag(GAMEOBJECT_FIELD_FLAGS,GO_FLAG_IN_USE),"Failed native lock cast changed progression or readiness");
        }
        if(mode==21||mode==22||mode==23)
        {
            Require(player.CastSpell(&door,218174,false)==SPELL_CAST_OK,"Native active Ox Kick cast rejected");
            for(uint32 elapsed=0;elapsed<1000;elapsed+=10)player.Unit::Update(10);
            Require(FuZanTempleDoor::Opened(scenario),"Actual ordinary Ox Kick did not dispatch native205/49141");
        }
        else door.Use(&player);
        Require(FuZanTempleDoor::Opened(scenario)&&door.HasFlag(GAMEOBJECT_FIELD_FLAGS,GO_FLAG_IN_USE)&&door.getLootState()==GO_ACTIVATED,"Eligible native205 did not acknowledge49141 and open original door");
        if(mode==12)Require(instance->nested&&scenario->GetCurrentStep()==2,"Real next-step reentry not exercised");
        auto counter=scenario->GetAchievementMgr().GetCriteriaProgressMap()->at(49141).Counter;
        door.Use(&player);Require(scenario->GetAchievementMgr().GetCriteriaProgressMap()->at(49141).Counter==counter,"Native door credit duplicated");
        if(mode==19||mode==20)
        {
            door.AI()->GossipHello(&player,false);
            Require(scenario->GetAchievementMgr().GetCriteriaProgressMap()->at(49141).Counter==counter&&door.getLootState()==GO_ACTIVATED&&
                door.HasFlag(GAMEOBJECT_FIELD_FLAGS,GO_FLAG_IN_USE),"Native report-after-use replayed credit or changed physical door");
        }
        if(mode==17)
        {
            map.RemoveFromMap(static_cast<GameObject*>(&door),false);door.ResetMap();scenario->SetCurrentStep(2);
            DoorObject recreated;
            Require(recreated.Create(3000+number,251591,&map,1,start,G3D::Quat(0,0,0,1),255,GO_STATE_READY),"Native checkpoint door recreation failed");
            recreated.*Access(DoorDbTag{})=138378;
            Require(map.AddToMap(static_cast<GameObject*>(&recreated)),"Recreated native door admission failed");recreated.Update(5000);
            recreated.Use(&player);
            auto freshAI=dynamic_cast<SmartGameObjectAI*>(recreated.AI());
            Require(freshAI&&!freshAI->GetScript()->HasRunEvent(0,SMART_EVENT_GOSSIP_HELLO)&&
                recreated.HasFlag(GAMEOBJECT_FIELD_FLAGS,GO_FLAG_IN_USE)&&recreated.getLootState()==GO_ACTIVATED&&
                scenario->GetAchievementMgr().GetCriteriaProgressMap()->at(49141).Counter==counter&&scenario->GetCurrentStep()==2,
                "Actual checkpoint receipt did not reopen native door without replay/advancement");
        }
#endif
        }
        std::cout<<"CASE native sanctuary door race="<<unsigned(race)<<" mode="<<mode<<" actual Use/SmartAI205/native49141/default door/cleanup PASS\n";
    }
    session->SetPlayer(nullptr);sScenarioMgr->RemoveScenario(map.GetInstanceId());
}
int main(int argc,char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);SetUnhandledExceptionFilter(CrashTrace);
    try{
        InitializeIntro(argc,argv,true);HashMapHolder<GameObject>::SetSize(20000);
        auto info=const_cast<GameObjectTemplate*>(sObjectMgr->GetGameObjectTemplate(251591));Require(info,"Native sanctuary door missing");
        uint32 id=sObjectMgr->GetScriptId("go_fuzan_temple_sanctuary_door");Require(id,"Native descriptor name missing");
#ifdef FUZAN_DOOR_BASELINE
        Require(!info->ScriptId||info->ScriptId==id,"Counterfactual refuses custom door script");info->ScriptId=0;
#elif defined(FUZAN_DOOR_LIVE)
        Require(info->ScriptId==id,"Actual live sanctuary door binding missing");
#else
        Require(!info->ScriptId,"Private pending delta refuses custom/existing door script");info->ScriptId=id;
#endif
        sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
        RegisterGameObjectAI(go_fuzan_temple_sanctuary_door);sScriptMgr->SwapScriptContext(true);
        uint32 number=0;
        for(uint8 race:{uint8(RACE_HUMAN),uint8(RACE_ORC)})
#ifdef FUZAN_DOOR_BASELINE
            for(uint32 mode:{0u,1u,2u})CheckDoor(race,++number,mode);
#elif defined(FUZAN_DOOR_FAULTS)
            CheckDoor(race,++number,16);
#else
        {
            for(uint32 mode=0;mode<16;++mode)CheckDoor(race,++number,mode);
            for(uint32 mode=17;mode<=23;++mode)CheckDoor(race,++number,mode);
        }
#endif
        CharacterDatabase.Close();LoginDatabase.Close();HotfixDatabase.Close();WorldDatabase.Close();
#ifdef FUZAN_DOOR_BASELINE
        std::cout<<"EXPECTED_BROKEN4 original player/pet native early door locks paired with two actual fresh-stage controls; not a whole old binary/client claim.\n";
#elif defined(FUZAN_DOOR_FAULTS)
        std::cout<<"PASS2 explicit missing-native-criteria-routing faults/recovery; diagnostics retained, ordinary clean40 separate.\n";
#else
        std::cout<<"PASS46 actual native sanctuary door cases; ordinary205/native49141/default model/visual/open state preserved, actual untriggered218174 Ox Kick and wrong3365 key rejection, report/use hook orders/checkpoint recreation/shared source/negative admissions. No full packet/client traversal or character save.\n";
#endif
        return 0;
    }catch(std::exception const& error){std::cerr<<error.what()<<'\n';return 1;}
}
