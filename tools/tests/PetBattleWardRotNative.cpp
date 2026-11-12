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
static void Cases(std::string const&choice){
 Require(choice.empty()||choice=="roots"||choice=="damage"||choice=="overlap"||choice=="reset"||choice=="round"||choice=="cleanup"||choice=="receipts"||choice=="foreign"||choice=="guards"||choice=="racial"||choice=="miss"||choice=="death","Unknown Ward/Rot group");auto run=[&](char const*s){return choice.empty()||choice==s;};
 if(run("roots"))for(uint32 root:{574u,476u})for(uint32 source:{0u,3u}){PetBattle b;TypeSetup(b);uint32 owner=Holder(root,source);TypeRoot(b,root,source);Require(Aura(b,owner,Parent(root))&&Aura(b,owner,Mark(root))&&b.Pets[owner]->States[124]==Type(root),"Native Ward/Rot root must create its real controller and defensive mark on the actual holder");Require(Aura(b,owner,Mark(root))->Duration==1,"Native defensive mark duration must stay one");for(auto p:b.Pets)if(p->ID!=owner)Require(p->States[124]==0,"Ward/Rot cannot transform an unrelated pet");}
 if(run("damage"))for(uint32 source:{0u,3u})for(uint32 mark:{968u,1417u,1419u,1420u}){PetBattle b;TypeSetup(b);uint32 owner=source?0:3;auto species=b.Pets[owner]->Species;PetBattleAbilityTurn turn{};auto bite=Bite(b,turn,source,owner);Require(bite.CalculateDamage(20,false)==20,"Native neutral Bite damage baseline changed");Require(b.AddAura(source,owner,mark,2,1,2833,0),"Native defensive mark setup failed");int32 expected=mark==1419?30:mark==1420?14:20;Require(bite.CalculateDamage(20,false)==expected,"Native defensive mark must change actual family damage without changing species");Require(b.Pets[owner]->Species==species,"Defensive mark cannot rewrite species");Aura(b,owner,mark)->Expire(&b);Require(bite.CalculateDamage(20,false)==20&&b.Pets[owner]->States[124]==0,"Removing a defensive mark restores actual native family damage");}
 if(run("overlap"))for(uint32 source:{0u,3u})for(bool olderFirst:{false,true}){PetBattle b;TypeSetup(b);uint32 owner=source?0:3;Require(b.AddAura(source,owner,1419,3,1,2839,0),"Critter overlap setup failed");auto old=Aura(b,owner,1419);Require(b.AddAura(source,owner,1417,2,1,2833,0),"Elemental overlap setup failed");auto newest=Aura(b,owner,1417);Require(b.Pets[owner]->States[124]==7,"Categorical type IDs cannot be added; actual last mark must select one type");if(olderFirst){old->Expire(&b);Require(b.Pets[owner]->States[124]==7,"Removing older mark cannot subtract from actual newer type");newest->Expire(&b);}else{newest->Expire(&b);Require(b.Pets[owner]->States[124]==5,"Removing newer mark restores the remaining real mark");old->Expire(&b);}Require(b.Pets[owner]->States[124]==0,"Final mark removal restores normal species type");auto events=b.RoundEvents.size();old->Expire(&b);newest->Expire(&b);Require(b.RoundEvents.size()==events,"Repeated mark cleanup stays idempotent");}
 if(run("reset"))for(uint32 source:{0u,3u}){PetBattle b;TypeSetup(b);uint32 other=source?0:3;Require(b.AddAura(other,other,1419,3,1,2839,0),"Unrelated active mark setup failed");TypeRoot(b,574,source);Require(Aura(b,other,1419)&&b.Pets[other]->States[124]==5,"Ward native state reset cannot erase another pet's active type mark");}
 if(run("round"))for(uint32 root:{574u,476u})for(uint32 source:{0u,3u}){PetBattle b;TypeSetup(b);uint32 owner=Holder(root,source);b.Pets[source]->Abilities[0]=root;b.Pets[source]->Cooldowns[0]=0;b.Pets[source]->States[BATTLEPET_STATE_Stat_Speed]=10000;for(auto t:b.Teams){t->Ready=true;t->ActiveAbilityId=t->ActivePetID==source?root:0;t->activeAbilityTurn=t->activeAbilityTurnMax=1;}b.ProceedRound();for(uint32 phase=1;phase<5;++phase){Require(Aura(b,owner,Parent(root))&&Aura(b,owner,Mark(root))&&b.Pets[owner]->States[124]==Type(root),"Real rounds must renew native one-round mark while its five-phase parent lives");TypeEnd(b);}Require(Aura(b,owner,Parent(root)),"Native parent retains its fifth processing phase");TypeEnd(b);Require(!Aura(b,owner,Parent(root))&&!Aura(b,owner,Mark(root))&&b.Pets[owner]->States[124]==0,"Full native parent expiry must clean its final defensive mark");}
 if(run("cleanup"))for(uint32 root:{574u,476u})for(uint32 source:{0u,3u}){PetBattle b;TypeSetup(b);uint32 owner=Holder(root,source);TypeRoot(b,root,source);b.Pets[source]->States[BATTLEPET_STATE_Stat_Accuracy]=-100;b.Pets[owner]->States[BATTLEPET_STATE_untargettable]=1;Aura(b,owner,Parent(root))->Expire(&b);Require(!Aura(b,owner,Mark(root))&&b.Pets[owner]->States[124]==0,"Authored Chance100 holder cleanup cannot miss or use hostile-target gating");}
 if(run("receipts"))for(uint32 source:{0u,3u})for(uint32 variant:{0u,1u,2u,3u}){PetBattle b;TypeSetup(b);TypeRoot(b,574,source);if(variant<2){uint32 markerSource=variant==0?source:(source?0:3),effect=variant==0?3064:2833;Require(b.AddAura(markerSource,source,1417,2,1,effect,0),"Foreign mark receipt setup failed");auto other=Aura(b,source,1417);Aura(b,source,820)->Expire(&b);Require(!other->Expired&&b.Pets[source]->States[124]==7,"Controller cleanup cannot remove another producer/source mark");}else if(variant==2){auto old=Aura(b,source,820);old->Expire(&b);TypeRoot(b,574,source);Require(b.Cast(source,820,0,PETBATTLE_ABILITY_TURN0_PROC_ON_AURA_REMOVED,PETBATTLE_CAST_TRIGGER_ALL,source)==PETBATTLE_CAST_OK,"Old cleanup context failed");Require(Aura(b,source,1417)&&b.Pets[source]->States[124]==7,"Old callback cannot remove the new active controller's mark");}else{Require(b.AddAura(source,source,1420,2,1,2870,0),"Unrelated flying mark setup failed");Aura(b,source,820)->Expire(&b);Require(!Aura(b,source,1417)&&Aura(b,source,1420)&&b.Pets[source]->States[124]==3,"Ward cleanup retains another type and its real state");}}
 if(run("foreign"))for(uint32 source:{0u,3u})for(uint32 controller:{820u,1418u}){PetBattle b;TypeSetup(b);uint32 owner=controller==820?source+1:(source?1:4);Require(b.AddAura(source,owner,controller,3,1,controller==820?2832:2830,0),"Stored-holder controller setup failed");TypeEnd(b);Require(b.Pets[owner]->States[124]==(controller==820?7:4)&&Aura(b,owner,controller==820?1417:968),"Periodic/start controller must address its actual stored holder, including a reserve");Require(b.Pets[source]->States[124]==0&&b.Pets[owner]->Species==b.Pets[source]->Species,"Stored source cannot replace the holder or its species");}
 if(run("guards"))for(uint32 source:{0u,3u}){PetBattle b;TypeSetup(b);for(int32 owner:{-3,-1,6,int32(source),int32(source+1)})Require(b.Cast(source,1418,0,PETBATTLE_ABILITY_TURN0_PROC_ON_ROUND_START,PETBATTLE_CAST_TRIGGER_ALL,owner)==PETBATTLE_CAST_INTERNAL_ERROR,"Rot start requires a bounded real issued source/holder controller");uint32 owner=source?0:3;Require(b.AddAura(source,owner,1418,5,1,2830,0),"Owned Rot context setup failed");Require(b.Cast(owner,1418,0,PETBATTLE_ABILITY_TURN0_PROC_ON_ROUND_START,PETBATTLE_CAST_TRIGGER_ALL,owner)==PETBATTLE_CAST_INTERNAL_ERROR,"Unissued source cannot borrow Rot controller");}
 if(run("racial"))for(uint32 source:{0u,3u}){PetBattle b;TypeSetup(b);uint32 owner=source?0:3;Racial(b,owner,BATTLEPET_PETTYPE_MECHANICAL);auto species=b.Pets[owner]->Species;Require(b.AddAura(source,owner,1419,2,1,2839,0),"Mechanical mark setup failed");Require(!b.Pets[owner]->States[BATTLEPET_STATE_Passive_Critter]&&b.Pets[owner]->States[BATTLEPET_STATE_Passive_Mechanical],"Defensive Critter type cannot replace original racial passives");TypeKill(b,owner);Require(b.Pets[owner]->IsAlive()&&b.Pets[owner]->Health==20&&b.Pets[owner]->Species==species&&b.Pets[owner]->States[124]==5,"Original immediate Mechanical reaction survives a different defensive type");}
 if(run("miss"))for(uint32 root:{574u,476u})for(uint32 source:{0u,3u}){PetBattle b;TypeSetup(b);uint32 owner=Holder(root,source);Require(b.AddAura(owner,owner,1419,3,1,2839,0),"Pre-existing missed-root mark setup failed");b.Pets[source]->States[BATTLEPET_STATE_Stat_Accuracy]=-100;TypeRoot(b,root,source);Require(!Aura(b,owner,Parent(root))&&!Aura(b,owner,Mark(root))&&b.Pets[owner]->States[124]==5,"Failed root/parent cannot create a type mark or erase an unrelated active type");}
 if(run("death"))for(uint32 root:{574u,476u})for(uint32 source:{0u,3u}){PetBattle b;TypeSetup(b);uint32 owner=Holder(root,source);TypeRoot(b,root,source);TypeKill(b,owner);Require(!b.Pets[owner]->IsAlive()&&!Aura(b,owner,Parent(root))&&!Aura(b,owner,Mark(root))&&b.Pets[owner]->States[124]==0,"Final holder death cleans its native parent and mark without retaining a defensive type");}
}
int main(int argc,char**argv){try{
 Require(argc==2,"Native26972 data path required");std::string error;auto config=std::getenv("LEGION_DIALOGUE_CONFIG");Require(config&&sConfigMgr->LoadInitial(config,{},error),"Native test configuration unavailable");
 MySQL::Library_Init();HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);Require(!HotfixDatabase.Open()&&HotfixDatabase.PrepareStatements(),"Native readonly hotfix connection failed");HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
 DB2Storage<BattlePetVisualEntry> visuals("BattlePetVisual.db2",BattlePetVisualLoadInfo::Instance());
 Load(sBattlePetAbilityStore,argv[1]);Load(sBattlePetAbilityTurnStore,argv[1]);Load(sBattlePetAbilityEffectStore,argv[1]);Load(sBattlePetAbilityStateStore,argv[1]);Load(sBattlePetStateStore,argv[1]);Load(sBattlePetSpeciesStore,argv[1]);Load(visuals,argv[1]);
 for(uint32 id:{242u,244u,795u,1417u,1419u,968u})for(auto s:sBattlePetAbilityStateStore)if(s->BattlePetAbilityID==id)std::cout<<"NATIVE_STATE "<<id<<" state="<<uint32(s->BattlePetStateID)<<" value="<<s->Value<<"\n";
 for(uint32 id:{500u,1577u,2608u,3449u,3493u,3496u,2831u,2838u}){auto e=sBattlePetAbilityEffectStore.LookupEntry(id);Require(e,"Native lifecycle row missing");auto v=visuals.LookupEntry(e->BattlePetVisualID);std::cout<<"NATIVE_VISUAL "<<id<<" visual="<<e->BattlePetVisualID;if(v)std::cout<<" function="<<v->SceneScriptFunction->Str[0]<<" range="<<uint32(v->RangeTypeEnum)<<" flags="<<uint32(v->Flags);std::cout<<"\n";}
 LoadGameTables(std::string(argv[1])+"../../");
 auto selected=std::getenv("LEGION_PET_WARD_ROT_CASE");Cases(selected?selected:"");
 HotfixDatabase.Close();MySQL::Library_End();std::cout<<"PASS actual native Ward/Rot cases: "<<(selected?selected:"all50")<<"; matching26972 dispatcher, species initialization, health/death/aura/round callbacks; no character data changed\n";return 0;
 }catch(std::exception const&e){std::cerr<<e.what()<<"\n";return 1;}}
