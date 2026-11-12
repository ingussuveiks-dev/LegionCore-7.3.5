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
static void Cases(std::string const&choice){Require(choice.empty()||choice=="begin"||choice=="rounds"||choice=="swaps"||choice=="original"||choice=="missing"||choice=="miss"||choice=="guards"||choice=="death"||choice=="mechanical"||choice=="other","Unknown native Roboflauge group");auto run=[&](char const*s){return choice.empty()||choice==s;};
 if(run("begin"))for(uint32 source:{0u,1u,3u,4u}){PetBattle b;RoboSetup(b,source);Require(b.Pets[source]->Species==1504&&b.Pets[source]->States[183]==1389,"Actual AddPet must read the matching species183 passive binding");Require(RoboMarks(b,b.Pets[source]->TeamID)==3,"Native AddPet and Begin must activate Roboflauge from the real species183 source before first-round snapshots");CheckRobo(b,source,2842);Require(b.Pets[source]->Abilities[0]==1079&&b.Pets[source]->Abilities[1]==455&&b.Pets[source]->Abilities[2]==1025,"Native passive cannot replace its live99505 active kit");ClearRoboMarks(b,b.Pets[source]->TeamID);Require(b.Cast(source,1389,1,PETBATTLE_ABILITY_TURN0_PROC_ON_NONE,PETBATTLE_CAST_TRIGGER_ALL)==PETBATTLE_CAST_OK,"Authorized native passive root1 failed");CheckRobo(b,source,2839);}
 if(run("rounds"))for(uint32 source:{0u,1u,3u,4u}){PetBattle b;RoboSetup(b,source);for(uint32 round=0;round<5;++round){TypeEnd(b);CheckRobo(b,source,2843);Require(b.Pets[source]->Cooldowns[0]==-1&&b.Pets[source]->Cooldowns[1]==-1&&b.Pets[source]->Cooldowns[2]==-1,"Passive round refresh must not consume an active ability or cooldown");}}
 if(run("swaps"))for(uint32 source:{0u,1u,3u,4u}){PetBattle b;RoboSetup(b,source);uint32 team=b.Pets[source]->TeamID,incoming=team*3+2;ClearRoboMarks(b,team);bool admitted=b.Teams[team]->CanSwap(incoming);auto previous=b.Teams[team]->ActivePetID;b.SwapPet(team,incoming);if(admitted){Require(RoboMarks(b,team)==3,"Native own-team swap must dispatch the issued Roboflauge passive to its living team");CheckRobo(b,source,2840);}else Require(b.Teams[team]->ActivePetID==previous&&RoboMarks(b,team)==0,"A refused native swap cannot refresh the passive");ClearRoboMarks(b,team);b.SwapPet(!team,(!team)*3+1);Require(RoboMarks(b,team)==0,"Opponent swaps cannot trigger another team's native passive");}
 if(run("original"))for(uint32 source:{0u,1u,3u,4u}){PetBattle b;RoboSetup(b,source);uint32 team=b.Pets[source]->TeamID,target=team*3+2,enemy=(!team)*3;auto species=b.Pets[target]->Species;PetBattleAbilityTurn turn{};auto e=Bite(b,turn,enemy,target);b.Pets[enemy]->States[BATTLEPET_STATE_Stat_CritChance]=0;Aura(b,target,1419)->Expire(&b);int32 center=e.CalculateDamage(20,false);Require(b.Cast(source,1389,0,PETBATTLE_ABILITY_TURN0_PROC_ON_ROUND_START,PETBATTLE_CAST_TRIGGER_ALL)==PETBATTLE_CAST_OK,"Actual round refresh failed");e.Flags=0;Require(e.CalculateDamage(20,false)==center+center/2&&b.Pets[target]->Species==species,"Native passive Critter defense must affect actual damage without rewriting species");Require(b.Pets[source]->States[BATTLEPET_STATE_Passive_Mechanical]==1&&!b.Pets[source]->States[BATTLEPET_STATE_Passive_Critter],"Roboflauge cannot replace the source's Mechanical racial with Critter");}
 if(run("missing"))for(uint32 source:{0u,1u,3u,4u}){PetBattle b;RoboSetup(b,source);uint32 team=b.Pets[source]->TeamID,target=team*3+2;ClearRoboMarks(b,team);auto saved=b.Pets[target];b.Pets[target].reset();b.Teams[team]->TeamPets[2].reset();b.Teams[team]->TeamPetCount=99;Require(b.Cast(source,1389,0,PETBATTLE_ABILITY_TURN0_PROC_ON_ROUND_START,PETBATTLE_CAST_TRIGGER_ALL)==PETBATTLE_CAST_OK,"Bounded native passive context failed");CheckRobo(b,source,2843,2);b.Teams[team]->TeamPetCount=3;b.Pets[target]=saved;b.Teams[team]->TeamPets[2]=saved;}
 if(run("miss"))for(uint32 source:{0u,1u,3u,4u}){PetBattle b;RoboSetup(b,source);uint32 team=b.Pets[source]->TeamID,target=team*3+2;ClearRoboMarks(b,team);Require(b.AddAura(source,target,1417,3,1,2833,0),"Unrelated native Elemental mark setup failed");b.Pets[source]->States[BATTLEPET_STATE_Stat_Accuracy]=-100;Require(b.Cast(source,1389,0,PETBATTLE_ABILITY_TURN0_PROC_ON_ROUND_START,PETBATTLE_CAST_TRIGGER_ALL)==PETBATTLE_CAST_OK&&RoboMarks(b,team)==0&&Aura(b,target,1417)&&b.Pets[target]->States[124]==7,"Native229 accuracy failure must preserve an unrelated actual mark");}
 if(run("guards"))for(uint32 source:{0u,1u,3u,4u})for(uint32 variant=0;variant<3;++variant){PetBattle b;RoboSetup(b,source);uint32 team=b.Pets[source]->TeamID;ClearRoboMarks(b,team);if(variant==0)b.Pets[source]->States[183]=0;else if(variant==1)b.Pets[source]->Captured=true;else b.Pets[source]->Species=OrdinarySpecies();Require(b.Cast(source,1389,0,PETBATTLE_ABILITY_TURN0_PROC_ON_ROUND_START,PETBATTLE_CAST_TRIGGER_ALL)==PETBATTLE_CAST_INTERNAL_ERROR,"Unissued/stale/captured native passive context must be rejected");PetBattleAbilityTurn turn{};PetBattleAbilityEffect e{};e.PetBattleInstance=&b;e.AbilityTurn=&turn;e.EffectInfo=sBattlePetAbilityEffectStore.LookupEntry(2843);e.Caster=source;e.AbilityID=1389;e.Targets={team*3+2};Require(!e.Execute()&&RoboMarks(b,team)==0,"Direct effect execution cannot bypass a missing native passive source");}
 if(run("death"))for(uint32 source:{0u,1u,3u,4u}){PetBattle b;RoboSetup(b,source);uint32 team=b.Pets[source]->TeamID;for(uint32 i=0;i<2&&b.Pets[source]->IsAlive();++i)RoboDamage(b,source);Require(!b.Pets[source]->IsAlive(),"Native source must reach actual final death after any Mechanical reaction");if(b.Teams[team]->ActivePetID==source)b.SwapPet(team,team*3+2);TypeEnd(b);TypeEnd(b);Require(RoboMarks(b,team)==0,"A dead native passive source must stop renewing and let its finite friendly marks expire");}
 if(run("mechanical"))for(uint32 source:{0u,1u,3u,4u}){PetBattle b;RoboSetup(b,source);RoboDamage(b,source);Require(b.Pets[source]->IsAlive()&&!b.Pets[source]->States[BATTLEPET_STATE_Passive_Mechanical]&&b.Pets[source]->States[183]==1389,"Native Mechanical source must retain its real one-use revival while defensively Critter");TypeEnd(b);CheckRobo(b,source,2843);}
 if(run("other"))for(uint32 source:{0u,1u,3u,4u}){PetBattle b;RoboSetup(b,source,1259);Require(b.Pets[source]->States[183]==251&&RoboMarks(b,0)==0&&RoboMarks(b,1)==0&&!Aura(b,source,251),"Roboflauge support must not invent activation semantics for another native183 ability");}
}
int main(int argc,char**argv){try{
 Require(argc==2,"Native26972 data path required");std::string error;auto config=std::getenv("LEGION_DIALOGUE_CONFIG");Require(config&&sConfigMgr->LoadInitial(config,{},error),"Native test configuration unavailable");
 MySQL::Library_Init();HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);Require(!HotfixDatabase.Open()&&HotfixDatabase.PrepareStatements(),"Native readonly hotfix connection failed");HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
 DB2Storage<BattlePetVisualEntry> visuals("BattlePetVisual.db2",BattlePetVisualLoadInfo::Instance());
 Load(sBattlePetSpeciesStateStore,argv[1]);Load(sBattlePetBreedStateStore,argv[1]);Load(sBattlePetBreedQualityStore,argv[1]);Load(sBattlePetAbilityStore,argv[1]);Load(sBattlePetAbilityTurnStore,argv[1]);Load(sBattlePetAbilityEffectStore,argv[1]);Load(sBattlePetAbilityStateStore,argv[1]);Load(sBattlePetStateStore,argv[1]);Load(sBattlePetSpeciesStore,argv[1]);Load(visuals,argv[1]);
 for(uint32 id:{242u,244u,795u,1417u,1419u,968u})for(auto s:sBattlePetAbilityStateStore)if(s->BattlePetAbilityID==id)std::cout<<"NATIVE_STATE "<<id<<" state="<<uint32(s->BattlePetStateID)<<" value="<<s->Value<<"\n";
 for(uint32 id:{500u,1577u,2608u,3449u,3493u,3496u,2831u,2838u}){auto e=sBattlePetAbilityEffectStore.LookupEntry(id);Require(e,"Native lifecycle row missing");auto v=visuals.LookupEntry(e->BattlePetVisualID);std::cout<<"NATIVE_VISUAL "<<id<<" visual="<<e->BattlePetVisualID;if(v)std::cout<<" function="<<v->SceneScriptFunction->Str[0]<<" range="<<uint32(v->RangeTypeEnum)<<" flags="<<uint32(v->Flags);std::cout<<"\n";}
 LoadGameTables(std::string(argv[1])+"../../");
 auto selected=std::getenv("LEGION_PET_ROBOFLAUGE_CASE");Cases(selected?selected:"");
 HotfixDatabase.Close();MySQL::Library_End();std::cout<<"PASS actual native Roboflauge cases: "<<(selected?selected:"all48")<<"; matching26972 dispatcher, species initialization, health/death/aura/round callbacks; no character data changed\n";return 0;
 }catch(std::exception const&e){std::cerr<<e.what()<<"\n";return 1;}}
