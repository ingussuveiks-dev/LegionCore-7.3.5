#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>
#include <cmath>
using uint32=uint32_t;using int32=int32_t;
#define TC_LOG_DEBUG(...) ((void)0)
struct Position{float x=-10963.1f,y=-1878.23f,z=-48.3f,o=0;};
struct Unit {uint32 map=1533;Position pos;uint32 GetMapId(){return map;}};
struct SpellTargetPosition {uint32 target_mapId=1533;float target_X=-10964,target_Y=-1891.98f,target_Z=-48.3921f,target_Orientation=1.513f;};
struct SpellMgr{bool installed=false;SpellTargetPosition position;SpellTargetPosition const* GetSpellTargetPosition(int id){assert(id==199727);return installed?&position:nullptr;}} mgr;auto sSpellMgr=&mgr;
enum {TARGET_CHECK_ENTRY,TARGET_OBJECT_TYPE_GOBJ,TARGET_OBJECT_TYPE_DEST,TARGET_OBJECT_TYPE_OBJ_AND_DEST,SPELL_EFFECT_TELEPORT_L,SPELL_EFFECT_TELEPORT};
struct TargetType{int GetCheckType(){return TARGET_CHECK_ENTRY;}int GetObjectType(){return TARGET_OBJECT_TYPE_DEST;}};
struct Info{uint32 Id=199727;struct {bool RequiresSpellFocus=false;}CastingReq;bool HasEffect(int){return false;}};
struct Targets{bool hasDst=false;Position dst;Unit*referer=nullptr;Unit*GetUnitTarget(){return referer;}
 void SetDst(Unit& u){dst=u.pos;hasDst=true;}void SetDst(float x,float y,float z,float o=0,int=0){dst={x,y,z,o};hasDst=true;}};
struct Spell{Unit* m_caster;Info info;Info*m_spellInfo=&info;Targets m_targets;int searches=0;
 void AddGOTarget(Unit*,int){}
 void Resolve(){TargetType targetType;std::vector<int>*condList=nullptr;Unit*focusObject=nullptr;int effMask=1;
#include "NearbyDestination.inc"
 ++searches;
 }
};
int main(){
 Unit player;Spell spell;spell.m_caster=&player;spell.m_targets.referer=&player;
 spell.Resolve();assert(spell.m_targets.hasDst&&spell.m_targets.dst.y==player.pos.y); // old jump at self
 mgr.installed=true;spell.Resolve();assert(spell.m_targets.dst.y==-1891.98f&&spell.m_targets.dst.z==-48.3921f&&spell.searches==0);
 float distance=std::sqrt(std::pow(spell.m_targets.dst.x-player.pos.x,2)+std::pow(spell.m_targets.dst.y-player.pos.y,2));assert(distance>13&&distance<15);
 player.map=0;spell.m_targets.hasDst=false;spell.Resolve();assert(!spell.m_targets.hasDst); // don't jump across maps
 std::cout<<"PASS: actual target46 resolver reproduces self destination and selects the installed grounded retreat point; unrelated maps excluded. Client jump playback pending\n";
}
