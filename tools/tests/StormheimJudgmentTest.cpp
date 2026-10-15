#include <array>
#include <mutex>
constexpr int QUEST_SPECIAL_FLAGS_SPEAKTO=128;
#include "JudgmentFixture.inc"
#include "Talked.inc"
constexpr uint32 GAMEOBJECT_TYPE_GOOBER=10;
struct Templates {
    std::map<uint32,GameObjectTemplate> objects;
    std::map<uint32,CreatureTemplate> actors;
    GameObjectTemplate* GetGameObjectTemplate(uint32 id){auto it=objects.find(id);return it==objects.end()?nullptr:&it->second;}
    CreatureTemplate* GetCreatureTemplate(uint32 id){auto it=actors.find(id);return it==actors.end()?nullptr:&it->second;}
    std::string GetScriptName(uint32 id){return id==1?"go_stormheim_judgment":id==2?"npc_stormheim_judgment_king":"custom";}
} templates;auto sObjectMgr=&templates;
struct PlayerScript {explicit PlayerScript(char const*){}virtual void OnUpdate(Player*,uint32){}virtual void OnMapChanged(Player*){}virtual void OnLogout(Player*){}};
constexpr uint32 SPELL_EFFECT_OPEN_LOCK=33;
struct OpeningScript {Unit* caster=nullptr;GameObject* original=nullptr,*hit=nullptr;bool prevented=false;Hook OnCheckCast,OnEffectHitTarget;Unit* GetCaster(){return caster;}GameObject* GetExplTargetGObj(){return original;}GameObject* GetHitGObj(){return hit;}void PreventHitDefaultEffect(int){prevented=true;}virtual void Register(){}};
#define SpellScript OpeningScript
#define SpellCheckCastFn(x) 1
#define SpellEffectFn(x,i,t) (i*100+t)
#include "Judgment.inc"
#undef SpellScript

// Extracted native lock validation and post-validation EffectOpenLock dispatch.
// Target resolution, cast timing/interrupts and packet delivery are fixtures.
using SkillType=int;using LockType=int;
enum {SKILL_NONE=0,SKILL_LOCKPICKING=1,SKILL_HERBALISM=2,SKILL_MINING=3,SKILL_FISHING=4,SKILL_INSCRIPTION=5,SKILL_ARCHAEOLOGY=6,
 LOCKTYPE_PICKLOCK=1,LOCKTYPE_HERBALISM=2,LOCKTYPE_MINING=3,LOCKTYPE_FISHING=19,LOCKTYPE_INSCRIPTION=20,LOCKTYPE_ARCHAELOGY=22,
 LOCK_KEY_ITEM=1,LOCK_KEY_SKILL=2,LOCK_KEY_SPELL=3,MAX_LOCK_CASE=8,TYPEID_PLAYER=4,TARGET_GAMEOBJECT_ITEM_TARGET=23,
 SPELL_FAILED_LOW_CASTLEVEL=2,SPELL_FAILED_CUSTOM_ERROR=3,ARMY_UNIT_BERSERK=0,
 SPELL_CUSTOM_ERROR_REQUIRES_2_WITHERED_BERSERKER=1,SPELL_CUSTOM_ERROR_REQUIRES_AT_LEAST_10_WITHERED=2,
 LOOT_SKINNING=1,ITEM_FIELD_DYNAMIC_FLAGS=1,ITEM_FLAG_UNLOCKED=1,ITEM_CHANGED=1};
