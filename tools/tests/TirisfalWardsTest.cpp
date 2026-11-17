#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <set>
using uint32=uint32_t;using int32=int32_t;
enum SpellCastResult {SPELL_CAST_OK, SPELL_FAILED_SPELL_IN_PROGRESS};
uint32 warnings=0,errors=0;
template<class...Args>void warn(Args const&...){++warnings;}
template<class...Args>void error(Args const&...){++errors;}
#define TC_LOG_WARN(...) warn(__VA_ARGS__)
#define TC_LOG_ERROR(...) error(__VA_ARGS__)
struct Creature {
 uint32 entry=103151,map=1539,guid=370945,removals=0,requests=0,reject=0;
 bool world=true,alive=true,expired=false,apply=true;std::set<uint32> auras{202925,204345};
 uint32 GetEntry()const{return entry;}uint32 GetMapId()const{return map;}uint32 GetGUIDLow()const{return guid;}
 bool IsInWorld()const{return world;}bool IsAlive()const{return alive;}bool IsQuestObjectExpired()const{return expired;}
 bool HasAura(uint32 id){return auras.count(id)!=0;}void RemoveAllAuras(){auras.clear();++removals;}
 SpellCastResult CastSpell(Creature* target,uint32 id,bool triggered){assert(target==this&&id==202014&&!triggered);++requests;
  if(reject){--reject;return SPELL_FAILED_SPELL_IN_PROGRESS;}if(apply)auras.insert(id);return SPELL_CAST_OK;}
};
struct SmartAI {
 Creature* me;uint32 resets=0,baseData=0,updates=0;explicit SmartAI(Creature*c):me(c){}
 virtual ~SmartAI()=default;virtual void Reset(){++resets;}virtual void SetData(uint32,uint32){++baseData;}
 virtual void UpdateAI(uint32){++updates;}
};
#include "Ward.inc"
void advance(TirisfalWards::WardAI&a,uint32 ms){for(uint32 time=0;time<ms;time+=100)a.UpdateAI(100);}

// Narrow endpoints for the actual native Spell destination prefix.
enum {TARGET_DIR_NONE=0,TARGET_DIR_RANDOM=1,SPELL_EFFECT_TELEPORT_L=1,SPELL_EFFECT_TELEPORT=2,TARGET_DEST_TARGET_ENEMY=63,
 TARGET_DEST_TARGET_SELECT=87,TARGET_DEST_CHANNEL_CASTER=106,TARGET_DEST_NEARBY_ENTRY=46,
 TARGET_DEST_DYNOBJ_ENEMY=28,TARGET_DEST_DYNOBJ_ALLY=29,TARGET_DEST_DYNOBJ_NONE=30,
 TARGET_DEST_DEST=42,TARGET_UNK_128=128,TARGET_DEST_TRAJ=89};
using SpellEffIndex=uint32;
struct WorldObject {int location=0;virtual ~WorldObject()=default;};
struct Unit:WorldObject {uint32 GetMapId()const{return 1539;}};
struct SpellImplicitTargetInfo {uint32 id;uint32 GetDirectionType()const{return TARGET_DIR_NONE;}uint32 GetTarget()const{return id;}};
struct SpellTargetPosition {uint32 target_mapId=1539;float target_X=0,target_Y=0,target_Z=0,target_Orientation=0;};
struct SpellMgr {SpellTargetPosition const* GetSpellTargetPosition(uint32 id){assert(id==202015);return nullptr;}} manager;
SpellMgr* sSpellMgr=&manager;
struct SpellInfo {uint32 Id=202015;bool HasEffect(uint32)const{return false;}};
struct Targets {bool dst=false;int location=-1;Unit* unit=nullptr;
 void SetDst(float,float,float,float,int=0){assert(false);}void SetDst(WorldObject const& o){dst=true;location=o.location;}
 bool HasDst()const{return dst;}Unit* GetUnitTarget(){return unit;}WorldObject* GetObjectTarget(){return unit;}};
