// Real native spell/lock/packet/GO/criteria paths. Earlier quest progression,
// native event-clock advancement and the fixture instance callback are controlled.
#define main BonfireBootstrapMain
#include "WarswordsBonfireNative.cpp"
#undef main
#include "RunestoneNative.inc"
#include "GameObjectModel.h"
#include "GameObjectPackets.h"
#include "MapManager.h"
#include "WarswordsInstance.inc"
#ifdef RUNESTONE_BASELINE
using RuneFactory = BaselinePrisonRunestone;
#else
using RuneFactory = go_prison_runestone;
#endif
struct LegacyRuneDispatch : Spell
{
    using Spell::Spell;
    SpellCastResult Open(GameObject* rune)
    {
        // Independently pinned pre-repair EffectOpenLock suffix: successful
        // CanOpenLock falls through SendLoot, whose goober Use skips Hello(true).
        gameObjTarget=rune;SkillType skill=SKILL_NONE;int32 required=0,value=0;
        auto result=CanOpenLock(0,99,skill,required,value);
        if(result==SPELL_CAST_OK)SendLoot(rune->GetGUID(),LOOT_SKINNING);
        return result;
    }
};
static void AdvanceRune(Player& player){for(uint32 n=0;n<40;++n)player.m_Events.Update(500);}
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);sObjectMgr->LoadPageTexts();sObjectMgr->LoadGameObjectTemplate();
 auto info=sObjectMgr->GetGameObjectTemplate(244940);Require(info&&info->type==GAMEOBJECT_TYPE_GOOBER&&info->GetLockId()==99&&info->goober.spell==194802&&info->goober.eventID==46970,"Actual rune template differs");
 Require(sSpellMgr->GetSpellInfo(8386)->CalcCastTime()==1000,"Actual Attacking duration differs");
 auto rules=WorldDatabase.Query("SELECT COUNT(*) FROM achievement_criteria_data WHERE criteria_id=29081");Require(rules&&!rules->Fetch()[0].GetUInt32(),"Rune criterion needs additional loader rules");
 auto event=WorldDatabase.Query("SELECT COUNT(*) FROM event_scripts WHERE id=46970");Require(event&&!event->Fetch()[0].GetUInt32(),"Native goober event gained a second credit producer");
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1511,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1511,{}}});
 sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new instance_warswords;new RuneFactory;sScriptMgr->SwapScriptContext(true);LoadGameObjectModelList(sWorld->GetDataPath());
#ifdef RUNESTONE_BASELINE
 uint32 const profiles=1;
#else
 uint32 const profiles=13;
