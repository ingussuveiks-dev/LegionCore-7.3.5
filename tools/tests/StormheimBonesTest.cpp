#include "HeartHandoffFixture.inc"
struct BoneHook {void operator+=(int){}};
struct BonesSpellScript {
 Unit* caster=nullptr,*original=nullptr,*target=nullptr;bool prevented=false;
 BoneHook OnCheckCast,OnEffectHitTarget,AfterCast;
 Unit* GetCaster(){return caster;}Unit* GetOriginalTarget(){return original;}
 Player* GetHitPlayer(){return target?target->ToPlayer():nullptr;}
 void PreventHitDefaultEffect(int){prevented=true;}virtual void Register(){}
};
#undef PrepareSpellScript
#define PrepareSpellScript(x) public:
#define SpellScript BonesSpellScript
#define SpellCheckCastFn(x) 1
#define SpellCastFn(x) 1
#define SpellEffectFn(x,i,t) 1
#include "Bones.inc"
#undef SpellScript
constexpr int SPELL_EFFECT_HANDLE_HIT_TARGET=1;
struct NativeEffect {int32 MiscValue=97340;};
struct NativeInfo {NativeEffect effect;NativeEffect* GetEffect(int,int){return &effect;}};
struct Spell {int effectHandleMode=1,m_diffMode=0;Unit* unitTarget=nullptr;NativeInfo info;NativeInfo* m_spellInfo=&info;void EffectKillCredit(int);};
#include "NativeCredit.inc"
struct ConversationData {uint32 id,idx,textId,unk1=134,unk2=0;};
struct ConversationActor {uint32 id=0,actorId=53077,creatureId=96254,displayId=64208,unk1=0,unk2=1,unk3=0,duration=3350;};
struct ConversationCreature {uint32 id=0,creatureId=0,unk1=0,unk2=0,duration=0;};
struct ConversationStore {
 std::vector<ConversationData> lines{{4740,0,0},{4741,1,2300},{4742,2,6600},{4743,3,10200}};
 std::vector<ConversationActor> actors{{}};std::vector<ConversationCreature> creatures;
 auto GetConversationData(uint32){return &lines;}auto GetConversationActor(uint32){return &actors;}auto GetConversationCreature(uint32){return &creatures;}
} conversationStore;auto sConversationDataStore=&conversationStore;
struct ConversationDynamicFieldActors {uint32 actorId,creatureId,displayId,unk1,unk2,unk3;};
struct ConversationDynamicFieldCreatures {uint64 GuidLow=0,GuidHight=0;uint32 unk1,unk2;};
struct ConversationDynamicFieldLines {uint32 id,textId,unk1,unk2;};
struct Conversation {
 uint32 count=0;float GetPositionX(){return 0;}float GetPositionY(){return 0;}
 void SetActor(ConversationDynamicFieldActors*,uint32,bool){}void SetCreature(ConversationDynamicFieldCreatures*,uint32,bool){}void SetLine(ConversationDynamicFieldLines*,uint32,bool){++count;}
 bool GenerateDynamicFieldData(uint32,WorldObject*,uint32&);
 uint32 Lifetime(){uint32 triggerEntry=2247;WorldObject object;auto caster=&object;
#include "ConversationLifetime.inc"
 return duration;}
};
#include "Conversation.inc"
struct Run {
 Player player;Creature buck,havi;
 Run(uint32 team=ALLIANCE){player.team=team;player.race=team==ALLIANCE?1:2;player.Accept(39804);buck.entry=98230;buck.dbGuid=338655;buck.guid={338655};buck.area=7803;buck.fields[UNIT_FIELD_NPC_FLAGS]=UNIT_NPC_FLAG_SPELLCLICK;havi.entry=92539;havi.guid={291280};havi.distance=40;creatures[buck.guid]=&buck;creatures[havi.guid]=&havi;}
 spell_stormheim_bones_butchering Cast(){spell_stormheim_bones_butchering s;s.caster=&player;s.original=&buck;s.target=&player;return s;}
 void Hit(spell_stormheim_bones_butchering& s){s.Credit(0);if(!s.prevented){Spell native;native.unitTarget=s.target;native.EffectKillCredit(0);}}
};
int main(){
 Conversation conversation;assert(conversation.Lifetime()==8350&&conversationStore.lines.back().textId>8350);conversationStore.actors[0].duration=13550;assert(conversation.Lifetime()==18550&&conversation.count==8);
 auto& q=store.quests[39804];q.id=39804;q.special=QUEST_SPECIAL_FLAGS_KILL;q.objectives={{0,97340,1,0,0}};q.PrevQuestID=39803;q.NextQuestIdChain=39796;q.NextQuestID=39796;
 auto& travel=store.quests[39803];travel.id=39803;travel.PrevQuestID=38624;travel.NextQuestID=39804;travel.NextQuestIdChain=39804;
 auto& next=store.quests[39796];next.id=39796;next.PrevQuestID=39804;store.quests[38624].id=38624;store.quests[38624].NextQuestID=39803;
 QuestMap _questTemplates;for(auto& entry:store.quests)_questTemplates[entry.first]=&entry.second;
 for(auto const& entry:_questTemplates){auto qinfo=entry.second;
#include "QuestPrevLoader.inc"
 }
 store.starters.emplace(97074,39803);store.starters.emplace(92539,39804);store.starters.emplace(92539,39796);
 for(auto team:{ALLIANCE,HORDE}){
  Run r(team);auto& p=r.player;p.quests.clear();assert(!p.CanTakeQuest(&travel,false));p.m_RewardedQuests.insert(38624);assert(p.CanTakeQuest(&travel,false));p.Accept(39803);assert(p.CanCompleteQuest(39803));p.CompleteQuest(39803);assert(p.CanRewardQuest(&travel,false));p.m_RewardedQuests.insert(39803);p.quests.erase(39803);assert(p.CanTakeQuest(&q,false));assert(p.GetNextQuest(r.havi.GetGUID(),&travel)==&q);p.Accept(39804);assert(!p.CanCompleteQuest(39804));
  auto cast=r.Cast();assert(cast.Check()==SPELL_CAST_OK);cast.Complete();assert(!p.GetQuestObjectiveData(39804,97340)&&!p.conversationAttempts);r.Hit(cast);assert(!cast.prevented&&p.GetQuestStatus(39804)==QUEST_STATUS_COMPLETE&&p.GetQuestObjectiveData(39804,97340)==1);cast.Complete();cast.Complete();assert(p.conversationAttempts==1&&p.HasAura(192170));assert(p.CanRewardQuest(&q,false));p.m_RewardedQuests.insert(39804);p.quests.erase(39804);assert(p.CanTakeQuest(&next,false)&&p.GetNextQuest(r.havi.GetGUID(),&q)==&next);
 }
 {Run r;auto cast=r.Cast();assert(cast.Check()==SPELL_CAST_OK);r.player.castResult=1;r.Hit(cast);cast.Complete();assert(r.player.GetQuestStatus(39804)==QUEST_STATUS_COMPLETE&&r.player.conversationAttempts==1&&!r.player.HasAura(192170));}
 for(int failure=0;failure<11;++failure){Run r;auto cast=r.Cast();assert(cast.Check()==SPELL_CAST_OK);if(failure==0)r.player.Accept(39804);if(failure==1)r.player.alive=false;if(failure==2)r.player.phase=false;if(failure==3)r.player.visible=false;if(failure==4)r.player.los=false;if(failure==5)r.buck.distance=7;if(failure==6)r.buck.world=false;if(failure==7)r.havi.world=false;if(failure==8)r.player.map=1;if(failure==9)r.player.quests.clear();if(failure==10)r.player.teleport=true;r.Hit(cast);cast.Complete();assert(cast.prevented&&!r.player.GetQuestObjectiveData(39804,97340)&&!r.player.conversationAttempts);}
 for(int failure=0;failure<8;++failure){Run r;auto cast=r.Cast();if(failure==0)cast.original=nullptr;if(failure==1)r.buck.dbGuid=22;if(failure==2)r.buck.fields[UNIT_FIELD_NPC_FLAGS]=0;if(failure==3)r.player.vehicle=true;if(failure==4)r.player.transport=true;if(failure==5)r.player.combat=true;if(failure==6)r.player.quests.clear();if(failure==7)r.havi.distance=61;assert(cast.Check()==SPELL_FAILED_BAD_TARGETS);}
 {Run r;Player nearby,far;nearby.guid={2};far.guid={3};nearby.Accept(39804);far.Accept(39804);far.rewardRange=false;GroupReference a{&r.player},b{&nearby},c{&far};a.following=&b;b.following=&c;Group group;group.first=&a;r.player.group=&group;auto cast=r.Cast();assert(cast.Check()==SPELL_CAST_OK);r.Hit(cast);cast.Complete();assert(r.player.GetQuestObjectiveData(39804,97340)==1&&nearby.GetQuestObjectiveData(39804,97340)==1&&!far.GetQuestObjectiveData(39804,97340));}
 {Run r;auto cast=r.Cast();cast.original=&r.player;assert(cast.Check()==SPELL_FAILED_BAD_TARGETS);r.player.map=1;auto other=r.Cast();assert(other.Check()==SPELL_CAST_OK);r.Hit(other);other.Complete();assert(!other.prevented&&!r.player.conversationAttempts);}
 {Run r;auto cast=r.Cast();q.objectives[0].Amount=2;assert(cast.Check()==SPELL_FAILED_BAD_TARGETS);q.objectives[0].Amount=1;}
 {Run r;auto a=r.Cast(),b=r.Cast();assert(a.Check()==SPELL_CAST_OK&&b.Check()==SPELL_CAST_OK);r.Hit(a);a.Complete();r.Hit(b);b.Complete();assert(b.prevented&&r.player.conversationAttempts==1&&r.player.GetQuestObjectiveData(39804,97340)==1);}
 spell_stormheim_bones_butchering startup;startup.Register();
 std::cout<<"PASS: actual Bones script + native effect134/group credit/completion/previous/race/reward/next handlers; travel39803, buck cast39804 and Havi39796, both factions, native party range, duplicate/stale/interrupted casts, failure cleanup and optional conversation failure. Spell timing, world/conditions, cast/network/rendering remain fixtures.\n";
}
