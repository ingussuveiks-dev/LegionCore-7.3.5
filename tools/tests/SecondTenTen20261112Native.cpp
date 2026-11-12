// Actual linked game objects, native26972 experience tables, memory-only players.
#define ALTHEA_INTRO_NATIVE_SUPPORT_ONLY
#include "AltheaIntroNative.cpp"
#include "PetBattle.h"
#include "PetBattleSystem.h"
#include "GameTables.h"
#include "BattlePetPackets.h"
struct SecondQueueTag{using Type=std::map<ObjectGuid,LFBTicket*> PetBattleSystem::*;friend Type Access(SecondQueueTag);};template struct FixtureAccess<SecondQueueTag,&PetBattleSystem::_LFBRequests>;
static void FillSecond(PetBattle&b,Player*player=nullptr){
 b.BattleType=PETBATTLE_TYPE_PVE;b.PveBattleType=PVE_PETBATTLE_TRAINER;b.BattleStatus=PETBATTLE_STATUS_RUNNING;b.Turn=1;b.RoundTurn=0;b.RoundFirstTeamCasting=0;
 for(uint32 t=0;t<2;++t){b.Teams[t]->ActivePetID=t*3;b.Teams[t]->TeamPetCount=3;for(uint32 j=0;j<3;++j){auto pet=std::make_shared<BattlePetInstance>();pet->ID=t*3+j;pet->TeamID=t;pet->PetBattleInstance=&b;pet->Health=100;pet->InfoMaxHealth=100;pet->Level=1;pet->Species=0;pet->XP=0;b.Pets[pet->ID]=pet;b.Teams[t]->TeamPets[j]=pet;b.FightedPets[pet->ID]=true;}}
 if(player){b.Teams[0]->PlayerGuid=player->GetGUID();b.Teams[0]->OwnerGuid=player->GetGUID();}
}
#include "SecondTenTen20261112SecondNative.h"
#include "SecondTenTen20261112ThirdNative.h"
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1600,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1600,{}}});
 MapInstanced parent(1600,300);InstanceMap map(1600,300,9999810,DIFFICULTY_N_SCENARIO,&parent);auto session=Session();
 {
 NativePlayer player(session,map,28001,{52,803,805,0});player.setFactionForRace(RACE_HUMAN);
 {auto b=sPetBattleSystem->CreateBattle();FillSecond(*b,&player);b->Turn=1;player._petBattleId=b->ID;WorldPackets::BattlePet::PetBattleInput input{WorldPacket(CMSG_PET_BATTLE_INPUT)};input.Round=0;input.MoveType=0;session->HandlePetBattleInput(input);Require(b->BattleStatus==PETBATTLE_STATUS_FINISHED&&b->Pets[0]->Health==90&&!player._petBattleId,"Actual WorldSession zero-valued input reaches forfeit and cleanup");sPetBattleSystem->RemoveBattle(b->ID);}
 for(bool ignore:{false,true}){PetBattle b;FillSecond(b,&player);b.Finish(1,true,ignore);Require(b.Pets[0]->Health==(ignore?100:90),"Actual native ordinary/ignored forfeit health contract");}
 {PetBattle b;FillSecond(b,&player);b.Pets[1]->States[BATTLEPET_STATE_Is_Dead]=1;b.Pets[2]->States[BATTLEPET_STATE_Is_Dead]=1;b.Teams[0]->ActiveAbilityId=55;b.Teams[0]->activeAbilityTurn=1;b.Teams[0]->activeAbilityTurnMax=3;Require(b.Teams[0]->GetAvailablesPets().empty(),"Native pending multi-turn cast blocks swaps");Require(b.Pets[0]->GetMaxXPForCurrentLevel()==50,"Matching native26972 level1 XP threshold");b.Finish(0,false,false);Require(b.Pets[0]->Level>=2,"Actual native XP survives a swap lock");auto level=b.Pets[0]->Level,xp=b.Pets[0]->XP;b.Finish(0,false,false);Require(b.Pets[0]->Level==level&&b.Pets[0]->XP==xp,"Actual linked repeated finish is inert");}
 uint32 bonusSpell=0;for(uint32 id=1;id<sSpellMgr->GetSpellInfoStoreSize();++id){auto info=sSpellMgr->GetSpellInfo(id);if(info&&info->HasAura(SPELL_AURA_MOD_BATTLE_PET_XP_GAIN)){bonusSpell=id;break;}}
 Require(bonusSpell,"Matching26972 XP bonus aura missing");auto bonus=player.AddAura(bonusSpell,&player);Require(bonus,"Actual native XP bonus application");bool changed=false;for(auto effect:bonus->GetAuraEffects())if(effect&&effect->GetAuraType()==SPELL_AURA_MOD_BATTLE_PET_XP_GAIN){std::cout<<"NATIVE XP bonus spell="<<bonusSpell<<" original amount="<<effect->GetAmount()<<"; fixture-controlled bonus=200\n";effect->ChangeAmount(200);changed=true;}Require(changed,"Native XP aura effect missing");
 {PetBattle b;FillSecond(b,&player);b.Pets[1]->States[BATTLEPET_STATE_Is_Dead]=1;b.Pets[2]->States[BATTLEPET_STATE_Is_Dead]=1;b.Finish(0,false,false);Require(b.Pets[0]->Level==4&&b.Pets[0]->XP==95,"Native full Finish crosses50/110/120 XP thresholds and retains95");}
 player.RemoveAura(bonusSpell);
 {auto secondSession=Session();{
 NativePlayer opponent(secondSession,map,28004,{82,803,805,0});opponent.setFactionForRace(RACE_HUMAN);PetBattleRequest request;request.OpponentGuid=opponent.GetGUID();Require(sPetBattleSystem->CanPlayerEnterInPetBattle(&player,&request)==PETBATTLE_REQUEST_TARGET_OUT_OF_RANGE,"Actual native opponent admission measures real separate players");
 sPetBattleSystem->JoinQueue(&player);sPetBattleSystem->JoinQueue(&opponent);auto&queues=sPetBattleSystem->*Access(SecondQueueTag{});auto left=queues.at(player.GetGUID()),right=queues.at(opponent.GetGUID());left->ProposalAnswer=right->ProposalAnswer=LFB_ANSWER_AGREE;sPetBattleSystem->Update(1000);Require(left->State==LFB_STATE_PROPOSAL&&right->State==LFB_STATE_PROPOSAL&&left->ProposalAnswer==LFB_ANSWER_PENDING&&right->ProposalAnswer==LFB_ANSWER_PENDING,"Actual full queue Update discards previous consent at proposal creation");sPetBattleSystem->LeaveQueue(&player);sPetBattleSystem->LeaveQueue(&opponent);
 }secondSession->SetPlayer(nullptr);delete secondSession;}
 {Actor wild(&map,299,28002,{53,803,805,0});Require(map.AddToMap(static_cast<Creature*>(&wild)),"Native wild cleanup actor admission");PetBattle b;FillSecond(b);b.PveBattleType=PVE_PETBATTLE_WILD;b.Pets[3]->OriginalCreature=wild.GetGUID();wild._petBattleId=ObjectGuid::Create<HighGuid::PetBattle>(999999);wild.SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_PACIFIED|UNIT_FLAG_IMMUNE_TO_PC);wild.SetControlled(true,UNIT_STATE_ROOT);b.Finish(0,false,false);Require(!wild._petBattleId&&!wild.HasFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_PACIFIED|UNIT_FLAG_IMMUNE_TO_PC)&&!wild.HasUnitState(UNIT_STATE_ROOT),"Actual native wild GUID flags/root/battle identity are released");}
 {auto guid=ObjectGuid::Create<HighGuid::Player>(28003);auto first=sPetBattleSystem->CreateRequest(guid);first->OpponentGuid=player.GetGUID();sPetBattleSystem->CreateRequest(guid);Require(!sPetBattleSystem->GetRequest(guid)->OpponentGuid,"Actual request replacement owns a fresh cleared request");sPetBattleSystem->RemoveRequest(guid);Require(!sPetBattleSystem->GetRequest(guid),"Native request removal leaves no pending entry");}
 RunSecondPetNative(map,player,session);
 RunThirdPetNative(map,player,session);
 }
 session->SetPlayer(nullptr);delete session;
 CloseAltheaNative();std::cout<<"PASS actual linked Finish ordinary/ignored forfeits,swap-independent XP,multiple native thresholds,repeated finish,wild actor cleanup,and owning request replacement;queue/source ASAN and actual client checks separate\n";return 0;
 }catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
