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
static void TypeSetup(PetBattle&b){Setup(b);for(auto s:sBattlePetSpeciesStore)if(s->PetTypeEnum==BATTLEPET_PETTYPE_HUMANOID){for(auto p:b.Pets)p->Species=s->ID;break;}for(auto p:b.Pets){p->States[BATTLEPET_STATE_Stat_CritChance]=0;p->States[BATTLEPET_STATE_Stat_Power]=0;}}
static uint32 Holder(uint32 root,uint32 source){return root==574?source:(source?0:3);}
static uint32 Parent(uint32 root){return root==574?820:1418;}
static uint32 Mark(uint32 root){return root==574?1417:968;}
static int32 Type(uint32 root){return root==574?7:4;}
static void TypeRoot(PetBattle&b,uint32 root,uint32 source){Require(b.Cast(source,root,1,PETBATTLE_ABILITY_TURN0_PROC_ON_NONE,PETBATTLE_CAST_TRIGGER_ALL)==PETBATTLE_CAST_OK,"Actual Ward/Rot root failed");}
static void TypeEnd(PetBattle&b){for(auto t:b.Teams){t->Ready=true;t->ActiveAbilityId=0;t->activeAbilityTurn=t->activeAbilityTurnMax=1;}b.ProceedRound();}
static PetBattleAbilityEffect Bite(PetBattle&b,PetBattleAbilityTurn&turn,uint32 source,uint32 target){PetBattleAbilityEffect e{};e.PetBattleInstance=&b;e.AbilityTurn=&turn;e.Caster=source;e.Target=target;e.AbilityID=110;for(auto t:sBattlePetAbilityTurnStore)if(t->BattlePetAbilityID==110&&t->OrderIndex==1)for(auto x:sBattlePetAbilityEffectStore)if(x->BattlePetAbilityTurnID==t->ID&&x->BattlePetEffectPropertiesID==24){e.EffectInfo=x;return e;}throw std::runtime_error("Native Bite damage row missing");}
static void TypeKill(PetBattle&b,uint32 victim){PetBattleAbilityTurn turn{};auto hit=Hit(b,turn,victim?0:3,victim);hit.Damage(victim,1000);}
static void Discharge(PetBattle&b,uint32 source){Require(b.Cast(source,1717,1,PETBATTLE_ABILITY_TURN0_PROC_ON_NONE,PETBATTLE_CAST_TRIGGER_ALL)==PETBATTLE_CAST_OK,"Actual Discharge root failed");}
static void Cases(std::string const&choice){
 Require(choice.empty()||choice=="roots"||choice=="round"||choice=="cleanup"||choice=="receipts"||choice=="foreign"||choice=="miss"||choice=="kill"||choice=="death"||choice=="refresh","Unknown Discharge group");auto run=[&](char const*s){return choice.empty()||choice==s;};
 if(run("roots"))for(uint32 source:{0u,3u}){PetBattle b;TypeSetup(b);uint32 enemy=source?0:3;int32 hp=b.Pets[enemy]->Health;Discharge(b,source);auto a=Aura(b,source,1718);auto mark=Aura(b,source,1419);Require(a&&a->Duration==2&&mark&&mark->Duration==-1&&mark->TriggerId==3203&&b.Pets[source]->States[124]==5,"Discharge must apply its native Critter mark with a two-round owner controller");Require(hp-b.Pets[enemy]->Health==40&&b.Pets[enemy]->States[124]==0,"Native Discharge damage and enemy type must stay unchanged");PetBattleAbilityTurn turn{};auto bite=Bite(b,turn,enemy,source);Require(bite.CalculateDamage(20,false)==30,"Actual incoming damage must use the Discharge Critter defense");}
 if(run("round"))for(uint32 source:{0u,3u}){PetBattle b;TypeSetup(b);b.Pets[source]->Abilities[0]=1717;b.Pets[source]->Cooldowns[0]=-1;for(auto t:b.Teams){t->Ready=true;t->ActiveAbilityId=t==b.Teams[b.Pets[source]->TeamID]?1717:0;t->activeAbilityTurn=t->activeAbilityTurnMax=1;}b.ProceedRound();Require(Aura(b,source,1718)&&Aura(b,source,1419)&&b.Pets[source]->States[124]==5,"Actual learned Discharge round must retain the issued Critter state");TypeEnd(b);TypeEnd(b);Require(!Aura(b,source,1718)&&!Aura(b,source,1419)&&b.Pets[source]->States[124]==0,"Discharge must remove its infinite mark when the finite native controller expires");Require(b.Pets[source]->Cooldowns[0]==2,"Discharge native cooldown5 must tick normally over three rounds");}
 if(run("cleanup"))for(uint32 source:{0u,3u}){PetBattle b;TypeSetup(b);Discharge(b,source);b.Pets[source]->States[BATTLEPET_STATE_Stat_Accuracy]=-100;b.Pets[source]->States[BATTLEPET_STATE_untargettable]=1;Aura(b,source,1718)->Expire(&b);Require(!Aura(b,source,1419)&&b.Pets[source]->States[124]==0,"Discharge guaranteed cleanup must not miss under negative accuracy");}
 if(run("receipts"))for(uint32 source:{0u,3u})for(uint32 receipt:{2839u,2840u,2842u,2843u}){PetBattle b;TypeSetup(b);Discharge(b,source);Require(b.AddAura(source,source,1419,3,1,receipt,0),"Foreign Critter producer setup failed");Aura(b,source,1718)->Expire(&b);Require(Aura(b,source,1419)&&Aura(b,source,1419)->TriggerId==receipt&&b.Pets[source]->States[124]==5,"Discharge cleanup must retain another Critter producer's actual mark");}
 if(run("receipts"))for(uint32 source:{0u,3u}){PetBattle b;TypeSetup(b);Discharge(b,source);uint32 other=source?0:3;Require(b.AddAura(other,source,1419,3,1,3203,0),"Other-source Discharge receipt setup failed");Aura(b,source,1718)->Expire(&b);Require(Aura(b,source,1419)&&Aura(b,source,1419)->CasterPetID==other&&b.Pets[source]->States[124]==5,"Discharge cleanup must preserve another original source even with its same producer ID");}
 if(run("foreign"))for(uint32 source:{0u,3u})for(uint32 holder:{1u,4u}){PetBattle b;TypeSetup(b);Require(b.AddAura(source,holder,1718,2,1,3202,0),"Stored Discharge holder setup failed");auto a=Aura(b,holder,1718);a->Process(&b);Require(Aura(b,holder,1419)&&b.Pets[holder]->States[124]==5,"Discharge periodic mark must use its actual reserve holder, not source or front");b.Pets[source]->States[BATTLEPET_STATE_Stat_Accuracy]=-100;b.Pets[holder]->States[BATTLEPET_STATE_untargettable]=1;a->Expire(&b);Require(!Aura(b,holder,1419)&&b.Pets[holder]->States[124]==0,"Foreign-source Discharge cleanup must reach the actual holder");}
 if(run("miss"))for(uint32 source:{0u,3u}){PetBattle b;TypeSetup(b);uint32 enemy=source?0:3;int32 hp=b.Pets[enemy]->Health;b.Pets[source]->States[BATTLEPET_STATE_Stat_Accuracy]=-100;Discharge(b,source);Require(!Aura(b,source,1718)&&!Aura(b,source,1419)&&b.Pets[source]->States[124]==0&&b.Pets[enemy]->Health==hp,"Failed native Discharge hit and parent must not create an orphan mark");}
 if(run("kill"))for(uint32 source:{0u,3u}){PetBattle b;TypeSetup(b);uint32 enemy=source?0:3;b.Pets[enemy]->Health=1;b.Pets[source]->Abilities[0]=1717;b.Pets[source]->Cooldowns[0]=-1;Require(b.Cast(source,1717,1,PETBATTLE_ABILITY_TURN0_PROC_ON_NONE,PETBATTLE_CAST_TRIGGER_NONE)==PETBATTLE_CAST_OK,"Lethal Discharge root failed");Require(!b.Pets[enemy]->IsAlive()&&Aura(b,source,1718)&&Aura(b,source,1419)&&b.Pets[source]->States[124]==5,"A lethal Discharge must retain its authored self-type cost for the next enemy");Require(b.Pets[source]->Cooldowns[0]==5,"Lethal Discharge must retain its native cooldown");}
 if(run("death"))for(uint32 source:{0u,3u}){PetBattle b;TypeSetup(b);Discharge(b,source);TypeKill(b,source);Require(!b.Pets[source]->IsAlive()&&!Aura(b,source,1718)&&!Aura(b,source,1419)&&b.Pets[source]->States[124]==0,"Final death must remove the Discharge controller and its infinite mark");}
 if(run("refresh"))for(uint32 source:{0u,3u}){PetBattle b;TypeSetup(b);Discharge(b,source);auto old=Aura(b,source,1718);Require(b.AddAura(source,source,1718,2,2,3202,0),"Fresh Discharge controller setup failed");auto fresh=b.PetAuras.back();fresh->Process(&b);old->Expire(&b);Require(Aura(b,source,1419)&&b.Pets[source]->States[124]==5,"Stale Discharge callback must retain a fresh same-source controller mark");fresh->Expire(&b);Require(!Aura(b,source,1419)&&b.Pets[source]->States[124]==0,"Last actual Discharge controller removes its own mark");}
}
int main(int argc,char**argv){try{
 Require(argc==2,"Native26972 data path required");std::string error;auto config=std::getenv("LEGION_DIALOGUE_CONFIG");Require(config&&sConfigMgr->LoadInitial(config,{},error),"Native test configuration unavailable");
 MySQL::Library_Init();HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);Require(!HotfixDatabase.Open()&&HotfixDatabase.PrepareStatements(),"Native readonly hotfix connection failed");HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
 DB2Storage<BattlePetVisualEntry> visuals("BattlePetVisual.db2",BattlePetVisualLoadInfo::Instance());
 Load(sBattlePetAbilityStore,argv[1]);Load(sBattlePetAbilityTurnStore,argv[1]);Load(sBattlePetAbilityEffectStore,argv[1]);Load(sBattlePetAbilityStateStore,argv[1]);Load(sBattlePetStateStore,argv[1]);Load(sBattlePetSpeciesStore,argv[1]);Load(visuals,argv[1]);
 for(uint32 id:{242u,244u,795u,1417u,1419u,968u})for(auto s:sBattlePetAbilityStateStore)if(s->BattlePetAbilityID==id)std::cout<<"NATIVE_STATE "<<id<<" state="<<uint32(s->BattlePetStateID)<<" value="<<s->Value<<"\n";
 for(uint32 id:{500u,1577u,2608u,3449u,3493u,3496u,2831u,2838u}){auto e=sBattlePetAbilityEffectStore.LookupEntry(id);Require(e,"Native lifecycle row missing");auto v=visuals.LookupEntry(e->BattlePetVisualID);std::cout<<"NATIVE_VISUAL "<<id<<" visual="<<e->BattlePetVisualID;if(v)std::cout<<" function="<<v->SceneScriptFunction->Str[0]<<" range="<<uint32(v->RangeTypeEnum)<<" flags="<<uint32(v->Flags);std::cout<<"\n";}
 LoadGameTables(std::string(argv[1])+"../../");
 auto selected=std::getenv("LEGION_PET_DISCHARGE_CASE");Cases(selected?selected:"");
 HotfixDatabase.Close();MySQL::Library_End();std::cout<<"PASS actual native Discharge cases: "<<(selected?selected:"all28")<<"; matching26972 dispatcher, species initialization, health/death/aura/round callbacks; no character data changed\n";return 0;
 }catch(std::exception const&e){std::cerr<<e.what()<<"\n";return 1;}}
