// Extracted production functions/branches. All data below are synthetic.
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <new>
#include <set>
#include <string>
#include <vector>
using uint8=uint8_t;using uint32=uint32_t;using uint64=uint64_t;using int32=int32_t;using int64=int64_t;struct ObjectGuid{using LowType=uint64;};
#define UI64FMTD "%llu"
#define TC_LOG_ERROR(...) (++diagnostics)
#define ROLLBACK(x) return false
using PetIds=std::map<uint32,uint32>;using PetIdsPair=PetIds::value_type;
int diagnostics=0;struct ObjMgr{uint32 next=2147483648u;uint32 GeneratePetNumber(){return next++;}}objectMgr;auto sObjectMgr=&objectMgr;
struct Field{uint64 value=0;std::string text;uint64 GetUInt64(){return value;}std::string GetString(){return text;}};struct Result{Field f;Field*Fetch(){return &f;}};using QueryResult=std::shared_ptr<Result>;
#include "DumpLow.inc"
#include "DumpStores.inc"
enum{EFFECT_0=0,EFFECT_1=1,ITEM_FIELD_ARTIFACT_XP=0,ITEM_MODIFIER_ARTIFACT_TIER=0,ITEM_CHANGED=1,QUEST_STATUS_INCOMPLETE=1,RATE_HEALTH=0,IN_MILLISECONDS=1000,SPELL_AURA_MOD_HEALTH_REGEN_PERCENT=1,SPELL_AURA_MOD_REGEN=2,SPELL_AURA_MOD_REGEN_DURING_COMBAT=3,SPELL_AURA_MOD_HEALTH_REGEN_IN_COMBAT=4,SPELL_AURA_MOD_RESURRECTED_HEALTH_BY_GUILD_MEMBER=5,POWER_MANA=0,SPELL_AURA_SCHOOL_ABSORB=69,SPELL_CAST_OK=0,TYPEID_PLAYER=4,DUMP_FILE_BROKEN=1,QUEST_AMPHITHEATER_ANGUISH_TUSKARRMAGEDDON=12935,TARGET_UNIT_SRC_AREA_ENEMY=15};using SpellEffIndex=int;using AuraEffectHandleModes=int;
struct ItemDynamicFieldArtifactPowers{uint32 ArtifactPowerId=1,PurchasedRank=5,CurrentRankWithBonus=5;};struct ArtifactPowerEntry{uint32 MaxPurchasableRank=20;};struct ArtifactPowerRankEntry{};struct GtArtifactLevelXPEntry{uint64 XP=100;};struct XPStore{GtArtifactLevelXPEntry row;auto GetRow(uint32){return &row;}}sArtifactLevelXPGameTable;struct PowerStore{ArtifactPowerEntry row;auto AssertEntry(uint32){return &row;}}sArtifactPowerStore;struct DB2{ArtifactPowerRankEntry rank;auto GetArtifactPowerRank(uint32,int){return &rank;}}sDB2Manager;
struct Player;struct Creature;struct Item{uint64 xp=(uint64(1)<<40)+17;uint32 level=36,tier=0;std::vector<ItemDynamicFieldArtifactPowers>powers{{}};struct Template{uint32 GetArtifactID(){return 1;}}t;uint32 GetTotalPurchasedArtifactPowers(){return level;}uint32 GetModifier(int){return tier;}void SetModifier(int,uint32 n){tier=n;}uint32 GetUInt32Value(int){return uint32(xp);}uint64 GetUInt64Value(int){return xp;}void SetUInt64Value(int,uint64 n){xp=n;}auto const&GetArtifactPowers(){return powers;}void SetArtifactPower(ItemDynamicFieldArtifactPowers*p){powers[0]=*p;}bool IsEquipped(){return true;}auto GetTemplate(){return &t;}void InitArtifactsTier(uint32){}void SetState(int,Player*){}uint64 GetGUID(){return 1;}};
template<class T,class P>void ApplyPct(T&v,P p){v=T(v*p/100);}
struct Position{float x=0,y=0,z=0,o=0;Position(float a=0,float b=0,float c=0,float d=0):x(a),y(b),z(c),o(d){}float GetPositionX()const{return x;}float GetPositionY()const{return y;}float GetPositionZ()const{return z;}};
struct Motion{unsigned jumps=0;float x=0;void MoveJump(float px,float,float,float,float){x=px;++jumps;}};
struct Unit{virtual~Unit()=default;uint64 hp=50,maxhp=100;uint64 guid=1;bool los=true,inRange=true;Motion motion;Creature*anchor=nullptr;bool failCast=false;unsigned casts=0;Player*playerOwner=nullptr;Player*GetCharmerOrOwnerPlayerOrPlayerItself(){return ToPlayer()?ToPlayer():playerOwner;}virtual Player*ToPlayer(){return nullptr;}virtual Creature*ToCreature(){return nullptr;}uint64 GetMaxHealth(Unit* =nullptr){return maxhp;}uint64 GetHealth(Unit* =nullptr){return hp;}void SetHealth(uint64 n){hp=std::min(n,maxhp);}bool IsWithinDist(Unit*u,float){return u->inRange;}bool IsWithinLOSInMap(Unit*u){return u->los;}Unit*ToUnit(){return this;}uint32 GetTypeId(){return ToPlayer()?4:3;}Motion*GetMotionMaster(){return &motion;}Creature*FindNearestCreature(uint32,float){return anchor;}int CastSpell(Unit*,uint32,bool){++casts;return failCast?99:0;}void CastSpell(float,float,float,uint32,bool){++casts;}void RemoveAurasDueToSpell(uint32,uint64){}uint64 GetGUID(){return guid;}};
using WorldObject=Unit;struct Creature:Unit{float matchX=-9999;bool despawn=false;unsigned rawRemovals=0;Creature*ToCreature()override{return this;}float GetDistance(Position const&p){return p.x==matchX?0.f:100.f;}Position GetNearPosition(float,float){return {};}void DespawnOrUnsummon(uint32=0){despawn=true;}void RemoveFromWorld(){++rawRemovals;}Creature*GetCreature(Unit&,uint64){return anchor;}};
struct Player:Unit{unsigned events=0;Unit*creditAnchor=nullptr;void GroupEventHappens(uint32 quest,Unit*who){assert(quest==12935);++events;creditAnchor=who;}bool poly=false,combat=false,regenCombat=false,stand=true,gm=false;int quest=1;float bonus=0,m_baseHealthRegen=0,guildMultiplier=1;int64 lastDelta=0;Item artifact;Player*ToPlayer()override{return this;}bool isGameMaster(){return gm;}uint32 GetQuestForUnLockSecondTier(){return 1;}int GetQuestStatus(uint32){return quest;}Item*GetArtifactWeapon(){return &artifact;}void ApplyArtifactPowerRank(Item*,ArtifactPowerRankEntry const*,bool){}void SendDirectMessage(void const*){}bool IsPolymorphed(){return poly;}bool isInCombat(){return combat;}bool HasAuraType(int){return regenCombat;}float GetTotalAuraMultiplier(int id){return id==5?guildMultiplier:1.f;}float GetTotalAuraModifier(int){return bonus;}uint32 getLevel(){return 110;}bool IsStandState(){return stand;}int64 ModifyHealth(int64 n){lastDelta=n;if(n>=0)SetHealth(hp+uint64(n));else hp-=std::min(hp,uint64(-n));return n;}void SetPower(int,uint32){}uint32 GetMaxPower(int){return 100;}void ResetPowers(float){}void RegenerateHealth();void RestoreProbe(float);};
struct World{float getRate(int){return 1.f;}}world;auto sWorld=&world;
struct ChatHandler{ChatHandler(Player*){}template<class...T>void PSendSysMessage(char const*,T...){}};namespace WorldPackets{namespace Artifact{struct ArtifactTraitsRefunded{uint64 Guid=0;int UnkInt=0,UnkInt2=0;void*Write(){return nullptr;}};}}
struct SpellScript{Unit*caster=nullptr,*target=nullptr;Unit*GetCaster(){return caster;}Unit*GetHitUnit(){return target;}struct Hook{template<class T>void operator+=(T){}}OnCast,OnHit,OnObjectAreaTargetSelect;virtual~SpellScript()=default;virtual void Register(){}};
struct DamageInfo{uint32 damage=0;uint32 GetDamage(){return damage;}};struct AuraEffect{};struct AuraScript{Unit*caster=nullptr,*owner=nullptr;Unit*GetCaster(){return caster;}Unit*GetUnitOwner(){return owner;}Unit*GetTarget(){return owner;}SpellScript::Hook OnEffectAbsorb,DoEffectCalcAmount;virtual~AuraScript()=default;virtual void Register(){}virtual uint32 CallSpecialFunction(uint32){return 0;}};
struct SpellScriptLoader{SpellScriptLoader(char const*){}virtual~SpellScriptLoader()=default;virtual SpellScript*GetSpellScript()const{return nullptr;}virtual AuraScript*GetAuraScript()const{return nullptr;}};
#define PrepareSpellScript(x) public:
#define PrepareAuraScript(x) public:
#define SpellCastFn(x) &x
#define SpellHitFn(x) &x
#define SpellObjectAreaTargetSelectFn(x,...) &x
#define AuraEffectAbsorbFn(x,...) &x
#define AuraEffectCalcAmountFn(x,...) &x
uint32 urand(uint32,uint32 upper){return upper;}
struct ScriptedAI{Creature*me;bool reset=false,combat=true;ScriptedAI(Creature*c):me(c){}virtual void EnterEvadeMode(){reset=true;combat=false;}};struct EvadeProbe:ScriptedAI{using ScriptedAI::ScriptedAI;uint64 uiWhisker=2;void EnterEvadeMode()override;void JustDied(Unit*);};
#include "Selected.inc"
void Check(bool v,char const*m){if(!v){std::cerr<<m<<'\n';std::exit(1);}}
int main(){
#if defined(CASE_ARTIFACT)
 Player p;spell_q_artifact_trait_unlock::spell_q_artifact_trait_unlock_SpellScript s;s.caster=&p;auto before=p.artifact.xp;s.HandleOnCast();
#ifdef COUNTERFACTUAL
 Check(p.artifact.xp==117,"original unlock loses high AP bits");
#else
 Check(p.artifact.xp==before+100&&p.artifact.tier==1,"full AP plus native refund retained");s.HandleOnCast();Check(p.artifact.xp==before+100,"tier gate refuses repeat refund");Player wrong;wrong.quest=0;s.caster=&wrong;s.HandleOnCast();Check(!wrong.artifact.tier&&wrong.artifact.xp==before,"wrong quest refused");Player low;low.artifact.level=34;s.caster=&low;s.HandleOnCast();Check(!low.artifact.tier,"native35 prerequisite retained");
#endif
#elif defined(CASE_CHAIN)
 Unit caster,next,a,b;a.maxhp=(uint64(1)<<32)+100;a.hp=0;b.maxhp=1000;b.hp=0;std::list<WorldObject*>list{&a,&b};auto chosen=SelectChain(&caster,&next,list,30);
#ifdef COUNTERFACTUAL
 Check(chosen==&b,"original deficit wraps and picks healthier candidate");
#else
 Check(chosen==&a,"largest actual deficit wins");a.los=false;Check(SelectChain(&caster,&next,list,30)==&b,"native LOS retained");b.inRange=false;Check(!SelectChain(&caster,&next,list,30),"native radius retained");
#endif
#elif defined(CASE_REGEN)
 Player p;p.maxhp=uint64(1)<<36;p.hp=p.maxhp/2;auto before=p.hp;p.RegenerateHealth();
#ifdef COUNTERFACTUAL
 Check(p.hp==before,"original low32 equal falsely reports full health");
#else
 Check(p.hp>before&&p.hp<=p.maxhp,"native64 health regenerates");p.poly=true;p.RegenerateHealth();Check(p.lastDelta>INT32_MAX&&p.hp<=p.maxhp,"large regen delta stays positive");p.hp=p.maxhp;p.RegenerateHealth();Check(p.hp==p.maxhp,"full health retained");Player normal;normal.hp=50;normal.maxhp=1000;normal.RegenerateHealth();Check(normal.hp==60,"ordinary1percent unchanged");
#endif
#elif defined(CASE_RESURRECTION)
 Player p;p.maxhp=uint64(1)<<36;p.hp=1;p.RestoreProbe(.5f);
#ifdef COUNTERFACTUAL
 Check(p.hp!=p.maxhp/2,"original resurrect amount narrows");
#else
 Check(p.hp==p.maxhp/2,"native64 resurrection fraction");p.hp=3;p.RestoreProbe(0);Check(p.hp==3,"zero caller-owned restore preserved");p.guildMultiplier=2;p.RestoreProbe(1);Check(p.hp==p.maxhp,"native max-health clamp and guild multiplier retained");
#endif
#elif defined(CASE_ANCESTRAL)
 Player p;p.maxhp=uint64(1)<<31;spell_sha_ancestral_protection s;s.owner=&p;DamageInfo damage{1};float amount=9;s.Absorb(nullptr,damage,amount);
#ifdef COUNTERFACTUAL
 Check(!s.canProc,"original doubled health wraps to0 and disables rez");
#else
 Check(s.canProc&&amount==9,"tiny hit on large health preserves eligibility");p.maxhp=100;damage.damage=200;s.Absorb(nullptr,damage,amount);Check(s.canProc,"exact2x native boundary retained");damage.damage=201;s.Absorb(nullptr,damage,amount);Check(!s.canProc,"above2x disqualifies");
#endif
#elif defined(CASE_BETRUG)
 using T=spell_betrug_mighty_smash::spell_betrug_mighty_smash_SpellScript;alignas(T)unsigned char storage[sizeof(T)];std::memset(storage,0xcd,sizeof(storage));auto s=new(storage)T;Creature caster,anchor;Player p;s->caster=&caster;s->target=&p;s->targetHit=1;s->HandleOnCast();
#ifdef COUNTERFACTUAL
 Check(s->bPos==0xcd&&s->pPos==0xcd,"missing anchor leaves both indices uninitialized");
#else
 s->HandleOnHit();Check(!s->positionsReady&&!p.motion.jumps,"missing anchor cannot move player");caster.anchor=&anchor;s->HandleOnCast();Check(!s->positionsReady,"unmatched pad refused");for(int i=0;i<4;++i){anchor.matchX=jumpPos[i].x;caster.failCast=false;s->HandleOnCast();Check(s->positionsReady&&s->bPos==i&&s->pPos==((i+2)%4),"all four native pad pairs");s->targetHit=2;s->targetHitCount=0;s->HandleOnHit();Check(p.motion.x==jumpPos[s->pPos].x,"ordinary passenger opposite pad");s->HandleOnHit();Check(p.motion.x==jumpPos[s->bPos].x,"one selected passenger boss pad");}caster.failCast=true;s->HandleOnCast();auto before=p.motion.jumps;s->HandleOnHit();Check(!s->positionsReady&&p.motion.jumps==before,"failed boss jump cannot move passengers");
#endif
 s->~T();
#elif defined(CASE_EVADE)
 Creature npc,pet;npc.anchor=&pet;EvadeProbe ai(&npc);ai.EnterEvadeMode();
#ifdef COUNTERFACTUAL
 Check(pet.rawRemovals==1&&!ai.reset&&ai.combat,"old evade bypasses native reset and only unregisters pet");Player owner;Creature guardian;guardian.playerOwner=&owner;ai.JustDied(&guardian);Check(owner.events==0,"original pet final blow loses owner credit");
#else
 Check(pet.despawn&&pet.rawRemovals==0&&ai.reset&&!ai.combat,"native pet cleanup and base evade complete");npc.anchor=nullptr;ai.EnterEvadeMode();Check(ai.reset,"missing pet still resets");Player owner;Creature guardian;guardian.playerOwner=&owner;ai.JustDied(&guardian);Check(owner.events==1&&owner.creditAnchor==&guardian,"pet final blow uses eligible owner and original killer anchor");ai.JustDied(&owner);Check(owner.events==2&&owner.creditAnchor==&owner,"ordinary player anchor retained");ai.JustDied(nullptr);Creature unowned;ai.JustDied(&unowned);Check(owner.events==2,"null/unowned killers grant no progress");
#endif
#elif defined(CASE_DUMPIDS)
 auto row=std::make_shared<Result>();row->f.value=(uint64(1)<<40)+17;WriterGUIDs ids;ids.insert(17);StoreGUID(row,0,ids);std::map<uint64,uint64>mapping;std::string line="INSERT INTO `x` VALUES ('1');";uint64 seed=(uint64(1)<<40)+100;ChangeGuid(line,1,mapping,seed);
#ifdef COUNTERFACTUAL
 Check(ids.size()==1&&GetNth(line,1)=="100"&&FormatGuid(row->f.value)=="17","old collector/seed/format lose high bits");
#else
 Check(ids.size()==2&&GetNth(line,1)==std::to_string(seed),"64bit collection and mapping seed");Check(FormatGuid(UINT64_MAX)==std::to_string(UINT64_MAX)&&FormatItem(UINT64_MAX)==std::to_string(UINT64_MAX),"full20digit identifier plus terminator");std::string zero="INSERT INTO `x` VALUES ('0');";Check(ChangeGuid(zero,1,mapping,seed,true)&&GetNth(zero,1)=="0"&&mapping.size()==1,"zero bag reference retained");
#endif
#elif defined(CASE_DUMPPETS)
 PetFrame f;std::string pet="INSERT INTO `character_pet` VALUES ('11', '299', '13');";Check(f.Apply(pet),"main pet rewrite");auto id=GetNth(pet,1);std::string declined="INSERT INTO `character_pet_declinedname` VALUES ('11', '13', 'genitive', 'dative');";Check(f.Apply(declined,true),"declined rewrite");
#ifdef COUNTERFACTUAL
 Check(GetNth(declined,2)=="13"&&GetNth(declined,3)=="1099511627793","old declined owner wrong column corrupts grammatical name");
#else
 Check(GetNth(declined,1)==id&&GetNth(declined,2)=="1099511627793"&&GetNth(declined,3)=="genitive","same PetNumber/proper owner column/name retained");Check(id=="2147483648","unsigned native PetNumber format");std::string other="INSERT INTO `character_pet` VALUES ('22', '299', '13');";Check(f.Apply(other),"second pet");std::string again="INSERT INTO `character_pet_declinedname` VALUES ('11', '13', 'genitive');";Check(f.Apply(again,true)&&GetNth(again,1)==id,"nonadjacent repeated old pet retains mapping");std::string child="INSERT INTO `pet_spell` VALUES ('11', '123', '1');";Check(f.Apply(child,false,true)&&GetNth(child,1)==id,"child follows mapped pet");std::string orphan="INSERT INTO `character_pet_declinedname` VALUES ('33', '13', 'genitive');";Check(!f.Apply(orphan,true),"orphan declined name cannot fabricate parent");for(auto bad:{"0","-1","4294967296","12x"}){std::string malformed="INSERT INTO `pet_spell` VALUES ('"+std::string(bad)+"', '123');";Check(!f.Apply(malformed,false,true),"invalid pet rejected");}
#endif
#elif defined(CASE_DUMPPARSE)
 std::string line=R"SQL(INSERT INTO `x` VALUES ('1', 'O\'Brien', 'path\\', '42');)SQL";auto name=GetNth(line,2);
#ifdef COUNTERFACTUAL
 Check(name!=R"SQL(O\'Brien)SQL","original escaped quote loses field boundaries");std::string malformed="12345678'fake'";size_t a,b;Check(FindNth(malformed,1,a,b),"missing VALUES wraps npos and accepts malformed line");
#else
 Check(name==R"SQL(O\'Brien)SQL"&&GetNth(line,3)==R"SQL(path\\)SQL"&&GetNth(line,4)=="42","escaped quotes/even backslashes preserve fields");Check(ChangeNth(line,4,"77")&&GetNth(line,2)==name&&GetNth(line,4)=="77","exact column replacement");for(auto text:{"12345678'fake'","INSERT INTO `x` VALUES ('dangling\\","INSERT INTO `x` VALUES ('unterminated"}){std::string bad=text;size_t a,b;Check(!FindNth(bad,1,a,b),"malformed input refused");}size_t a,b;Check(!FindNth(line,0,a,b)&&!FindNth(line,-1,a,b),"invalid field index refused");std::string first=R"SQL(INSERT INTO `x` VALUES ('one\'quote', 'two');)SQL";Check(GetNth(first,1)==R"SQL(one\'quote)SQL"&&GetNth(first,2)=="two","escaped first field terminates without original infinite scan");
#endif
#endif
#ifdef COUNTERFACTUAL
 std::cout<<"COUNTERFACTUAL pinned original defect demonstrated;not successful ordinary import/gameplay\n";
#else
 std::cout<<"PASS actual extracted functions/branches under ASAN;synthetic data and modeled external endpoints;no persistent import/character writes or client traversal\n";
#endif
}
