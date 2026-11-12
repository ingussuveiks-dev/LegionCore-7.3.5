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
static uint32 OrdinarySpecies(){for(auto s:sBattlePetSpeciesStore)if(s->PetTypeEnum==BATTLEPET_PETTYPE_HUMANOID){bool passive=false;for(auto st:sBattlePetSpeciesStateStore)if(st->SpeciesID==s->ID&&st->BattlePetStateID==183)passive=true;if(!passive)return s->ID;}throw std::runtime_error("Native ordinary species missing");}
static void RoboSetup(PetBattle&b,uint32 source,uint32 overrideSpecies=1504){b.BattleType=PETBATTLE_TYPE_PVE;b.PveBattleType=PVE_PETBATTLE_TRAINER;uint32 ordinary=OrdinarySpecies();for(uint32 team=0;team<2;++team)for(uint32 slot=0;slot<3;++slot){uint32 id=team*3+slot;auto p=std::make_shared<BattlePetInstance>();p->Slot=slot;p->Species=id==source?overrideSpecies:ordinary;p->Level=25;p->Quality=3;p->Breed=3;p->Health=100000;p->Abilities[0]=id==source?1079:110;p->Abilities[1]=id==source?455:282;p->Abilities[2]=id==source?1025:321;b.AddPet(team,p);Require(b.Pets[id]!=nullptr,"Native AddPet rejected the configured valid slot");}b.Begin();}
static uint32 RoboMarks(PetBattle const&b,uint32 team){uint32 count=0;for(auto a:b.PetAuras)if(!a->Expired&&a->AbilityID==1419&&b.Pets[a->TargetPetID]&&b.Pets[a->TargetPetID]->TeamID==team)++count;return count;}
static void ClearRoboMarks(PetBattle&b,uint32 team){std::vector<PetBattleAura*> list;for(auto a:b.PetAuras)if(!a->Expired&&a->AbilityID==1419&&b.Pets[a->TargetPetID]&&b.Pets[a->TargetPetID]->TeamID==team)list.push_back(a);for(auto a:list)a->Expire(&b);}
static void CheckRobo(PetBattle&b,uint32 source,uint32 trigger,uint32 expected=3){uint32 team=b.Pets[source]->TeamID;Require(RoboMarks(b,team)==expected&&RoboMarks(b,!team)==0,"Native Roboflauge must mark exactly the actual living own roster, without leaking to its opponent");for(auto a:b.PetAuras)if(!a->Expired){Require(a->AbilityID!=1389,"An inherent Roboflauge passive must not fabricate an infinite controller aura");if(a->AbilityID==1419)Require(a->CasterPetID==source&&a->TriggerId==trigger&&a->MaxDuration==1&&b.Pets[a->TargetPetID]->States[124]==5,"Native Roboflauge marks must preserve original source, actual proc receipt and finite category");}}
static void RoboDamage(PetBattle&b,uint32 source){PetBattleAbilityTurn turn{};auto e=Hit(b,turn,source?0:3,source);e.Damage(source,b.Pets[source]->GetMaxHealth()*10);}

