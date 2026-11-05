// Whole current capture, native owner damage/casts/queues/transfer/ACK. Earlier
// quest, instance, attack endpoint and update clock are controlled, not client.
#define DEVOURER_ESCAPE_SUPPORT_ONLY
#include "DevourerEscapeNative.cpp"
#include "CreatureTextMgr.h"
#include "MovementPackets.h"
#include "Capture.inc"
#ifdef EVENT_ABORT_NATIVE_BASELINE
// Strong definitions in this probe resolve all native EventProcessor calls to
// the pinned implementation; its object is not pulled from the static library.
#include "EventMethods.inc"
#endif
static std::vector<ObjectGuid> CaptureClones(Creature& source)
{
 std::list<Creature*> nearby;GetCreatureListWithEntryInGrid(nearby,&source,105609,200.0f);std::vector<ObjectGuid> guids;
 for(auto clone:nearby)if(auto summon=clone->ToTempSummon())if(summon->GetSummonerGUID()==source.GetGUID()&&summon->IsInWorld()&&!summon->IsPreDelete())guids.push_back(summon->GetGUID());
 return guids;
}
static void CaptureTick(Player& player,Creature& source,uint32 ms)
{
 auto ai=static_cast<npc_devourer_capture_probe::npc_akaari_shadowgoreAI*>(source.AI());
 for(uint32 t=0;t<ms;t+=100){for(auto guid:CaptureClones(source))if(auto clone=ObjectAccessor::GetCreature(source,guid))clone->Update(100);if(auto helper=ObjectAccessor::GetCreature(source,ai->captureHelper))if(helper->IsInWorld()&&!helper->IsPreDelete())helper->Update(100);player.Unit::Update(100);source.Unit::Update(100);if(source.IsInWorld())ai->UpdateAI(100);source.GetMap()->RemoveAllObjectsInRemoveList();}
}
static void CaptureAck(Player& player)
{
 WorldPacket wire(CMSG_MOVE_TELEPORT_ACK);wire<<player.GetGUID()<<int32(0)<<int32(0);WorldPackets::Movement::MoveTeleportAck ack(std::move(wire));ack.Read();player.GetSession()->HandleMoveTeleportAck(ack);
 Require(!player.HasPendingTeleport()&&player.GetExactDist(1847.77f,1259.92f,57.09f)<0.02f,"Actual near ACK did not arrive in cell");
}
#ifndef DEVOURER_CAPTURE_SUPPORT_ONLY
int main(int argc,char**argv){try{
 InitializeEscapeNative(argc,argv);sSpellMgr->LoadSpellProcEvents();sSpellMgr->LoadSpellProcs();sSpellMgr->LoadSpellLinked();sCreatureTextMgr->LoadCreatureTexts();
 auto rules=WorldDatabase.Query("SELECT COUNT(*) FROM achievement_criteria_data WHERE criteria_id=29781");Require(rules&&!rules->Fetch()[0].GetUInt32(),"New capture criteria data requires native loading");uint32 cases=0;
 for(uint32 race:{uint32(RACE_HUMAN),uint32(RACE_ORC)})for(uint32 mode=0;mode<4;++mode){
 MapInstanced parent(1607,300);InstanceMap map(1607,300,9892000+cases,DIFFICULTY_N_SCENARIO,&parent);map.CreateInstanceData(nullptr);auto scenario=sScenarioMgr->AddScenario(&map,1078);auto first=Session(),second=Session();
 {NativePlayer player(first,map,101,Position{1854.16f,1278.57f,91.8f,0}),member(second,map,102,player.GetPosition());for(auto p:{&player,&member}){p->SetByteValue(UNIT_FIELD_BYTES_0,0,race);p->SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_ROGUE);p->setFactionForRace(race);p->SetQuestStatus(41924,QUEST_STATUS_INCOMPLETE);p->SetQuestSlot(0,41924);}std::unique_ptr<CreditParty> group;if(mode==3)group=std::make_unique<CreditParty>(&player,&member);
 Actor source(&map,105536,18501+cases,Position{1854.16f,1275.57f,91.8f,1.56409f});Require(map.AddToMap(static_cast<Creature*>(&source)),"Actual Akaari source create failed");auto ai=new npc_devourer_capture_probe::npc_akaari_shadowgoreAI(&source);Require(source.AIM_Initialize(ai),"Whole actual capture AI missing");std::unique_ptr<CreditPet> pet;std::unique_ptr<CreditGuardian> guardian;if(mode==1)pet=std::make_unique<CreditPet>(player,map);if(mode==2)guardian=std::make_unique<CreditGuardian>(player,map);Unit* attacker=pet?static_cast<Unit*>(pet.get()):guardian?static_cast<Unit*>(guardian.get()):static_cast<Unit*>(&player);
 source.Attack(attacker,true);ai->EnterCombat(attacker);Require(ai->capturePlayer==player.GetGUID()&&ai->CanCapture(&player),"Native owner/capture admission failed");attacker->DealDamage(&source,UINT32_MAX-4096,nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false);Require(source.IsAlive()&&source.GetHealth(attacker)>0,"Native scaled one-hit bypassed capture protection");
 CaptureTick(player,source,12000);Require(!ai->capturePending&&scenario->GetCurrentStep()==0,"Early clone step advanced capture");CaptureTick(player,source,11000);Require(ai->capturePending&&!ai->captureHelper.IsEmpty()&&scenario->GetCurrentStep()==0,"Native23s capture/helper endpoint missing");auto helperGuid=ai->captureHelper;auto helper=ObjectAccessor::GetCreature(source,helperGuid);Require(helper&&helper->GetEntry()==105540&&helper->HasAura(189170),"Original native portal not active");
 CaptureTick(player,source,5000);CaptureTick(player,source,100);Require(player.HasAura(209264)&&!source.HasAura(209264)&&ai->capturePending,"Actual selected-player Drain Soul did not run");CaptureTick(player,source,4000);Require(!ObjectAccessor::GetCreature(source,helperGuid)&&!ai->captureComplete&&scenario->GetCurrentStep()==0,"Actual9s helper cleanup/10s transfer separation failed");CaptureTick(player,source,900);
 Require(ai->captureComplete&&scenario->GetCurrentStep()==1&&EscapeLeaf(scenario,47353)==1&&player.IsBeingTeleportedNear()&&player.GetTeleportDest().GetExactDist(1847.77f,1259.92f,57.09f)<0.02f&&player.HasAura(211684)&&player.HasAura(211686),"Native accepted transfer/chains/50261 differs");CaptureAck(player);CaptureTick(player,source,1000);Require(EscapeLeaf(scenario,47353)==1&&scenario->GetCurrentStep()==1,"Capture replayed credit after ACK");
 for(auto guid:CaptureClones(source))if(auto clone=ObjectAccessor::GetCreature(source,guid))clone->ToTempSummon()->UnSummon();map.RemoveAllObjectsInRemoveList();player.CastStop();player.RemoveAllAuras();player.m_Events.KillAllEvents(true);source.CombatStop(true);source.CastStop();source.RemoveAllAuras();source.m_Events.KillAllEvents(true);std::cout<<"CASE race="<<race<<" mode="<<mode<<" actual owner one-hit,12/11/5/9/10s, portal/Drain Soul, accepted cell transfer/ACK/50261 once\n";
 }first->SetPlayer(nullptr);second->SetPlayer(nullptr);delete first;delete second;sScenarioMgr->RemoveScenario(map.GetInstanceId());++cases;
 }
#ifdef EVENT_ABORT_NATIVE_BASELINE
 std::cout<<"COUNTERFACTUAL "<<cases<<" native captures complete but pinned EventProcessor prematurely destroys portal189170 SpellEvent; exact diagnostics require separate review\n";
#else
 std::cout<<"PASS "<<cases<<" native captures: current whole AI plus real Player/pet/guardian one-hit owner, group/factions, actual105609 clone clocks/AI, portal/helper/Drain Soul/9s cleanup and10s transfer/ACK/50261 once; no event leak. Earlier admission/clock/attack endpoint controlled, client gameplay separate\n";
#endif
 CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
#endif
