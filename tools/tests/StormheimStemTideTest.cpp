#include <crtdbg.h>
#include <mutex>
constexpr int QUEST_SPECIAL_FLAGS_SPEAKTO=128,TARGET_UNIT_NEARBY_ENTRY=38,UNIT_FLAG_NOT_SELECTABLE=0x02000000,SPELL_EFFECT_HANDLE_HIT_TARGET=1;
constexpr int GAMEOBJECT_TYPE_SPELL_FOCUS=8,GAMEOBJECT_TYPE_UI_LINK=48;
#include "StemFixture.inc"
#include "NativeGossipUse.inc"
#include "Talked.inc"
void WorldObject::GetPlayerListInGrid(std::list<Player*>& out,float radius)const{for(auto const& v:players)if(v.second->world&&v.second->map==map&&GetExactDist(v.second)<=radius)out.push_back(v.second);}
void Creature::GetPlayerListInGrid(std::list<Player*>& out,float radius){WorldObject::GetPlayerListInGrid(out,radius);}
void GetGameObjectListWithEntryInGrid(std::list<GameObject*>& out,WorldObject* p,uint32 entry,float range){for(auto const& v:objects)if(v.second->entry==entry&&v.second->world&&v.second->map==p->map&&p->GetExactDist(v.second)<=range)out.push_back(v.second);}
struct Item{uint32 entry=129161;ObjectGuid owner;uint32 GetEntry(){return entry;}ObjectGuid GetOwnerGUID(){return owner;}};
struct PlayerScript{explicit PlayerScript(char const*){}virtual void OnUpdate(Player*,uint32){}virtual void OnLogout(Player*){}virtual void OnMapChanged(Player*){}};
struct HornSpell{
 Unit* caster=nullptr,*hit=nullptr;Item* item=nullptr;Hook OnCheckCast,OnObjectTargetSelect,OnEffectHitTarget;
 Unit* GetCaster(){return caster;}Unit* GetHitUnit(){return hit;}Item* GetCastItem(){return item;}virtual void Register(){}
};
#define SpellScript HornSpell
#define SpellCheckCastFn(x) 1
#define SpellObjectTargetSelectFn(x,i,t) (i*1000+t)
#define SpellEffectFn(x,i,t) (i*1000+t)
#include "StemTide.inc"
#undef SpellScript
struct Spell{int effectHandleMode=SPELL_EFFECT_HANDLE_HIT_TARGET;Unit* unitTarget=nullptr;uint32 m_diffMode=0;SpellInfo data;SpellInfo* m_spellInfo=&data;void EffectKillCredit(SpellEffIndex);};
#include "SpellGroupCredit.inc"
using namespace StormheimStemTide;
Creature markers[3];GameObject portals[3];std::unique_ptr<go_stormheim_stem_portal> portalAI[3];std::unique_ptr<npc_stormheim_stem_marker> markerAI[3];uint64 serial=10000;
struct Visit{
 Player p;Item horn;player_stormheim_stem_tide script;bool noDragon=false,noImpact=false,failImpact=false;
 std::vector<std::unique_ptr<Creature>> summons;
 Visit(uint32 team=ALLIANCE){
  p.guid={++serial};p.team=team;p.race=team==ALLIANCE?1:2;p.Accept(QuestId);p.bags[Horn]=1;horn.owner=p.guid;players[p.guid]=&p;At(0);
  p.judgmentSummon=[&](uint32 entry,Position const& point,ObjectGuid viewer)->Creature*{
   assert(viewer==p.guid);bool dragon=entry>=98039&&entry<=98041;if((dragon&&noDragon)||(!dragon&&noImpact))return nullptr;
   auto c=std::make_unique<Creature>();c->entry=entry;c->guid={++serial};static_cast<Position&>(*c)=point;c->castResult=failImpact?SPELL_FAILED_BAD_TARGETS:SPELL_CAST_OK;creatures[c->guid]=c.get();summons.push_back(std::move(c));return summons.back().get();
  };
 }
 ~Visit(){script.OnLogout(&p);players.erase(p.guid);for(auto& c:summons)creatures.erase(c->guid);for(auto& ai:portalAI)ai->UpdateAI(500);}
 void At(uint32 i){static_cast<Position&>(p)=Portals[i].point;}
 std::unique_ptr<spell_stormheim_stem_horn> Prepare(){auto s=std::make_unique<spell_stormheim_stem_horn>();s->caster=&p;s->item=&horn;return s;}
 bool Cast(){auto s=Prepare();if(s->Check()!=SPELL_CAST_OK)return false;WorldObject* target=nullptr;s->Select(target);assert(target);s->hit=target->ToUnit();s->Hit(0);SpellInfo call;call.Id=Call;markerAI[uint32(Identity(target->ToCreature()))]->SpellHit(&p,&call);return true;}
 void Advance(uint32 time){p.Advance(time);script.OnUpdate(&p,time);for(auto& ai:portalAI)ai->UpdateAI(time);}
};
struct TargetSpell{SpellCastResult CheckTarget(Unit*,WorldObject*,bool){return SPELL_FAILED_BAD_TARGETS;}} targetSpell;
struct ConditionSourceInfo{WorldObject* mConditionTargets[1]={};};
struct ConditionMgr{bool IsObjectMeetToConditions(ConditionSourceInfo& info,std::set<uint32> const& entries){return entries.count(info.mConditionTargets[0]->GetEntry());}} conditionMgr;auto sConditionMgr=&conditionMgr;
struct WorldObjectSpellTargetCheck{Unit* _caster;TargetSpell* _spellInfo;ConditionSourceInfo* _condSrcInfo;std::set<uint32>* _condList;bool operator()(WorldObject*);};
#include "NativeConditionAdmission.inc"
struct GameObjectFocusCheck{Unit const* i_unit;uint32 i_focusId;GameObjectFocusCheck(Unit const*,uint32);bool operator()(GameObject*)const;};
#include "NativeFocusCheck.inc"
int main()
{
 _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);_CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE);_CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);
 auto& q=store.quests[QuestId];q.id=QuestId;q.special=QUEST_SPECIAL_FLAGS_KILL;q.objectives={{0,97983,1,0,0},{0,97984,1,0,1},{0,97985,1,0,2},{0,98186,3,28,3}};
 for(uint32 i=0;i<3;++i){auto const& def=Portals[i];auto& c=markers[i];c.entry=def.credit;c.dbGuid=def.marker;c.guid={def.marker};static_cast<Position&>(c)=def.point;creatures[c.guid]=&c;markerAI[i]=std::make_unique<npc_stormheim_stem_marker>(&c);auto& go=portals[i];go.entry=244681;go.dbGuid=def.object;go.guid={def.object};static_cast<Position&>(go)=def.point;objects[go.guid]=&go;portalAI[i]=std::make_unique<go_stormheim_stem_portal>(&go);}
 for(auto team:{ALLIANCE,HORDE}){
  Visit v(team);auto hook=v.Prepare();hook->Register();assert(hook->OnCheckCast.values==std::vector<int>{1}&&hook->OnObjectTargetSelect.values==std::vector<int>{38}&&hook->OnEffectHitTarget.values==std::vector<int>{3});
  for(uint32 i:{2u,0u,1u}){v.At(i);assert(v.Cast()&&runs.count(v.p.guid));auto& fly=*v.summons.back();assert(fly.entry==Portals[i].dragon&&fly.gravityDisabled&&fly.canFly&&fly.personal==v.p.guid&&fly.bound&&fly.motion.flights.size()==1);
   assert(!v.Cast());auto count=v.p.GetQuestObjectiveData(QuestId,98186);v.Advance(4999);assert(!v.p.GetQuestObjectiveData(QuestId,Portals[i].credit)&&v.p.GetQuestObjectiveData(QuestId,98186)==count);v.Advance(1);assert(v.p.GetQuestObjectiveData(QuestId,Portals[i].credit)==1&&v.p.GetQuestObjectiveData(QuestId,98186)==count+1&&portals[i].hidden.count(v.p.guid)&&portals[i].world&&fly.motion.flights.size()==2);v.Advance(5000);assert(!runs.count(v.p.guid)&&!fly.world);
  }
  assert(v.p.packets==6&&v.p.completions==1&&v.p.CanRewardQuest(&q,false));v.Advance(5000);assert(v.p.packets==6);
 }
 {Visit a,b;GroupReference last{&b.p,nullptr},first{&a.p,&last};Group group{&first};a.p.group=b.p.group=&group;assert(a.Cast()&&b.Cast());a.Advance(5000);assert(a.p.packets==2&&!b.p.packets&&portals[0].hidden.count(a.p.guid)&&!portals[0].hidden.count(b.p.guid));b.Advance(5000);assert(b.p.packets==2);}
 {Visit v;v.noDragon=v.noImpact=true;assert(v.Cast());v.Advance(5000);assert(v.p.packets==2);} // optional summon failure cannot block quest
 {Visit v;v.failImpact=true;assert(v.Cast());v.Advance(5000);assert(v.p.packets==2&&v.summons.back()->casts==std::vector<uint32>{196084});}
 {Visit v;auto old=v.Prepare();assert(old->Check()==SPELL_CAST_OK);v.p.Accept(QuestId);assert(old->Check()!=SPELL_CAST_OK);WorldObject* target=&markers[0];old->Select(target);assert(!target);assert(v.Cast());v.Advance(5000);assert(v.p.packets==2);}
 {Visit v;assert(v.Cast());v.Advance(2000);v.script.OnLogout(&v.p);assert(!runs.count(v.p.guid));assert(v.Cast());v.Advance(3000);assert(!v.p.packets);v.Advance(2000);assert(v.p.packets==2);} // old callback cannot consume a new cast's ticket
 {Visit v;assert(v.Cast());v.p.CancelQuestObjects(QuestId);v.p.Accept(QuestId);v.Advance(5000);assert(!v.p.packets);assert(v.Cast());v.Advance(5000);assert(v.p.packets==2);v.p.quests.erase(QuestId);portalAI[0]->UpdateAI(500);assert(!portals[0].hidden.count(v.p.guid));}
 {Visit v;assert(v.Cast());v.Advance(5000);v.p.world=false;portalAI[0]->UpdateAI(500);assert(!portals[0].hidden.count(v.p.guid));v.p.world=true;portalAI[0]->UpdateAI(500);assert(portals[0].hidden.count(v.p.guid));v.p.map=1463;portalAI[0]->UpdateAI(500);assert(!portals[0].hidden.count(v.p.guid));v.p.map=1220;v.p.m_RewardedQuests.insert(QuestId);v.p.quests.erase(QuestId);portalAI[0]->UpdateAI(500);for(uint32 i=0;i<3;++i){v.At(i);portalAI[i]->UpdateAI(500);assert(portals[i].hidden.count(v.p.guid));}}
 for(int failure=0;failure<14;++failure){Visit v;assert(v.Cast());
  if(failure==0)v.p.alive=false;if(failure==1)v.p.map=1463;if(failure==2)v.p.zone=1;if(failure==3)v.p.teleport=true;if(failure==4)v.p.vehicle=true;if(failure==5)v.p.transport=true;if(failure==6)v.p.flying=true;
  if(failure==7)v.p.phase=false;if(failure==8)v.p.visible=false;if(failure==9)v.p.los=false;if(failure==10)v.p.x+=81;if(failure==11)v.p.bags.clear();if(failure==12)v.p.phases.insert(8306);if(failure==13)portals[0].spawned=false;
  v.Advance(5000);assert(!v.p.packets&&!runs.count(v.p.guid));portals[0].spawned=true;
 }
 {Visit v;v.p.x+=21;assert(!v.Cast());v.At(0);v.p.bags.clear();assert(!v.Cast());v.p.bags[Horn]=1;auto s=v.Prepare();s->item=nullptr;assert(s->Check()!=SPELL_CAST_OK);s=v.Prepare();v.horn.owner={999};assert(s->Check()!=SPELL_CAST_OK);}
 {Visit v;Creature decoy;decoy.entry=97983;decoy.guid={9999};static_cast<Position&>(decoy)=Portals[0].point;creatures[decoy.guid]=&decoy;assert(v.Cast()&&runs[v.p.guid].marker==markers[0].guid);creatures.erase(decoy.guid);}
 {Visit a,b;GroupReference last{&b.p,nullptr},first{&a.p,&last};Group group{&first};a.p.group=b.p.group=&group;Spell native;native.unitTarget=&a.p;native.EffectKillCredit(0);a.p.KilledMonsterCredit(98186);a.p.TalkedToCreature(97983,markers[0].guid);assert(!a.p.packets&&!b.p.packets);auto& other=store.quests[999];other.id=999;other.special=QUEST_SPECIAL_FLAGS_KILL;other.objectives={{0,97983,1,0,0}};a.p.Accept(999);b.p.Accept(999);native.EffectKillCredit(0);assert(a.p.GetQuestObjectiveData(999,97983)==1&&b.p.GetQuestObjectiveData(999,97983)==1&&!a.p.GetQuestObjectiveData(QuestId,97983));}
 {Visit v;ConditionSourceInfo info;std::set<uint32> entries{97983,97984,97985};WorldObjectSpellTargetCheck check{&v.p,&targetSpell,&info,&entries};assert(check(&markers[0]));Creature decoy;decoy.entry=123;assert(!check(&decoy));markers[0].expired=true;assert(!check(&markers[0]));markers[0].expired=false;} // actual native condition admission prefix
 {Visit v;auto& go=portals[0];go.info.type=8;GameObjectFocusCheck check(&v.p,1887);assert(check(&go));v.p.x+=13;assert(!check(&go));v.At(0);go.info.spellFocus.spellFocusType=1;assert(!check(&go));go.info.spellFocus.spellFocusType=1887;go.info.type=10;assert(!check(&go));go.info.type=8;} // actual native required-focus predicate
 std::cout<<"PASS: complete production40003 spell/marker/portal/player handlers; clocked5000ms acceptance/cast tickets, three distinct counters and aggregate, group separation, retry/abandon/transfer/logout/saved visibility, optional summon/cast failure, native generic group guards and condition-admission prefix. Scheduler, inventory, world/grid, spline/visibility/network/condition and full cast-engine services are fixtures; actual client flight/impact/quest handoff remain pending.\n";
}
