// Matching Legion26972 stores and actual linked pet-battle dispatcher.
// Teams live only in memory; no character/login/save or world server process.
#include <unordered_set>
#include <array>
#include "Common.h"
#include "SharedDefines.h"
#include "UnitDefines.h"
#include "DB2Stores.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include "ObjectGuid.h"
#include "Position.h"
#include "Timer.h"
#include "PetBattle.h"
#include "PetBattleAbilityEffect.h"
#include "GameTables.h"
#include <cstdlib>
#include <iostream>
#include <stdexcept>
static_assert(sizeof(PetBattleAbilityEffect)==0x50,"Bend Time owner context must reuse existing execution padding");
static void Require(bool value,char const*message){if(!value)throw std::runtime_error(message);}
template<class T>static void Load(T&store,char const*path){Require(store.Load(path,0),"Matching native DB2 missing");store.LoadFromDB();}
static void Fill(PetBattle&b){
 b.BattleType=PETBATTLE_TYPE_PVE;b.PveBattleType=PVE_PETBATTLE_TRAINER;b.BattleStatus=PETBATTLE_STATUS_RUNNING;b.Turn=1;b.RoundTurn=0;b.RoundFirstTeamCasting=0;
 for(uint32 t=0;t<2;++t){b.Teams[t]->ActivePetID=t*3;b.Teams[t]->TeamPetCount=3;
  for(uint32 s=0;s<3;++s){auto p=std::make_shared<BattlePetInstance>();p->ID=t*3+s;p->TeamID=t;p->PetBattleInstance=&b;p->Health=100;p->InfoMaxHealth=100;p->Level=1;p->Species=0;b.Pets[p->ID]=p;b.Teams[t]->TeamPets[s]=p;}}
}
static PetBattleAbilityEffect Hit(PetBattle&b,PetBattleAbilityTurn&turn,uint32 caster,uint32 target){PetBattleAbilityEffect e{};e.PetBattleInstance=&b;e.AbilityTurn=&turn;e.EffectInfo=sBattlePetAbilityEffectStore.LookupEntry(3482);Require(e.EffectInfo&&e.EffectInfo->BattlePetEffectPropertiesID==24,"Native damage row changed");e.Caster=caster;e.Target=target;e.AbilityID=1927;e.IsTriggered=true;return e;}
static PetBattleAura* Aura(PetBattle&b,uint32 target,uint32 ability){for(auto a:b.PetAuras)if(!a->Expired&&a->TargetPetID==target&&a->AbilityID==ability)return a;return nullptr;}
static void TriggerReceipt(PetBattle const&b,uint32 target){uint32 count=0;for(auto const&e:b.RoundEvents)if(e.EventType==PETBATTLE_EVENT_TRIGGER_ABILITY&&e.SourcePetID==int32(target)&&e.AbilityEffectID==3482)++count;Require(count==1,"Racial resurrection must emit one native trigger event from its actual recipient");}
static void Racial(PetBattle&b,uint32 target,uint32 family){for(auto s:sBattlePetSpeciesStore)if(s->PetTypeEnum==family){b.Pets[target]->Species=s->ID;Require(b.Cast(target,291,1,PETBATTLE_ABILITY_TURN0_PROC_ON_NONE,PETBATTLE_CAST_TRIGGER_ALL)==PETBATTLE_CAST_OK,"Native initialization failed");return;}throw std::runtime_error("Matching native species missing");}
static void SameFamily(PetBattle& b,uint32 ability) {
 auto data=sBattlePetAbilityStore.LookupEntry(ability);Require(data,"Matching sacrifice ability missing");
 for(auto s:sBattlePetSpeciesStore)if(s->PetTypeEnum==data->PetTypeEnum){for(auto pet:b.Pets)if(pet)pet->Species=s->ID;return;}
 throw std::runtime_error("Matching same-family neutral species missing");
}
static void Prepare(PetBattle& b,uint32 ability,uint32 caster,bool mechanical=false) {
 Fill(b);SameFamily(b,ability);if(mechanical)Racial(b,caster,BATTLEPET_PETTYPE_MECHANICAL);
 for(auto pet:b.Pets)if(pet){pet->Health=pet->GetMaxHealth();pet->States[BATTLEPET_STATE_Stat_CritChance]=0;pet->States[BATTLEPET_STATE_Stat_Power]=0;}
}
static void CastSacrifice(PetBattle& b,uint32 caster,uint32 ability) {
 Require(b.Cast(caster,ability,1,PETBATTLE_ABILITY_TURN0_PROC_ON_NONE,PETBATTLE_CAST_TRIGGER_ALL)==PETBATTLE_CAST_OK,"Actual authored sacrifice Cast failed");
}
static void Setup(PetBattle&b){Fill(b);for(auto p:b.Pets)for(uint32 slot=0;slot<3;++slot){p->Abilities[slot]=slot==0?110:slot==1?282:321;p->Cooldowns[slot]=7-int32(slot)*2;p->Lockdowns[slot]=0;}}
static std::array<int32,3> CDs(PetBattle&b,uint32 id){return{{b.Pets[id]->Cooldowns[0],b.Pets[id]->Cooldowns[1],b.Pets[id]->Cooldowns[2]}};}
static PetBattleAura* Bend(PetBattle&b,uint32 caster){b.Pets[caster]->Abilities[0]=1905;b.Pets[caster]->Cooldowns[0]=-1;Require(b.Cast(caster,1905,1,PETBATTLE_ABILITY_TURN0_PROC_ON_NONE,PETBATTLE_CAST_TRIGGER_NONE)==PETBATTLE_CAST_OK,"Native Bend Time root Cast failed");auto a=Aura(b,caster,1904);Require(a&&a->Duration==2,"Native Bend Time must apply its authored aura without an accuracy bonus");return a;}
static uint32 CancelCount(PetBattle&b,uint32 owner){uint32 count=0;for(auto const&e:b.RoundEvents)if(e.EventType==PETBATTLE_EVENT_BUFF_CANCEL)for(auto const&u:e.Updates)if(u.Buff.AbilityID==1904&&u.TargetPetID==owner)++count;return count;}
static void ResetToOne(PetBattle&b,uint32 pet){Require(CDs(b,pet)==std::array<int32,3>{{1,1,1}},"Bend Time must reset only the actual incoming pet to its three authored cooldown values");}
static void Cases(std::string const&choice){
 Require(choice.empty()||choice=="root"||choice=="swap"||choice=="admission"||choice=="death"||choice=="cooldowns"||choice=="expiry"||choice=="round"||choice=="foreign","Unknown Bend Time regression group");auto run=[&](char const*s){return choice.empty()||choice==s;};
 if(run("root"))for(uint32 caster:{0u,3u}){
  for(int32 accuracy:{0,-100}){PetBattle b;Setup(b);b.Pets[caster]->States[BATTLEPET_STATE_Stat_Accuracy]=accuracy;Bend(b,caster);b.Cast(caster,1906,1,PETBATTLE_ABILITY_TURN0_PROC_ON_NONE,PETBATTLE_CAST_TRIGGER_ALL);Require(Aura(b,caster,1907),"Both native zero-accuracy positive aura rows must apply; Cheat Death later reaction remains separate");}
  PetBattle b;Setup(b);b.Pets[caster]->States[BATTLEPET_STATE_Stat_Accuracy]=-100;b.Cast(caster,794,1,PETBATTLE_ABILITY_TURN0_PROC_ON_NONE,PETBATTLE_CAST_TRIGGER_ALL);Require(!Aura(b,caster,795),"A real nonzero positive-aura accuracy must retain its native failure");
 }
 if(run("swap"))for(uint32 caster:{0u,3u})for(uint32 slot:{1u,2u}){PetBattle b;Setup(b);auto a=Bend(b,caster);uint32 team=b.Pets[caster]->TeamID,incoming=caster+slot,enemy=caster?0:3;auto own=CDs(b,caster),before=CDs(b,incoming);b.SwapPet(!team,enemy+1);Require(!a->Expired&&CDs(b,incoming)==before,"An enemy swap cannot consume Bend Time or change the intended incoming cooldowns");b.SwapPet(team,incoming);ResetToOne(b,incoming);Require(a->Expired&&CancelCount(b,caster)==1&&CDs(b,caster)==own,"Bend Time must consume its actual owner aura once and preserve the outgoing cooldowns");uint32 other=caster+(slot==1?2:1);auto unchanged=CDs(b,other);b.SwapPet(team,other);Require(CDs(b,other)==unchanged&&CancelCount(b,caster)==1,"A second swap cannot replay an already consumed Bend Time");}
 if(run("admission"))for(uint32 caster:{0u,3u}){PetBattle b;Setup(b);auto a=Bend(b,caster);uint32 team=b.Pets[caster]->TeamID,incoming=caster+1,enemy=caster?0:3;auto before=CDs(b,incoming);b.SwapPet(team,caster);b.SwapPet(team,enemy);b.Pets[caster]->States[BATTLEPET_STATE_swapOutLock]=1;b.SwapPet(team,incoming);b.Pets[caster]->States[BATTLEPET_STATE_swapOutLock]=0;b.Pets[incoming]->States[BATTLEPET_STATE_swapInLock]=1;b.SwapPet(team,incoming);b.Pets[incoming]->States[BATTLEPET_STATE_swapInLock]=0;b.Pets[incoming]->States[BATTLEPET_STATE_Is_Dead]=1;b.SwapPet(team,incoming);b.Pets[incoming]->States[BATTLEPET_STATE_Is_Dead]=0;auto pet=b.Pets[incoming];b.Pets[incoming].reset();b.SwapPet(team,incoming);b.Pets[incoming]=pet;Require(!a->Expired&&CDs(b,incoming)==before,"Same-front,foreign,locked,dead or missing swaps cannot consume Bend Time");for(int32 owner:{-1,-3,6,int32(caster),int32(caster+2)})Require(b.Cast(caster,1904,0,PETBATTLE_ABILITY_TURN0_PROC_ON_SWAP_IN,PETBATTLE_CAST_TRIGGER_ALL,owner)==PETBATTLE_CAST_INTERNAL_ERROR,"Bend Time requires bounded actual aura-owner context");Require(b.Cast(enemy,1904,0,PETBATTLE_ABILITY_TURN0_PROC_ON_SWAP_IN,PETBATTLE_CAST_TRIGGER_ALL,caster)==PETBATTLE_CAST_INTERNAL_ERROR,"An unrelated caster cannot reuse another Bend Time aura");Require(!a->Expired&&CDs(b,incoming)==before,"Rejected Bend Time contexts cannot mutate cooldowns or consume the aura");}
 if(run("death"))for(uint32 caster:{0u,3u}){PetBattle b;Setup(b);auto a=Bend(b,caster);uint32 enemy=caster?0:3;Require(b.AddAura(enemy,caster,379,3,1,1,0),"Native Poison control aura missing");auto poison=Aura(b,caster,379);PetBattleAbilityTurn turn{};auto hit=Hit(b,turn,enemy,caster);hit.Damage(caster,1000);Require(!b.Pets[caster]->IsAlive()&&!a->Expired&&poison->Expired,"Bend Time must survive its holder death while ordinary Poison retains native death cleanup");b.SwapPet(b.Pets[caster]->TeamID,caster+1);ResetToOne(b,caster+1);Require(a->Expired&&CancelCount(b,caster)==1,"Forced replacement after death must consume the actual holder aura once");}
 if(run("cooldowns"))for(uint32 caster:{0u,3u})for(int32 ready:{-1,0,1}){PetBattle b;Setup(b);Bend(b,caster);uint32 incoming=caster+1,team=b.Pets[caster]->TeamID;b.Pets[incoming]->Abilities[1]=0;b.Pets[incoming]->Cooldowns[1]=8;b.Pets[incoming]->Cooldowns[2]=ready;b.Pets[incoming]->Lockdowns[0]=3;b.SwapPet(team,incoming);Require(b.Pets[incoming]->Cooldowns[0]==1&&b.Pets[incoming]->Cooldowns[1]==8&&b.Pets[incoming]->Cooldowns[2]==ready&&b.Pets[incoming]->Lockdowns[0]==3,"Bend Time must preserve unlearned/ready slots and native lockdowns");for(auto t:b.Teams)t->Ready=true;b.ProceedRound();Require(b.Pets[incoming]->Cooldowns[0]==0&&!b.CanCast(team,110),"The native end-round reset must emit zero while a real lockdown still prevents use");}
 if(run("expiry"))for(uint32 caster:{0u,3u}){PetBattle b;Setup(b);auto a=Bend(b,caster);a->Process(&b);a->Process(&b);Require(!a->Expired,"Bend Time retains its native two processing phases");a->Process(&b);Require(a->Expired&&CancelCount(b,caster)==1,"Bend Time finite expiry must cancel its actual owner aura once");auto before=CDs(b,caster+1);b.SwapPet(b.Pets[caster]->TeamID,caster+1);Require(CDs(b,caster+1)==before,"An expired Bend Time cannot reset a later incoming pet");auto events=b.RoundEvents.size();a->Expire(&b);Require(b.RoundEvents.size()==events,"Bend Time expiry must remain idempotent");}
 if(run("round"))for(uint32 caster:{0u,3u}){PetBattle b;Setup(b);uint32 team=b.Pets[caster]->TeamID;b.Pets[caster]->Abilities[0]=1905;b.Pets[caster]->Cooldowns[0]=-1;for(auto t:b.Teams){t->Ready=true;t->ActiveAbilityId=t->ActivePetID==caster?1905:0;t->activeAbilityTurn=t->activeAbilityTurnMax=1;}b.ProceedRound();Require(Aura(b,caster,1904),"Actual native round must create Bend Time from its real root");b.SwapPet(team,caster+1);ResetToOne(b,caster+1);for(auto t:b.Teams)t->Ready=true;b.ProceedRound();Require(!Aura(b,caster,1904)&&CDs(b,caster+1)==std::array<int32,3>{{0,0,0}}&&b.CanCast(team,110),"Actual native swap/round must expose ready zero cooldowns after one consumed Bend Time");}
 if(run("foreign"))for(uint32 owner:{0u,3u}){PetBattle b;Setup(b);uint32 source=owner?0:3;Require(b.AddAura(source,owner,1904,2,1,3447,0),"Native stored-owner Bend Time setup failed");b.Pets[owner]->States[BATTLEPET_STATE_untargettable]=1;auto foreign=CDs(b,source);b.SwapPet(b.Pets[owner]->TeamID,owner+1);ResetToOne(b,owner+1);Require(!Aura(b,owner,1904)&&CancelCount(b,owner)==1&&CDs(b,source)==foreign,"Stored foreign-source Bend Time must use its actual holder team and clean its untargetable owner without hostile-hit gating");}
}
int main(int argc,char**argv){try{
 Require(argc==2,"Native26972 data path required");std::string error;auto config=std::getenv("LEGION_DIALOGUE_CONFIG");Require(config&&sConfigMgr->LoadInitial(config,{},error),"Native test configuration unavailable");
 MySQL::Library_Init();HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);Require(!HotfixDatabase.Open()&&HotfixDatabase.PrepareStatements(),"Native readonly hotfix connection failed");HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
 DB2Storage<BattlePetVisualEntry> visuals("BattlePetVisual.db2",BattlePetVisualLoadInfo::Instance());
 Load(sBattlePetAbilityStore,argv[1]);Load(sBattlePetAbilityTurnStore,argv[1]);Load(sBattlePetAbilityEffectStore,argv[1]);Load(sBattlePetAbilityStateStore,argv[1]);Load(sBattlePetStateStore,argv[1]);Load(sBattlePetSpeciesStore,argv[1]);Load(visuals,argv[1]);
 for(uint32 id:{242u,244u,795u,1417u,1419u,968u})for(auto s:sBattlePetAbilityStateStore)if(s->BattlePetAbilityID==id)std::cout<<"NATIVE_STATE "<<id<<" state="<<uint32(s->BattlePetStateID)<<" value="<<s->Value<<"\n";
 for(uint32 id:{500u,1577u,2608u,3449u,3493u,3496u,2831u,2838u}){auto e=sBattlePetAbilityEffectStore.LookupEntry(id);Require(e,"Native lifecycle row missing");auto v=visuals.LookupEntry(e->BattlePetVisualID);std::cout<<"NATIVE_VISUAL "<<id<<" visual="<<e->BattlePetVisualID;if(v)std::cout<<" function="<<v->SceneScriptFunction->Str[0]<<" range="<<uint32(v->RangeTypeEnum)<<" flags="<<uint32(v->Flags);std::cout<<"\n";}
 LoadGameTables(std::string(argv[1])+"../../");
 auto selected=std::getenv("LEGION_PET_BEND_CASE");Cases(selected?selected:"");
 HotfixDatabase.Close();MySQL::Library_End();std::cout<<"PASS actual native Bend Time cases: "<<(selected?selected:"all26")<<"; matching26972 dispatcher, species initialization, health/death/aura/round callbacks; no character data changed\n";return 0;
 }catch(std::exception const&e){std::cerr<<e.what()<<"\n";return 1;}}
