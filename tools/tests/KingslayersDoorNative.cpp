// Actual registered GO AI, GameObject::Use/native state machine and Opening.
// Quest/earlier scenario state is seeded; no authenticated client or save.
#define ALTHEA_CREDIT_NATIVE_SUPPORT_ONLY
#include "AltheaCreditNative.cpp"
#include "GameObjectModel.h"
#include "GameObjectPackets.h"
#include "Spell.h"
#ifdef KINGSLAYERS_DOOR_BASELINE
#include "DoorBefore.inc"
using DoorScript=go_kingslayers_door_baseline;
#else
#include "DoorCurrent.inc"
using DoorScript=go_kingslayers_door;
#endif
struct DoorGuidTag{using Type=uint64 GameObject::*;friend Type Access(DoorGuidTag);};
template struct FixtureAccess<DoorGuidTag,&GameObject::m_DBTableGuid>;
struct DoorObject:GameObject{
    ~DoorObject(){if(IsInWorld())GetMap()->RemoveFromMap(static_cast<GameObject*>(this),false);ResetMap();}
};
static uint64 DoorCounter(Scenario* scenario,uint32 id){auto const* rows=scenario->GetAchievementMgr().GetCriteriaProgressMap();auto found=rows->find(id);return found==rows->end()?0:found->second.Counter;}
static void PumpDoor(NativePlayer& player,uint32 milliseconds){for(uint32 elapsed=0;elapsed<milliseconds;elapsed+=10)player.Unit::Update(10);}
int main(int argc,char** argv){try{
 InitializeAltheaNative(argc,argv);sObjectMgr->LoadPageTexts();sObjectMgr->LoadGameObjectTemplate();LoadGameObjectModelList(sWorld->GetDataPath());HashMapHolder<GameObject>::SetSize(20000);
 auto scripts=WorldDatabase.Query("SELECT COUNT(*) FROM gameobject_scripts WHERE id=133953");Require(scripts&&!scripts->Fetch()[0].GetUInt32(),"Native source DB scripts must be loaded before testing");
 auto rules=WorldDatabase.Query("SELECT COUNT(*) FROM achievement_criteria_data WHERE criteria_id IN(30469,30480)");Require(rules&&!rules->Fetch()[0].GetUInt32(),"Door criteria have extra World admission rules to load");
 sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new DoorScript;sScriptMgr->SwapScriptContext(true);
 uint32 number=0;
 uint32 modes=4;
#ifndef KINGSLAYERS_DOOR_BASELINE
 modes=24;
#endif
 for(uint8 race:{uint8(RACE_HUMAN),uint8(RACE_ORC)})for(uint32 mode=0;mode<modes;++mode){
  MapInstanced parent(1620,300);InstanceMap map(1620,300,9998000+number++,DIFFICULTY_N_SCENARIO,&parent);
  auto scenario=sScenarioMgr->AddScenario(&map,1123);scenario->SetCurrentStep(mode==1?4:5);auto session=Session();
  {
   Position position{-8634.95f,419.141f,103.702f,2.23265f};
   NativePlayer player(session,map,101,{-8637.45f,419.141f,103.702f,0});player.SetByteValue(UNIT_FIELD_BYTES_0,0,race);player.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_ROGUE);player.setFactionForRace(race);
   player.SetQuestStatus(race==RACE_HUMAN?42504:42627,QUEST_STATUS_INCOMPLETE);player.SetQuestSlot(0,race==RACE_HUMAN?42504:42627);
   DoorObject door;Require(door.Create(2200+number,251053,&map,1,position,G3D::Quat(0,0,std::sin(position.GetOrientation()/2),std::cos(position.GetOrientation()/2)),255,GO_STATE_READY),"Native original door create failed");
   door.*Access(DoorGuidTag{})=133953;Require(map.AddToMap(static_cast<GameObject*>(&door)),"Native door admission failed");door.Update(5000);
   Require(dynamic_cast<DoorScript::go_kingslayers_doorAI*>(door.AI())&&door.getLootState()==GO_READY&&door.GetGoState()==GO_STATE_READY,"Native original factory/ready state differs");
   Require(door.GetGOInfo()->displayId==4413&&door.GetGOInfo()->GetLockId()==0&&door.GetGOInfo()->GetAutoCloseTime()==0,"Native door authored metadata differs");
   auto quest=race==RACE_HUMAN?42504u:42627u;
   std::unique_ptr<Actor> garona;
   if(mode==18){garona=std::make_unique<Actor>(&map,108214,13000+number,Position{-8643.02f,421.559f,102.051f,0});Require(map.AddToMap(static_cast<Creature*>(garona.get())),"Native Garona admission failed");garona->AddAura(132653,garona.get());Require(garona->HasAura(132653),"Native Garona initial stealth absent");}
   if(mode==6)player.SetQuestStatus(quest,QUEST_STATUS_NONE);
   if(mode==7)player.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_WARRIOR);
   if(mode==8)player.Unit::setDeathState(CORPSE);
   if(mode==9)player.Relocate(position.GetPositionX()+40,position.GetPositionY(),position.GetPositionZ(),0);
   if(mode==10)player.SetPhaseMask(2,false);
   if(mode==11)player.SetGameMaster(true);
   if(mode==12)door.SetFlag(GAMEOBJECT_FIELD_FLAGS,GO_FLAG_NOT_SELECTABLE);
   if(mode==13)door.SetFlag(GAMEOBJECT_FIELD_FLAGS,GO_FLAG_LOCKED);
   if(mode==14)door.*Access(DoorGuidTag{})=133954;
   if(mode==15)scenario->SetCurrentStep(3);
   if(mode==16){player.UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,51654);scenario->SetCurrentStep(6);}
   if(mode==17)door.UseDoorOrButton(); // Uncredited open state: explicit-use recovery only.
   if(mode==21){player.SetQuestStatus(quest,QUEST_STATUS_NONE);door.Use(&player);Require(door.GetGoState()==GO_STATE_READY&&!DoorCounter(scenario,48597),"Abandoned door attempt opened");player.SetQuestStatus(quest,QUEST_STATUS_INCOMPLETE);}
   if(mode==22){CreditActor althea(&map,108218,14000+number,player.GetPosition());Require(map.AddToMap(static_cast<Creature*>(&althea)),"Native Althea admission failed");althea.SetLootRecipient(&player);althea.LowerPlayerDamageReq(uint32(althea.GetMaxHealth()));player.Kill(&althea,false);Require(Counter(scenario)==1&&scenario->GetCurrentStep()==5,"Native victory must still wait for this door");CleanStories(map);}
   auto peerSession=mode==20?Session():nullptr;std::unique_ptr<NativePlayer> peer;std::unique_ptr<CreditParty> party;
   if(peerSession){peer=std::make_unique<NativePlayer>(peerSession,map,102,player.GetPosition());peer->SetByteValue(UNIT_FIELD_BYTES_0,0,race);peer->SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_ROGUE);peer->setFactionForRace(race);peer->SetQuestStatus(quest,QUEST_STATUS_INCOMPLETE);party=std::make_unique<CreditParty>(&player,peer.get());}
   std::cout<<"TRACE door race="<<unsigned(race)<<" mode="<<mode<<" model="<<bool(door.m_model)<<" los="<<player.IsWithinLOSInMap(&door)<<std::endl;
   if(mode==2)door.AI()->GossipHello(&player,true);
   else if(mode==3){WorldPackets::GameObject::GameObjReportUse packet{WorldPacket(CMSG_GAME_OBJ_REPORT_USE)};packet.Guid=door.GetGUID();session->HandleGameobjectReportUse(packet);}
   else if(mode==4||mode==5){
    // Controlled effect endpoint: the authored lock0 door uses direct Use.
    // Ordinary spell preparation rejects it; do not claim a full Opening cast.
    auto info=const_cast<GameObjectTemplate*>(door.GetGOInfo());uint32 lock=info->door.open;if(mode==5)info->door.open=UINT32_MAX;
    TriggerCastData data;Spell spell(&player,sSpellMgr->GetSpellInfo(3365),data);SpellCastTargets targets;targets.SetCaster(&player);targets.SetGOTarget(&door);spell.InitExplicitTargets(targets);
    spell.HandleEffects(nullptr,nullptr,&door,EFFECT_0,SPELL_EFFECT_HANDLE_HIT_TARGET);info->door.open=lock;
   }
   else if(mode==23){Require(player.CastSpell(&door,3365,true)!=SPELL_CAST_OK,"Lock0 door unexpectedly admits ordinary Opening preparation");PumpDoor(player,10000);}
   else {WorldPackets::GameObject::GameObjectUse packet{WorldPacket(CMSG_GAME_OBJ_USE)};packet.Guid=door.GetGUID();session->HandleGameObjectUse(packet);}
