#include <array>
#include <sstream>
#include "AscendantFixture.inc"
enum {CONDITION_QUEST_OBJECTIVE_DONE=41};
struct ConditionSourceInfo {WorldObject* targets[2];ConditionSourceInfo(WorldObject* a,WorldObject* b=nullptr):targets{a,b}{}};
struct Condition {uint32 ConditionType=0,ConditionTarget=0,ConditionValue1=0,ConditionValue2=0,ConditionValue3=0,ElseGroup=0,ReferenceId=0;bool isLoaded()const{return true;}bool Meets(ConditionSourceInfo& info){auto object=info.targets[ConditionTarget];bool condMeets=false;switch(ConditionType){
#include "CommunionMenuConditions.inc"
}return condMeets;}};
using ConditionList=std::vector<Condition*>;using ConditionReferenceContainer=std::map<uint32,ConditionList>;
struct ConditionMgr {ConditionReferenceContainer ConditionReferenceStore;bool IsObjectMeetToConditionList(ConditionSourceInfo&,ConditionList const&)const;bool IsObjectMeetToConditions(WorldObject*,ConditionList const&);bool IsObjectMeetToConditions(WorldObject*,WorldObject*,ConditionList const&);bool IsObjectMeetToConditions(ConditionSourceInfo&,ConditionList const&)const;} conditionMgr;
#include "ShieldsConditionGroups.inc"
constexpr float INTERACTION_DISTANCE=5;
struct CommunionScript {
 Unit* caster=nullptr;Player* hit=nullptr;bool prevented=false;Hook OnCheckCast,OnEffectHitTarget;
 Unit* GetCaster(){return caster;}Player* GetHitPlayer(){return hit;}
 void PreventHitDefaultEffect(int){prevented=true;}virtual bool Load(){return true;}virtual void Register(){}
};
struct PlayerScript {explicit PlayerScript(char const*){}virtual void OnLogin(Player*){}};
#define SpellScript CommunionScript
#define SpellCheckCastFn(x) 1
#define SpellEffectFn(x,i,e) 2
#define TC_LOG_INFO(...) (void)0
#include "AscendantProduction.inc"
enum {SPELL_EFFECT_HANDLE_HIT_TARGET=1};
struct NativeCreditSpell {int effectHandleMode=1;Unit* unitTarget=nullptr;uint32 m_diffMode=0;
 struct Info {struct Effect {uint32 MiscValue=94477;} effect;Effect* GetEffect(int,uint32){return &effect;}} info;Info* m_spellInfo=&info;
 void EffectKillCredit(SpellEffIndex);
};
#include "NativeGroupEffect.inc"
void Finish(spell_stormheim_communing& cast){cast.Credit(0);if(!cast.prevented){NativeCreditSpell native;native.unitTarget=cast.hit;native.EffectKillCredit(0);}}
void Four(Player& p){for(auto id:{242994u,242995u,242996u,242998u})p.KillCreditGO(id,ObjectGuid::Empty);}
int main(int argc,char**argv){
 _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);_CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE);_CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);
 assert(argc==2);std::ifstream input(argv[1]);assert(input);std::string line;std::list<Condition> menu;ConditionList gates;
 while(std::getline(input,line)){std::istringstream in(line);std::string kind;in>>kind;
  if(kind=="quest"){uint32 id,ex,start,package;in>>id;auto& q=store.quests[id];q.id=id;in>>q.flags>>ex>>q.AllowableRaces>>start>>q.NextQuestIdChain>>q.PrevQuestID>>q.NextQuestID>>q.ExclusiveGroup>>q.special>>package;}
  if(kind=="objective"){uint32 qid,id;QuestObjective o;int index;in>>qid>>id>>o.Type>>o.ObjectID>>o.Amount>>index>>o.Flags;o.StorageIndex=int8(index);store.quests[qid].objectives.push_back(o);}
  if(kind=="starter"){uint32 entry,id;in>>entry>>id;store.starters.emplace(entry,id);}
  if(kind=="condition"){menu.emplace_back();auto& c=menu.back();uint32 negative;in>>c.ConditionType>>c.ConditionTarget>>c.ConditionValue1>>c.ConditionValue2>>c.ConditionValue3>>negative>>c.ElseGroup;assert(!negative&&!c.ElseGroup);gates.push_back(&c);}
 }
 auto& q=store.quests[39092];q.special|=QUEST_SPECIAL_FLAGS_CAST|QUEST_SPECIAL_FLAGS_KILL;assert(q.objectives.size()==5&&q.objectives.back().Flags==2);
 {auto& prev=store.quests[39063];prev.id=39063;prev.NextQuestID=39092;QuestMap _questTemplates;for(auto& row:store.quests)_questTemplates[row.first]=&row.second;for(auto const& row:_questTemplates){auto qinfo=row.second;
#include "QuestPrevLoader.inc"
  }
  Player p;assert(!p.CanTakeQuest(&q,false));p.m_RewardedQuests.insert(39063);assert(p.CanTakeQuest(&q,false)&&!p.CanTakeQuest(&store.quests[39122],false));p.m_RewardedQuests.insert(39092);assert(p.CanTakeQuest(&store.quests[39122],false));Creature genn;genn.entry=95804;genn.guid={901};creatures[genn.guid]=&genn;assert(p.GetNextQuest(genn.guid,&q)==&store.quests[39122]);creatures.erase(genn.guid);
 }
 Creature statue;statue.entry=94393;statue.dbGuid=293383;statue.guid={900};creatures[statue.guid]=&statue;npc_stormheim_ascendant_statue ai(&statue);
 {Player p,near;p.Accept(39092);near.Accept(39092);GroupReference b{&near,nullptr},a{&p,&b};Group group;group.first=&a;p.group=&group;
  NativeCreditSpell native;native.unitTarget=&p;native.EffectKillCredit(0);assert(p.GetQuestObjectiveData(39092,94477)==1&&near.GetQuestObjectiveData(39092,94477)==1&&!p.CanCompleteQuest(39092));
 } // Actual effect134 and quest handlers reproduce premature group credit; SEQUENCED is not an admission gate.
 std::array<uint32,4> order{242994,242995,242996,242998};unsigned orders=0;
 do{Player p;p.Accept(39092);unsigned count=0;
  for(auto id:order){assert(gates.size()==5&&!conditionMgr.IsObjectMeetToConditions(&p,gates));p.lastCast=0;ai.sGossipSelect(&p,18543,0);assert(!p.lastCast);spell_stormheim_communing early;early.caster=&p;early.hit=&p;assert(early.Load()&&early.Check()==SPELL_FAILED_BAD_TARGETS);Finish(early);assert(!p.GetQuestObjectiveData(39092,94477));
   GameObject go;go.entry=go.info.entry=id;go.info.goober.questID=0;go.info.goober.spell=(id==242994?186965:id==242995?187780:id==242996?186964:186972);go.Use(&p);assert(p.GetQuestObjectiveData(39092,id)==1&&!p.CanCompleteQuest(39092));go.flags=0;go.Use(&p);assert(p.GetQuestObjectiveData(39092,id)==1);++count;
  }
  assert(count==4&&conditionMgr.IsObjectMeetToConditions(&p,gates));p.auras.clear(); // Expired blessings do not invalidate completed objective records.
  ai.sGossipSelect(&p,18543,0);assert(p.lastCast==186982&&!p.lastTriggered&&!p.GetQuestObjectiveData(39092,94477));
  spell_stormheim_communing cast;cast.caster=&p;cast.hit=&p;assert(cast.Load()&&cast.Check()==SPELL_CAST_OK);cast.Register();assert(cast.OnCheckCast.values.size()==1&&cast.OnEffectHitTarget.values.size()==1);Finish(cast);assert(cast.prevented&&p.GetQuestObjectiveData(39092,94477)==1&&p.CanRewardQuest(&q,false));auto packets=p.packets;Finish(cast);assert(p.packets==packets);++orders;
 }while(std::next_permutation(order.begin(),order.end()));assert(orders==24);
 for(unsigned failure=0;failure<10;++failure){Player p;p.Accept(39092);Four(p);spell_stormheim_communing cast;cast.caster=&p;cast.hit=&p;assert(cast.Check()==SPELL_CAST_OK);
  switch(failure){case 0:statue.distance=6;break;case 1:p.phase=false;break;case 2:p.alive=false;break;case 3:statue.world=false;break;case 4:p.world=false;break;case 5:p.map=1;break;case 6:statue.dbGuid=1;break;case 7:p.quests.clear();break;case 8:p.Accept(39092);Four(p);break;case 9:p.zone=1;break;}
  assert(cast.Check()==SPELL_FAILED_BAD_TARGETS);Finish(cast);assert(cast.prevented&&!p.GetQuestObjectiveData(39092,94477));statue.distance=0;statue.world=true;statue.dbGuid=293383;
 }
 {Player p,near;p.Accept(39092);near.Accept(39092);Four(p);Four(near);GroupReference b{&near,nullptr},a{&p,&b};Group g;g.first=&a;p.group=&g;
  spell_stormheim_communing interrupted;interrupted.caster=&p;interrupted.hit=&p;assert(interrupted.Check()==SPELL_CAST_OK);assert(!p.GetQuestObjectiveData(39092,94477)); // Interrupted/failed casts never dispatch a hit.
  spell_stormheim_communing wrong;wrong.caster=&p;wrong.hit=&near;assert(wrong.Check()==SPELL_CAST_OK);Finish(wrong);assert(wrong.prevented&&!near.GetQuestObjectiveData(39092,94477));
  spell_stormheim_communing cast;cast.caster=&p;cast.hit=&p;assert(cast.Check()==SPELL_CAST_OK);Finish(cast);assert(p.GetQuestObjectiveData(39092,94477)==1&&!near.GetQuestObjectiveData(39092,94477));
 }
 player_stormheim_communion_recovery recovery;
 for(unsigned count=0;count<=4;++count){Player p;p.Accept(39092);for(unsigned i=0;i<count;++i)p.KillCreditGO(order[i],ObjectGuid::Empty);p.data[{39092,94477}]=1;auto before=p.data;recovery.OnLogin(&p);assert(p.GetQuestObjectiveData(39092,94477)==int(count==4));for(unsigned i=0;i<4;++i)assert((p.GetQuestObjectiveData(39092,order[i])==before[{39092,order[i]}]));}
 for(auto state:{QUEST_STATUS_COMPLETE,QUEST_STATUS_REWARDED,QUEST_STATUS_NONE}){Player p;p.quests[39092].Status=state;p.data[{39092,94477}]=1;recovery.OnLogin(&p);assert(p.GetQuestObjectiveData(39092,94477)==1);}
 {Player p;p.Accept(39092);Four(p);ai.sGossipSelect(&p,999,0);assert(ai.delegated==1&&!p.lastCast);statue.dbGuid=1;ai.sGossipSelect(&p,18543,0);assert(!p.lastCast);statue.dbGuid=293383;}
 {Player p;Spell opening;opening.m_caster=&p;SkillType type;int32 required=0,value=0;assert(opening.CanOpenLock(0,43,type,required,value)==SPELL_CAST_OK);opening.info.effect.MiscValue=5;assert(opening.CanOpenLock(0,43,type,required,value)!=SPELL_CAST_OK);}
 std::cout<<"PASS: actual communion AI/spell/recovery and native quest/goober/group/lock handlers;24 shrine orders, early/repeat/group/interrupt/wrong-hit/phase/range/death/map/logout/reaccept boundaries, personal completion and scoped legacy repair. Cast dispatch, world lookup and packets are fixtures; rendered rites and saved login remain client checks.\n";
}
