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
 QuestStatus state=QUEST_STATUS_NONE;uint32 introQuest=40716;
 QuestStatus prerequisite=QUEST_STATUS_REWARDED;
 uint32 zone=7502,area=7502;bool aura=false;unsigned casts=0,removals=0;
 void SetQuestUpdate(uint32);
 void GetZoneAndAreaId(uint32& z,uint32& a)const{z=zone;a=area;}
 uint32 GetMapId()const{return 1220;}uint32 getGender()const{return 0;}
 uint32 getRaceMask()const{return 1;}uint32 classMask=256;uint32 getClassMask()const{return classMask;}
 uint32 GetOldZoneID()const{return zone;}uint32 GetOldAreaID()const{return area;}
 QuestStatus GetQuestStatus(uint32 id)const{if(id==40824)return prerequisite;assert(id==introQuest||id==44099);return state;}
 bool HasAura(uint32 id)const{assert(id==204860||id==224338||id==196908);return id==196908?false:aura;}
 void CastSpell(Player*,uint32 id,bool){assert(id==204860||id==224338||id==196908);if(id!=196908)aura=true;++casts;}
 void RemoveAurasDueToSpell(uint32 id){assert(id==204860||id==224338||id==196908);if(aura){aura=false;++removals;}}
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
 // The later campaign messenger has the same legacy mask defect, but requires
 // a rewarded Path of the Dreadscar and the city streets, not the Underbelly.
 SpellArea campaign;
 campaign.spellId=224338;campaign.areaId=7581;
 campaign.questStart=40824;campaign.questStartStatus=64;
 campaign.questEnd=44099;campaign.questEndStatus=1;
 SpellArea oldCampaign=campaign;oldCampaign.questEndStatus=74;
 manager.starts.clear();manager.ends.clear();
 manager.starts.emplace(40824,&campaign);manager.ends.emplace(44099,&campaign);
 for(auto prerequisite:{QUEST_STATUS_NONE,QUEST_STATUS_INCOMPLETE,QUEST_STATUS_COMPLETE,QUEST_STATUS_REWARDED})
  for(auto status:{QUEST_STATUS_NONE,QUEST_STATUS_INCOMPLETE,QUEST_STATUS_COMPLETE,QUEST_STATUS_FAILED,QUEST_STATUS_REWARDED}){
   p={};p.area=7581;p.prerequisite=prerequisite;p.state=status;
   bool allowed=prerequisite==QUEST_STATUS_REWARDED&&status==QUEST_STATUS_NONE;
   assert(campaign.IsFitToRequirements(&p,p.zone,p.area)==allowed);
   p.SetQuestUpdate(40824);assert(p.aura==allowed);
  }
 p={};p.area=7581;assert(!oldCampaign.IsFitToRequirements(&p,p.zone,p.area));
 p.SetQuestUpdate(40824);assert(p.aura&&p.casts==1);
 p.SetQuestUpdate(44099);assert(p.casts==1); // no duplicate summon while aura remains
 p.state=QUEST_STATUS_INCOMPLETE;p.SetQuestUpdate(44099);assert(!p.aura&&p.removals==1);
 p.state=QUEST_STATUS_NONE;p.SetQuestUpdate(44099);assert(p.aura&&p.casts==2); // abandonment retry
 p.state=QUEST_STATUS_REWARDED;p.SetQuestUpdate(44099);assert(!p.aura);
 p={};p.area=7581;p.classMask=1;assert(!campaign.IsFitToRequirements(&p,p.zone,p.area));
 p={};p.zone=8392;p.area=8392;assert(!campaign.IsFitToRequirements(&p,p.zone,p.area));
 p={};p.zone=1107;p.area=1107;assert(!campaign.IsFitToRequirements(&p,p.zone,p.area));
 SpellArea hunter;hunter.spellId=196908;hunter.questEnd=40384;hunter.classmask=4;
 SpellArea oldHunter=hunter;oldHunter.questEndStatus=74;
 manager.starts.clear();manager.ends.clear();manager.ends.emplace(40384,&hunter);
 for(auto status:{QUEST_STATUS_NONE,QUEST_STATUS_INCOMPLETE,QUEST_STATUS_COMPLETE,QUEST_STATUS_FAILED,QUEST_STATUS_REWARDED}){
  p={};p.introQuest=40384;p.classMask=4;p.state=status;
  assert(hunter.IsFitToRequirements(&p,7502,7502)==(status==QUEST_STATUS_NONE));
  p.SetQuestUpdate(40384);assert(!p.aura&&p.casts==(status==QUEST_STATUS_NONE?1u:0u));
 }
 p={};p.introQuest=40384;p.classMask=4;
 assert(!oldHunter.IsFitToRequirements(&p,7502,7502));
 p.SetQuestUpdate(40384);assert(!p.aura&&p.casts==1);
 p.state=QUEST_STATUS_INCOMPLETE;p.SetQuestUpdate(40384);assert(!p.aura&&p.casts==1);
 p.state=QUEST_STATUS_NONE;p.SetQuestUpdate(40384);assert(!p.aura&&p.casts==2);
 for(auto cls:{1u,2u,256u,2048u}){p.classMask=cls;assert(!hunter.IsFitToRequirements(&p,7502,7502));}
 p.classMask=4;assert(!hunter.IsFitToRequirements(&p,1519,1519));
 // Unlike the warlock messenger, native196908 is a pure summon, not an aura.
 // Do not mock a persistent marker or claim duplicate-summon suppression.
 std::cout<<"PASS: hunter Snowfeather initial/abandoned offer, old-mask rejection, all quest statuses, class/area admission and no further offer after acceptance; native summon has no aura marker.\n";
 std::cout<<"PASS: actual SpellArea predicate and quest aura lifecycle admit a new/abandoned warlock, suppress duplicate casts, remove on acceptance, reject completed/rewarded quests, other classes and other zones.\n";
 std::cout<<"PASS: A Mutual Friend requires rewarded 40824 and absent 44099 in Dalaran City; native quest updates handle acceptance/abandonment without admitting the Underbelly or class hall.\n";
}
