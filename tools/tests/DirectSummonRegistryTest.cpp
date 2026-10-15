#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>
#include <vector>
using uint32=uint32_t;
#define ASSERT(x) assert(x)
constexpr int TEMPSUMMON_MANUAL_DESPAWN=1,TEMPSUMMON_DEAD_DESPAWN=2,TEMPSUMMON_TIMED_DESPAWN=3,UNIT_FIELD_CREATED_BY_SPELL=0;
struct SpellInfo{struct{uint32 RequiredAreasID;}CastingReq;};
struct SpellMgr{SpellInfo const* GetSpellInfo(uint32){return nullptr;}}spellMgr;
auto sSpellMgr=&spellMgr;
struct Unit{
 std::map<uint32,std::vector<uint32>> tempSummonList;
 uint32 getFaction(){return 35;}uint32 getLevel(){return 103;}uint32 GetEffectiveLevel(){return 103;}bool IsPlayer(){return true;}
};
struct TempSummon{
 Unit* owner;void* m_Properties=nullptr;uint32 m_timer=0,m_lifetime=0,m_type=TEMPSUMMON_MANUAL_DESPAWN;
 uint32 m_templateSpells[1]={0};bool m_ControlledByPlayer=false;
 bool isPet(){return false;}bool isTrigger(){return false;}Unit* GetSummoner(){return owner;}
 uint32 GetUInt32Value(int){return 0;}uint32 GetEntry(){return 110548;}uint32 GetGUID(){return 1;}
 void SetRequiredAreas(uint32){}void setFaction(uint32){}void SetLevel(uint32){}void SetEffectiveLevel(uint32){}
 void InitBaseStat(uint32,bool&){}void InitStats(uint32);
};
// Execute the real InitStats prefix through its no-properties early return,
// followed by its real registry insertion. Property-specific slot/faction
// branches are irrelevant to this null-properties contract and are omitted.
#include "DirectSummonRegistry.inc"
int main(){Unit player;TempSummon direct{&player};direct.InitStats(0);assert(player.tempSummonList.empty());
 std::cout<<"PASS: native direct-summon InitStats returns before owner registry insertion; recovery fixture must keep the list empty\n";}
