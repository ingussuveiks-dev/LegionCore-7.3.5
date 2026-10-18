#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
int main(int argc,char**argv){if(argc!=2)return 1;
#define S(T,N,F,I) DB2Storage<T> N(F ".db2",I::Instance());if(!N.Load(argv[1],0))return 2
S(LockEntry,locks,"Lock",LockLoadInfo);S(SpellEntry,spells,"Spell",SpellLoadInfo);
S(SpellEffectEntry,effects,"SpellEffect",SpellEffectLoadInfo);S(SpellMiscEntry,misc,"SpellMisc",SpellMiscLoadInfo);
S(SpellCastTimesEntry,times,"SpellCastTimes",SpellCastTimesLoadInfo);S(PlayerConditionEntry,conditions,"PlayerCondition",PlayerConditionLoadInfo);
S(ConversationLineEntry,lines,"ConversationLine",ConversationLineLoadInfo);
#undef S
auto lock=locks.LookupEntry(2617);if(!lock||lock->Type[0]!=3||lock->Index[0]!=229547)return 3;
bool cast=false,open=false,slow=false,pose=false,remove=false,unusedCredit=false;
for(auto m:misc)if(m->SpellID==229547){auto t=times.LookupEntry(m->CastingTimeIndex);if(t&&t->Base==18000)cast=true;}
for(auto e:effects){
 if(e->SpellID==229547&&e->Effect==33&&e->ImplicitTarget[0]==23)open=true;
 if(e->SpellID==229539&&e->EffectAura==33&&e->EffectBasePoints==75)slow=true;
 if(e->SpellID==229539&&e->EffectAura==312&&e->EffectMiscValue[0]==175)pose=true;
 if(e->SpellID==229554&&e->Effect==203&&e->EffectTriggerSpell==229539)remove=true;
 if(e->SpellID==229542&&e->Effect==134&&e->EffectMiscValue[0]==114962)unusedCredit=true;
}
if(!cast||!open||!slow||!pose||!remove||!unusedCredit)return 4;
auto c=conditions.LookupEntry(45843);if(!c||c->PrevQuestID[0]!=44720||c->CurrQuestID[0]!=44771||c->AuraSpellID[0]!=229539||c->AuraSpellLogic!=65536||c->AuraStacks[0])return 5;
auto line=lines.LookupEntry(8966);if(!line||line->BroadcastTextID!=123956)return 6;
for(uint32 id:{229547u,229539u,229554u}){auto spell=spells.LookupEntry(id);if(!spell)return 7;std::cout<<id<<" "<<spell->Name->Str[0]<<'\n';}
std::cout<<"PASS: native Lock2617,18s GO cast,75% slow/pose175, Give Grog removal, unused premature-credit spell, packed PlayerCondition45843 and delivery conversation line.\n";
}
