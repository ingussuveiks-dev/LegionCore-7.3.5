#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>
using uint32=uint32_t;
enum {GENDER_NONE=2,CLASSMASK_ALL_PLAYABLE=0xFFF};
#include "NursingStatus.inc"
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
 uint32 zone=7503,area=7726;bool aura=false;unsigned casts=0,removals=0;
 void SetQuestUpdate(uint32);
 void GetZoneAndAreaId(uint32& z,uint32& a)const{z=zone;a=area;}
 uint32 GetMapId()const{return 1220;}uint32 getGender()const{return 0;}
 uint32 getRaceMask()const{return 1;}uint32 getClassMask()const{return 1;}
 uint32 GetOldZoneID()const{return zone;}uint32 GetOldAreaID()const{return area;}
 QuestStatus GetQuestStatus(uint32 id)const{assert(id==39318);return state;}
 bool HasAura(uint32 id)const{assert(id==189182);return aura;}
 void CastSpell(Player*,uint32 id,bool){assert(id==189182);aura=true;++casts;}
 void RemoveAurasDueToSpell(uint32 id){assert(id==189182);if(aura){aura=false;++removals;}}
 void UpdateAvailableQuestLines(){}
};
struct SpellArea {
 uint32 spellId=189182;int areaId=7726;
 uint32 gender=GENDER_NONE,raceMask=0;int classmask=0;
 uint32 questStart=0,questStartStatus=74,questEnd=39318,questEndStatus=10;
 int auraSpell=0;uint32 RequiredAreasID=0,active_event=0;bool autocast=false;
 std::vector<uint32> areaGroupMembers;
 bool IsFitToRequirements(Player const*,uint32,uint32)const;
};
#include "NursingArea.inc"
#include "NursingQuestUpdate.inc"
int main(){
 SpellArea fixed,old;old.questEndStatus=66;
 manager.ends.emplace(39318,&fixed);
 Player p;p.state=QUEST_STATUS_INCOMPLETE;
 assert(!old.IsFitToRequirements(&p,p.zone,p.area));
 assert(fixed.IsFitToRequirements(&p,p.zone,p.area));
 for(auto status:{QUEST_STATUS_NONE,QUEST_STATUS_INCOMPLETE,QUEST_STATUS_COMPLETE,QUEST_STATUS_FAILED,QUEST_STATUS_REWARDED}){
  p={};p.state=status;
  bool allowed=status==QUEST_STATUS_INCOMPLETE||status==QUEST_STATUS_COMPLETE;
  assert(fixed.IsFitToRequirements(&p,p.zone,p.area)==allowed);
  p.SetQuestUpdate(39318);assert(!p.aura&&!p.casts); // Never grant before taking Julan's totem.
  p.aura=true;p.SetQuestUpdate(39318);assert(p.aura==allowed && !p.casts);
  auto removed=p.removals;p.SetQuestUpdate(39318);assert(p.removals==removed);
 }
 p={};p.state=QUEST_STATUS_INCOMPLETE;p.aura=true;
 p.SetQuestUpdate(39318);assert(p.aura);
 p.state=QUEST_STATUS_COMPLETE;p.SetQuestUpdate(39318);assert(p.aura);
 p.state=QUEST_STATUS_REWARDED;p.SetQuestUpdate(39318);assert(!p.aura&&p.removals==1);
 p={};p.state=QUEST_STATUS_INCOMPLETE;p.aura=true;
 p.state=QUEST_STATUS_NONE;p.SetQuestUpdate(39318);assert(!p.aura);
 p.state=QUEST_STATUS_INCOMPLETE;p.SetQuestUpdate(39318);assert(!p.aura&&!p.casts);
 // Returning/relogging with an active quest permits manual re-acquisition.
 assert(fixed.IsFitToRequirements(&p,7503,7726));
 assert(!fixed.IsFitToRequirements(&p,7503,7725));
 assert(!fixed.IsFitToRequirements(&p,1519,1519));
 std::cout<<"PASS: actual SpellArea common predicate reproduces old incomplete-quest exclusion; actual quest-update handler retains manual totem while active/complete, removes on abandon/fail/reward, never auto-grants, permits retry and rejects wrong areas.\n";
}
