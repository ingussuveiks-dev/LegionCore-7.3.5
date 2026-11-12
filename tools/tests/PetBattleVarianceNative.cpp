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
static PetBattleAbilityEffect Range(PetBattle&b,PetBattleAbilityTurn&turn,uint32 source,uint32 id){auto row=sBattlePetAbilityEffectStore.LookupEntry(id);Require(row&&row->BattlePetEffectPropertiesID==222,"Matching native range effect missing");auto t=sBattlePetAbilityTurnStore.LookupEntry(row->BattlePetAbilityTurnID);Require(t,"Matching native range turn missing");PetBattleAbilityEffect e{};e.EffectInfo=row;e.PetBattleInstance=&b;e.AbilityTurn=&turn;e.Caster=source;e.Target=source?0:3;e.Targets={e.Target};e.AbilityID=t->BattlePetAbilityID;return e;}
static void Neutral(PetBattle&b,uint32 ability){auto a=sBattlePetAbilityStore.LookupEntry(ability);Require(a,"Native range ability missing");for(auto s:sBattlePetSpeciesStore)if(s->PetTypeEnum==a->PetTypeEnum){for(auto p:b.Pets){p->Species=s->ID;p->States[BATTLEPET_STATE_Stat_Stamina]=19998000;p->Health=p->InfoMaxHealth=p->GetMaxHealth();}return;}throw std::runtime_error("Native neutral species missing");}
static void SampleRange(PetBattle&b,PetBattleAbilityEffect&e,uint32 draws){int32 center=e.CalculateDamage(e.EffectInfo->Param[0]);Require(center>0,"Native positive range center required");int64 variance=e.EffectInfo->Param[2];int32 low=int64(center)*(200-variance)/200,high=int64(center)*(200+variance)/200;for(uint32 i=0;i<draws;++i){auto pet=b.Pets[e.Target];pet->Health=pet->GetMaxHealth();int32 before=pet->Health;e.Flags=0;e.StopChain=false;e.AbilityTurn->ChainFailure=0;b.RoundEvents.clear();Require(e.Execute(),"Actual native222 damage execution failed");int32 damage=before-pet->Health;Require(damage>=low&&damage<=high,"Native range damage must use the percentage spread around its modified points, never Variance as an absolute maximum");}}
static void Cases(std::string const&choice){Require(choice.empty()||choice=="control","Unknown native variance group");
 if(choice=="control"){for(uint32 source:{0u,3u}){PetBattle b;TypeSetup(b);PetBattleAbilityTurn turn{};auto e=Range(b,turn,source,2799);Neutral(b,e.AbilityID);SampleRange(b,e,256);}return;}
 uint32 rows=0;for(auto row:sBattlePetAbilityEffectStore)if(row->BattlePetEffectPropertiesID==222){++rows;Require(row->Param[0]>0&&row->Param[1]==100&&row->Param[2]>0&&row->Param[2]<=200&&!row->Param[3]&&!row->Param[4]&&!row->Param[5],"Native222 catalogue changed; review points/accuracy/variance/override metadata before widening scope");for(uint32 source:{0u,3u}){PetBattle b;TypeSetup(b);PetBattleAbilityTurn turn{};auto e=Range(b,turn,source,row->ID);Neutral(b,e.AbilityID);SampleRange(b,e,128);}}Require(rows==63,"Matching26972 must retain all63 native variance rows");
 for(uint32 source:{0u,3u})for(uint32 mode=0;mode<5;++mode){PetBattle b;TypeSetup(b);PetBattleAbilityTurn turn{};auto e=Range(b,turn,source,2799);Neutral(b,e.AbilityID);if(mode==0)b.Pets[source]->States[BATTLEPET_STATE_Stat_Power]=22000;if(mode==1)b.Pets[source]->States[BATTLEPET_STATE_Stat_CritChance]=100;if(mode==2)b.Pets[source]->States[BATTLEPET_STATE_Mod_DamageDealtPercent]=50;if(mode==3)b.Pets[e.Target]->States[BATTLEPET_STATE_Mod_DamageTakenPercent]=-25;if(mode==4){b.Pets[source]->States[BATTLEPET_STATE_Add_FlatDamageDealt]=7;b.Pets[e.Target]->States[BATTLEPET_STATE_Add_FlatDamageTaken]=3;}SampleRange(b,e,128);}
 for(uint32 source:{0u,3u}){PetBattle b;TypeSetup(b);PetBattleAbilityTurn turn{};auto e=Range(b,turn,source,3208);Neutral(b,e.AbilityID);auto target=b.Pets[e.Target];target->States[BATTLEPET_STATE_Stat_Stamina]=0;target->Health=target->InfoMaxHealth=100;b.Pets[source]->States[BATTLEPET_STATE_Stat_Power]=22000;target->States[BATTLEPET_STATE_Passive_Magic]=1;for(uint32 i=0;i<32;++i){target->Health=100;e.Flags=0;Require(e.Execute()&&target->Health==65,"Native variance must precede Magic's final35percent damage cap");b.RoundEvents.clear();}}
 for(uint32 source:{0u,3u})for(bool immune:{false,true}){PetBattle b;TypeSetup(b);PetBattleAbilityTurn turn{};auto e=Range(b,turn,source,3208);Neutral(b,e.AbilityID);int32 hp=b.Pets[e.Target]->Health;if(immune)b.Pets[e.Target]->States[BATTLEPET_STATE_untargettable]=1;else b.Pets[source]->States[BATTLEPET_STATE_Stat_Accuracy]=-100;Require(!e.Execute()&&b.Pets[e.Target]->Health==hp,"Native variance must preserve misses and untargettable damage refusal");}
 for(uint32 source:{0u,3u}){PetBattle b;TypeSetup(b);PetBattleAbilityTurn turn{};auto e=Range(b,turn,source,3208);Neutral(b,e.AbilityID);b.Pets[e.Target]->Health=1;Require(e.Execute()&&!b.Pets[e.Target]->IsAlive()&&b.PetXDied.size()==1&&b.PetXDied[0]==e.Target,"Native variance lethal hit must retain actual single death receipt");}
}
int main(int argc,char**argv){try{
 Require(argc==2,"Native26972 data path required");std::string error;auto config=std::getenv("LEGION_DIALOGUE_CONFIG");Require(config&&sConfigMgr->LoadInitial(config,{},error),"Native test configuration unavailable");
 MySQL::Library_Init();HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);Require(!HotfixDatabase.Open()&&HotfixDatabase.PrepareStatements(),"Native readonly hotfix connection failed");HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
 DB2Storage<BattlePetVisualEntry> visuals("BattlePetVisual.db2",BattlePetVisualLoadInfo::Instance());
 Load(sBattlePetEffectPropertiesStore,argv[1]);Load(sBattlePetAbilityStore,argv[1]);Load(sBattlePetAbilityTurnStore,argv[1]);Load(sBattlePetAbilityEffectStore,argv[1]);Load(sBattlePetAbilityStateStore,argv[1]);Load(sBattlePetStateStore,argv[1]);Load(sBattlePetSpeciesStore,argv[1]);Load(visuals,argv[1]);
 for(uint32 id:{242u,244u,795u,1417u,1419u,968u})for(auto s:sBattlePetAbilityStateStore)if(s->BattlePetAbilityID==id)std::cout<<"NATIVE_STATE "<<id<<" state="<<uint32(s->BattlePetStateID)<<" value="<<s->Value<<"\n";
 for(uint32 id:{500u,1577u,2608u,3449u,3493u,3496u,2831u,2838u}){auto e=sBattlePetAbilityEffectStore.LookupEntry(id);Require(e,"Native lifecycle row missing");auto v=visuals.LookupEntry(e->BattlePetVisualID);std::cout<<"NATIVE_VISUAL "<<id<<" visual="<<e->BattlePetVisualID;if(v)std::cout<<" function="<<v->SceneScriptFunction->Str[0]<<" range="<<uint32(v->RangeTypeEnum)<<" flags="<<uint32(v->Flags);std::cout<<"\n";}
 LoadGameTables(std::string(argv[1])+"../../");
 auto selected=std::getenv("LEGION_PET_VARIANCE_CASE");Cases(selected?selected:"");
 HotfixDatabase.Close();MySQL::Library_End();std::cout<<"PASS actual native percentage variance cases: "<<(selected?selected:"all144")<<"; matching26972 dispatcher, species initialization, health/death/aura/round callbacks; no character data changed\n";return 0;
 }catch(std::exception const&e){std::cerr<<e.what()<<"\n";return 1;}}