struct Spell {Targets m_targets;Unit* m_caster;SpellInfo* m_spellInfo;
 void SelectImplicitDestDestTargets(SpellEffIndex,SpellImplicitTargetInfo const&);
 void AddUnitTarget(Unit*,uint32){assert(false);}void SelectImplicitTrajTargets(){assert(false);}
};
#include "Destination.inc"
int main(){_set_abort_behavior(0,_CALL_REPORTFAULT);uint32 cases=0;
 for(uint32 guid:{369686u,370945u,370946u,370947u,370948u,370949u,370950u}){
  Creature c;c.guid=guid;TirisfalWards::WardAI ai(&c);warnings=errors=0;
  advance(ai,2000);assert(!c.requests&&c.removals==0);ai.SetData(1,1);ai.SetData(1,1);assert(c.removals==1&&c.auras.empty());
  advance(ai,200);assert(ai.confirmed&&!ai.pending&&c.HasAura(202014)&&c.requests==1&&warnings==0&&errors==0);
  for(int repeat=0;repeat<10;++repeat)ai.SetData(1,1);advance(ai,120000);assert(c.requests==1&&c.removals==1&&c.HasAura(202014));++cases;
 }
 {Creature c;c.apply=false;TirisfalWards::WardAI ai(&c);ai.SetData(1,1);advance(ai,400);assert(!ai.confirmed&&ai.pending&&c.requests==1);
  c.auras.insert(202014);advance(ai,100);assert(ai.confirmed&&!ai.pending&&c.requests==1);++cases;} // real delayed aura acknowledgement
 {Creature c;c.reject=1;TirisfalWards::WardAI ai(&c);warnings=errors=0;ai.SetData(1,1);advance(ai,1500);
  assert(ai.confirmed&&c.requests==2&&c.removals==1&&warnings==1&&errors==0);++cases;}
 {Creature c;c.apply=false;TirisfalWards::WardAI ai(&c);warnings=errors=0;ai.SetData(1,1);advance(ai,15000);
  assert(!ai.pending&&!ai.confirmed&&c.requests==10&&errors==1&&c.removals==1);
  c.apply=true;ai.SetData(1,1);advance(ai,200);assert(ai.confirmed&&c.requests==11&&c.removals==2);++cases;}
 for(int blocker=0;blocker<4;++blocker){Creature c;TirisfalWards::WardAI ai(&c);ai.SetData(1,1);
  if(blocker==0)c.alive=false;if(blocker==1)c.world=false;if(blocker==2)c.expired=true;if(blocker==3)c.map=1;
  advance(ai,100);assert(!ai.pending&&!c.requests&&!ai.confirmed);++cases;}
 {Creature c;TirisfalWards::WardAI ai(&c);ai.SetData(1,1);ai.Reset();advance(ai,500);assert(ai.resets==1&&!c.requests&&!ai.pending);
  c.auras.insert(202925);ai.SetData(1,1);advance(ai,200);assert(ai.confirmed&&c.removals==2);++cases;}
 for(int variant=0;variant<5;++variant){Creature c;TirisfalWards::WardAI ai(&c);if(variant==0)c.entry=1;if(variant==1)c.map=1;
  ai.SetData(variant==2?2:1,variant==3?2:1);if(variant<4){assert(ai.baseData==1&&c.removals==0);}
  else {ai.SetData(2,2);assert(ai.baseData==1);advance(ai,200);assert(ai.confirmed);}
  assert(ai.updates==0||variant==4);++cases;}
 // The actual native106 prefix chooses explicit self or defaults to the
 // caster destination, without entering the CHANNEL selection category.
 Unit ward,other;ward.location=17;other.location=29;SpellInfo info;
 for(int variant=0;variant<3;++variant){Spell s;s.m_caster=&ward;s.m_spellInfo=&info;
  if(variant==0)s.m_targets.unit=&ward;if(variant==1)s.m_targets.unit=&other;
  s.SelectImplicitDestDestTargets(0,{106});assert(s.m_targets.dst&&s.m_targets.location==(variant==1?29:17));++cases;}
 std::cout<<"PASS "<<cases<<" production ward/destination cases: seven real spawn IDs, native persistent aura receipt, duplicate events, rejected/deferred/failed casts, bounded retry, reset/expired/dead/map cleanup, shared SmartAI delegation and actual destination106 selection. World/cast endpoints controlled; no client or original LINK2 payload claim.\n";
}
