// Readonly databases; real Finish and owned journal objects only in memory.
#define ALTHEA_CREDIT_NATIVE_SUPPORT_ONLY
#include "AltheaCreditNative.cpp"
#include "PetBattle.h"
#include "PetBattleAbilityEffect.h"
static void HealthFill(PetBattle& b,Player& player,uint32 team){
 b.BattleType=PETBATTLE_TYPE_PVE;b.PveBattleType=PVE_PETBATTLE_TRAINER;b.BattleStatus=PETBATTLE_STATUS_RUNNING;b.Turn=1;
 uint32 species=0;for(auto s:sBattlePetSpeciesStore)if(s->PetTypeEnum==BATTLEPET_PETTYPE_HUMANOID){species=s->ID;break;}Require(species,"Native ordinary family missing");
 for(uint32 t=0;t<2;++t){b.Teams[t]->ActivePetID=t*3;b.Teams[t]->TeamPetCount=3;
  for(uint32 slot=0;slot<3;++slot){auto p=std::make_shared<BattlePetInstance>();static_cast<BattlePet&>(*p)=BattlePet{};p->ID=t*3+slot;p->Slot=slot;p->TeamID=t;p->PetBattleInstance=&b;p->Level=25;p->Species=species;p->Health=p->InfoMaxHealth=100;p->Quality=3;p->Breed=3;p->OldLevel=25;p->JournalID=ObjectGuid::Create<HighGuid::BattlePet>(88000+p->ID);p->OriginalBattlePet=std::make_shared<BattlePet>(static_cast<BattlePet const&>(*p));b.Pets[p->ID]=p;b.Teams[t]->TeamPets[slot]=p;}}
 b.Teams[team]->PlayerGuid=b.Teams[team]->OwnerGuid=player.GetGUID();
}

