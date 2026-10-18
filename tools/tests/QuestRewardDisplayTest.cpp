#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>
#include <vector>
using uint8=uint8_t;using uint32=uint32_t;
constexpr int MAX_SPELL_EFFECTS=32,MAX_SPELL_REAGENTS=8,QUEST_REWARD_DISPLAY_SPELL_COUNT=3,EFFECT_0=0,TARGET_UNIT_CASTER=1;
enum{SPELL_EFFECT_DUMMY=3,SPELL_EFFECT_CREATE_ITEM=24,SPELL_EFFECT_LEARN_SPELL=36,SPELL_EFFECT_APPLY_AURA=6,SPELL_EFFECT_TRIGGER_SPELL=64,SPELL_EFFECT_CREATE_ITEM_2=157};
struct Target{uint32 id=0;uint32 GetTarget()const{return id;}};
struct FakeEffect{uint32 Effect=0,TriggerSpell=0,ItemType=0;Target TargetA,TargetB;};
struct SpellInfo{uint32 Id=0,EffectMask=0;FakeEffect* Effects[32]{};struct{int Reagent[8]{};}Reagents;bool loot=false;bool IsLootCrafting()const{return loot;}
 bool HasEffect(int type)const{for(int i=0;i<32;++i)if((EffectMask&(uint32(1)<<i))&&Effects[i]&&Effects[i]->Effect==uint32(type))return true;return false;}};
int errors=0;std::vector<uint32> casts;
#define TC_LOG_ERROR(...) (++errors)
struct Player;
struct ChatHandler{explicit ChatHandler(Player*){}template<class... T>void PSendSysMessage(char const*,T...){++errors;}};
struct ItemStore{void* GetItemTemplate(int){return nullptr;}} items;auto sObjectMgr=&items;
struct SpellMgr{std::map<uint32,SpellInfo*> spells;SpellInfo const* GetSpellInfo(uint32 id){auto i=spells.find(id);return i==spells.end()?nullptr:i->second;}
 static bool IsSpellValid(SpellInfo const*,Player* =nullptr,bool=true);} mgr;auto sSpellMgr=&mgr;
#include "RewardDisplayHelper.inc"
#include "RewardDisplayValidity.inc"
struct Quest{uint32 RewardDisplaySpell[3]{},RewardSpell=0;uint32 GetQuestId(){return 44721;}};
void Load(Quest* qinfo){
#include "RewardDisplayLoader.inc"
}
struct Unit{bool IsUnit(){return true;}Unit* ToUnit(){return this;}void CastSpell(Player*,uint32 id,bool){casts.push_back(id);}};
struct Player:Unit{void CastSpell(Player*,uint32 id,bool){casts.push_back(id);}void Reward(Quest const* quest,Unit* questGiver){
#include "RewardDisplayAward.inc"
}};
int main(){
 FakeEffect labelLearn;labelLearn.Effect=36;FakeEffect dummy;dummy.Effect=3;SpellInfo label;label.Id=230408;label.EffectMask=3;label.Effects[0]=&labelLearn;label.Effects[1]=&dummy;
 mgr.spells[label.Id]=&label;assert(IsQuestRewardDisplayOnly(&label)&&!SpellMgr::IsSpellValid(&label));
 Quest q;q.RewardDisplaySpell[0]=230408;errors=0;Load(&q);assert(q.RewardDisplaySpell[0]==230408&&!errors);
 Player player;Unit giver;player.Reward(&q,&giver);assert(casts.empty()); // preserve packet field, never learn0/cast
 // Real executable rewards retain validity and ordinary fallback casting.
 FakeEffect childDummy;childDummy.Effect=3;SpellInfo child;child.Id=900;child.EffectMask=1;child.Effects[0]=&childDummy;mgr.spells[900]=&child;
 FakeEffect learning;learning.Effect=36;learning.TriggerSpell=900;learning.TargetA.id=1;SpellInfo valid;valid.Id=901;valid.EffectMask=1;valid.Effects[0]=&learning;mgr.spells[901]=&valid;
 assert(!IsQuestRewardDisplayOnly(&valid)&&SpellMgr::IsSpellValid(&valid));q.RewardDisplaySpell[0]=901;Load(&q);player.Reward(&q,&giver);assert(casts==std::vector<uint32>{901});
 q.RewardDisplaySpell[0]=0;q.RewardSpell=230408;errors=0;Load(&q);assert(!q.RewardSpell&&errors); // never relax real reward validation
 q.RewardDisplaySpell[0]=999999;errors=0;Load(&q);assert(!q.RewardDisplaySpell[0]&&errors);
 // Narrow structural admission: target, child, item, aura and unrelated dummy cannot masquerade as label.
 labelLearn.TargetA.id=1;assert(!IsQuestRewardDisplayOnly(&label));labelLearn.TargetA.id=0;
 labelLearn.TriggerSpell=900;assert(!IsQuestRewardDisplayOnly(&label));labelLearn.TriggerSpell=0;
 dummy.Effect=24;assert(!IsQuestRewardDisplayOnly(&label));dummy.Effect=6;assert(!IsQuestRewardDisplayOnly(&label));dummy.Effect=3;
 label.EffectMask=2;assert(!IsQuestRewardDisplayOnly(&label));assert(!IsQuestRewardDisplayOnly(nullptr));
 std::cout<<"PASS: production structural helper, native IsSpellValid, display/real-reward loader and award blocks; native label retained without casting, executable rewards unchanged, invalid IDs and real invalid learn spells still rejected. Packet/storage services outside fixture.\n";
}