#ifdef KINGSLAYERS_DOOR_BASELINE
   if(mode<2){Require(door.getLootState()==GO_ACTIVATED&&door.GetGoState()==GO_STATE_ACTIVE&&DoorCounter(scenario,48597)==0,"Old direct-use missing credit no longer reproduces");std::cout<<"COUNTERFACTUAL real native direct door Use opens without51654\n";}
   if(mode==2){Require(door.GetGoState()==GO_STATE_READY&&DoorCounter(scenario,48597)==1,"Old true hook without activation changed");std::cout<<"COUNTERFACTUAL original true gossip grants51654 while native door is still closed\n";}
   if(mode==3)Require(door.GetGoState()==GO_STATE_READY&&!DoorCounter(scenario,48597),"Original report control changed");
#else
   bool refused=mode==3||(mode>=5&&mode<=13)||mode==15||mode==23;
   if(refused)Require(door.GetGoState()==GO_STATE_READY&&!DoorCounter(scenario,48597),"Refused/report/failed opening changed native door or credit");
   else if(mode==14)Require(door.GetGoState()==GO_STATE_ACTIVE&&!DoorCounter(scenario,48597),"Other native source did not retain ordinary opening without quest credit");
   else {Require(door.getLootState()==GO_ACTIVATED&&door.GetGoState()==GO_STATE_ACTIVE&&DoorCounter(scenario,48597)==1,"Repaired native door opening/credit missing");Require(scenario->GetCurrentStep()==(mode==16||mode==22?6u:5u),"Door bypassed independent Althea victory");Require(door.m_model&&!door.m_model->isCollisionEnabled(),"Opened door still has blocking model");
    if(mode==18){Require(!garona->HasAura(132653)&&garona->GetMotionMaster()->GetCurrentMovementGeneratorType()==POINT_MOTION_TYPE,"Original Garona move/stealth cleanup absent");garona->AddAura(132653,garona.get());door.Use(&player);Require(garona->HasAura(132653)&&DoorCounter(scenario,48597)==1,"Repeated use replayed original Garona transition");}
    if(mode==19){door.ResetDoorOrButton();door.m_isUpdate=false;door.Update(300000);Require(door.getLootState()==GO_READY&&door.GetGoState()==GO_STATE_READY,"Native door reset not ready for reentry");
     // Diff alone does not advance absolute GameTime; control the respawn
     // endpoint before recreating the AI, without changing production timers.
     door.SetRespawnTime(0);Require(door.AIM_Initialize(),"Native reentry AI recreation failed");door.Use(&player);Require(DoorCounter(scenario,48597)==1&&door.GetGoState()==GO_STATE_ACTIVE,"Reentry replayed credit or failed normal opening");}
    if(peer){door.Use(peer.get());Require(DoorCounter(scenario,48597)==1,"Group repeated shared door event");}
   }
#endif
   std::cout<<"TRACE result door-state="<<door.GetGoState()<<" loot="<<door.getLootState()<<" leaf="<<DoorCounter(scenario,48597)<<" collision="<<(door.m_model?door.m_model->isCollisionEnabled():false)<<std::endl;
   party.reset();peer.reset();if(peerSession){peerSession->SetPlayer(nullptr);delete peerSession;}
  }
  session->SetPlayer(nullptr);delete session;sScenarioMgr->RemoveScenario(map.GetInstanceId());
 }
 std::cout<<"PASS "<<number<<" native registered door/opcode/report/quest/class/death/range/phase/GM/flags/source/stage controls, failure/retry/reentry/group, original Garona movement and real mesh; independent native Althea victory preserved. Earlier quest state and Opening effect/hook endpoints controlled; ordinary lock0 Opening preparation rejected; no authenticated client/full traversal claim\n";std::cout.flush();CloseAltheaNative();
}catch(std::exception const& error){std::cerr<<error.what()<<'\n';return 1;}}
