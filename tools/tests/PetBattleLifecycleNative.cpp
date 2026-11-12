// Matching Legion26972 stores and actual linked pet-battle dispatcher.
// Teams live only in memory; no character/login/save or world server process.
#include <unordered_set>
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
static void Cases(std::string const&choice){
 Require(choice.empty()||choice=="mechanical"||choice=="undead"||choice=="miss"||choice=="auras"||choice=="round"||choice=="context","Unknown native lifecycle regression case");
 auto run=[&](char const*name){return choice.empty()||choice==name;};
 if(run("mechanical"))for(uint32 target:{0u,1u,3u,5u}){PetBattle b;Fill(b);Racial(b,target,BATTLEPET_PETTYPE_MECHANICAL);uint32 caster=b.Pets[target]->TeamID?0:3;PetBattleAbilityTurn turn{};auto e=Hit(b,turn,caster,target);b.Teams[b.Pets[target]->TeamID]->ActiveAbilityId=110;e.Damage(target,1000);Require(b.Pets[target]->IsAlive()&&b.Pets[target]->Health==20,"Mechanical native death must resurrect the actual victim to twenty percent");TriggerReceipt(b,target);Require(b.Pets[caster]->Health==100&&b.PetXDied.empty()&&b.RoundResult==PETBATTLE_ROUND_RESULT_NONE,"Immediate resurrection must preserve the other pet and current round");Require(!b.Pets[target]->States[BATTLEPET_STATE_Passive_Mechanical]&&Aura(b,target,244),"Native Failsafe is consumed once and retains its native aura");e.Flags=0;e.Damage(target,1000);Require(!b.Pets[target]->IsAlive()&&b.Pets[target]->Health==0&&b.PetXDied.size()==1,"A second lethal hit must kill the Mechanical pet exactly once");auto count=b.RoundEvents.size();e.Flags=0;e.Kill(target);Require(b.PetXDied.size()==1&&b.RoundEvents.size()==count,"Repeated death cannot replay native packets");}
 if(run("undead"))for(uint32 target:{0u,2u,3u,4u}){PetBattle b;Fill(b);Racial(b,target,BATTLEPET_PETTYPE_UNDEAD);uint32 caster=b.Pets[target]->TeamID?0:3;PetBattleAbilityTurn turn{};auto e=Hit(b,turn,caster,target);e.Damage(target,1000);Require(b.Pets[target]->IsAlive()&&b.Pets[target]->Health==1,"Undead native death must invoke Damned on the actual victim");auto a=Aura(b,target,242);Require(a&&a->Duration==1&&b.Pets[target]->States[BATTLEPET_STATE_unkillable]==1&&b.Pets[target]->States[BATTLEPET_STATE_Mod_DamageDealtPercent]==-25,"Native Undead one-round immortality and damage penalty must be applied");Require(b.PetXDied.empty()&&b.RoundResult==PETBATTLE_ROUND_RESULT_NONE,"Undead resurrection cannot end the current round");e.Flags=0;e.Damage(target,1000);Require(b.Pets[target]->IsAlive()&&b.Pets[target]->Health==0&&b.PetXDied.empty(),"Lethal hits during native immortality cannot kill the pet");a->Process(&b);Require(!a->Expired,"Undead retains its authored first processing phase");a->Process(&b);Require(a->Expired&&!b.Pets[target]->IsAlive()&&b.PetXDied.size()==1&&b.Pets[caster]->Health==100&&b.Pets[caster]->IsAlive(),"Undead expiry must kill only its owner once");Require(!b.Pets[target]->States[BATTLEPET_STATE_unkillable]&&!b.Pets[target]->States[BATTLEPET_STATE_Mod_DamageDealtPercent],"Expired native Undead modifiers must clean up");auto count=b.RoundEvents.size();a->Expire(&b);Require(b.RoundEvents.size()==count,"Undead expiry is idempotent");}
 if(run("miss")){PetBattle b;Fill(b);PetBattleAbilityTurn turn{};auto e=Hit(b,turn,0,3);e.Flags=PETBATTLE_EVENT_FLAG_MISS;e.Kill(3);Require(b.Pets[3]->IsAlive()&&b.Pets[3]->Health==100&&b.RoundEvents.empty()&&b.PetXDied.empty(),"A failed kill cannot mark a living pet dead");}
 if(run("auras")){PetBattle b;Fill(b);Racial(b,3,BATTLEPET_PETTYPE_MECHANICAL);Require(b.AddAura(0,3,1417,3,1,1,0),"Native marker application failed");auto a=b.PetAuras.back();PetBattleAbilityTurn turn{};auto e=Hit(b,turn,0,3);e.Damage(3,1000);Require(b.Pets[3]->IsAlive()&&!a->Expired&&b.Pets[3]->States[BATTLEPET_STATE_Special_TypeOverride]==7,"Immediate native resurrection must retain the victim auras");a->Expire(&b);Require(a->Expired&&!b.Pets[3]->States[BATTLEPET_STATE_Special_TypeOverride],"Retained aura still follows its native cleanup");}
 if(run("round"))for(uint32 family:{BATTLEPET_PETTYPE_MECHANICAL,BATTLEPET_PETTYPE_UNDEAD}){PetBattle b;Fill(b);Racial(b,3,family);b.Pets[3]->Health=1;b.Pets[0]->States[BATTLEPET_STATE_Stat_Speed]=10000;for(auto team:b.Teams){team->Ready=true;team->ActiveAbilityId=110;team->activeAbilityTurn=1;team->activeAbilityTurnMax=1;}b.ProceedRound();Require(b.Pets[0]->Health<100&&b.Pets[3]->IsAlive(),"The revived slower pet must execute its queued native Bite in the same round");}
 if(run("context")){PetBattle b;Fill(b);{PetBattle instant;Fill(instant);Racial(instant,3,BATTLEPET_PETTYPE_MECHANICAL);instant.Pets[3]->Health=20;PetBattleAbilityTurn turn{};auto hit=Hit(instant,turn,0,3);hit.Kill(3);Require(instant.Pets[3]->IsAlive()&&instant.Pets[3]->Health==20,"Native resurrection must clear death even when the requested health equals the existing health");}Require(b.Cast(6,110,1,PETBATTLE_ABILITY_TURN0_PROC_ON_NONE,PETBATTLE_CAST_TRIGGER_ALL)==PETBATTLE_CAST_INTERNAL_ERROR,"An out-of-range native caster must be rejected before array access");b.Pets[1].reset();Require(b.Cast(1,723,1,PETBATTLE_ABILITY_TURN0_PROC_ON_NONE,PETBATTLE_CAST_TRIGGER_ALL,PETBATTLE_RACIAL_TRIGGER)==PETBATTLE_CAST_INTERNAL_ERROR,"A missing native caster cannot authorize resurrection");Require(b.Cast(0,110,1,PETBATTLE_ABILITY_TURN0_PROC_ON_NONE,PETBATTLE_CAST_TRIGGER_ALL,PETBATTLE_RACIAL_TRIGGER)==PETBATTLE_CAST_INTERNAL_ERROR,"Racial context cannot authorize an unrelated ability");Require(b.Cast(0,723,1,PETBATTLE_ABILITY_TURN0_PROC_ON_NONE,PETBATTLE_CAST_TRIGGER_ALL,PETBATTLE_RACIAL_TRIGGER)==PETBATTLE_CAST_INTERNAL_ERROR,"A living caster cannot reuse native death context");}
}
int main(int argc,char**argv){try{
 Require(argc==2,"Native26972 data path required");std::string error;auto config=std::getenv("LEGION_DIALOGUE_CONFIG");Require(config&&sConfigMgr->LoadInitial(config,{},error),"Native test configuration unavailable");
 MySQL::Library_Init();HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);Require(!HotfixDatabase.Open()&&HotfixDatabase.PrepareStatements(),"Native readonly hotfix connection failed");HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
 DB2Storage<BattlePetVisualEntry> visuals("BattlePetVisual.db2",BattlePetVisualLoadInfo::Instance());
 Load(sBattlePetAbilityStore,argv[1]);Load(sBattlePetAbilityTurnStore,argv[1]);Load(sBattlePetAbilityEffectStore,argv[1]);Load(sBattlePetAbilityStateStore,argv[1]);Load(sBattlePetStateStore,argv[1]);Load(sBattlePetSpeciesStore,argv[1]);Load(visuals,argv[1]);
 for(uint32 id:{242u,244u,795u,1417u,1419u,968u})for(auto s:sBattlePetAbilityStateStore)if(s->BattlePetAbilityID==id)std::cout<<"NATIVE_STATE "<<id<<" state="<<uint32(s->BattlePetStateID)<<" value="<<s->Value<<"\n";
 for(uint32 id:{500u,1577u,2608u,3449u,3493u,3496u,2831u,2838u}){auto e=sBattlePetAbilityEffectStore.LookupEntry(id);Require(e,"Native lifecycle row missing");auto v=visuals.LookupEntry(e->BattlePetVisualID);std::cout<<"NATIVE_VISUAL "<<id<<" visual="<<e->BattlePetVisualID;if(v)std::cout<<" function="<<v->SceneScriptFunction->Str[0]<<" range="<<uint32(v->RangeTypeEnum)<<" flags="<<uint32(v->Flags);std::cout<<"\n";}
 LoadGameTables(std::string(argv[1])+"../../");
 auto selected=std::getenv("LEGION_PET_LIFECYCLE_CASE");Cases(selected?selected:"");
 HotfixDatabase.Close();MySQL::Library_End();std::cout<<"PASS actual native lifecycle cases: "<<(selected?selected:"all13")<<"; matching26972 dispatcher, species initialization, health/death/aura/round callbacks; no character data changed\n";return 0;
 }catch(std::exception const&e){std::cerr<<e.what()<<"\n";return 1;}}
