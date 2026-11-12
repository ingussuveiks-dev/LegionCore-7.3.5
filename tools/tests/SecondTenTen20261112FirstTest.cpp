#include "SecondTenTen20261112Fixture.h"
#include "Selected.inc"
static void Fill(PetBattle&b,Player&p){players[0]=&p;b.Teams[0]->PlayerGuid=p.guid;for(int t=0;t<2;++t){auto pet=std::make_shared<BattlePetInstance>();pet->ID=t*3;b.Pets[t*3]=pet;b.Teams[t]->TeamPets[0]=pet;b.Teams[t]->TeamPetCount=1;b.FightedPets[t*3]=true;}p.pets[0]=b.Pets[0];}
int main(int argc,char**argv){try{Require(argc==2,"case");int test=std::stoi(argv[1]);Player p;PetBattle b;Fill(b,p);auto pet=b.Pets[0];
switch(test){
case 0:b.Finish(1,true,false);Require(pet->Health==90,"Ordinary forfeit applies health penalty");break;
case 1:{Creature c;wild=&c;b.BattleType=PETBATTLE_TYPE_PVE;b.PveBattleType=PVE_PETBATTLE_WILD;b.Pets[3]->OriginalCreature=100;b.Finish(0,false,false);Require(c.released==1&&wilds.leaves==1&&!c._petBattleId,"Actual wild GUID is released exactly once");break;}
case 2:b.Finish(0,false,false);{auto xp=pet->XP;auto finishes=scripts.finishes;b.Finish(0,false,false);Require(pet->XP==xp&&scripts.finishes==finishes,"Repeated finish cannot replay XP or callbacks");}break;
case 3:b.Teams[0]->locked=true;b.Finish(0,false,false);Require(pet->Level==2,"A temporary swap lock cannot cancel earned experience");break;
case 4:pet->earned=1000;b.Finish(0,false,false);Require(pet->Level==5&&pet->XP==0&&p.levels==4,"All crossed native thresholds trigger their level side effects");{PetBattle cap;Fill(cap,p);auto c=cap.Pets[0];c->Level=24;c->XP=2399;c->earned=1000;cap.Finish(0,false,false);Require(c->Level==25&&c->XP==0,"Maximum level discards remainder and stops leveling");}break;
case 5:{systemFixture.b=&b;WorldSession s;s._player=&p;WorldPackets::BattlePet::PetBattleInput in;s.HandlePetBattleInput(in);Require(systemFixture.forfeits==1,"MoveType zero must reach its leave switch arm");break;}
case 6:{LFBTicket a,b;Require(!QueueBefore(&a,&a)&&!QueueBefore(&a,&b)&&!QueueBefore(&b,&a),"Equal-weight ordering must be irreflexive");break;}
case 7:{LFBTicket a,b;NewConsent(&a,&b);Require(a.ProposalAnswer==LFBAnswer::LFB_ANSWER_PENDING&&b.ProposalAnswer==LFBAnswer::LFB_ANSWER_PENDING,"A fresh proposal requires fresh answers from both sides");break;}
case 8:{PetBattleSystem requests;requests.CreateRequest(1);requests.CreateRequest(1);Require(PetBattleRequest::alive==1,"Replacing a pending request must release its earlier allocation");break;}
case 9:{Player other;other.guid=2;other._petBattleId.Clear();p._petBattleId.Clear();players[1]=&other;p.range=false;PetBattleRequest request;request.OpponentGuid=other.guid;auto result=systemFixture.CanPlayerEnterInPetBattle(&p,&request);Require(result==PETBATTLE_REQUEST_TARGET_OUT_OF_RANGE,"Admission must measure the actual two participants");p.range=true;p.los=false;Require(systemFixture.CanPlayerEnterInPetBattle(&p,&request)==PETBATTLE_REQUEST_NOT_HERE_OBSTRUCTED,"Admission must test actual participant LOS");p.los=true;Require(systemFixture.CanPlayerEnterInPetBattle(&p,&request)==PETBATTLE_REQUEST_OK,"Eligible nearby visible participants remain admitted");break;}
default:throw std::runtime_error("unknown case");}
std::cout<<"PASS complete production methods or explicitly identified queue projections; actual native linkage/client traversal separate\n";return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