static uint32 HealthCases=0;
static void Sacrifice(PetBattle&b,uint32 id){Require(b.Cast(id,282,1,PETBATTLE_ABILITY_TURN0_PROC_ON_NONE,PETBATTLE_CAST_TRIGGER_ALL)==PETBATTLE_CAST_OK,"Actual native sacrifice cast failed");Require(!b.Pets[id]->IsAlive()&&b.Pets[id]->Health==100,"Native terminal sacrifice must retain its reversible in-battle health before Finish");}
static PetBattleAbilityEffect Hit(PetBattle&b,PetBattleAbilityTurn&turn,uint32 id){PetBattleAbilityEffect e{};e.PetBattleInstance=&b;e.AbilityTurn=&turn;e.EffectInfo=sBattlePetAbilityEffectStore.LookupEntry(3482);Require(e.EffectInfo&&e.EffectInfo->BattlePetEffectPropertiesID==24,"Native hit row missing");e.Caster=id<3?3:0;e.Target=id;e.AbilityID=1927;e.IsTriggered=true;return e;}
static void Racial(PetBattle&b,uint32 id,uint32 family){for(auto s:sBattlePetSpeciesStore)if(s->PetTypeEnum==family){b.Pets[id]->Species=s->ID;Require(b.Cast(id,291,1,PETBATTLE_ABILITY_TURN0_PROC_ON_NONE,PETBATTLE_CAST_TRIGGER_ALL)==PETBATTLE_CAST_OK,"Actual racial initialization failed");return;}throw std::runtime_error("Native family missing");}
static void Saved(PetBattle&b,uint32 id,int32 hp,bool alive){auto p=b.Pets[id];Require(b.BattleStatus==PETBATTLE_STATUS_FINISHED&&p->Health==hp&&p->IsAlive()==alive,"Finish must retain the actual final life state and corresponding health");Require(p->OriginalBattlePet&&p->OriginalBattlePet->needSave&&p->OriginalBattlePet->Health==hp,"Actual owned journal update must save final-dead health as zero and preserve living health");auto level=p->Level,xp=p->XP;b.Finish(p->TeamID,false,false);Require(p->Health==hp&&p->OriginalBattlePet->Health==hp&&p->Level==level&&p->XP==xp,"Repeated Finish cannot revive,penalize or re-award the saved pet");}
static void Cases(Player&player,std::string const&choice){
 Require(choice.empty()||choice=="sacrifice"||choice=="forfeit"||choice=="undeadexpiry","Unknown final-health case");
 for(uint32 team=0;team<2;++team)for(uint32 slot=0;slot<3;++slot){uint32 id=team*3+slot;
  if(choice.empty()||choice=="sacrifice"||choice=="forfeit")for(uint32 mode=0;mode<4;++mode){if(choice=="sacrifice"&&mode!=0)continue;if(choice=="forfeit"&&mode!=2)continue;PetBattle b;HealthFill(b,player,team);Sacrifice(b,id);bool aborted=mode>=2;uint32 winner=mode==1?team:!team;b.Finish(winner,aborted,mode==3);Require(b.Pets[id]->Health==0&&b.Pets[id]->OriginalBattlePet->Health==0,"A final-dead owned pet must not retain positive journal health after actual Finish");Saved(b,id,0,false);for(uint32 s=0;s<3;++s)if(s!=slot)Require(b.Pets[team*3+s]->Health==(mode==2?90:100),"Final-dead normalization cannot change a living reserve or ordinary forfeit penalty");
   auto journal=std::make_shared<BattlePetInstance>();static_cast<BattlePet&>(*journal)=*b.Pets[id]->OriginalBattlePet;auto clone=BattlePetInstance::CloneForBattle(journal);Require(clone->Health==0,"Actual journal clone must retain saved zero health");PetBattle retry;retry.AddPet(team,clone);Require(retry.Pets[team*3]&&!retry.Pets[team*3]->IsAlive()&&retry.Pets[team*3]->Health==0,"Native re-entry must keep a final-dead journal pet unavailable");++HealthCases;}
  if(!choice.empty()&&choice!="undeadexpiry")continue;
  if(choice.empty()){
   for(uint32 mode=0;mode<4;++mode){PetBattle b;HealthFill(b,player,team);b.Pets[id]->Health=80;b.Finish(mode==1?team:!team,mode>=2,mode==3);Saved(b,id,mode==2?72:80,true);++HealthCases;}
   {PetBattle b;HealthFill(b,player,team);PetBattleAbilityTurn turn{};auto e=Hit(b,turn,id);e.Damage(id,1000);Require(!b.Pets[id]->IsAlive()&&b.Pets[id]->Health==0,"Actual native damage death missing");b.Finish(!team,false,false);Saved(b,id,0,false);++HealthCases;}
   {PetBattle b;HealthFill(b,player,team);b.Pets[id]->Health=-10;b.Finish(!team,false,false);Saved(b,id,0,true);++HealthCases;}
   {PetBattle b;HealthFill(b,player,team);Racial(b,id,BATTLEPET_PETTYPE_MECHANICAL);PetBattleAbilityTurn turn{};auto e=Hit(b,turn,id);e.Kill(id);Require(b.Pets[id]->IsAlive()&&b.Pets[id]->Health==20,"Actual Mechanical revival missing");b.Finish(!team,false,false);Saved(b,id,20,true);++HealthCases;}
   {PetBattle b;HealthFill(b,player,team);Racial(b,id,BATTLEPET_PETTYPE_UNDEAD);PetBattleAbilityTurn turn{};auto e=Hit(b,turn,id);e.Kill(id);Require(b.Pets[id]->IsAlive()&&b.Pets[id]->Health==1,"Actual current Undead revival missing");b.Finish(!team,false,false);Saved(b,id,1,true);++HealthCases;}
   {PetBattle b;HealthFill(b,player,team);Require(b.Cast(id,1906,1,PETBATTLE_ABILITY_TURN0_PROC_ON_NONE,PETBATTLE_CAST_TRIGGER_ALL)==PETBATTLE_CAST_OK,"Actual Cheat Death root missing");PetBattleAbilityTurn turn{};auto e=Hit(b,turn,id);e.Damage(id,1000);Require(!b.Pets[id]->IsAlive(),"Actual delayed Cheat Death setup missing");Require(b.Cast(id,1907,0,PETBATTLE_ABILITY_TURN0_PROC_ON_ROUND_END,PETBATTLE_CAST_TRIGGER_ALL,id)==PETBATTLE_CAST_OK&&b.Pets[id]->IsAlive()&&b.Pets[id]->Health==50,"Actual owned Cheat Death revival missing");b.Finish(!team,false,false);Saved(b,id,50,true);++HealthCases;}
   {PetBattle b;HealthFill(b,player,team);Sacrifice(b,id);b.Pets[id]->OriginalBattlePet.reset();b.Finish(!team,false,false);Require(!b.Pets[id]->IsAlive()&&!b.Pets[id]->Health&&b.BattleStatus==PETBATTLE_STATUS_FINISHED,"A missing original journal cannot prevent final-dead owned health cleanup");++HealthCases;}
   {PetBattle b;HealthFill(b,player,team);Sacrifice(b,id);b.Teams[team]->PlayerGuid=ObjectGuid::Create<HighGuid::Player>(999998);b.Finish(!team,false,false);Require(!b.Pets[id]->IsAlive()&&b.Pets[id]->Health==100&&!b.Pets[id]->OriginalBattlePet->needSave,"Missing actual player keeps the existing offline skip;no journal save is fabricated");++HealthCases;}
   {PetBattle b;HealthFill(b,player,team);uint32 enemy=(!team)*3+slot;Sacrifice(b,enemy);b.Finish(!team,false,false);Require(!b.Pets[enemy]->IsAlive()&&b.Pets[enemy]->Health==100&&!b.Pets[enemy]->OriginalBattlePet->needSave,"NPC/unowned death metadata must not be globally rewritten or persisted by owned journal repair");++HealthCases;}
  }
  {PetBattle b;HealthFill(b,player,team);Racial(b,id,BATTLEPET_PETTYPE_UNDEAD);PetBattleAbilityTurn turn{};auto e=Hit(b,turn,id);e.Kill(id);PetBattleAura*a=nullptr;for(auto v:b.PetAuras)if(v->AbilityID==242&&!v->Expired&&v->TargetPetID==id)a=v;Require(a,"Actual Undead aura missing");a->Process(&b);a->Process(&b);Require(a->Expired&&!b.Pets[id]->IsAlive()&&b.Pets[id]->Health==0,"Actual expired Undead must retain its native already-zero final health");b.Finish(!team,false,false);Require(b.Pets[id]->Health==0&&b.Pets[id]->OriginalBattlePet->Health==0,"A final-dead owned pet must not retain positive journal health after actual Finish");Saved(b,id,0,false);++HealthCases;}
 }
}
int main(int argc,char**argv){try{
 InitializeAltheaNative(argc,argv);
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1600,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1600,{}}});
 MapInstanced parent(1600,300);InstanceMap map(1600,300,9999831,DIFFICULTY_N_SCENARIO,&parent);auto session=Session();std::string choice=std::getenv("LEGION_PET_FINAL_HEALTH_CASE")?std::getenv("LEGION_PET_FINAL_HEALTH_CASE"):"";
 {NativePlayer player(session,map,28302,{52,803,805,0});player.setFactionForRace(RACE_HUMAN);Cases(player,choice);}
 session->SetPlayer(nullptr);delete session;CloseAltheaNative();Require(!choice.empty()||HealthCases==102,"Native final-health case inventory incomplete");std::cout<<"PASS actual native final-health cases:"<<(choice.empty()?"all102":choice)<<" executed="<<HealthCases<<"; actual sacrifice,racials,delayed revival,Finish,journal,re-entry; no persistent account or character writes\n";return 0;
 }catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
