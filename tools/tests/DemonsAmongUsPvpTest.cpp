#include <cassert>
#include <cstdint>
#include <vector>
#include <iostream>
using uint32=uint32_t;using uint8=uint8_t;using ObjectGuid=int;
enum {UNIT_FIELD_BYTES_2,UNIT_BYTES_2_OFFSET_PVP_FLAG,UNIT_BYTE2_FLAG_PVP=1,GROUP_UPDATE_FLAG_STATUS,FACTION_TEMPLATE_FLAG_PVP=0x800,MAX_KILL_CREDIT=2};
struct Player;
struct Unit{
 bool player=false;uint8 flags=0;
 void SetByteFlag(int,int,int value){flags|=value;}void RemoveByteFlag(int,int,int value){flags&=~value;}
 bool IsPlayer(){return player;}Player* ToPlayer();void SetPvP(bool);
};
struct CreatureTemplate{uint32 Entry=0,faction=2780,KillCredit[2]={101105,0};};
struct Player:Unit{
 std::vector<uint32> credits;void SetGroupUpdateFlag(int){}
 void KilledMonsterCredit(uint32 entry,ObjectGuid=0){credits.push_back(entry);}
 void KilledMonster(CreatureTemplate const*,ObjectGuid);
};
Player*Unit::ToPlayer(){return player?static_cast<Player*>(this):nullptr;}
#include "SetPvP.inc"
#include "KilledMonster.inc"
struct FactionTemplateEntry{uint32 Flags=0;};
struct Store{FactionTemplateEntry old{2081},pve{0};FactionTemplateEntry const*LookupEntry(uint32 id){return id==2780?&old:id==2898?&pve:nullptr;}}store;
auto& sFactionTemplateStore=store;
struct Creature:Unit{void ApplyFaction(CreatureTemplate const* cInfo){
#include "InitPvp.inc"
}};
int main(){
 for(uint32 entry:{101100,101104}){
  CreatureTemplate data;data.Entry=entry;Creature actor;Player owner;
  actor.ApplyFaction(&data);assert(actor.flags&UNIT_BYTE2_FLAG_PVP);
  std::cout<<"COUNTERFACTUAL "<<entry<<" native faction2780 sets NPC public PvP byte; user reports client requires PvP before attack\n";
  owner.KilledMonster(&data,99);assert((owner.credits==std::vector<uint32>{entry,101105}));
  owner.credits.clear();data.faction=2898;actor.ApplyFaction(&data);assert(!(actor.flags&UNIT_BYTE2_FLAG_PVP));
  owner.KilledMonster(&data,99);assert((owner.credits==std::vector<uint32>{entry,101105}));
  assert(!owner.flags); // no player PvP toggle
 }
 std::cout<<"PASS actual Creature faction-to-PvP branch/Unit byte setter and native kill-credit mapping for both demon templates; player PvP untouched. Client attack and full quest remain pending\n";
}
