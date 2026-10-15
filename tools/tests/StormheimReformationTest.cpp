#include <fstream>
#include <string>
constexpr int UNIT_FIELD_CREATED_BY_SPELL=30;
#include "ReformationFixture.inc"
#include "ObjectiveGetters.inc"
#include "Reformation.inc"
constexpr int SPELL_EFFECT_HANDLE_HIT_TARGET=1;
struct NativeEffect {int32 MiscValue=97281;};
struct NativeInfo {NativeEffect effect;NativeEffect* GetEffect(int,int){return &effect;}};
struct Spell {int effectHandleMode=1,m_diffMode=0;Unit* unitTarget=nullptr;NativeInfo info;NativeInfo* m_spellInfo=&info;void EffectKillCredit(int);};
#include "NativeCredit.inc"

// Execute the complete native target46 selector. The world search, spell
// metadata and target containers are fixtures, not a client spell engine.
namespace Targeting
{
enum {TARGET_REFERENCE_TYPE_CASTER,TARGET_REFERENCE_TYPE_DEST,
 TARGET_CHECK_ENEMY,TARGET_CHECK_ALLY,TARGET_CHECK_PARTY,TARGET_CHECK_RAID,
 TARGET_CHECK_RAID_CLASS,TARGET_CHECK_SUMMON,TARGET_CHECK_ENTRY,TARGET_CHECK_DEFAULT,
 TARGET_OBJECT_TYPE_GOBJ,TARGET_OBJECT_TYPE_DEST,TARGET_OBJECT_TYPE_OBJ_AND_DEST,
 TARGET_OBJECT_TYPE_UNIT,TARGET_OBJECT_TYPE_CORPSE,SPELL_EFFECT_TELEPORT_L,
 SPELL_EFFECT_TELEPORT,SPELL_FAILED_BAD_IMPLICIT_TARGETS};
using ConditionList=std::vector<int>;
struct Unit;struct GameObject;
struct WorldObject {virtual ~WorldObject()=default;virtual Unit* ToUnit(){return nullptr;}virtual GameObject* ToGameObject(){return nullptr;}};
struct GameObject:WorldObject {};
struct Unit:WorldObject {Unit* ToUnit()override{return this;}uint32 map=1220;float x=3530,y=889,z=53.4f;uint32 GetMapId(){return map;}bool isDead(){return false;}};
struct SpellTargetPosition {uint32 target_mapId=0;float target_X=0,target_Y=0,target_Z=0,target_Orientation=0;};
struct SpellMgr {std::map<uint32,SpellTargetPosition> positions;SpellTargetPosition const* GetSpellTargetPosition(uint32 id){auto i=positions.find(id);return i==positions.end()?nullptr:&i->second;}} mgr;
auto sSpellMgr=&mgr;
struct SpellImplicitTargetInfo {int reference=TARGET_REFERENCE_TYPE_DEST,check=TARGET_CHECK_ENTRY,object=TARGET_OBJECT_TYPE_OBJ_AND_DEST;int GetReferenceType()const{return reference;}int GetCheckType()const{return check;}int GetObjectType()const{return object;}};
struct Effect {ConditionList* ImplicitTargetConditions=nullptr;};
struct SpellInfo {uint32 Id=192069;struct {bool RequiresSpellFocus=false;} CastingReq;Effect effect;Effect* Effects[1]{&effect};float GetMaxRange(bool,Unit*,void*)const{return 0;}bool IsPositive()const{return true;}bool HasEffect(int)const{return false;}};
struct Targets {Unit* target=nullptr;bool has=false;SpellTargetPosition dst;Unit* GetUnitTarget(){return target;}void SetUnitTarget(WorldObject*){}void SetDst(float x,float y,float z,float o,uint32 map=1220){has=true;dst={map,x,y,z,o};}void SetDst(WorldObject& object){auto& u=static_cast<Unit&>(object);SetDst(u.x,u.y,u.z,0,u.map);}};
struct Spell {
 SpellInfo info;SpellInfo* m_spellInfo=&info;Unit caster;Unit* m_caster=&caster;Targets m_targets;WorldObject* focusObject=nullptr;uint32 searches=0;bool failed=false;WorldObject* found=nullptr;
 void SelectImplicitNearbyTargets(SpellEffIndex,SpellImplicitTargetInfo const&,uint32);
 void AddGOTarget(WorldObject*,uint32){}void AddUnitTarget(WorldObject*,uint32,bool=false,bool=false){}
 WorldObject* SearchNearbyTarget(float,int,int,ConditionList*){++searches;return found;}
 void CallScriptObjectTargetSelectHandlers(WorldObject*&,int){}void SelectImplicitChainTargets(int,SpellImplicitTargetInfo const&,WorldObject*,uint32){}
 void SendCastResult(int){}void finish(bool success){failed=!success;}
};
#include "NearbyTargets.inc"
void Test(char const* path)
{
 std::ifstream input(path);uint32 id;SpellTargetPosition p;input>>id>>p.target_mapId>>p.target_X>>p.target_Y>>p.target_Z>>p.target_Orientation;
 assert(input&&id==192069&&p.target_mapId==1220);SpellImplicitTargetInfo target;
 // Before: self-targeting goober192069 inherits the user's varying position.
 {Spell s;s.m_targets.target=&s.caster;s.SelectImplicitNearbyTargets(0,target,1);assert(s.m_targets.has&&s.m_targets.dst.target_X==s.caster.x&&!s.searches);}
 mgr.positions[id]=p;
 for(float x:{3528.f,3532.f,3537.f}){Spell s;s.caster.x=x;s.m_targets.target=&s.caster;s.SelectImplicitNearbyTargets(0,target,1);assert(s.m_targets.has&&!s.failed&&!s.searches&&s.m_targets.dst.target_X==p.target_X&&s.m_targets.dst.target_Z==p.target_Z&&s.m_targets.dst.target_Orientation==p.target_Orientation);}
 {Spell s;s.caster.map=1;s.m_targets.target=&s.caster;s.SelectImplicitNearbyTargets(0,target,1);assert(!s.m_targets.has&&!s.searches);}
 {Spell s;s.info.Id=123;s.m_targets.target=&s.caster;s.SelectImplicitNearbyTargets(0,target,1);assert(s.m_targets.dst.target_X==s.caster.x);}
 {Spell s;s.info.Id=123;s.SelectImplicitNearbyTargets(0,target,1);assert(s.failed&&s.searches==1);}
}
}