#endif
 uint32 cases=0,blockedLos=0;
 for(uint32 profile=0;profile<profiles;++profile)for(uint32 race:{RACE_HUMAN,RACE_ORC}){
  MapInstanced parent(1511,300);InstanceMap map(1511,300,9988100+race+100*profile,DIFFICULTY_N_SCENARIO,&parent);map.CreateInstanceData(nullptr);
  auto scenario=sScenarioMgr->AddScenario(&map,944);scenario->SetCurrentStep(3);auto session=Session();
  {
   NativePlayer player(session,map,901,{3493.38f,1631.03f,5.30085f,0});player.SetByteValue(UNIT_FIELD_BYTES_0,0,race);player.SetQuestStatus(40043,profile==10?QUEST_STATUS_COMPLETE:QUEST_STATUS_INCOMPLETE);player.SetQuestSlot(0,40043);
   auto count=[&](){auto rows=scenario->GetAchievementMgr().GetCriteriaProgressMap();auto it=rows->find(45863);return it==rows->end()?uint64(0):it->second.Counter;};
   Position poses[4]={{3493.38f,1633.03f,5.30085f,0},{3550.11f,1544.56f,4.90815f,0},{3366.14f,1616.58f,9.50231f,0},{3407.48f,1631.98f,9.62016f,0}};
   uint32 stones=(profile==3||profile==8)?1:4;
#ifdef RUNESTONE_BASELINE
   stones=1;
#endif
   for(uint32 index=0;index<stones;++index){
    auto pose=poses[index];player.Relocate(pose.GetPositionX(),pose.GetPositionY()-2,pose.GetPositionZ(),0);
    if(profile==12){static float const dx[4]={-2,2,0,0},dy[4]={0,0,2,-2};player.Relocate(pose.GetPositionX()+dx[index],pose.GetPositionY()+dy[index],pose.GetPositionZ(),0);}
    auto rune=new GameObject;Require(rune->Create(sObjectMgr->GetGenerator<HighGuid::GameObject>()->Generate(),244940,&map,1,pose,G3D::Quat(),255,GO_STATE_READY),"Rune native create failed");
    rune->*Access(BonfireDbTag{})=134130+index;Require(map.AddToMap(rune),"Rune native insertion failed");Require(dynamic_cast<RuneFactory::go_prison_runestoneAI*>(rune->AI()),"Rune actual AI binding missing");Require(rune->m_model,"Actual rune collision model must load");
    bool los=player.IsWithinLOSInMap(rune);if(!los)++blockedLos;
#ifndef RUNESTONE_BASELINE
    auto ai=dynamic_cast<RuneFactory::go_prison_runestoneAI*>(rune->AI());
    Require(reinterpret_cast<char*>(&ai->claimed)-reinterpret_cast<char*>(ai)==0x10&&reinterpret_cast<char*>(&ai->presenting)-reinterpret_cast<char*>(ai)==0x18,"Actual AI offsets used by linked claim/owner proof differ");
#endif
    auto packetUse=[&]{WorldPackets::GameObject::GameObjectUse packet{WorldPacket(CMSG_GAME_OBJ_USE,0)};packet.Guid=rune->GetGUID();session->HandleGameObjectUse(packet);};
    auto report=[&]{WorldPackets::GameObject::GameObjReportUse packet{WorldPacket(CMSG_GAME_OBJ_REPORT_USE,0)};packet.Guid=rune->GetGUID();session->HandleGameobjectReportUse(packet);};
    SpellCastResult result=SPELL_CAST_OK;
#ifdef RUNESTONE_BASELINE
    TriggerCastData cast;cast.triggerFlags=TRIGGERED_FULL_MASK;LegacyRuneDispatch legacy(&player,sSpellMgr->GetSpellInfo(8386),cast);result=legacy.Open(rune);
#else
    switch(profile){
     case 1:rune->Use(&player);break;
     case 2:packetUse();report();break;
     case 3:report();break;
     case 4:rune->Use(&player);player.InterruptNonMeleeSpells(false,8386);break;
     case 5:result=player.CastSpell(rune,3365,false);break;
     case 6:player.SetQuestStatus(40043,QUEST_STATUS_NONE);result=player.CastSpell(rune,8386,false);break;
     case 7:result=player.CastSpell(rune,8386,false);player.SetQuestStatus(40043,QUEST_STATUS_NONE);break;
     case 8:result=player.CastSpell(rune,8386,false);rune->RemoveFromWorld();break;
     case 11:scenario->SetCurrentStep(2);result=player.CastSpell(rune,8386,false);break;
     default:result=player.CastSpell(rune,8386,false);report();break;
    }
#endif
    AdvanceRune(player);
#ifdef RUNESTONE_BASELINE
    Require(result==SPELL_CAST_OK&&count()==0&&scenario->GetCurrentStep()==3&&rune->GetGoState()==GO_STATE_ACTIVE,"Pinned native CanOpenLock/SendLoot goober suffix no longer reproduces");
    std::cout<<"COUNTERFACTUAL native lock99/SendLoot/old-AI Use shows destruction but0 credit;subset lacks new callback,not a full ordinary passing cast\n";
#else
    if(profile>=4&&profile<=7||profile==11){
     Require(count()==index&&rune->GetGoState()==GO_STATE_READY,"Rejected/interrupted/abandoned cast credited or destroyed rune");
     player.SetQuestStatus(40043,QUEST_STATUS_INCOMPLETE);if(profile==11)scenario->SetCurrentStep(3);
     result=player.CastSpell(rune,8386,false);AdvanceRune(player);
    }
    if(profile==3||profile==8){Require(!count()&&scenario->GetCurrentStep()==3,"Report-only or removed pending object credited rune");}
    else{
     Require(count()==index+1&&scenario->GetCurrentStep()==(index==3?4u:3u)&&rune->GetGoState()==GO_STATE_ACTIVE,"Actual rune count/transition/presentation differs");
     packetUse();report();player.CastSpell(rune,8386,true);AdvanceRune(player);
     Require(count()==index+1,"Repeated same-object native request duplicated progress");
    }
#endif
    std::cout<<"TRACE rune profile="<<profile<<" race="<<race<<" object="<<index<<" cast="<<result<<" los="<<los<<" criterion="<<count()<<" step="<<scenario->GetCurrentStep()<<'\n';
    if(rune->IsInWorld())rune->RemoveFromWorld();rune->RemoveFromGrid();rune->ResetMap();delete rune;map.RemoveAllObjectsInRemoveList();
#ifndef RUNESTONE_BASELINE
    if(profile==9){
     auto reloaded=new GameObject;Require(reloaded->Create(sObjectMgr->GetGenerator<HighGuid::GameObject>()->Generate(),244940,&map,1,pose,G3D::Quat(),255,GO_STATE_READY),"Reloaded rune create failed");
     reloaded->*Access(BonfireDbTag{})=134130+index;Require(map.AddToMap(reloaded),"Reloaded rune insertion failed");
     Require(reloaded->GetGoState()==GO_STATE_ACTIVE,"Actual instance did not restore destroyed presentation after object reload");
     reloaded->Use(&player);player.CastSpell(reloaded,8386,true);AdvanceRune(player);Require(count()==index+1,"Object/AI reload lost the instance-owned destruction receipt");
     reloaded->RemoveFromWorld();reloaded->RemoveFromGrid();reloaded->ResetMap();delete reloaded;map.RemoveAllObjectsInRemoveList();
    }
#endif
   }
   if(profile==9){scenario->SetCurrentStep(0);for(uint32 key=134130;key<=134133;++key)Require(map.GetInstanceScript()->GetData(key)==0,"New native scenario attempt did not clear old receipts");}
   CleanStories(map);
  }
  session->SetPlayer(nullptr);delete session;sScenarioMgr->RemoveScenario(map.GetInstanceId());++cases;
 }
 CloseAltheaNative();std::cout<<"PASS native rune fixtures="<<cases<<" actual-model blocked-LOS casts="<<blockedLos<<";controlled prior progression/event clock,not authenticated client traversal\n";return 0;
}catch(std::exception const&error){std::cerr<<error.what()<<'\n';return 1;}}
