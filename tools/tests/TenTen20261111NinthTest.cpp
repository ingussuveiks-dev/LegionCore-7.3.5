#include "TenTen20261111PetFixture.h"
#include "Selected.inc"
static void Fill(PetBattle&b,int count=3){for(int t=0;t<2;++t)for(int j=0;j<count;++j){auto p=std::make_shared<BattlePetInstance>();p->ID=t*3+j;p->TeamID=t;b.Pets[p->ID]=p;b.Teams[t]->TeamPets[j]=p;b.Teams[t]->TeamPetCount=count;}}
static void check(bool x){if(!x)std::exit(1);}
int main(int argc,char**argv){int c=argc>1?atoi(argv[1]):0;PetBattle b;Fill(b);BattlePetAbilityEffectEntry data;data.BattlePetEffectPropertiesID=1;Handlers[1]={&PetBattleAbilityEffect::HandleDummy,PETBATTLE_TARGET_TARGET_TEAM};PetBattleAbilityEffect e;e.PetBattleInstance=&b;e.EffectInfo=&data;
switch(c){
case 0:{for(auto pair:{std::pair<int,int>{1,100},{5,200},{3,300}}){auto a=new PetBattleAura;a->ID=pair.first;a->AbilityID=pair.second;b.PetAuras.push_back(a);}b.AddAura(0,0,100,3,1,0,1);check(b.RoundEvents.back().slot==1);b.AddAura(0,0,400,3,1,0,0);check(b.PetAuras.back()->ID==6);break;}
case 1:{for(int t=0;t<2;++t){b.Pets[t*3]->attack=false;b.Teams[t]->ActiveAbilityId=30;b.Teams[t]->activeAbilityTurnMax=4;}b.Update(1000);check(!b.Teams[0]->ActiveAbilityId&&!b.Teams[1]->ActiveAbilityId);b.Teams[0]->finish=true;b.Teams[0]->Ready=b.Teams[1]->Ready=true;auto n=b.Teams[1]->updates;b.Update(1000);check(b.Teams[1]->updates==n&&!b.Teams[0]->Ready&&!b.Teams[1]->Ready);break;}
case 2:{b.SwapPet(0,-2);check(b.Teams[0]->ActivePetID==0);b.SwapPet(0,3);check(b.Teams[0]->ActivePetID==0);b.Pets[1]->States[BATTLEPET_STATE_swapInLock]=1;b.SwapPet(0,1);check(b.Teams[0]->ActivePetID==0);b.Pets[1]->States[BATTLEPET_STATE_swapInLock]=0;b.SwapPet(0,1);check(b.Teams[0]->ActivePetID==1);break;}
case 3:{e.Target=3;b.Pets[0]->States[BATTLEPET_STATE_Stat_CritChance]=100;b.Pets[3]->States[BATTLEPET_STATE_Stat_CritChance]=0;check(e.CalculateDamage(10)==20&&lastRoll==100);break;}
case 4:{e.AddTarget(PETBATTLE_TARGET_TARGET_TEAM);check(e.Targets==std::vector<uint32>({3,4,5}));e.Targets.clear();e.AddTarget(PETBATTLE_TARGET_HEAD);check(e.Targets==std::vector<uint32>({0,3}));for(int n=0;n<3;++n){e.Targets.clear();check(e.AddTarget(PetBattleAbilityImplicitTarget(PETBATTLE_TARGET_CASTER_TEAM_0+n)));check(e.Targets==std::vector<uint32>{uint32(n)});}break;}
case 5:{e.Targets={0,1,2};check(e.Execute());check(PetBattleAbilityEffect::hits==e.Targets);break;}
case 6:{data.BattlePetEffectPropertiesID=MAX_PETBATTLE_EFFECT_TYPES;e.Targets={0};e.SelectTargets();check(e.Targets.empty()&&!e.Execute()&&errorLogs==1);break;}
case 7:{e.AbilityID=726;data.Param[0]=4;b.Pets[0]->Health=50;check(e.HandleHealPercent()&&b.Pets[0]->Health==54);break;}
case 8:{PetBattleAura a;a.Duration=a.MaxDuration=-1;for(int n=0;n<5;++n){a.Process(&b);check(a.Duration==-1&&!a.Expired);}break;}
case 9:{BattlePetAbilityEntry ability;BattlePetAbilityTurnEntry turn;turn.BattlePetAbilityID=900;turn.ID=900;data.BattlePetAbilityTurnID=900;sBattlePetAbilityStore.push_back(&ability);sBattlePetAbilityTurnStore.push_back(&turn);sBattlePetAbilityEffectStore.push_back(&data);b.RoundEvents.emplace_back();b.RoundEvents.back().BuffTurn=99;PetBattleAura a;a.AbilityID=900;a.Turn=3;a.MaxDuration=9;a.Duration=9;a.Process(&b);check(b.RoundEvents.size()==3);auto it=b.RoundEvents.begin();check(it++->BuffTurn==99);check(it->BuffTurn==3);break;}
default:return 2;}
std::cout<<"PASS complete production pet battle scope "<<c<<"; DB2/cast/session endpoints controlled\n";}
