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
static void Cases(std::string const& choice) {
 Require(choice.empty()||choice=="damage"||choice=="terminal"||choice=="failsafe"||choice=="team"||choice=="lethal"||choice=="guards"||choice=="round","Unknown sacrifice regression group");
 auto run=[&](char const* name){return choice.empty()||choice==name;};
 if(run("damage"))for(uint32 ability:{282u,663u,836u})for(uint32 caster:{0u,3u})for(int32 power:{0,300}){
  PetBattle b;Prepare(b,ability,caster);uint32 enemy=caster?0:3;b.Pets[caster]->States[BATTLEPET_STATE_Stat_Power]=power;
  CastSacrifice(b,caster,ability);int32 remaining=ability==663?85:60;
  Require(b.Pets[enemy]->IsAlive()&&b.Pets[enemy]->Health==remaining,"Native percentage sacrifice damage must reach only its enemy without a second power multiplier");
  Require(!b.Pets[caster]->IsAlive()&&b.PetXDied.size()==1,"A successful ordinary sacrifice must kill its own user once");
  if(ability==663)Require(Aura(b,enemy,664),"Corpse Explosion retains its authored enemy infection");
 }
 if(run("terminal"))for(uint32 caster:{0u,3u}){
  PetBattle b;Prepare(b,836,caster);CastSacrifice(b,caster,836);uint32 enemy=caster?0:3;
  Require(b.Pets[enemy]->IsAlive()&&!b.Pets[caster]->IsAlive()&&b.PetXDied.size()==1,"The native sacrifice terminal effect must kill only its user");
  auto count=b.RoundEvents.size();CastSacrifice(b,caster,836);Require(b.RoundEvents.size()==count,"A completed sacrifice cannot replay in the closed round");
 }
 if(run("failsafe"))for(uint32 ability:{282u,1025u})for(uint32 caster:{0u,3u}){
  PetBattle b;Prepare(b,ability,caster,true);CastSacrifice(b,caster,ability);
  Require(!b.Pets[caster]->IsAlive()&&!b.Pets[caster]->States[BATTLEPET_STATE_Passive_Mechanical]&&!Aura(b,caster,244),"Explode and Armageddon must prevent native Failsafe resurrection");
 }
 if(run("team"))for(uint32 caster:{0u,3u}){
  PetBattle b;Prepare(b,1025,caster);CastSacrifice(b,caster,1025);uint32 team=b.Pets[caster]->TeamID;
  for(auto pet:b.Teams[!team]->TeamPets)Require(pet->IsAlive()&&pet->Health==80,"Armageddon must address each of the three authored enemy slots once");
  for(auto pet:b.Teams[team]->TeamPets)if(pet->ID!=caster)Require(pet->IsAlive()&&pet->Health==100,"Armageddon cannot damage allied reserves");
 }
 if(run("lethal"))for(uint32 ability:{282u,663u,836u,1025u})for(uint32 caster:{0u,3u}){
  PetBattle b;Prepare(b,ability,caster);uint32 enemy=caster?0:3;b.Pets[enemy]->Health=1;CastSacrifice(b,caster,ability);
  Require(!b.Pets[caster]->IsAlive()&&!b.Pets[enemy]->IsAlive(),"A lethal first hit must still execute the authored terminal sacrifice");
  Require(b.PetXDied.size()==2,"Lethal enemy and caster deaths must be recorded exactly once each");
  if(ability==1025)for(uint32 slot=1;slot<3;++slot)Require(b.Pets[enemy+slot]->Health==80,"Armageddon must continue to the other enemies after its first lethal hit");
 }
 if(run("guards"))for(uint32 caster:{0u,3u}){
  PetBattle b;Prepare(b,836,caster);PetBattleAbilityTurn turn{};auto e=Hit(b,turn,caster,caster);e.AbilityID=836;e.EffectInfo=sBattlePetAbilityEffectStore.LookupEntry(1722);
  b.Pets[caster]->States[BATTLEPET_STATE_Clone_Active]=1;e.HandleKill();Require(b.Pets[caster]->IsAlive()&&b.PetXDied.empty(),"Native caster immunity must protect the clone execution context");
  b.Pets[caster]->States[BATTLEPET_STATE_Clone_Active]=0;b.Pets[caster]->States[BATTLEPET_STATE_Stat_Accuracy]=-100;e.Flags=0;e.HandleKill();Require(b.Pets[caster]->IsAlive()&&b.PetXDied.empty(),"A failed terminal effect cannot sacrifice a living pet");
  Require(!e.AddTarget(6)&&!e.AddTarget(uint32(-1)),"Native target admission must reject out-of-range slots before array access");
  for(bool missing:{false,true}){PetBattle absent;Prepare(absent,282,caster);uint32 enemy=caster?0:3;if(missing)absent.Pets[enemy].reset();else{absent.Pets[enemy]->Health=0;absent.Pets[enemy]->States[BATTLEPET_STATE_Is_Dead]=1;}CastSacrifice(absent,caster,282);Require(!absent.Pets[caster]->IsAlive()&&absent.PetXDied.size()==1,"Missing/dead enemy admission must retain the authored user sacrifice without an invalid target");}
  {PetBattle holes;Prepare(holes,1025,caster);uint32 enemy=caster?0:3;holes.Pets[enemy]->Health=0;holes.Pets[enemy]->States[BATTLEPET_STATE_Is_Dead]=1;holes.Pets[enemy+1].reset();holes.Teams[!holes.Pets[caster]->TeamID]->TeamPets[1].reset();CastSacrifice(holes,caster,1025);Require(holes.Pets[enemy+2]->Health==80&&holes.Pets[enemy+2]->IsAlive()&&holes.PetXDied.size()==1,"An absent/dead Armageddon slot cannot retarget or block the later enemy slot");}
 }
 if(run("round"))for(uint32 ability:{282u,663u,836u,1025u})for(uint32 caster:{0u,3u}){
  PetBattle b;Prepare(b,ability,caster);uint32 enemy=caster?0:3;b.Pets[caster]->States[BATTLEPET_STATE_Stat_Speed]=10000;
  for(auto team:b.Teams){team->Ready=true;team->ActiveAbilityId=team->ActivePetID==caster?ability:110;team->activeAbilityTurn=1;team->activeAbilityTurnMax=1;}
  b.ProceedRound();Require(!b.Pets[caster]->IsAlive()&&b.Pets[enemy]->IsAlive(),"Actual native round must retain the sacrifice recipient and terminal sequence");
 }
}
int main(int argc,char**argv){try{
 Require(argc==2,"Native26972 data path required");std::string error;auto config=std::getenv("LEGION_DIALOGUE_CONFIG");Require(config&&sConfigMgr->LoadInitial(config,{},error),"Native test configuration unavailable");
 MySQL::Library_Init();HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);Require(!HotfixDatabase.Open()&&HotfixDatabase.PrepareStatements(),"Native readonly hotfix connection failed");HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
 DB2Storage<BattlePetVisualEntry> visuals("BattlePetVisual.db2",BattlePetVisualLoadInfo::Instance());
 Load(sBattlePetAbilityStore,argv[1]);Load(sBattlePetAbilityTurnStore,argv[1]);Load(sBattlePetAbilityEffectStore,argv[1]);Load(sBattlePetAbilityStateStore,argv[1]);Load(sBattlePetStateStore,argv[1]);Load(sBattlePetSpeciesStore,argv[1]);Load(visuals,argv[1]);
 for(uint32 id:{242u,244u,795u,1417u,1419u,968u})for(auto s:sBattlePetAbilityStateStore)if(s->BattlePetAbilityID==id)std::cout<<"NATIVE_STATE "<<id<<" state="<<uint32(s->BattlePetStateID)<<" value="<<s->Value<<"\n";
 for(uint32 id:{500u,1577u,2608u,3449u,3493u,3496u,2831u,2838u}){auto e=sBattlePetAbilityEffectStore.LookupEntry(id);Require(e,"Native lifecycle row missing");auto v=visuals.LookupEntry(e->BattlePetVisualID);std::cout<<"NATIVE_VISUAL "<<id<<" visual="<<e->BattlePetVisualID;if(v)std::cout<<" function="<<v->SceneScriptFunction->Str[0]<<" range="<<uint32(v->RangeTypeEnum)<<" flags="<<uint32(v->Flags);std::cout<<"\n";}
 LoadGameTables(std::string(argv[1])+"../../");
 auto selected=std::getenv("LEGION_PET_SACRIFICE_CASE");Cases(selected?selected:"");
 HotfixDatabase.Close();MySQL::Library_End();std::cout<<"PASS actual native sacrifice cases: "<<(selected?selected:"all44")<<"; matching26972 dispatcher, species initialization, health/death/aura/round callbacks; no character data changed\n";return 0;
 }catch(std::exception const&e){std::cerr<<e.what()<<"\n";return 1;}}
