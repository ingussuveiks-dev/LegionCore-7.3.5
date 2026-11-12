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
static void Frenzy(PetBattle&b,uint32 source){Require(b.Cast(source,1398,1,PETBATTLE_ABILITY_TURN0_PROC_ON_NONE,PETBATTLE_CAST_TRIGGER_ALL)==PETBATTLE_CAST_OK,"Actual Feathered Frenzy root failed");}
static void Cases(std::string const&choice){Require(choice.empty()||choice=="roots"||choice=="round"||choice=="miss"||choice=="block"||choice=="kill"||choice=="death"||choice=="refresh"||choice=="overlap"||choice=="parent"||choice=="dead","Unknown Feathered Frenzy group");auto run=[&](char const*s){return choice.empty()||choice==s;};
 if(run("roots"))for(uint32 source:{0u,3u}){PetBattle b;TypeSetup(b);uint32 enemy=source?0:3;int32 hp=b.Pets[enemy]->Health;Frenzy(b,source);auto parent=Aura(b,source,1399);auto mark=Aura(b,source,1420);Require(parent&&parent->Duration==1&&mark&&mark->Duration==1&&mark->TriggerId==2870&&b.Pets[source]->States[124]==3,"Feathered Frenzy must apply its native Flying mark and one-round damage controller to the actual user");Require(b.Pets[source]->States[23]==50&&b.Pets[enemy]->States[124]==0&&hp-b.Pets[enemy]->Health>=17&&hp-b.Pets[enemy]->Health<=32,"Frenzy native damage range/bonus and unrelated enemy must remain correct");PetBattleAbilityTurn t{};auto incoming=Bite(b,t,enemy,source),outgoing=Bite(b,t,source,enemy);Require(incoming.CalculateDamage(20,false)==14&&outgoing.CalculateDamage(20,false)==30,"Frenzy must apply Flying defense and native50percent damage bonus without changing ability school");}
 if(run("round"))for(uint32 source:{0u,3u}){PetBattle b;TypeSetup(b);b.Pets[source]->Abilities[0]=1398;b.Pets[source]->Cooldowns[0]=-1;for(auto t:b.Teams){t->Ready=true;t->ActiveAbilityId=t==b.Teams[b.Pets[source]->TeamID]?1398:0;t->activeAbilityTurn=t->activeAbilityTurnMax=1;}b.ProceedRound();Require(Aura(b,source,1399)&&Aura(b,source,1420)&&b.Pets[source]->States[124]==3,"Actual learned Frenzy round must retain both native benefits");TypeEnd(b);Require(!Aura(b,source,1399)&&!Aura(b,source,1420)&&b.Pets[source]->States[124]==0&&b.Pets[source]->States[23]==0&&b.Pets[source]->Cooldowns[0]==1,"Native one-round Frenzy benefits expire together and cooldown3 ticks normally");}
 if(run("miss"))for(uint32 source:{0u,3u}){PetBattle b;TypeSetup(b);uint32 enemy=source?0:3;int32 hp=b.Pets[enemy]->Health;b.Pets[source]->States[BATTLEPET_STATE_Stat_Accuracy]=-100;Frenzy(b,source);Require(b.Pets[enemy]->Health==hp&&!Aura(b,source,1399)&&!Aura(b,source,1420)&&b.Pets[source]->States[124]==0,"Native miss must reach StopChain158 before Frenzy benefits");}
 if(run("block"))for(uint32 source:{0u,3u}){PetBattle b;TypeSetup(b);uint32 enemy=source?0:3;int32 hp=b.Pets[enemy]->Health;b.Pets[enemy]->States[BATTLEPET_STATE_Special_BlockedAttackCount]=1;Frenzy(b,source);Require(b.Pets[enemy]->Health==hp&&!Aura(b,source,1399)&&!Aura(b,source,1420)&&b.Pets[source]->States[124]==0,"Native block must reach StopChain158 before Frenzy benefits");}
 if(run("kill"))for(uint32 source:{0u,3u}){PetBattle b;TypeSetup(b);uint32 enemy=source?0:3;b.Pets[enemy]->Health=1;b.Pets[source]->Abilities[0]=1398;b.Pets[source]->Cooldowns[0]=-1;Require(b.Cast(source,1398,1,PETBATTLE_ABILITY_TURN0_PROC_ON_NONE,PETBATTLE_CAST_TRIGGER_NONE)==PETBATTLE_CAST_OK,"Lethal Frenzy root failed");Require(!b.Pets[enemy]->IsAlive()&&Aura(b,source,1399)&&Aura(b,source,1420)&&b.Pets[source]->States[124]==3&&b.Pets[source]->Cooldowns[0]==3,"A lethal Frenzy must retain its authored self bonuses and native cooldown for the next enemy");}
 if(run("death"))for(uint32 source:{0u,3u}){PetBattle b;TypeSetup(b);Frenzy(b,source);TypeKill(b,source);Require(!b.Pets[source]->IsAlive()&&!Aura(b,source,1399)&&!Aura(b,source,1420)&&b.Pets[source]->States[124]==0&&b.Pets[source]->States[23]==0,"Final death must remove both actual Frenzy benefits");}
 if(run("refresh"))for(uint32 source:{0u,3u}){PetBattle b;TypeSetup(b);Frenzy(b,source);auto old=Aura(b,source,1399);Frenzy(b,source);Require(old->Expired&&Aura(b,source,1399)&&Aura(b,source,1420)&&b.Pets[source]->States[23]==50&&b.Pets[source]->States[124]==3,"Repeated Frenzy replaces rather than stacks its own two native benefits");TypeEnd(b);TypeEnd(b);Require(!Aura(b,source,1399)&&!Aura(b,source,1420)&&b.Pets[source]->States[124]==0&&b.Pets[source]->States[23]==0,"Refreshed native benefits retain finite cleanup");}
 if(run("overlap"))for(uint32 source:{0u,3u})for(bool olderFirst:{false,true}){PetBattle b;TypeSetup(b);Require(b.AddAura(source,source,1419,3,1,2839,0),"Prior Critter mark setup failed");auto old=Aura(b,source,1419);Frenzy(b,source);auto flying=Aura(b,source,1420);Require(flying&&b.Pets[source]->States[124]==3,"Frenzy selects its actual latest Flying category");if(olderFirst){old->Expire(&b);Require(b.Pets[source]->States[124]==3,"Removing older Critter cannot erase live Frenzy Flying");flying->Expire(&b);}else{flying->Expire(&b);Require(b.Pets[source]->States[124]==5,"Frenzy removal must restore an unrelated surviving native Critter mark");old->Expire(&b);}Require(b.Pets[source]->States[124]==0,"Removing both categorical receipts restores original family");}
 if(run("parent"))for(uint32 source:{0u,3u})for(bool foreign:{false,true}){PetBattle b;TypeSetup(b);if(foreign)Require(b.AddAura(source?0:3,source,1399,1,1,2845,0),"Foreign Frenzy parent setup failed");PetBattleAbilityTurn t{};PetBattleAbilityEffect e{};e.EffectInfo=sBattlePetAbilityEffectStore.LookupEntry(2870);e.AbilityTurn=&t;e.PetBattleInstance=&b;e.AbilityID=1398;e.Caster=source;e.SelectTargets();Require(!e.Execute()&&!Aura(b,source,1420)&&b.Pets[source]->States[124]==0,"Missing or foreign-source Frenzy controller must reject an orphan type mark");}
 if(run("dead"))for(uint32 source:{0u,3u}){PetBattle b;TypeSetup(b);TypeKill(b,source);b.RoundResult=PETBATTLE_ROUND_RESULT_NONE;Require(b.Cast(source,1398,1,PETBATTLE_ABILITY_TURN0_PROC_ON_NONE,PETBATTLE_CAST_TRIGGER_ALL)==PETBATTLE_CAST_INTERNAL_ERROR&&!Aura(b,source,1399)&&!Aura(b,source,1420),"A dead caster cannot issue the Frenzy self-benefit continuation");}
}
int main(int argc,char**argv){try{
 Require(argc==2,"Native26972 data path required");std::string error;auto config=std::getenv("LEGION_DIALOGUE_CONFIG");Require(config&&sConfigMgr->LoadInitial(config,{},error),"Native test configuration unavailable");
 MySQL::Library_Init();HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);Require(!HotfixDatabase.Open()&&HotfixDatabase.PrepareStatements(),"Native readonly hotfix connection failed");HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
 DB2Storage<BattlePetVisualEntry> visuals("BattlePetVisual.db2",BattlePetVisualLoadInfo::Instance());
 Load(sBattlePetAbilityStore,argv[1]);Load(sBattlePetAbilityTurnStore,argv[1]);Load(sBattlePetAbilityEffectStore,argv[1]);Load(sBattlePetAbilityStateStore,argv[1]);Load(sBattlePetStateStore,argv[1]);Load(sBattlePetSpeciesStore,argv[1]);Load(visuals,argv[1]);
 for(uint32 id:{242u,244u,795u,1417u,1419u,968u})for(auto s:sBattlePetAbilityStateStore)if(s->BattlePetAbilityID==id)std::cout<<"NATIVE_STATE "<<id<<" state="<<uint32(s->BattlePetStateID)<<" value="<<s->Value<<"\n";
 for(uint32 id:{500u,1577u,2608u,3449u,3493u,3496u,2831u,2838u}){auto e=sBattlePetAbilityEffectStore.LookupEntry(id);Require(e,"Native lifecycle row missing");auto v=visuals.LookupEntry(e->BattlePetVisualID);std::cout<<"NATIVE_VISUAL "<<id<<" visual="<<e->BattlePetVisualID;if(v)std::cout<<" function="<<v->SceneScriptFunction->Str[0]<<" range="<<uint32(v->RangeTypeEnum)<<" flags="<<uint32(v->Flags);std::cout<<"\n";}
 LoadGameTables(std::string(argv[1])+"../../");
 auto selected=std::getenv("LEGION_PET_FEATHERED_FRENZY_CASE");Cases(selected?selected:"");
 HotfixDatabase.Close();MySQL::Library_End();std::cout<<"PASS actual native FeatheredFrenzy cases: "<<(selected?selected:"all24")<<"; matching26972 dispatcher, species initialization, health/death/aura/round callbacks; no character data changed\n";return 0;
 }catch(std::exception const&e){std::cerr<<e.what()<<"\n";return 1;}}
