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
static void HourSetup(PetBattle&b,uint32 caster){Setup(b);SameFamily(b,1927);for(auto p:b.Pets){p->States[BATTLEPET_STATE_Stat_CritChance]=0;p->States[BATTLEPET_STATE_Stat_Power]=0;}b.Pets[caster]->Abilities[0]=1927;b.Pets[caster]->States[BATTLEPET_STATE_Stat_Speed]=10000;}
static void HourRoot(PetBattle&b,uint32 caster){Require(b.Cast(caster,1927,1,PETBATTLE_ABILITY_TURN0_PROC_ON_NONE,PETBATTLE_CAST_TRIGGER_ALL)==PETBATTLE_CAST_OK,"Native Hourglass root failed");}
static void HourEnd(PetBattle&b){for(auto t:b.Teams){t->Ready=true;t->ActiveAbilityId=0;t->activeAbilityTurn=t->activeAbilityTurnMax=1;}b.ProceedRound();}
static uint32 HourCancels(PetBattle&b,uint32 owner){uint32 n=0;for(auto const&e:b.RoundEvents)if(e.EventType==PETBATTLE_EVENT_BUFF_CANCEL)for(auto const&u:e.Updates)if(u.Buff.AbilityID==1932&&u.TargetPetID==owner)++n;return n;}
static void HourController(PetBattle&b,uint32 owner){auto a=Aura(b,owner,1932);Require(a&&a->Duration==2,"Native lethal Hourglass must create its authored1932 controller");}
static void HourOtherReady(PetBattle&b,uint32 owner,int32 expected){for(auto p:b.Teams[b.Pets[owner]->TeamID]->TeamPets)if(p&&p->ID!=owner)Require(CDs(b,p->ID)==std::array<int32,3>{{expected,expected,expected}},"Hourglass must reset both actual other roster pets");}
static void Cases(std::string const&choice){
 Require(choice.empty()||choice=="lethal"||choice=="nonlethal"||choice=="admission"||choice=="racial"||choice=="roster"||choice=="death"||choice=="round"||choice=="foreign"||choice=="cooldowns"||choice=="missing","Unknown Hourglass regression group");auto run=[&](char const*s){return choice.empty()||choice==s;};
 if(run("lethal"))for(uint32 caster:{0u,3u}){PetBattle b;HourSetup(b,caster);uint32 enemy=caster?0:3;b.Pets[enemy]->Health=1;auto own=CDs(b,caster),opponent=CDs(b,enemy);HourRoot(b,caster);HourController(b,caster);Require(CDs(b,caster)==own&&CDs(b,enemy)==opponent,"Hourglass producer must preserve both front cooldowns before the ordinary round tick");Require(CDs(b,caster+1)==std::array<int32,3>{{7,5,3}}&&CDs(b,caster+2)==std::array<int32,3>{{7,5,3}},"Hourglass reset waits for the authored round-end event");HourRoot(b,caster);HourEnd(b);HourOtherReady(b,caster,0);Require(!Aura(b,caster,1932),"Hourglass controller must be consumed once");
 }
 if(run("nonlethal"))for(uint32 caster:{0u,3u})for(bool miss:{false,true}){PetBattle b;HourSetup(b,caster);uint32 enemy=caster?0:3;if(miss)b.Pets[caster]->States[BATTLEPET_STATE_Stat_Accuracy]=-100;HourRoot(b,caster);Require(!Aura(b,caster,1932)&&b.Pets[enemy]->IsAlive(),"Nonlethal or missed Hourglass must not create a reset controller");auto before=CDs(b,caster+1);HourEnd(b);Require(CDs(b,caster+1)==std::array<int32,3>{{before[0]-1,before[1]-1,before[2]-1}},"Nonlethal/miss Hourglass may only apply the ordinary cooldown tick");}
 if(run("admission"))for(uint32 caster:{0u,3u}){PetBattle b;HourSetup(b,caster);uint32 enemy=caster?0:3;b.Pets[enemy]->Health=0;b.Pets[enemy]->States[BATTLEPET_STATE_Is_Dead]=1;HourRoot(b,caster);Require(!Aura(b,caster,1932),"An already dead opponent cannot supply a new Hourglass kill");for(int32 owner:{-3,-1,6,int32(caster),int32(caster+1)})Require(b.Cast(caster,1932,0,PETBATTLE_ABILITY_TURN0_PROC_ON_ROUND_END,PETBATTLE_CAST_TRIGGER_ALL,owner)==PETBATTLE_CAST_INTERNAL_ERROR,"Hourglass round-end requires bounded actual owned aura context");Require(b.AddAura(caster,caster,1932,2,1,3483,0),"Owned admission setup failed");Require(b.Cast(enemy,1932,0,PETBATTLE_ABILITY_TURN0_PROC_ON_ROUND_END,PETBATTLE_CAST_TRIGGER_ALL,caster)==PETBATTLE_CAST_INTERNAL_ERROR,"Unissued source cannot reuse an actual Hourglass holder");}
 if(run("racial"))for(uint32 caster:{0u,3u})for(uint32 family:{uint32(BATTLEPET_PETTYPE_MECHANICAL),uint32(BATTLEPET_PETTYPE_UNDEAD)}){PetBattle b;HourSetup(b,caster);uint32 enemy=caster?0:3;Racial(b,enemy,family);b.Pets[enemy]->Health=1;HourRoot(b,caster);Require(b.Pets[enemy]->IsAlive()&&!Aura(b,caster,1932),"An immediate native racial resurrection is not a final Hourglass kill");TriggerReceipt(b,enemy);}
 if(run("roster"))for(uint32 slot:{0u,1u,2u})for(uint32 team:{0u,1u}){PetBattle b;uint32 owner=team*3+slot;HourSetup(b,team*3);Require(b.AddAura(owner,owner,1932,2,1,3483,0),"Native roster controller setup failed");auto own=CDs(b,owner);Require(b.Cast(owner,1932,0,PETBATTLE_ABILITY_TURN0_PROC_ON_ROUND_END,PETBATTLE_CAST_TRIGGER_ALL,owner)==PETBATTLE_CAST_OK,"Actual owned controller dispatch failed");HourOtherReady(b,owner,1);Require(CDs(b,owner)==own&&!Aura(b,owner,1932)&&HourCancels(b,owner)==1,"Actual1932 reset excludes its holder and cleans only its controller once");}
 if(run("death"))for(uint32 owner:{0u,3u}){PetBattle b;HourSetup(b,owner);Require(b.AddAura(owner,owner,1932,2,1,3483,0),"Death retention setup failed");Require(b.AddAura(owner?0:3,owner,379,3,1,1,0),"Poison death control setup failed");PetBattleAbilityTurn turn{};auto hit=Hit(b,turn,owner?0:3,owner);hit.Damage(owner,1000);Require(Aura(b,owner,1932)&&!Aura(b,owner,379),"Hourglass retains its controller after holder death while ordinary Poison expires");HourEnd(b);HourOtherReady(b,owner,0);Require(!Aura(b,owner,1932),"A dead Hourglass holder must still clean its issued controller");}
 if(run("round"))for(uint32 caster:{0u,3u}){PetBattle b;HourSetup(b,caster);uint32 enemy=caster?0:3;b.Pets[enemy]->Health=1;b.Pets[caster]->Cooldowns[0]=0;for(auto t:b.Teams){t->Ready=true;t->ActiveAbilityId=t->ActivePetID==caster?1927:0;t->activeAbilityTurn=t->activeAbilityTurnMax=1;}b.ProceedRound();HourOtherReady(b,caster,0);Require(!b.Pets[enemy]->IsAlive()&&!Aura(b,caster,1932),"Full native Hourglass round must kill, reset both other pets to ready0 and consume its controller");HourEnd(b);Require(CDs(b,caster+1)==std::array<int32,3>{{-1,-1,-1}},"A consumed Hourglass must not replay on the next round");}
 if(run("foreign"))for(uint32 owner:{0u,3u}){PetBattle b;HourSetup(b,owner);uint32 source=owner?0:3;Require(b.AddAura(source,owner,1932,2,1,3483,0),"Native foreign stored context setup failed");b.Pets[owner]->States[BATTLEPET_STATE_untargettable]=1;auto sourceBefore=CDs(b,source);Require(b.Cast(source,1932,0,PETBATTLE_ABILITY_TURN0_PROC_ON_ROUND_END,PETBATTLE_CAST_TRIGGER_ALL,owner)==PETBATTLE_CAST_OK,"Stored foreign source cannot replace its actual owner context");HourOtherReady(b,owner,1);Require(!Aura(b,owner,1932)&&CDs(b,source)==sourceBefore,"Actual holder team, cooldowns and cleanup must not be replaced by stored original source");}
 if(run("cooldowns"))for(uint32 owner:{0u,3u})for(int32 ready:{-1,0,1}){PetBattle b;HourSetup(b,owner);Require(b.AddAura(owner,owner,1932,2,1,3483,0),"Cooldown controller setup failed");auto p=b.Pets[owner+1];p->Abilities[1]=0;p->Cooldowns[1]=8;p->Cooldowns[2]=ready;p->Lockdowns[0]=3;b.Cast(owner,1932,0,PETBATTLE_ABILITY_TURN0_PROC_ON_ROUND_END,PETBATTLE_CAST_TRIGGER_ALL,owner);Require(CDs(b,owner+1)==std::array<int32,3>{{1,8,ready}}&&p->Lockdowns[0]==3,"Hourglass preserves ready/unlearned slots and real lockdowns");Require(CDs(b,owner+2)==std::array<int32,3>{{1,1,1}},"Second other pet keeps its own three native rows");}
 if(run("missing"))for(uint32 owner:{0u,3u})for(bool missing:{false,true}){PetBattle b;HourSetup(b,owner);uint32 team=b.Pets[owner]->TeamID;auto absent=b.Pets[owner+1];if(missing){b.Pets[owner+1].reset();b.Teams[team]->TeamPets[1].reset();}else{absent->Health=0;absent->States[BATTLEPET_STATE_Is_Dead]=1;}Require(b.AddAura(owner,owner,1932,2,1,3483,0),"Missing/dead roster controller setup failed");b.Cast(owner,1932,0,PETBATTLE_ABILITY_TURN0_PROC_ON_ROUND_END,PETBATTLE_CAST_TRIGGER_ALL,owner);Require(CDs(b,owner+2)==std::array<int32,3>{{1,1,1}}&&absent->Cooldowns[0]==7&&absent->Cooldowns[1]==5&&absent->Cooldowns[2]==3&&!Aura(b,owner,1932),"Missing/dead other slot cannot compact native rows, mutate a dead pet or prevent live sibling reset and cleanup");}
}
int main(int argc,char**argv){try{
 Require(argc==2,"Native26972 data path required");std::string error;auto config=std::getenv("LEGION_DIALOGUE_CONFIG");Require(config&&sConfigMgr->LoadInitial(config,{},error),"Native test configuration unavailable");
 MySQL::Library_Init();HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);Require(!HotfixDatabase.Open()&&HotfixDatabase.PrepareStatements(),"Native readonly hotfix connection failed");HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
 DB2Storage<BattlePetVisualEntry> visuals("BattlePetVisual.db2",BattlePetVisualLoadInfo::Instance());
 Load(sBattlePetAbilityStore,argv[1]);Load(sBattlePetAbilityTurnStore,argv[1]);Load(sBattlePetAbilityEffectStore,argv[1]);Load(sBattlePetAbilityStateStore,argv[1]);Load(sBattlePetStateStore,argv[1]);Load(sBattlePetSpeciesStore,argv[1]);Load(visuals,argv[1]);
 for(uint32 id:{242u,244u,795u,1417u,1419u,968u})for(auto s:sBattlePetAbilityStateStore)if(s->BattlePetAbilityID==id)std::cout<<"NATIVE_STATE "<<id<<" state="<<uint32(s->BattlePetStateID)<<" value="<<s->Value<<"\n";
 for(uint32 id:{500u,1577u,2608u,3449u,3493u,3496u,2831u,2838u}){auto e=sBattlePetAbilityEffectStore.LookupEntry(id);Require(e,"Native lifecycle row missing");auto v=visuals.LookupEntry(e->BattlePetVisualID);std::cout<<"NATIVE_VISUAL "<<id<<" visual="<<e->BattlePetVisualID;if(v)std::cout<<" function="<<v->SceneScriptFunction->Str[0]<<" range="<<uint32(v->RangeTypeEnum)<<" flags="<<uint32(v->Flags);std::cout<<"\n";}
 LoadGameTables(std::string(argv[1])+"../../");
 auto selected=std::getenv("LEGION_PET_HOURGLASS_CASE");Cases(selected?selected:"");
 HotfixDatabase.Close();MySQL::Library_End();std::cout<<"PASS actual native Hourglass cases: "<<(selected?selected:"all34")<<"; matching26972 dispatcher, species initialization, health/death/aura/round callbacks; no character data changed\n";return 0;
 }catch(std::exception const&e){std::cerr<<e.what()<<"\n";return 1;}}
