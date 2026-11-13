#define PET_LOGOUT_SUPPORT_ONLY
#include "PetBattleLogoutNative.cpp"
#include "SocialMgr.h"
#include <thread>
struct LogoutSocialTag { using Type=PlayerSocial* Player::*;friend Type Access(LogoutSocialTag); };
template struct FixtureAccess<LogoutSocialTag,&Player::m_social>;
static void DrainWrites() {
 bool done=false;auto receipt=CharacterDatabase.AsyncQuery("SELECT 1").WithCallback([&](QueryResult r){Require(bool(r),"Scratch barrier missing");done=true;});
 for(uint32 i=0;!done&&i<10000;++i){receipt.InvokeIfReady();if(!done)std::this_thread::sleep_for(std::chrono::milliseconds(1));}Require(done,"Scratch character writes did not drain");
}
static uint32 savedCases;
static void SaveCase(Map& map,uint32 battleType,bool creation,bool detached,bool dead,bool webbed) {
 auto session=Session();auto player=new NativePlayer(session,map,29000+savedCases,{5478,470,150,0});player->SetName("NativeLogout");player->setFactionForRace(RACE_HUMAN);player->*Access(LogoutSocialTag{})=sSocialMgr->LoadFromDB({},player->GetGUID());
 auto b=sPetBattleSystem->CreateBattle();HealthFill(*b,*player,0);auto battleID=b->ID;auto playerID=player->GetGUID();player->_petBattleId=battleID;b->BattleType=PetBattleType(battleType);
 for(uint32 slot=0;slot<3;++slot){auto p=b->Pets[slot];p->JournalID=ObjectGuid::Create<HighGuid::BattlePet>(90000+savedCases*3+slot);p->OriginalBattlePet->JournalID=p->JournalID;p->OriginalBattlePet->AccountID=0;p->OriginalBattlePet->needSave=true;player->_battlePets.emplace(p->JournalID,p->OriginalBattlePet);}
 if(battleType==PETBATTLE_TYPE_PVP_MATCHMAKING)player->m_recallLoc=WorldLocation(1220,5479,471,151,1);
 player->SaveToDB(true);DrainWrites();
 auto journalID=b->Pets[0]->JournalID.GetCounter();auto journal=b->Pets[0]->OriginalBattlePet;
 if(dead)Sacrifice(*b,0);if(webbed)b->Pets[0]->States[BATTLEPET_STATE_Mechanic_IsWebbed]=1;
 if(battleType==PETBATTLE_TYPE_PVP_MATCHMAKING&&detached){player->GetTeleportDest().WorldRelocate(0,1,2,3,0);player->SetSemaphoreTeleportFar(true);}
 if(creation){b->BattleStatus=PETBATTLE_STATUS_CREATION;for(auto t:b->Teams)t->ActivePetID=0x7fffffff;}
 if(detached)map.RemovePlayerFromMap(player,false);
 session->SetCanLogout();session->Update(1);Require(!session->GetPlayer()&&!HashMapHolder<Player>::Find(playerID),"Actual LogoutPlayer must delete and unregister its Player");DrainWrites();
 int32 expected=creation?100:dead?0:100-b->GetForfeitHealthPenalityPct();
 auto row=CharacterDatabase.Query(Trinity::StringFormat("SELECT health FROM account_battlepet WHERE id=%llu",journalID).c_str());std::cout<<"SAVE_OBSERVE case="<<savedCases<<" type="<<battleType<<" creation="<<creation<<" detached="<<detached<<" dead="<<dead<<" webbed="<<webbed<<" expected="<<expected<<" saved="<<(row?row->Fetch()[0].GetInt32():-1)<<" journal="<<journal->Health<<" dirty="<<journal->needSave<<" battleStatus="<<b->BattleStatus<<'\n';Require(row&&row->Fetch()[0].GetInt32()==expected,"Actual LogoutPlayer/SaveToDB must persist final pet health in scratch account_battlepet");
 auto character=CharacterDatabase.Query(Trinity::StringFormat("SELECT online,map,position_x FROM characters WHERE guid=%llu",playerID.GetCounter()).c_str());Require(character&&!character->Fetch()[0].GetBool(),"Actual logout must persist offline character state");
 if(battleType==PETBATTLE_TYPE_PVP_MATCHMAKING)Require(character->Fetch()[1].GetUInt32()==1220&&character->Fetch()[2].GetFloat()==5479,"Actual logout must save matchmaking recall rather than arena pose");
 Require(b->BattleStatus==PETBATTLE_STATUS_FINISHED&&journal->Health==expected,"Actual logout must finish its battle and retain matching in-memory journal");
 b->Finish(1,true,false);Require(journal->Health==expected,"Repeated finish must not change durable result");
 delete session;sPetBattleSystem->RemoveBattle(battleID);++savedCases;
}
int main(int argc,char**argv){try{
 InitializeAltheaNative(argc,argv);dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{0,{}},{1220,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{0,{}},{1220,{}}});CharacterDatabase.Close();LoginDatabase.Close();
 auto character=sConfigMgr->GetStringDefault("CharacterDatabaseInfo","");auto auth=sConfigMgr->GetStringDefault("LoginDatabaseInfo","");auto cname=sConfigMgr->GetStringDefault("PetLogoutScratchCharacter","");auto aname=sConfigMgr->GetStringDefault("PetLogoutScratchAuth","");
 Require(cname.find("codex_pet_logout_")==0&&cname.size()==std::string("codex_pet_logout_").size()+16&&character.substr(character.find_last_of(';')+1)==cname,"Refusing non-scratch character schema");Require(aname.find("codex_pet_auth_")==0&&aname.size()==std::string("codex_pet_auth_").size()+16&&auth.substr(auth.find_last_of(';')+1)==aname,"Refusing non-scratch auth schema");
 CharacterDatabase.SetConnectionInfo(character,1,1);LoginDatabase.SetConnectionInfo(auth,1,1);Require(!CharacterDatabase.Open()&&CharacterDatabase.PrepareStatements()&&!LoginDatabase.Open()&&LoginDatabase.PrepareStatements(),"Scratch pools failed");
 auto initial=CharacterDatabase.Query("SELECT (SELECT COUNT(*) FROM characters),(SELECT COUNT(*) FROM account_battlepet)");Require(initial&&!initial->Fetch()[0].GetUInt32()&&!initial->Fetch()[1].GetUInt32(),"Scratch schema must be empty");
 Map map(1220,300,9999993,DIFFICULTY_NONE);
 for(uint32 type:{uint32(PETBATTLE_TYPE_PVE),uint32(PETBATTLE_TYPE_PVP_DUEL),uint32(PETBATTLE_TYPE_PVP_MATCHMAKING)})for(bool detached:{false,true}){SaveCase(map,type,false,detached,false,false);SaveCase(map,type,false,detached,true,false);SaveCase(map,type,false,detached,false,true);SaveCase(map,type,true,detached,false,false);}
 DrainWrites();CharacterDatabase.Close();LoginDatabase.Close();WorldDatabase.Close();HotfixDatabase.Close();MySQL::Library_End();Require(savedCases==24,"Scratch logout inventory incomplete");std::cout<<"PASS actual full-session logout/save cases=24;native Update/LogoutPlayer/SaveToDB/account_battlepet readback,detached and online,dead/webbed/creation,PvE/duel/matchmaking recall;schema-only scratch databases\n";return 0;
 }catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