int main(int argc,char** argv)
{
 assert(argc==2);Targeting::Test(argv[1]);
 auto& q=store.quests[38815];q.id=38815;q.special=QUEST_SPECIAL_FLAGS_KILL;q.PrevQuestID=38816;q.NextQuestID=38818;q.NextQuestIdChain=38818;q.objectives={{QUEST_OBJECTIVE_MONSTER,97281,1,0,1}};
 for(uint32 id:{38816u,38817u}){auto& p=store.quests[id];p.id=id;p.NextQuestID=38815;}
 auto& next=store.quests[38818];next.id=38818;next.PrevQuestID=38815;
 QuestMap _questTemplates;for(auto& entry:store.quests)_questTemplates[entry.first]=&entry.second;
 for(auto const& entry:_questTemplates){auto qinfo=entry.second;
#include "QuestPrevLoader.inc"
 }
 store.starters.emplace(93234,38818);
 for(auto team:{ALLIANCE,HORDE})for(uint32 previous:{38816u,38817u}){
  Player p;p.team=team;p.race=team==ALLIANCE?1:2;assert(!p.CanTakeQuest(&q,false));p.m_RewardedQuests.insert(previous);assert(p.CanTakeQuest(&q,false));p.Accept(38815);
  p.KillCreditGO(244450,{115183});p.KillCreditGO(241877,{1});assert(!p.GetQuestObjectiveData(38815,97281));
  Spell spell;spell.unitTarget=&p;spell.effectHandleMode=0;spell.EffectKillCredit(1);assert(!p.GetQuestObjectiveData(38815,97281));
  Unit pet;pet.ownerPlayer=&p;spell.unitTarget=&pet;spell.effectHandleMode=1;spell.EffectKillCredit(1);assert(!p.GetQuestObjectiveData(38815,97281));
  Creature helper;helper.entry=97281;helper.fields[UNIT_FIELD_CREATED_BY_SPELL]=192069;npc_stormheim_reformation_bones ai(&helper);
  bool normalCast=false;helper.castTargetHook=[&](Unit* target,uint32 spell){normalCast=target==&helper&&spell==192062;return 1;};
  ai.IsSummonedBy(&p);assert(ai.started&&helper.bound&&normalCast&&helper.casts.size()==1&&!helper.triggeredCasts.front()&&!p.completions);ai.IsSummonedBy(&p);assert(helper.casts.size()==1);
  spell.unitTarget=&p;spell.EffectKillCredit(1);assert(p.GetQuestObjectiveData(38815,97281)==1&&p.GetQuestStatus(38815)==QUEST_STATUS_COMPLETE&&p.completions==1);
  spell.EffectKillCredit(1);assert(p.completions==1&&p.packets==1&&p.CanRewardQuest(&q,false));
  // Optional cast failure leaves the saved objective/endgiver route intact.
  assert(!p.CanTakeQuest(&next,false));p.m_RewardedQuests.insert(38815);p.quests.erase(38815);
  for(uint64 guid:{338671u,341220u}){Creature ash;ash.entry=93234;ash.guid={guid};creatures[ash.guid]=&ash;
   assert(p.CanTakeQuest(&next,false)&&p.GetNextQuest(ash.GetGUID(),&q)==&next);creatures.erase(ash.guid);}
  Player retry;retry.Accept(38815);retry.quests.erase(38815);retry.Accept(38815);spell.unitTarget=&retry;spell.EffectKillCredit(1);assert(retry.GetQuestStatus(38815)==QUEST_STATUS_COMPLETE);
 }
 // Scoping includes other summons, stale quest states, maps and actors.
 for(int why=0;why<12;++why){Player p;p.Accept(38815);Creature c;c.entry=97281;c.fields[UNIT_FIELD_CREATED_BY_SPELL]=192069;
  Unit pet;pet.ownerPlayer=&p;Unit* summoner=&p;
  if(why==0)summoner=nullptr;if(why==1)summoner=&pet;if(why==2)c.entry=93234;if(why==3)c.summoned=false;
  if(why==4)c.fields[UNIT_FIELD_CREATED_BY_SPELL]=187830;if(why==5)c.x+=2;if(why==6)p.map=1463;
  if(why==7)p.instance=1;if(why==8)p.zone=1;if(why==9)p.world=false;if(why==10)p.alive=false;if(why==11)p.quests.erase(38815);
  npc_stormheim_reformation_bones ai(&c);ai.IsSummonedBy(summoner);assert(!ai.started&&!c.bound&&c.casts.empty()&&ai.delegated==1);
 }
 {Player p;p.Accept(38815);Creature c;c.entry=97281;c.fields[UNIT_FIELD_CREATED_BY_SPELL]=192069;npc_stormheim_reformation_bones ai(&c);ai.IsSummonedBy(&p);assert(c.bound);p.CancelQuestObjects(38815);assert(c.IsQuestObjectExpired());}
 // Native getters follow the unchanged saved slot, not the old object entry.
 {Player p;p.Accept(38815);p.quests[38815].ObjectiveData[1]=1;p.quests[38815].Status=QUEST_STATUS_COMPLETE;q.objectives[0].ObjectID=244450;assert(p.GetQuestObjectiveData(38815,244450)==1);q.objectives[0].ObjectID=97281;assert(p.GetQuestObjectiveData(38815,97281)==1&&!p.GetQuestObjectiveData(38815,244450)&&p.CanRewardQuest(&q,false));}
 for(int status:{QUEST_STATUS_COMPLETE,QUEST_STATUS_NONE,QUEST_STATUS_REWARDED}){Player p;p.Accept(38815);p.quests[38815].Status=status;Creature c;c.entry=97281;c.fields[UNIT_FIELD_CREATED_BY_SPELL]=192069;npc_stormheim_reformation_bones ai(&c);ai.IsSummonedBy(&p);assert(ai.started==(status==QUEST_STATUS_COMPLETE));}
 std::cout<<"PASS: complete native target46 selector reproduces feet fallback and reads repaired altar destination; GO use cannot grant restored97281/storage1 objective; native effect90 player-only credit/once/retry and both-faction alternative prerequisites/93234 handoff; scoped helper cast, optional failure, duplicate callback and quest cleanup. World/network/rendering remain fixtures.\n";
}
