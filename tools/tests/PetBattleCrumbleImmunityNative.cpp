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

static uint32 ImmuneCases=0;
static void ImmuneSetup(PetBattle&b,uint32 target){RoboSetup(b,target,1811);for(auto p:b.Pets){p->States[BATTLEPET_STATE_Stat_Power]=0;p->States[BATTLEPET_STATE_Stat_CritChance]=0;}b.SwapPet(target/3,target);Require(b.Teams[target/3]->ActivePetID==int32(target),"Native actual Rocko front admission failed");Require(Aura(b,target,1739)&&b.Pets[target]->States[24]==-100,"Actual native Crumble source/control setup failed");}
static uint32 Enemy(uint32 target){return target<3?3:0;}
static bool Flag(PetBattle&b,uint32 target,uint32 flag){for(auto const&e:b.RoundEvents)if(e.EventType==PETBATTLE_EVENT_SET_HEALTH&&(e.Flags&flag))for(auto const&u:e.Updates)if(u.TargetPetID==target)return true;return false;}
static void ImmuneBite(PetBattle&b,uint32 target){Require(b.Cast(Enemy(target),110,1,PETBATTLE_ABILITY_TURN0_PROC_ON_NONE,PETBATTLE_CAST_TRIGGER_ALL)==PETBATTLE_CAST_OK,"Actual native ordinary Bite cast failed");}
static void Cases(std::string const&choice){Require(choice.empty()||choice=="crit"||choice=="bonus"||choice=="vulnerable","Unknown Crumble immunity control");
 for(uint32 target=0;target<6;++target){uint32 source=Enemy(target);
  for(uint32 mode=0;mode<8;++mode){if(!choice.empty()&&mode!=(choice=="crit"?1:choice=="bonus"?2:3))continue;PetBattle b;ImmuneSetup(b,target);b.RoundEvents.clear();auto hp=b.Pets[target]->Health;
   if(mode==1||mode==5)b.Pets[source]->States[BATTLEPET_STATE_Stat_CritChance]=100;
   if(mode==2)b.Pets[source]->States[BATTLEPET_STATE_Mod_DamageDealtPercent]=25;
   if(mode==3)b.Pets[target]->States[BATTLEPET_STATE_Mod_DamageTakenPercent]+=50;
   if(mode==4)b.Pets[source]->States[BATTLEPET_STATE_Stat_Power]=10000;
   if(mode==5)b.Pets[target]->States[BATTLEPET_STATE_Add_FlatDamageTaken]=100;
   if(mode<6)ImmuneBite(b,target);
   if(mode==6){b.Pets[source]->States[BATTLEPET_STATE_Stat_CritChance]=100;Require(b.AddAura(source,target,379,3,1,1,0),"Actual native Poison aura setup failed");TypeEnd(b);}
   if(mode==7){Require(b.Cast(source,315,1,PETBATTLE_ABILITY_TURN0_PROC_ON_NONE,PETBATTLE_CAST_TRIGGER_ALL)==PETBATTLE_CAST_OK&&Aura(b,source,316),"Actual Spiked Skin producer failed");b.Pets[target]->Health=1;hp=1;Require(b.Cast(target,1587,1,PETBATTLE_ABILITY_TURN0_PROC_ON_NONE,PETBATTLE_CAST_TRIGGER_ALL)==PETBATTLE_CAST_OK,"Actual Pound/reflection dispatch failed");}
   if(b.Pets[target]->Health!=hp||!b.Pets[target]->IsAlive())std::cerr<<"IMMUNITY_CONTEXT target="<<target<<" mode="<<mode<<" before="<<hp<<" after="<<b.Pets[target]->Health<<" alive="<<b.Pets[target]->IsAlive()<<"\n";
   Require(b.Pets[target]->Health==hp&&b.Pets[target]->IsAlive(),"Actual Crumble ordinary damage immunity must survive critical,power and vulnerability modifiers");
   if(mode!=6)Require(Flag(b,target,PETBATTLE_EVENT_FLAG_IMMUNE),"A successful ordinary attack against native Crumble must report immune without changing health");
   Require(b.PetXDied.empty(),"Native damage immunity cannot create an actual death receipt");++ImmuneCases;
  }
  if(!choice.empty())continue;
  {PetBattle b;ImmuneSetup(b,target);b.RoundEvents.clear();b.Pets[target]->States[BATTLEPET_STATE_Stat_Dodge]=100;b.Pets[source]->States[BATTLEPET_STATE_Stat_CritChance]=100;auto hp=b.Pets[target]->Health;ImmuneBite(b,target);Require(b.Pets[target]->Health==hp&&Flag(b,target,PETBATTLE_EVENT_FLAG_MISS)&&!Flag(b,target,PETBATTLE_EVENT_FLAG_IMMUNE),"An existing failed hit retains its original miss cause instead of gaining immunity");++ImmuneCases;}
  {PetBattle b;ImmuneSetup(b,target);b.Pets[target]->States[BATTLEPET_STATE_Special_BlockedAttackCount]=1;b.Pets[source]->States[BATTLEPET_STATE_Stat_CritChance]=100;auto hp=b.Pets[target]->Health;ImmuneBite(b,target);Require(b.Pets[target]->Health==hp&&b.Pets[target]->States[BATTLEPET_STATE_Special_BlockedAttackCount]==1,"Crumble immunity must precede consumption of a finite native block charge");++ImmuneCases;}
  {PetBattle b;ImmuneSetup(b,target);auto max=b.Pets[target]->Health;b.Pets[target]->Health=max-40;PetBattleAbilityTurn turn{};auto e=Hit(b,turn,source,target);Require(e.Heal(target,20)&&b.Pets[target]->Health==max-20,"Ordinary native healing must remain independent from hostile Crumble damage admission");++ImmuneCases;}
  {PetBattle b;ImmuneSetup(b,target);auto hp=b.Pets[target]->Health;PetBattleAbilityTurn turn{};auto e=Hit(b,turn,target,target);Require(e.Damage(target,20)&&b.Pets[target]->Health==hp-20,"An actual self-owned payment endpoint cannot be converted into hostile immunity");++ImmuneCases;}
  {PetBattle b;ImmuneSetup(b,target);auto hp=b.Pets[target]->Health;PetBattleAbilityTurn turn{};auto e=Hit(b,turn,source,target);Require(e.Damage(target,20,true)&&b.Pets[target]->Health==hp-20,"Explicit unavoidable endpoint semantics remain unchanged;no percentage bypass policy is invented");++ImmuneCases;}
  {PetBattle b;ImmuneSetup(b,target);b.Pets[target]->Species=OrdinarySpecies();auto hp=b.Pets[target]->Health;PetBattleAbilityTurn turn{};auto e=Hit(b,turn,source,target);Require(e.Damage(target,20)&&b.Pets[target]->Health==hp-20,"Unrelated species cannot borrow native Crumble immunity");++ImmuneCases;}
 }
 Require(!choice.empty()||ImmuneCases==84,"Full native Crumble immunity inventory incomplete");
}
int main(int argc,char**argv){try{
 Require(argc==2,"Native26972 data path required");std::string error;auto config=std::getenv("LEGION_DIALOGUE_CONFIG");Require(config&&sConfigMgr->LoadInitial(config,{},error),"Native test configuration unavailable");
 MySQL::Library_Init();HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);Require(!HotfixDatabase.Open()&&HotfixDatabase.PrepareStatements(),"Native readonly hotfix connection failed");HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
 DB2Storage<BattlePetVisualEntry> visuals("BattlePetVisual.db2",BattlePetVisualLoadInfo::Instance());
 Load(sBattlePetSpeciesStateStore,argv[1]);Load(sBattlePetBreedStateStore,argv[1]);Load(sBattlePetBreedQualityStore,argv[1]);Load(sBattlePetAbilityStore,argv[1]);Load(sBattlePetAbilityTurnStore,argv[1]);Load(sBattlePetAbilityEffectStore,argv[1]);Load(sBattlePetAbilityStateStore,argv[1]);Load(sBattlePetStateStore,argv[1]);Load(sBattlePetSpeciesStore,argv[1]);Load(visuals,argv[1]);
 for(uint32 id:{242u,244u,795u,1417u,1419u,968u})for(auto s:sBattlePetAbilityStateStore)if(s->BattlePetAbilityID==id)std::cout<<"NATIVE_STATE "<<id<<" state="<<uint32(s->BattlePetStateID)<<" value="<<s->Value<<"\n";
 for(uint32 id:{500u,1577u,2608u,3449u,3493u,3496u,2831u,2838u}){auto e=sBattlePetAbilityEffectStore.LookupEntry(id);Require(e,"Native lifecycle row missing");auto v=visuals.LookupEntry(e->BattlePetVisualID);std::cout<<"NATIVE_VISUAL "<<id<<" visual="<<e->BattlePetVisualID;if(v)std::cout<<" function="<<v->SceneScriptFunction->Str[0]<<" range="<<uint32(v->RangeTypeEnum)<<" flags="<<uint32(v->Flags);std::cout<<"\n";}
 LoadGameTables(std::string(argv[1])+"../../");
 auto selected=std::getenv("LEGION_PET_CRUMBLE_IMMUNITY_CASE");Cases(selected?selected:"");
 HotfixDatabase.Close();MySQL::Library_End();std::cout<<"PASS actual native Crumble ordinary-immunity cases:"<<(selected?selected:"all84")<<" executed="<<ImmuneCases<<"; real typed damage,critical,buff,vulnerability,poison and reflection;percentage/unavoidable/client policy separate\n";return 0;
 }catch(std::exception const&e){std::cerr<<e.what()<<"\n";return 1;}}
