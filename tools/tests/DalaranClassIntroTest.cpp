#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>
using uint32=uint32_t;
enum {GENDER_NONE=2,CLASSMASK_ALL_PLAYABLE=0xFFF};
#include "ClassIntroStatus.inc"
struct SpellArea;
using SpellAreaForAreaMap=std::multimap<uint32,SpellArea const*>;
using SpellAreaForQuestMapBounds=std::pair<SpellAreaForAreaMap::const_iterator,SpellAreaForAreaMap::const_iterator>;
struct SpellManager {
 SpellAreaForAreaMap starts,ends;
 SpellAreaForQuestMapBounds GetSpellAreaForQuestMapBounds(uint32 id)const{return starts.equal_range(id);}
 SpellAreaForQuestMapBounds GetSpellAreaForQuestEndMapBounds(uint32 id)const{return ends.equal_range(id);}
} manager;
auto sSpellMgr=&manager;
struct GameEvents {bool IsActiveEvent(uint32)const{return false;}} events;
auto sGameEventMgr=&events;
struct Player {
 QuestStatus state=QUEST_STATUS_NONE;
 uint32 zone=7502,area=7502;bool aura=false;unsigned casts=0,removals=0;
 void SetQuestUpdate(uint32);
 void GetZoneAndAreaId(uint32& z,uint32& a)const{z=zone;a=area;}
 uint32 GetMapId()const{return 1220;}uint32 getGender()const{return 0;}
 uint32 getRaceMask()const{return 1;}uint32 classMask=256;uint32 getClassMask()const{return classMask;}
 uint32 GetOldZoneID()const{return zone;}uint32 GetOldAreaID()const{return area;}
 QuestStatus GetQuestStatus(uint32 id)const{assert(id==40716);return state;}
 bool HasAura(uint32 id)const{assert(id==204860);return aura;}
 void CastSpell(Player*,uint32 id,bool){assert(id==204860);aura=true;++casts;}
 void RemoveAurasDueToSpell(uint32 id){assert(id==204860);if(aura){aura=false;++removals;}}
 void UpdateAvailableQuestLines(){}
};
struct SpellArea {
 uint32 spellId=204860;int areaId=7502;
 uint32 gender=GENDER_NONE,raceMask=0;int classmask=256;
 uint32 questStart=0,questStartStatus=74,questEnd=40716,questEndStatus=1;
 int auraSpell=0;uint32 RequiredAreasID=0,active_event=0;bool autocast=true;
 std::vector<uint32> areaGroupMembers;
 bool IsFitToRequirements(Player const*,uint32,uint32)const;
};
#include "ClassIntroArea.inc"
#include "ClassIntroQuestUpdate.inc"
int main(){
 SpellArea fixed,old;old.questEndStatus=74;
 manager.ends.emplace(40716,&fixed);
 Player p;
 assert(!old.IsFitToRequirements(&p,7502,7502));
 assert(fixed.IsFitToRequirements(&p,7502,7502));
 for(auto status:{QUEST_STATUS_NONE,QUEST_STATUS_INCOMPLETE,QUEST_STATUS_COMPLETE,QUEST_STATUS_FAILED,QUEST_STATUS_REWARDED}){
  p={};p.state=status;
  assert(fixed.IsFitToRequirements(&p,7502,7502)==(status==QUEST_STATUS_NONE));
  p.SetQuestUpdate(40716);
  assert(p.aura==(status==QUEST_STATUS_NONE));
 }
 p={};p.SetQuestUpdate(40716);assert(p.aura&&p.casts==1);
 p.SetQuestUpdate(40716);assert(p.casts==1);
 p.state=QUEST_STATUS_INCOMPLETE;p.SetQuestUpdate(40716);assert(!p.aura&&p.removals==1);
 p.state=QUEST_STATUS_NONE;p.SetQuestUpdate(40716);assert(p.aura&&p.casts==2); // abandon permits a fresh offer
 p.state=QUEST_STATUS_REWARDED;p.SetQuestUpdate(40716);assert(!p.aura);
 p={};p.classMask=2048;assert(!fixed.IsFitToRequirements(&p,7502,7502));
 p.classMask=1;assert(!fixed.IsFitToRequirements(&p,7502,7502));
 p={};assert(!fixed.IsFitToRequirements(&p,1519,1519));
 std::cout<<"PASS: actual SpellArea predicate and quest aura lifecycle admit a new/abandoned warlock, suppress duplicate casts, remove on acceptance, reject completed/rewarded quests, other classes and other zones.\n";
}