struct LockEntry {uint32 Type[8]{0,2},Index[8]{0,5},Skill[8]{};};
struct LockStore {LockEntry value;LockEntry const* LookupEntry(uint32 id){return id==1691?&value:nullptr;}} locks;auto& sLockStore=locks;
struct Item {uint32 GetEntry(){return 0;}void SetFlag(uint32,uint32){}void SetState(uint32,Player*){}Player* GetOwner(){return nullptr;}};
struct LockEffect {int MiscValue=5;struct Target {uint32 GetTarget(){return 23;}} TargetA,TargetB;int CalcValue(Unit*,void*){return 0;}};
struct LockSpellInfo {uint32 Id=3365;LockEffect effect;LockEffect* GetEffect(uint32,uint32){return &effect;}};
struct Spell {
    Unit* m_caster;LockSpellInfo info;LockSpellInfo* m_spellInfo=&info;uint32 m_diffMode=0;Item* m_CastItem=nullptr;uint32 m_customError=0;
    GameObject* gameObjTarget;Item* itemTarget=nullptr;uint32 sends=0,failed=0;
    spell_stormheim_judgment_opening opening;bool started=false;
    Spell(Unit* p,GameObject* g):m_caster(p),gameObjTarget(g){opening.caster=p;opening.original=g;}
    bool Start(){started=true;if(opening.Check()!=SPELL_CAST_OK){++failed;return false;}return true;}
    SpellCastResult CanOpenLock(uint32,uint32,SkillType&,int32&,int32&);void Complete();
    void SendCastResult(int){++failed;}void SendLoot(ObjectGuid,int){++sends;}
};
#include "CanOpenLock.inc"
void Spell::Complete(){if(!started&&!Start())return;opening.hit=gameObjTarget;opening.Hit(0);if(opening.prevented)return;Player* player=m_caster->ToPlayer();if(!player)return;uint32 lockId=gameObjTarget->GetGOInfo()->goober.open;uint32 effIndex=0;ObjectGuid guid=gameObjTarget->GetGUID();
#include "OpenLockCompletion.inc"
}
uint64 nextGuid=50000;
std::array<GameObject,3> bowls;
std::array<std::unique_ptr<go_stormheim_judgment_ai>,3> bowlAI;
struct Run {
    Player p;player_stormheim_judgment script;bool failKing=false,failHelper=false,failVisual=false;
    std::vector<std::unique_ptr<Creature>> actors;std::vector<std::unique_ptr<SmartAI>> ais;
    Run(uint32 team=ALLIANCE){
        p.guid={++nextGuid};p.team=team;p.race=team==ALLIANCE?1:2;p.Accept(38811);players[p.guid]=&p;
        p.judgmentSummon=[&](uint32 entry,Position const& pos,ObjectGuid viewer)->Creature*{
            assert(viewer==p.guid); // visibility owner is supplied before native construction
            if((entry==35845&&failHelper)||(entry!=35845&&failKing))return nullptr;
            auto c=std::make_unique<Creature>();c->guid={++nextGuid};c->entry=entry;static_cast<Position&>(*c)=pos;c->personal=viewer;
            c->castTargetHook=[&](Unit*,uint32 id){return id==184831&&failVisual?1:0;};
            SmartAI* ai=entry==35845?new SmartAI(c.get()):new npc_stormheim_judgment_king(c.get());
            creatures[c->guid]=c.get();actors.push_back(std::move(c));ais.emplace_back(ai);ai->IsSummonedBy(&p);return actors.back().get();
        };
    }
    ~Run(){script.OnLogout(&p);for(auto& c:actors)creatures.erase(c->guid);players.erase(p.guid);}
    void Move(int i){static_cast<Position&>(p)=StormheimJudgment::Sites[i].bowlPoint;}
    void Open(int i){Move(i);Spell s(&p,&bowls[i]);s.Complete();assert(!s.sends);}
    void Tick(uint32 ms){script.OnUpdate(&p,ms);for(auto& c:actors)if(c->world&&c->alive&&c->IsAIEnabled)c->AI()->UpdateAI(ms);}
    Creature* King(){auto it=StormheimJudgment::runs.find(p.guid);return it==StormheimJudgment::runs.end()?nullptr:p.GetMap()->GetCreature(it->second.king);}
    void Fight(int i){Open(i);Tick(7499);assert(!King());Tick(1);assert(King()&&King()->personal==p.guid&&King()->talkGroups==std::vector<uint8>{0});
        uint32 d=1000;King()->AI()->DamageTaken(&p,d,0);assert(!d);
        if(i!=2){Tick(5999);assert(King()->talks==1);Tick(1);assert(King()->talks==2);Tick(4999);assert(!King()->AI()->CanAIAttack(&p));Tick(1);}
        else{Tick(3999);assert(!King()->AI()->CanAIAttack(&p));Tick(1);}
        assert(King()->AI()->CanAIAttack(&p));d=1000;King()->AI()->DamageTaken(&p,d,0);assert(d==1000);}
    void Kill(Unit* killer=nullptr){auto king=King();assert(king);king->alive=false;
        // Native Unit::Kill delivers generic reward before CreatureAI::JustDied.
        p.KilledMonsterCredit(king->entry,king->guid);king->AI()->JustDied(killer?killer:&p);}
};
int main()
{
    auto& q=store.quests[38811];q.id=38811;q.PrevQuestID=38810;q.NextQuestID=38817;q.NextQuestIdChain=38817;q.special=QUEST_SPECIAL_FLAGS_KILL;
    for(int i=0;i<3;++i){auto const& s=StormheimJudgment::Sites[i];q.objectives.push_back({0,s.king,1,0,int8(i)});
        templates.objects[s.bowl]={};templates.actors[s.king]={};auto& b=bowls[i];b.guid={uint64(40000+i)};b.dbGuid=s.spawn;b.entry=s.bowl;b.flags=262148;static_cast<Position&>(b)=s.bowlPoint;objects[b.guid]=&b;bowlAI[i]=std::make_unique<go_stormheim_judgment_ai>(&b);}
    auto& previous=store.quests[38810];previous.id=38810;previous.NextQuestID=38811;
    auto& next=store.quests[38817];next.id=38817;next.PrevQuestID=39791;
    auto& alt=store.quests[39791];alt.id=39791;alt.NextQuestID=38817;
    QuestMap _questTemplates;for(auto& e:store.quests)_questTemplates[e.first]=&e.second;for(auto& entry:_questTemplates){auto qinfo=entry.second;
#include "QuestPrevLoader.inc"
    }
    store.starters.emplace(93446,38817);Creature iounn;iounn.entry=93446;iounn.guid={291874};creatures[iounn.guid]=&iounn;
    for(auto team:{ALLIANCE,HORDE}){Run r(team);r.p.quests.clear();assert(!r.p.CanTakeQuest(&q,false));r.p.m_RewardedQuests.insert(38810);assert(r.p.CanTakeQuest(&q,false));r.p.Accept(38811);
        for(int i=0;i<3;++i){r.Move(i);go_stormheim_judgment script;assert(script.OnGossipHello(&r.p,&bowls[i]));bowlAI[i]->GossipHello(&r.p,false);bowlAI[i]->GossipUse(&r.p);r.Tick(10000);assert(!r.King()&&!StormheimJudgment::runs.count(r.p.guid));
            Spell wrong(&r.p,&bowls[i]);wrong.info.effect.MiscValue=13;wrong.Complete();assert(wrong.failed&&!StormheimJudgment::runs.count(r.p.guid));
            r.Fight(i);auto king=r.King();auto id=king->entry;assert(r.p.GetQuestObjectiveData(38811,id)==0);
            auto count=r.actors.size();r.Open(i);r.Tick(1);assert(r.King()==king&&r.actors.size()==count);
            Unit pet;pet.ownerPlayer=&r.p;r.Kill(i==1?&pet:nullptr);assert(r.p.GetQuestObjectiveData(38811,id)==1&&king->despawnDelay==3000);
            king->AI()->JustDied(&r.p);assert(r.p.GetQuestObjectiveData(38811,id)==1);r.Open(i);assert(!StormheimJudgment::runs.count(r.p.guid));
        }
        assert(r.p.GetQuestStatus(38811)==QUEST_STATUS_COMPLETE&&r.p.completions==1&&r.p.CanRewardQuest(&q,false));
        assert(!r.p.CanTakeQuest(&next,false));r.p.m_RewardedQuests.insert(38811);r.p.quests.erase(38811);
        assert(r.p.CanTakeQuest(&next,false)&&r.p.GetNextQuest(iounn.guid,&q)==&next);
    }
    {Run a,b;a.Fight(0);b.Fight(0);auto king=a.King();assert(king!=b.King());uint32 d=999;king->AI()->DamageTaken(&b.p,d,0);assert(!d&&!king->AI()->CanAIAttack(&b.p));a.Kill();assert(a.p.GetQuestObjectiveData(38811,92889)==1&&!b.p.GetQuestObjectiveData(38811,92889));b.Kill();assert(b.p.GetQuestObjectiveData(38811,92889)==1);}
    {Run r;r.Open(0);auto old=r.actors.back().get();r.p.quests.erase(38811);r.Tick(7500);assert(!old->world&&!r.King());r.p.Accept(38811);r.Fight(0);r.Kill();assert(r.p.GetQuestObjectiveData(38811,92889)==1);}
    for(int failure=0;failure<10;++failure){Run r;r.Open(1);auto helper=r.actors.back().get();
        if(failure==0)r.p.alive=false;if(failure==1)r.p.map=1;if(failure==2)r.p.phase=false;if(failure==3)r.p.visible=false;if(failure==4)r.p.teleport=true;
        if(failure==5)r.p.x+=90;if(failure==6)r.p.vehicle=true;if(failure==7)r.p.transport=true;
        if(failure==8)r.p.Accept(38811);if(failure==9)bowls[1].spawned=false;
        r.Tick(7500);assert(!r.King()&&!helper->world&&!r.p.GetQuestObjectiveData(38811,92918));bowls[1].spawned=true;
    }
    {Run r;r.failKing=true;r.Open(2);r.Tick(7500);assert(!StormheimJudgment::runs.count(r.p.guid));r.failKing=false;r.Fight(2);r.Kill();assert(r.p.GetQuestObjectiveData(38811,92920)==1);}
    for(int failure=0;failure<2;++failure){Run r;r.failHelper=failure==0;r.failVisual=failure==1;r.Fight(2);r.Kill();assert(r.p.GetQuestObjectiveData(38811,92920)==1);}
    {Run r;r.Open(0);r.Tick(7500);auto king=r.King();king->AI()->EnterEvadeMode();assert(!r.King()&&!king->world);r.Fight(0);r.Kill();}
    {Run r;r.Open(0);r.Tick(180000);assert(!r.King()&&!StormheimJudgment::runs.count(r.p.guid));}
    {Run r;r.Fight(1);auto king=r.King();r.script.OnMapChanged(&r.p);assert(!king->world&&!r.King());r.Fight(1);r.script.OnLogout(&r.p);assert(!r.King());}
    {Run r;r.Move(0);Spell cast(&r.p,&bowls[0]);assert(cast.Start());r.p.Accept(38811);cast.Complete();assert(!StormheimJudgment::runs.count(r.p.guid));r.Fight(0);r.Kill();}
    {Run r;r.Move(0);Spell cast(&r.p,&bowls[0]);assert(cast.Start());cast.gameObjTarget=&bowls[1];cast.Complete();assert(!StormheimJudgment::runs.count(r.p.guid));}
    {Run r;r.Move(0);Spell cast(&r.p,&bowls[0]);assert(cast.Start());r.p.x+=50;cast.Complete();assert(!StormheimJudgment::runs.count(r.p.guid));}
    {Run r;r.Open(0);r.Tick(7500);auto king=r.King();king->alive=false;king->AI()->JustDied(&r.p);assert(!r.p.GetQuestObjectiveData(38811,92889));}
    {Run r;for(auto id:{92889u,92918u,92920u}){r.p.KilledMonsterCredit(id);r.p.TalkedToCreature(id,{});assert(!r.p.GetQuestObjectiveData(38811,id));}}
    {Run r;auto& other=store.quests[999];other.id=999;other.special=QUEST_SPECIAL_FLAGS_KILL;other.objectives={{0,92889,1,0,0}};r.p.Accept(999);r.p.KilledMonsterCredit(92889);assert(r.p.GetQuestObjectiveData(999,92889)==1&&!r.p.GetQuestObjectiveData(38811,92889));}
    {Run r;GameObject other=bowls[0];other.guid={99999};other.dbGuid=99999;go_stormheim_judgment_ai ai(&other);Spell cast(&r.p,&other);cast.Complete();assert(cast.sends==1&&ai.delegated==0&&!StormheimJudgment::runs.count(r.p.guid));}
    {Run r;float original=bowls[0].x;bowls[0].x+=10;r.Move(0);Spell cast(&r.p,&bowls[0]);cast.Complete();assert(cast.failed&&!StormheimJudgment::runs.count(r.p.guid));bowls[0].x=original;}
    for(uint32 entry=241870;entry<=241874;++entry){Run r;GameObject bones=bowls[0];bones.entry=entry;bones.guid={entry};bones.dbGuid=115138;bones.info.ScriptId=0;go_stormheim_judgment_ai ai(&bones);Spell opening(&r.p,&bones);opening.Complete();assert(!opening.failed&&!opening.opening.prevented&&opening.sends==1&&!StormheimJudgment::runs.count(r.p.guid));}
    for(auto entry:{244450u,241778u}){Run r;GameObject rune=bowls[0];rune.entry=entry;rune.guid={entry==241778?25164548u:115183u};rune.dbGuid=rune.guid.value;rune.info.ScriptId=0;go_stormheim_judgment_ai ai(&rune);Spell opening(&r.p,&rune);opening.Complete();assert(!opening.failed&&!opening.opening.prevented&&opening.sends==1&&!StormheimJudgment::runs.count(r.p.guid));}
    {Run r;Creature unowned;unowned.entry=92889;npc_stormheim_judgment_king ai(&unowned);ai.IsSummonedBy(&r.p);assert(!ai.owned);ai.UpdateAI(100);assert(ai.delegated>=2);}
    {Run r;auto saved=templates.actors[92889].ScriptID;templates.actors[92889].ScriptID=3;r.Open(0);assert(!StormheimJudgment::runs.count(r.p.guid));templates.actors[92889].ScriptID=saved;}
    std::cout<<"PASS: extracted native lock1691 opening gate, exact three bowls, 7.5s ritual, protected dialogue then owned fights/deaths/pet credit, each objective once, no generic/group/talk shortcut, concurrent owners, cancel/lifetime/map/death/phase/distance/timeout/evade/retry and visual failures; both factions and Iounn38817 handoff. World/cast/network/geometry are fixtures here.\n";
}
