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
static PetBattleAura* Cheat(PetBattle&b,uint32 owner){Require(b.Cast(owner,1906,1,PETBATTLE_ABILITY_TURN0_PROC_ON_NONE,PETBATTLE_CAST_TRIGGER_ALL)==PETBATTLE_CAST_OK,"Native Cheat Death root failed");auto a=Aura(b,owner,1907);Require(a&&a->Duration==0&&a->MaxDuration==0,"Native Cheat Death must create its authored zero-duration controller");return a;}
static void MortalHit(PetBattle&b,uint32 victim){PetBattleAbilityTurn turn{};auto hit=Hit(b,turn,victim?0:3,victim);hit.Damage(victim,1000);}
static void End(PetBattle&b){for(auto t:b.Teams){t->Ready=true;t->ActiveAbilityId=0;t->activeAbilityTurn=t->activeAbilityTurnMax=1;}b.ProceedRound();}
static void InvokeCheat(PetBattle&b,uint32 source,uint32 owner){Require(b.Cast(source,1907,0,PETBATTLE_ABILITY_TURN0_PROC_ON_ROUND_END,PETBATTLE_CAST_TRIGGER_ALL,owner)==PETBATTLE_CAST_OK,"Actual owned Cheat Death end dispatch failed");}
static uint32 CheatCancels(PetBattle&b,uint32 owner){uint32 n=0;for(auto const&e:b.RoundEvents)if(e.EventType==PETBATTLE_EVENT_BUFF_CANCEL)for(auto const&u:e.Updates)if(u.Buff.AbilityID==1907&&u.TargetPetID==owner)++n;return n;}
static void AliveHalf(PetBattle&b,uint32 owner){Require(b.Pets[owner]->IsAlive()&&b.Pets[owner]->Health==b.Pets[owner]->GetMaxHealth()/2,"Cheat Death must revive only its actual holder to native50 percent");Require(!Aura(b,owner,1907),"Successful Cheat Death must consume its owned controller");}
static void Cases(std::string const&choice){
 Require(choice.empty()||choice=="lethal"||choice=="round"||choice=="idle"||choice=="periodic"||choice=="capture"||choice=="otherdeath"||choice=="foreign"||choice=="context"||choice=="multiple"||choice=="equal"||choice=="mechanical"||choice=="expiry"||choice=="unrelated","Unknown Cheat Death group");auto run=[&](char const*s){return choice.empty()||choice==s;};
 if(run("lethal"))for(uint32 owner:{0u,3u}){PetBattle b;Setup(b);Cheat(b,owner);MortalHit(b,owner);Require(!b.Pets[owner]->IsAlive()&&Aura(b,owner,1907),"Native Cheat Death controller must survive its holder death until round end");Require(b.PetXDied.size()==1&&b.PetXDied[0]==owner,"Actual native death receipt missing");InvokeCheat(b,owner,owner);AliveHalf(b,owner);Require(b.PetXDied.empty()&&b.RoundResult==PETBATTLE_ROUND_RESULT_NONE&&CheatCancels(b,owner)==1,"Revived holder must remove only its actual death receipt and consume once");Require(b.Cast(owner,1907,0,PETBATTLE_ABILITY_TURN0_PROC_ON_ROUND_END,PETBATTLE_CAST_TRIGGER_ALL,owner)==PETBATTLE_CAST_INTERNAL_ERROR,"Consumed Cheat Death cannot replay");}
 if(run("round"))for(uint32 owner:{0u,3u}){PetBattle b;Setup(b);b.Pets[owner]->Health=1;b.Pets[owner]->States[BATTLEPET_STATE_Stat_Speed]=10000;b.Pets[owner]->Abilities[0]=1906;b.Pets[owner]->Cooldowns[0]=0;for(auto p:b.Pets)p->States[BATTLEPET_STATE_Stat_CritChance]=0;for(auto t:b.Teams){t->Ready=true;t->ActiveAbilityId=t->ActivePetID==owner?1906:110;t->activeAbilityTurn=t->activeAbilityTurnMax=1;}b.ProceedRound();AliveHalf(b,owner);auto health=b.Pets[owner]->Health;End(b);Require(b.Pets[owner]->Health==health&&!Aura(b,owner,1907),"Full native root/attack/end sequence cannot replay a consumed revival");}
 if(run("idle"))for(uint32 owner:{0u,3u}){PetBattle b;Setup(b);Cheat(b,owner);End(b);Require(b.Pets[owner]->IsAlive()&&b.Pets[owner]->Health==100&&!Aura(b,owner,1907),"Unused native zero-duration Cheat Death must expire after its one end check");MortalHit(b,owner);End(b);Require(!b.Pets[owner]->IsAlive(),"An expired Cheat Death cannot revive a later death");}
 if(run("periodic"))for(uint32 owner:{0u,3u}){PetBattle b;Setup(b);Cheat(b,owner);Require(b.AddAura(owner?0:3,owner,379,3,1,1,0),"Actual native Poison setup failed");b.Pets[owner]->Health=1;End(b);AliveHalf(b,owner);Require(!Aura(b,owner,379),"Ordinary periodic aura must retain its final-death cleanup");}
 if(run("capture"))for(uint32 owner:{0u,3u}){PetBattle b;Setup(b);Cheat(b,owner);b.Catch(owner?0:3,owner,3482);Require(b.Pets[owner]->Captured&&b.Pets[owner]->Caged,"Native capture setup failed");Require(b.Cast(owner,1907,0,PETBATTLE_ABILITY_TURN0_PROC_ON_ROUND_END,PETBATTLE_CAST_TRIGGER_ALL,owner)==PETBATTLE_CAST_INTERNAL_ERROR,"Captured holder cannot enter a revival context");End(b);Require(!b.Pets[owner]->IsAlive()&&b.Pets[owner]->Captured&&b.Pets[owner]->Caged&&!Aura(b,owner,1907),"Cheat Death cannot revive or retain a captured holder");}
 if(run("otherdeath"))for(uint32 owner:{0u,3u}){PetBattle b;Setup(b);Cheat(b,owner);uint32 enemy=owner?0:3;MortalHit(b,owner);MortalHit(b,enemy);InvokeCheat(b,owner,owner);AliveHalf(b,owner);Require(b.PetXDied.size()==1&&b.PetXDied[0]==enemy&&b.RoundResult==PETBATTLE_ROUND_RESULT_CATCH_OR_KILL,"Reviving its holder cannot erase another actual death or normalize that result");}
 if(run("foreign"))for(uint32 owner:{0u,3u})for(bool deadSource:{false,true}){PetBattle b;Setup(b);uint32 source=owner?0:3;Require(b.AddAura(source,owner,1907,0,1,3448,0),"Foreign stored source setup failed");b.Pets[owner]->States[BATTLEPET_STATE_untargettable]=1;if(deadSource)MortalHit(b,source);MortalHit(b,owner);InvokeCheat(b,source,owner);AliveHalf(b,owner);Require(CheatCancels(b,owner)==1&&b.Pets[source]->Health==(deadSource?0:100),"Owned revival retains source metadata, bypasses hostile gating only for its holder and never heals the source");}
 if(run("context"))for(uint32 owner:{0u,3u}){PetBattle b;Setup(b);for(int32 invalid:{-3,-1,6,int32(owner),int32(owner+1)})Require(b.Cast(owner,1907,0,PETBATTLE_ABILITY_TURN0_PROC_ON_ROUND_END,PETBATTLE_CAST_TRIGGER_ALL,invalid)==PETBATTLE_CAST_INTERNAL_ERROR,"Revival requires a bounded actual issued source/holder aura");Cheat(b,owner);Require(b.Cast(owner?0:3,1907,0,PETBATTLE_ABILITY_TURN0_PROC_ON_ROUND_END,PETBATTLE_CAST_TRIGGER_ALL,owner)==PETBATTLE_CAST_INTERNAL_ERROR,"Unissued original source cannot borrow a holder aura");Require(b.Cast(owner,1907,1,PETBATTLE_ABILITY_TURN0_PROC_ON_ROUND_END,PETBATTLE_CAST_TRIGGER_ALL,owner)==PETBATTLE_CAST_INTERNAL_ERROR,"Owned round-end requires native turn0");}
 if(run("multiple"))for(uint32 owner:{0u,3u}){PetBattle b;Setup(b);Cheat(b,owner);Require(b.AddAura(owner,owner+1,1907,0,1,3448,0),"Second actual holder setup failed");MortalHit(b,owner);MortalHit(b,owner+1);End(b);AliveHalf(b,owner);AliveHalf(b,owner+1);}
 if(run("equal"))for(uint32 owner:{0u,3u}){PetBattle b;Setup(b);Cheat(b,owner);MortalHit(b,owner);b.Pets[owner]->Health=50;InvokeCheat(b,owner,owner);AliveHalf(b,owner);Require(b.PetXDied.empty(),"Equal stored health must still clear death and its current receipt");}
 if(run("mechanical"))for(uint32 owner:{0u,3u}){PetBattle b;Setup(b);Racial(b,owner,BATTLEPET_PETTYPE_MECHANICAL);Cheat(b,owner);MortalHit(b,owner);Require(b.Pets[owner]->IsAlive()&&b.Pets[owner]->Health==20,"Native Mechanical reaction must precede a later end controller");End(b);Require(b.Pets[owner]->Health==20&&!Aura(b,owner,1907),"A native immediate survivor cannot receive a second50 percent revival");}
 if(run("expiry"))for(uint32 owner:{0u,3u}){PetBattle b;Setup(b);auto a=Cheat(b,owner);a->Process(&b);Require(!a->Expired&&a->Duration==0&&a->Turn==1,"Native zero-duration expiry must wait for its issued end check");InvokeCheat(b,owner,owner);Require(!a->Expired&&b.Pets[owner]->Health==100,"Native alive check stops before resurrect/remove rows");End(b);Require(!Aura(b,owner,1907)&&b.Pets[owner]->Health==100,"Finite end cleanup must remove the unused controller without healing");}
 if(run("unrelated"))for(uint32 owner:{0u,3u}){PetBattle b;Setup(b);Cheat(b,owner);Require(b.AddAura(owner?0:3,owner,379,3,1,1,0),"Native death cleanup control missing");MortalHit(b,owner);Require(!Aura(b,owner,379)&&Aura(b,owner,1907),"Only the pending revival is retained; ordinary Poison expires");End(b);AliveHalf(b,owner);}
}
int main(int argc,char**argv){try{
 Require(argc==2,"Native26972 data path required");std::string error;auto config=std::getenv("LEGION_DIALOGUE_CONFIG");Require(config&&sConfigMgr->LoadInitial(config,{},error),"Native test configuration unavailable");
 MySQL::Library_Init();HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);Require(!HotfixDatabase.Open()&&HotfixDatabase.PrepareStatements(),"Native readonly hotfix connection failed");HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
 DB2Storage<BattlePetVisualEntry> visuals("BattlePetVisual.db2",BattlePetVisualLoadInfo::Instance());
 Load(sBattlePetAbilityStore,argv[1]);Load(sBattlePetAbilityTurnStore,argv[1]);Load(sBattlePetAbilityEffectStore,argv[1]);Load(sBattlePetAbilityStateStore,argv[1]);Load(sBattlePetStateStore,argv[1]);Load(sBattlePetSpeciesStore,argv[1]);Load(visuals,argv[1]);
 for(uint32 id:{242u,244u,795u,1417u,1419u,968u})for(auto s:sBattlePetAbilityStateStore)if(s->BattlePetAbilityID==id)std::cout<<"NATIVE_STATE "<<id<<" state="<<uint32(s->BattlePetStateID)<<" value="<<s->Value<<"\n";
 for(uint32 id:{500u,1577u,2608u,3449u,3493u,3496u,2831u,2838u}){auto e=sBattlePetAbilityEffectStore.LookupEntry(id);Require(e,"Native lifecycle row missing");auto v=visuals.LookupEntry(e->BattlePetVisualID);std::cout<<"NATIVE_VISUAL "<<id<<" visual="<<e->BattlePetVisualID;if(v)std::cout<<" function="<<v->SceneScriptFunction->Str[0]<<" range="<<uint32(v->RangeTypeEnum)<<" flags="<<uint32(v->Flags);std::cout<<"\n";}
 LoadGameTables(std::string(argv[1])+"../../");
 auto selected=std::getenv("LEGION_PET_CHEAT_CASE");Cases(selected?selected:"");
 HotfixDatabase.Close();MySQL::Library_End();std::cout<<"PASS actual native Cheat Death cases: "<<(selected?selected:"all28")<<"; matching26972 dispatcher, species initialization, health/death/aura/round callbacks; no character data changed\n";return 0;
 }catch(std::exception const&e){std::cerr<<e.what()<<"\n";return 1;}}
