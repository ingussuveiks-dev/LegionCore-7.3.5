#include <array>
#include <mutex>
constexpr int QUEST_SPECIAL_FLAGS_SPEAKTO=128;
#include "RunewoodFixture.inc"
#include "Talked.inc"
using Map=FakeMap;
struct TemplateInfo {uint32 ScriptID=1;};
struct Templates {TemplateInfo info;TemplateInfo* GetCreatureTemplate(uint32){return &info;}std::string GetScriptName(uint32){return "npc_stormheim_runewood_stone";}} templates;auto sObjectMgr=&templates;
struct SpellTargetPosition {uint32 target_mapId=1220;float target_X,target_Y,target_Z;};
struct Spells {std::map<uint32,SpellTargetPosition> positions{{184582,{1220,3116.57f,1247.91f,125.037f}},{184792,{1220,2992.34f,1126.41f,130.259f}},{184794,{1220,3181.52f,1054.34f,117.037f}}};SpellTargetPosition* GetSpellTargetPosition(uint32 id){return &positions.at(id);}} spells;auto sSpellMgr=&spells;
struct RuneHook{void operator+=(int){}};
struct RuneSpellScript {uint32 m_scriptSpellId=0;Unit* caster=nullptr,*original=nullptr,*target=nullptr;RuneHook OnCheckCast,OnEffectHitTarget,AfterCast;Unit* GetCaster(){return caster;}Unit* GetOriginalTarget(){return original;}Creature* GetHitCreature(){return target?target->ToCreature():nullptr;}virtual void Register(){}};
struct PlayerScript {explicit PlayerScript(char const*){}virtual void OnUpdate(Player*,uint32){}virtual void OnLogin(Player*){}virtual void OnMapChanged(Player*){}virtual void OnLogout(Player*){}};
struct RuneSmartAI:SmartAI {using SmartAI::SmartAI;virtual void IsSummonedBy(Unit*){}virtual void SpellHit(Unit*,struct SpellInfo const*){++delegated;}};
struct AuraApplication {Unit* target;Unit* GetTarget()const{return target;}};
struct RuneAuraEffect {int m_diffMode=0;SpellInfo info;int GetEffIndex()const{return 0;}SpellInfo const* GetSpellInfo()const{return &info;}void HandleSummonController(AuraApplication const*,uint8,bool)const;};
#define AuraEffect RuneAuraEffect
#include "Controller.inc"
#undef AuraEffect
#undef PrepareSpellScript
#define PrepareSpellScript(x) public:
#define SpellScript RuneSpellScript
#define SmartAI RuneSmartAI
#define SpellCheckCastFn(x) 1
#define SpellEffectFn(x,i,t) 1
#define SpellCastFn(x) 1
#include "Runewood.inc"
#undef SmartAI
#undef SpellScript
uint64 nextGuid=1000;
struct Run {
 Player p;player_stormheim_runewood script;std::vector<std::unique_ptr<Creature>> actors;std::vector<std::unique_ptr<npc_stormheim_runewood_stone>> ais;bool fail=false;
 Run(uint32 team=ALLIANCE){p.guid={++nextGuid};p.team=team;p.race=team==ALLIANCE?1:2;p.Accept(38778);p.castTargetHook=[&](Unit*,uint32 id){for(size_t i=0;i<3;++i){auto const& site=StormheimRunewood::Sites[i];if(id==site.controller){p.auras.insert(id);RuneAuraEffect aura;aura.info.TriggerSpell=site.summon;AuraApplication app{&p};aura.HandleSummonController(&app,AURA_EFFECT_HANDLE_REAL,true);return 0;}if(id==site.summon){spell_stormheim_runewood_summon summon;summon.caster=&p;summon.m_scriptSpellId=id;assert(summon.Check()==SPELL_CAST_OK);if(fail)return 1;auto c=std::make_unique<Creature>();c->guid={++nextGuid};c->entry=site.entry;static_cast<Position&>(*c)=site.point;c->fields[UNIT_FIELD_NPC_FLAGS]=UNIT_NPC_FLAG_SPELLCLICK;auto ai=std::make_unique<npc_stormheim_runewood_stone>(c.get());creatures[c->guid]=c.get();ai->IsSummonedBy(&p);ais.push_back(std::move(ai));actors.push_back(std::move(c));return 0;}}return 0;};}
 ~Run(){script.OnLogout(&p);for(auto const& c:actors)creatures.erase(c->guid);}
 void Visit(int i){static_cast<Position&>(p)=StormheimRunewood::Sites[i].point;script.OnUpdate(&p,500);}
 Creature* Stone(int i){return ObjectAccessor::GetCreature(p,StormheimRunewood::attempts.at(p.guid).actors[i]);}
 spell_stormheim_runewood_turn Cast(int i){auto c=Stone(i);spell_stormheim_runewood_turn s;s.caster=&p;s.original=c;s.target=c;return s;}
 void Turn(int i){auto c=Stone(i);auto s=Cast(i);assert(s.Check()==SPELL_CAST_OK);SpellInfo info;c->AI()->SpellHit(&p,&info);assert(!p.GetQuestObjectiveData(38778,93196)||p.GetQuestStatus(38778)==QUEST_STATUS_INCOMPLETE);s.Hit(0);s.Complete();assert(p.GetQuestObjectiveData(38778,StormheimRunewood::Sites[i].marker)==1&&!c->HasFlag(UNIT_FIELD_NPC_FLAGS,UNIT_NPC_FLAG_SPELLCLICK));}
};
int main(){
 auto& q=store.quests[38778];q.id=38778;q.special=QUEST_SPECIAL_FLAGS_KILL;q.PrevQuestID=39796;q.NextQuestID=38810;q.NextQuestIdChain=38810;q.objectives={{0,93196,3,0,0},{0,93185,1,28,1},{0,93340,1,28,2},{0,93341,1,28,3}};
 auto& travel=store.quests[39796];travel.id=39796;travel.PrevQuestID=39804;travel.NextQuestID=38778;travel.NextQuestIdChain=38778;store.quests[39804].id=39804;store.quests[39804].NextQuestID=39796;auto& next=store.quests[38810];next.id=38810;next.PrevQuestID=38778;
 QuestMap _questTemplates;for(auto& e:store.quests)_questTemplates[e.first]=&e.second;for(auto const& entry:_questTemplates){auto qinfo=entry.second;
#include "QuestPrevLoader.inc"
 }
 Creature vydhar;vydhar.guid={93231};vydhar.entry=93231;creatures[vydhar.guid]=&vydhar;store.starters.emplace(93231,38778);store.starters.emplace(93231,38810);
 for(auto team:{ALLIANCE,HORDE}){Run r(team);r.p.quests.clear();assert(!r.p.CanTakeQuest(&travel,false));r.p.m_RewardedQuests.insert(39804);assert(r.p.CanTakeQuest(&travel,false));r.p.Accept(39796);assert(r.p.CanCompleteQuest(39796));r.p.CompleteQuest(39796);assert(r.p.CanRewardQuest(&travel,false));r.p.m_RewardedQuests.insert(39796);r.p.quests.erase(39796);assert(r.p.CanTakeQuest(&q,false)&&r.p.GetNextQuest(vydhar.GetGUID(),&travel)==&q);r.p.Accept(38778);
 for(int i=0;i<3;++i){r.Visit(i);assert(r.Stone(i)&&!r.p.GetQuestObjectiveData(38778,StormheimRunewood::Sites[i].marker));r.Turn(i);assert(r.p.GetQuestObjectiveData(38778,93196)==i+1);auto repeat=r.Cast(i);assert(repeat.Check()==SPELL_FAILED_BAD_TARGETS);r.script.OnUpdate(&r.p,15000);assert(r.p.GetQuestObjectiveData(38778,93196)==i+1);}
 assert(r.p.GetQuestStatus(38778)==QUEST_STATUS_COMPLETE&&r.p.CanRewardQuest(&q,false));r.p.m_RewardedQuests.insert(38778);r.p.quests.erase(38778);assert(r.p.CanTakeQuest(&next,false)&&r.p.GetNextQuest(vydhar.GetGUID(),&q)==&next);}
 {Run r;r.Visit(0);auto s=r.Cast(0);assert(s.Check()==SPELL_CAST_OK);s.Complete();assert(!r.p.GetQuestObjectiveData(38778,93196));r.fail=true;r.Visit(1);assert(!r.Stone(1));r.fail=false;for(int i=0;i<4;++i)r.script.OnUpdate(&r.p,500);assert(r.Stone(1));}
 for(int why=0;why<7;++why){Run r;r.Visit(0);auto s=r.Cast(0);assert(s.Check()==SPELL_CAST_OK);if(why==0)r.p.Accept(38778);if(why==1)r.p.phase=false;if(why==2)r.p.visible=false;if(why==3)r.p.los=false;if(why==4)r.p.alive=false;if(why==5)r.p.teleport=true;if(why==6)r.p.x+=7;s.Hit(0);s.Complete();assert(!r.p.GetQuestObjectiveData(38778,93196));}
 {Run r,other;r.Visit(0);other.Visit(0);auto s=r.Cast(0);s.original=other.Stone(0);s.target=other.Stone(0);assert(s.Check()==SPELL_FAILED_BAD_TARGETS);r.Turn(0);assert(!other.p.GetQuestObjectiveData(38778,93196));}
 {Run r;r.Visit(0);auto c=r.Stone(0);r.p.map=1;r.script.OnMapChanged(&r.p);assert(!c->world&&!StormheimRunewood::attempts.count(r.p.guid)&&!r.p.HasAura(192155));}
 {Run r;r.Visit(0);r.Turn(0);r.script.OnLogout(&r.p);r.script.OnLogin(&r.p);r.Visit(0);assert(!r.Stone(0)&&r.p.GetQuestObjectiveData(38778,93196)==1);r.Visit(1);assert(r.Stone(1));}
 {Run r;r.p.data[{38778,93196}]=3;r.p.quests[38778].Status=QUEST_STATUS_COMPLETE;r.script.OnLogin(&r.p);assert(r.p.GetQuestStatus(38778)==QUEST_STATUS_INCOMPLETE&&!r.p.GetQuestObjectiveData(38778,93196));r.p.data[{38778,93185}]=1;r.p.data[{38778,93340}]=1;r.p.data[{38778,93341}]=1;r.script.OnLogin(&r.p);assert(r.p.GetQuestStatus(38778)==QUEST_STATUS_COMPLETE&&r.p.GetQuestObjectiveData(38778,93196)==3);}
 {Run r;auto& ordinary=store.quests[55555];ordinary=q;ordinary.id=55555;r.p.Accept(55555);for(uint32 entry:{93196u,93185u,93340u,93341u}){r.p.KilledMonsterCredit(entry);assert(!r.p.GetQuestObjectiveData(38778,entry)&&r.p.GetQuestObjectiveData(55555,entry)==1);r.p.data[{55555,entry}]=0;r.p.TalkedToCreature(entry,{42});assert(!r.p.GetQuestObjectiveData(38778,entry)&&r.p.GetQuestObjectiveData(55555,entry)==1);}}
 {Run r;r.Visit(0);auto old=r.Stone(0);r.p.Accept(38778);r.script.OnUpdate(&r.p,500);assert(!old->world&&!StormheimRunewood::attempts.count(r.p.guid));r.Visit(0);assert(r.Stone(0)&&r.Stone(0)!=old);}
 {Creature unrelated;unrelated.entry=93182;npc_stormheim_runewood_stone ai(&unrelated);SpellInfo spell;ai.SpellHit(nullptr,&spell);assert(ai.delegated==1);}
 {Run r;r.p.world=false;spell_stormheim_runewood_summon summon;summon.caster=&r.p;summon.m_scriptSpellId=184582;assert(summon.Check()==SPELL_FAILED_BAD_TARGETS);r.p.world=true;r.p.map=1;for(auto const& site:StormheimRunewood::Sites)r.p.auras.insert(site.controller);r.script.OnLogin(&r.p);for(auto const& site:StormheimRunewood::Sites)assert(!r.p.HasAura(site.controller));}
 {Run r;r.Visit(0);auto old=r.Stone(0);r.p.phase=false;r.script.OnUpdate(&r.p,500);assert(!old->world);r.p.phase=true;for(int i=0;i<4;++i)r.script.OnUpdate(&r.p,500);assert(r.Stone(0)&&r.Stone(0)!=old);r.Turn(0);assert(r.p.GetQuestObjectiveData(38778,93196)==1);}
 {Run r;r.Visit(0);Creature pet;spell_stormheim_runewood_turn spell;spell.caster=&pet;spell.original=r.Stone(0);spell.target=r.Stone(0);assert(spell.Check()==SPELL_CAST_OK);spell.Hit(0);spell.Complete();SpellInfo info;r.Stone(0)->AI()->SpellHit(&pet,&info);assert(!r.p.GetQuestObjectiveData(38778,93196));}
 for(int why=0;why<2;++why){Run r;r.Visit(0);auto old=r.Stone(0);if(why==0)r.p.alive=false;else r.p.zone=0;r.script.OnUpdate(&r.p,500);assert(!old->world&&!StormheimRunewood::attempts.count(r.p.guid)&&!r.p.HasAura(192155));}
 spell_stormheim_runewood_turn turn;turn.Register();spell_stormheim_runewood_summon summon;summon.Register();
 std::cout<<"PASS: production Runewood lifecycle, native summon-controller handler, actual unique markers/total/completion and generic-credit guard; both-race39796 travel/Vydhar38778/38810 handoff, three distinct turns, duplicate/stale/no-hit failure, personal isolation, failed summon retry, logout/map/death and legacy normalization. Movement, summoning engine, spell targeting, packets and rendering remain fixtures.\n";
}