static uint32 CrumbleCases=0;
static void CrumbleSetup(PetBattle&b,uint32 source,uint32 species=1811){RoboSetup(b,source,species);for(auto p:b.Pets){p->States[BATTLEPET_STATE_Stat_Power]=0;p->States[BATTLEPET_STATE_Stat_CritChance]=0;}}
static PetBattleAura* SelfCrumble(PetBattle&b,uint32 source){auto a=Aura(b,source,1739);if(!a){Require(b.AddAura(source,source,1739,9,0,3226,0),"Native isolated self-controller setup failed");a=Aura(b,source,1739);}Require(a&&a->CasterPetID==source&&a->TargetPetID==source&&a->MaxDuration==9&&a->TriggerId==3226,"Native self Crumbling controller metadata differs");return a;}
static void ApplyCrumble(PetBattle&b,uint32 source){Require(b.Cast(source,1586,0,PETBATTLE_ABILITY_TURN0_PROC_ON_APPLY,PETBATTLE_CAST_TRIGGER_ALL)==PETBATTLE_CAST_OK,"Native Crumble apply dispatcher failed");}
static void OwnFinal(PetBattle&b,uint32 source){Require(!b.Pets[source]->IsAlive(),"Native Crumble terminal expiry must finally kill its own actual source");for(uint32 id=0;id<6;++id)if(id!=source)Require(b.Pets[id]->IsAlive(),"Native Crumble terminal expiry must not kill any opponent or other ally");}
static uint32 DeadEvents(PetBattle&b,uint32 target){uint32 n=0;for(auto const&e:b.RoundEvents)if(e.EventType==PETBATTLE_EVENT_SET_STATE)for(auto const&u:e.Updates)if(u.TargetPetID==target&&u.State.ID==BATTLEPET_STATE_Is_Dead&&u.State.Value==1)++n;return n;}
static void Cases(std::string const&choice){
 Require(choice.empty()||choice=="init"||choice=="round"||choice=="repeat"||choice=="swap"||choice=="early"||choice=="capture"||choice=="terminal"||choice=="authority"||choice=="owner"||choice=="active"||choice=="dead"||choice=="ordinary"||choice=="bounds","Unknown Crumble case");auto run=[&](char const*c){return choice.empty()||choice==c;};
 for(uint32 source=0;source<6;++source){uint32 team=source/3,enemy=team?0:3;
  if(run("init")){PetBattle b;CrumbleSetup(b,source);auto a=Aura(b,source,1739);Require(a&&a->CasterPetID==source&&a->TargetPetID==source&&a->Duration==9&&a->MaxDuration==9&&a->TriggerId==3226,"Native Rocko Begin must issue exactly its own authored D9 Crumbling controller before the initial snapshot");uint32 count=0;for(auto v:b.PetAuras)if(v->AbilityID==1739&&!v->Expired)++count;Require(count==1&&b.Pets[source]->States[183]==1586&&b.Pets[source]->States[24]==-100,"Only the actual native source owns one controller and its unchanged native immunity state");++CrumbleCases;}
  if(run("round")){PetBattle b;CrumbleSetup(b,source);auto a=Aura(b,source,1739);Require(a,"Native round sequence requires actual Begin producer");for(uint32 round=1;round<=10;++round){TypeEnd(b);if(round<10)Require(b.Pets[source]->IsAlive()&&!a->Expired&&a->Duration==9-int32(round),"Native D9 Crumbling must retain its original nine completed processing phases");else{OwnFinal(b,source);Require(a->Expired&&a->Duration==0&&DeadEvents(b,source)==0,"Native completed round clears its current events after one expiry");}}++CrumbleCases;}
  if(run("repeat")){PetBattle b;CrumbleSetup(b,source);auto a=SelfCrumble(b,source);TypeEnd(b);TypeEnd(b);Require(a->Duration==7,"Actual prior round phases missing");auto count=b.PetAuras.size(),events=b.RoundEvents.size();ApplyCrumble(b,source);Require(a->Duration==7&&b.PetAuras.size()==count&&b.RoundEvents.size()==events,"Repeated inherent Crumble activation cannot reset or duplicate its original countdown");++CrumbleCases;}
  if(run("swap")){PetBattle b;CrumbleSetup(b,source);auto a=SelfCrumble(b,source);uint32 other=team*3+(source%3+1)%3;b.SwapPet(team,other);for(uint32 i=0;i<3;++i)TypeEnd(b);Require(a->Duration==6,"Real swap cannot refresh a source-bound countdown");b.SwapPet(team,source);for(uint32 i=0;i<7;++i)TypeEnd(b);OwnFinal(b,source);++CrumbleCases;}
  if(run("early")){PetBattle b;CrumbleSetup(b,source);auto a=SelfCrumble(b,source);PetBattleAbilityTurn turn{};auto e=Hit(b,turn,enemy,source);e.Kill(source);Require(!b.Pets[source]->IsAlive()&&b.Pets[enemy]->IsAlive()&&b.PetXDied.size()==1&&b.PetXDied[0]==source,"A real early death must cancel only its own Crumbling without reentrant extra deaths");Require(a->Expired&&DeadEvents(b,source)==1,"Actual early death must retain exactly one native dead-state receipt and ordinary aura cleanup");++CrumbleCases;}
  if(run("capture")){PetBattle b;CrumbleSetup(b,source);auto a=SelfCrumble(b,source);b.Catch(enemy,source,3482);Require(!b.Pets[source]->IsAlive()&&b.Pets[source]->Captured&&b.Pets[source]->Caged&&b.Pets[enemy]->IsAlive()&&b.PetXDied.size()==1&&b.PetXDied[0]==source&&a->Expired,"Actual generic capture endpoint cannot recursively execute the dying source's Crumbling kill row");++CrumbleCases;}
  if(run("terminal")){PetBattle b;CrumbleSetup(b,source);auto a=SelfCrumble(b,source);a->Expire(&b);OwnFinal(b,source);Require(b.PetXDied.size()==1&&b.PetXDied[0]==source&&DeadEvents(b,source)==1,"Native Crumbling removal must create exactly one actual source death");auto events=b.RoundEvents.size();a->Expire(&b);Require(b.RoundEvents.size()==events&&b.PetXDied.size()==1,"Repeated controller removal cannot replay its terminal phase");++CrumbleCases;}
  if(run("authority")){PetBattle b;CrumbleSetup(b,source);auto a=SelfCrumble(b,source);b.Pets[source]->Species=OrdinarySpecies();auto count=b.PetAuras.size();ApplyCrumble(b,source);a->Expire(&b);Require(b.Pets[source]->IsAlive()&&b.Pets[enemy]->IsAlive()&&b.PetAuras.size()==count,"Other species cannot borrow native183=1586 activation or its terminal authority");++CrumbleCases;}
  if(run("owner")){PetBattle b;CrumbleSetup(b,source);SelfCrumble(b,source);b.Cast(source,1739,0,PETBATTLE_ABILITY_TURN0_PROC_ON_AURA_REMOVED,PETBATTLE_CAST_TRIGGER_ALL,enemy);Require(b.Pets[source]->IsAlive()&&b.Pets[enemy]->IsAlive()&&Aura(b,source,1739),"An unrelated aura-owner context cannot issue the native source's terminal death");++CrumbleCases;}
  if(run("active")){PetBattle b;CrumbleSetup(b,source);SelfCrumble(b,source);b.Cast(source,1739,0,PETBATTLE_ABILITY_TURN0_PROC_ON_AURA_REMOVED,PETBATTLE_CAST_TRIGGER_ALL,source);Require(b.Pets[source]->IsAlive()&&b.Pets[enemy]->IsAlive()&&Aura(b,source,1739),"An unexpired issued countdown cannot authorize a forged terminal callback");++CrumbleCases;}
  if(run("dead")){PetBattle b;CrumbleSetup(b,source);SelfCrumble(b,source);PetBattleAbilityTurn turn{};auto e=Hit(b,turn,enemy,source);e.Kill(source);auto count=b.PetAuras.size(),deaths=b.PetXDied.size();ApplyCrumble(b,source);b.Cast(source,1739,0,PETBATTLE_ABILITY_TURN0_PROC_ON_AURA_REMOVED,PETBATTLE_CAST_TRIGGER_ALL,source);Require(!b.Pets[source]->IsAlive()&&b.Pets[enemy]->IsAlive()&&b.PetAuras.size()==count&&b.PetXDied.size()==deaths&&!Aura(b,source,1739),"A final-dead source cannot restart or replay its inherent controller");++CrumbleCases;}
  if(run("ordinary")){PetBattle b;CrumbleSetup(b,source,OrdinarySpecies());ApplyCrumble(b,source);Require(!Aura(b,source,1739),"An ordinary source cannot acquire the native Rocko controller");for(uint32 id=0;id<6;++id)Require(b.Pets[id]->IsAlive(),"Ordinary source admission cannot kill any actor");++CrumbleCases;}
 }
 if(run("bounds")){PetBattle b;CrumbleSetup(b,0);Require(b.Cast(6,1586,0,PETBATTLE_ABILITY_TURN0_PROC_ON_APPLY,PETBATTLE_CAST_TRIGGER_ALL)==PETBATTLE_CAST_INTERNAL_ERROR,"Invalid source ID must be rejected before its indexed read");++CrumbleCases;auto p=b.Pets[1];b.Pets[1].reset();Require(b.Cast(1,1586,0,PETBATTLE_ABILITY_TURN0_PROC_ON_APPLY,PETBATTLE_CAST_TRIGGER_ALL)==PETBATTLE_CAST_INTERNAL_ERROR,"Missing source must fail ordinary Cast admission");b.Pets[1]=p;++CrumbleCases;auto team=b.Teams[0];b.Teams[0]=nullptr;ApplyCrumble(b,0);b.Teams[0]=team;Require(b.Pets[0]->IsAlive(),"Missing actual roster cannot issue a producer");++CrumbleCases;b.Pets[0]->States[183]=1390;auto count=b.PetAuras.size();ApplyCrumble(b,0);Require(b.PetAuras.size()==count,"Mismatched native183 binding cannot issue Crumbling");++CrumbleCases;}
 Require(!choice.empty()||CrumbleCases==76,"Native Crumble inventory must execute all76 cases");
}
int main(int argc,char**argv){try{
 Require(argc==2,"Native26972 data path required");std::string error;auto config=std::getenv("LEGION_DIALOGUE_CONFIG");Require(config&&sConfigMgr->LoadInitial(config,{},error),"Native test configuration unavailable");
 MySQL::Library_Init();HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);Require(!HotfixDatabase.Open()&&HotfixDatabase.PrepareStatements(),"Native readonly hotfix connection failed");HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
 DB2Storage<BattlePetVisualEntry> visuals("BattlePetVisual.db2",BattlePetVisualLoadInfo::Instance());
 Load(sBattlePetSpeciesStateStore,argv[1]);Load(sBattlePetBreedStateStore,argv[1]);Load(sBattlePetBreedQualityStore,argv[1]);Load(sBattlePetAbilityStore,argv[1]);Load(sBattlePetAbilityTurnStore,argv[1]);Load(sBattlePetAbilityEffectStore,argv[1]);Load(sBattlePetAbilityStateStore,argv[1]);Load(sBattlePetStateStore,argv[1]);Load(sBattlePetSpeciesStore,argv[1]);Load(visuals,argv[1]);
 for(uint32 id:{242u,244u,795u,1417u,1419u,968u})for(auto s:sBattlePetAbilityStateStore)if(s->BattlePetAbilityID==id)std::cout<<"NATIVE_STATE "<<id<<" state="<<uint32(s->BattlePetStateID)<<" value="<<s->Value<<"\n";
 for(uint32 id:{500u,1577u,2608u,3449u,3493u,3496u,2831u,2838u}){auto e=sBattlePetAbilityEffectStore.LookupEntry(id);Require(e,"Native lifecycle row missing");auto v=visuals.LookupEntry(e->BattlePetVisualID);std::cout<<"NATIVE_VISUAL "<<id<<" visual="<<e->BattlePetVisualID;if(v)std::cout<<" function="<<v->SceneScriptFunction->Str[0]<<" range="<<uint32(v->RangeTypeEnum)<<" flags="<<uint32(v->Flags);std::cout<<"\n";}
 LoadGameTables(std::string(argv[1])+"../../");
 auto selected=std::getenv("LEGION_PET_CRUMBLE_CASE");Cases(selected?selected:"");
 HotfixDatabase.Close();MySQL::Library_End();std::cout<<"PASS actual native Crumble lifecycle cases:"<<(selected?selected:"all76")<<" executed="<<CrumbleCases<<"; matching26972 producer,aura ownership,native10-round expiry,death and cleanup;immunity calculation and client traversal separate\n";return 0;
 }catch(std::exception const&e){std::cerr<<e.what()<<"\n";return 1;}}
