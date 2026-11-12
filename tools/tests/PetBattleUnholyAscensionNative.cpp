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
static void Ascend(PetBattle& b,uint32 caster){Require(b.Cast(caster,321,1,PETBATTLE_ABILITY_TURN0_PROC_ON_NONE,PETBATTLE_CAST_TRIGGER_ALL)==PETBATTLE_CAST_OK,"Actual Unholy Ascension Cast failed");}
static void TeamAuras(PetBattle&b,uint32 caster){for(auto pet:b.Teams[!b.Pets[caster]->TeamID]->TeamPets){if(!pet)continue;Require(pet->IsAlive()&&pet->Health==100&&Aura(b,pet->ID,320)&&pet->States[BATTLEPET_STATE_Mod_DamageTakenPercent]==25,"Unholy Ascension must leave each enemy alive with its own native twenty-five-percent aura");}}
static void Cases(std::string const&choice){
 Require(choice.empty()||choice=="team"||choice=="clone"||choice=="failed"||choice=="holes"||choice=="expiry"||choice=="reapply"||choice=="mechanical"||choice=="round","Unknown Unholy Ascension regression group");auto run=[&](char const*s){return choice.empty()||choice==s;};
 if(run("team"))for(uint32 caster:{0u,3u}){PetBattle b;Prepare(b,321,caster);Ascend(b,caster);TeamAuras(b,caster);Require(!b.Pets[caster]->IsAlive()&&b.PetXDied.size()==1,"Unholy Ascension must sacrifice only its own user once");for(auto pet:b.Teams[b.Pets[caster]->TeamID]->TeamPets)if(pet->ID!=caster)Require(pet->IsAlive()&&pet->Health==100&&!Aura(b,pet->ID,320),"Unholy Ascension cannot affect allied reserves");}
 if(run("clone"))for(uint32 caster:{0u,3u}){PetBattle b;Prepare(b,321,caster);b.Pets[caster]->States[BATTLEPET_STATE_Clone_Active]=1;Ascend(b,caster);Require(b.Pets[caster]->IsAlive()&&b.PetXDied.empty(),"Unholy Ascension must honor its native caster immunity");TeamAuras(b,caster);}
 if(run("failed"))for(uint32 caster:{0u,3u}){PetBattle b;Prepare(b,321,caster);b.Pets[caster]->States[BATTLEPET_STATE_Stat_Accuracy]=-100;Ascend(b,caster);Require(!b.Pets[caster]->IsAlive()&&b.PetXDied.size()==1,"Authored two-hundred-accuracy sacrifice must survive failed debuff casts");for(auto pet:b.Teams[!b.Pets[caster]->TeamID]->TeamPets)Require(pet->IsAlive()&&!Aura(b,pet->ID,320)&&!pet->States[BATTLEPET_STATE_Mod_DamageTakenPercent],"A failed Unholy Ascension debuff cannot change another recipient");}
 if(run("holes"))for(uint32 caster:{0u,3u}){PetBattle b;Prepare(b,321,caster);uint32 enemy=caster?0:3;b.Pets[enemy]->Health=0;b.Pets[enemy]->States[BATTLEPET_STATE_Is_Dead]=1;b.Pets[enemy+1].reset();b.Teams[!b.Pets[caster]->TeamID]->TeamPets[1].reset();Ascend(b,caster);Require(b.Pets[enemy+2]->IsAlive()&&Aura(b,enemy+2,320)&&b.Pets[enemy+2]->States[BATTLEPET_STATE_Mod_DamageTakenPercent]==25&&b.PetXDied.size()==1,"Dead/missing Unholy Ascension slots must not retarget or block the later enemy");}
 if(run("expiry"))for(uint32 caster:{0u,3u}){PetBattle b;Prepare(b,321,caster);Ascend(b,caster);TeamAuras(b,caster);for(auto pet:b.Teams[!b.Pets[caster]->TeamID]->TeamPets){auto a=Aura(b,pet->ID,320);Require(a->Duration==9,"Native Unholy Ascension duration must be retained");for(uint32 turn=0;turn<9;++turn)a->Process(&b);Require(!a->Expired,"Unholy Ascension retains its authored nine processing phases");a->Process(&b);Require(a->Expired&&!pet->States[BATTLEPET_STATE_Mod_DamageTakenPercent]&&pet->IsAlive(),"Unholy Ascension expiry must remove only its actual recipient modifier");auto count=b.RoundEvents.size();a->Expire(&b);Require(b.RoundEvents.size()==count,"Unholy Ascension cleanup is idempotent");}}
 if(run("reapply"))for(uint32 caster:{0u,3u}){PetBattle b;Prepare(b,321,caster);uint32 ally=caster+1;b.AddAura(caster,ally,1417,3,1,1,0);auto unrelated=Aura(b,ally,1417);b.Pets[caster]->States[BATTLEPET_STATE_Clone_Active]=1;Ascend(b,caster);Ascend(b,caster);TeamAuras(b,caster);for(auto pet:b.Teams[!b.Pets[caster]->TeamID]->TeamPets){uint32 count=0;for(auto a:b.PetAuras)if(!a->Expired&&a->AbilityID==320&&a->TargetPetID==pet->ID)++count;Require(count==1,"Repeated Unholy Ascension cannot multiply a recipient aura");}Require(unrelated&&!unrelated->Expired&&b.Pets[ally]->States[BATTLEPET_STATE_Special_TypeOverride]==7,"Unholy Ascension cannot remove an unrelated allied aura");}
 if(run("mechanical"))for(uint32 caster:{0u,3u}){PetBattle b;Prepare(b,321,caster,true);Ascend(b,caster);Require(b.Pets[caster]->IsAlive()&&b.Pets[caster]->Health==20&&Aura(b,caster,244)&&b.PetXDied.empty(),"Unholy Ascension must retain Mechanical resurrection without an authored prohibition");TeamAuras(b,caster);}
 if(run("round"))for(uint32 caster:{0u,3u}){PetBattle b;Prepare(b,321,caster);b.Pets[caster]->States[BATTLEPET_STATE_Stat_Speed]=10000;for(auto t:b.Teams){t->Ready=true;t->ActiveAbilityId=t->ActivePetID==caster?321:110;t->activeAbilityTurn=t->activeAbilityTurnMax=1;}b.ProceedRound();Require(!b.Pets[caster]->IsAlive()&&b.Pets[caster?0:3]->IsAlive(),"Actual Unholy Ascension round must preserve the living enemy and sacrifice its user");TeamAuras(b,caster);}
}
int main(int argc,char**argv){try{
 Require(argc==2,"Native26972 data path required");std::string error;auto config=std::getenv("LEGION_DIALOGUE_CONFIG");Require(config&&sConfigMgr->LoadInitial(config,{},error),"Native test configuration unavailable");
 MySQL::Library_Init();HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);Require(!HotfixDatabase.Open()&&HotfixDatabase.PrepareStatements(),"Native readonly hotfix connection failed");HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
 DB2Storage<BattlePetVisualEntry> visuals("BattlePetVisual.db2",BattlePetVisualLoadInfo::Instance());
 Load(sBattlePetAbilityStore,argv[1]);Load(sBattlePetAbilityTurnStore,argv[1]);Load(sBattlePetAbilityEffectStore,argv[1]);Load(sBattlePetAbilityStateStore,argv[1]);Load(sBattlePetStateStore,argv[1]);Load(sBattlePetSpeciesStore,argv[1]);Load(visuals,argv[1]);
 for(uint32 id:{242u,244u,795u,1417u,1419u,968u})for(auto s:sBattlePetAbilityStateStore)if(s->BattlePetAbilityID==id)std::cout<<"NATIVE_STATE "<<id<<" state="<<uint32(s->BattlePetStateID)<<" value="<<s->Value<<"\n";
 for(uint32 id:{500u,1577u,2608u,3449u,3493u,3496u,2831u,2838u}){auto e=sBattlePetAbilityEffectStore.LookupEntry(id);Require(e,"Native lifecycle row missing");auto v=visuals.LookupEntry(e->BattlePetVisualID);std::cout<<"NATIVE_VISUAL "<<id<<" visual="<<e->BattlePetVisualID;if(v)std::cout<<" function="<<v->SceneScriptFunction->Str[0]<<" range="<<uint32(v->RangeTypeEnum)<<" flags="<<uint32(v->Flags);std::cout<<"\n";}
 LoadGameTables(std::string(argv[1])+"../../");
 auto selected=std::getenv("LEGION_PET_UNHOLY_CASE");Cases(selected?selected:"");
 HotfixDatabase.Close();MySQL::Library_End();std::cout<<"PASS actual native Unholy Ascension cases: "<<(selected?selected:"all16")<<"; matching26972 dispatcher, species initialization, health/death/aura/round callbacks; no character data changed\n";return 0;
 }catch(std::exception const&e){std::cerr<<e.what()<<"\n";return 1;}}
