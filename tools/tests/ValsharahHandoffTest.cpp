#include <cassert>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>
using uint32=uint32_t;using ObjectGuid=unsigned;
enum {QUEST_STATUS_COMPLETE=1,TEMPSUMMON_MANUAL_DESPAWN=0};
struct Position {float x,y,z,o;};struct Player;struct Creature;
std::map<unsigned,Creature*> world;
struct TempSummon {Player* owner;Player* GetSummoner(){return owner;}};
struct Creature {TempSummon summon;bool alive=true,despawned=false;bool IsAlive(){return alive&&!despawned;}TempSummon* ToTempSummon(){return &summon;}void DespawnOrUnsummon(){despawned=true;}};
struct Player {unsigned guid,map=1220,spawns=0;bool alive=true,teleport=false,movie=false;Position pos{2891.17f,5894.22f,297.23f,0};std::map<unsigned,int> quests;std::set<unsigned> rewarded;std::vector<unsigned> summons,spells;std::vector<std::pair<unsigned,std::function<void()>>> events;
 unsigned GetMapId(){return map;}int GetQuestStatus(unsigned q){return quests[q];}bool GetQuestRewardStatus(unsigned q){return rewarded.count(q)!=0;}bool IsAlive(){return alive;}bool IsBeingTeleported(){return teleport;}bool isWatchingMovie(){return movie;}
 float GetDistance(Position p){return std::sqrt((pos.x-p.x)*(pos.x-p.x)+(pos.y-p.y)*(pos.y-p.y)+(pos.z-p.z)*(pos.z-p.z));}
 std::vector<unsigned>* GetSummonList(unsigned id){assert(id==102938);return &summons;}unsigned GetGUID(){return guid;}
 Creature* SummonCreature(unsigned id,Position p,unsigned type,unsigned time,unsigned vehicle,unsigned owner){assert(id==102938&&type==0&&time==0&&vehicle==0&&owner==guid&&p.x==2891.17f);auto key=guid*100+(++spawns);auto c=new Creature{{this}};world[key]=c;summons.push_back(key);return c;}
 void AddDelayedEvent(unsigned time,std::function<void()> fn){events.emplace_back(time,fn);}void CastSpell(Player* target,unsigned spell,bool triggered){assert(target==this&&triggered);spells.push_back(spell);}
 void Next(){assert(!events.empty());auto e=events.front();events.erase(events.begin());e.second();}
};
struct ObjectAccessor {static Creature* GetCreature(Player&,unsigned guid){auto i=world.find(guid);return i==world.end()?nullptr:i->second;}};
struct SpellScene {unsigned MiscValue;};struct Quest{unsigned id;unsigned GetQuestId()const{return id;}};
#include "ValHandoff.inc"
struct Scene {
#include "ValHandoffScene.inc"
};
struct Hooks {
#include "ValHandoffPlayer.inc"
};
int main(){
 Player p{1},other{2};p.quests[38753]=1;other.quests[38753]=1;
 p.movie=true;ValsharahHandoff::AfterMovie(&p,2);assert(p.spells.empty()&&p.events.size()==1&&p.spawns==0);p.Next();assert(p.spells.empty());
 p.movie=false;p.teleport=true;p.Next();assert(p.spells.empty());p.events.clear();p.teleport=false;ValsharahHandoff::AfterMovie(&p,2);assert(p.spells==std::vector<unsigned>{218440}&&p.events[0].first==15000);
 Scene scene;SpellScene wrong{1146},native{1350};assert(!scene.OnTrigger(&p,&wrong,"TYRANDE"));scene.OnTrigger(&p,&native,"complete");assert(p.spawns==0);scene.OnTrigger(&p,&native,"TYRANDE");assert(p.spawns==1);p.Next();assert(p.spawns==1);scene.OnTrigger(&p,&native,"TYRANDE");assert(p.spawns==1);
 ValsharahHandoff::EnsureTyrande(&other);assert(other.spawns==1);ValsharahHandoff::Clear(&p);assert(world[101]->despawned&&!world[201]->despawned);p.Next();assert(p.spawns==2);
 p.events.clear();ValsharahHandoff::Clear(&p);p.alive=false;ValsharahHandoff::Recover(&p);assert(p.spawns==2);p.alive=true;p.Next();assert(p.spawns==3);p.events.clear();
 Hooks hooks;Quest find{38753},love{41054};hooks.OnQuestReward(&p,&find);assert(!world[103]->despawned);p.quests[38753]=0;p.rewarded.insert(38753);assert(ValsharahHandoff::Needed(&p));hooks.OnLogin(&p);p.Next();assert(p.spawns==3);p.events.clear();p.rewarded.insert(41054);hooks.OnQuestReward(&p,&love);assert(world[103]->despawned);ValsharahHandoff::Recover(&p);assert(p.events.empty());
 Player fresh{3};ValsharahHandoff::EnsureTyrande(&fresh);assert(fresh.spawns==0);fresh.quests[38753]=1;fresh.pos.z+=200;ValsharahHandoff::EnsureTyrande(&fresh);assert(fresh.spawns==0);fresh.pos=p.pos;fresh.map=1;hooks.OnMapChanged(&fresh);assert(fresh.events.empty());
 // A canceled/missing scene still recovers its ender, without any credit API.
 Player fallback{4};fallback.quests[38753]=1;ValsharahHandoff::AfterMovie(&fallback,2);fallback.Next();assert(fallback.spawns==1);hooks.OnLogout(&fallback);assert(world[401]->despawned);
 // A long death or moving away cannot permanently end recovery.
 Player late{5};late.quests[38753]=1;late.alive=false;ValsharahHandoff::AfterMovie(&late,0);assert(late.events.size()==1&&late.spawns==0);late.alive=true;late.Next();assert(late.spawns==1);
 Player away{6};away.quests[38753]=1;away.pos.z+=200;ValsharahHandoff::AfterMovie(&away,0);assert(away.events.size()==1&&away.spells.empty()&&away.spawns==0);away.pos=p.pos;away.Next();assert(away.spawns==1);
 for(auto const& pair:world)delete pair.second;
 std::cout<<"PASS: production movie/teleport wait, exact native scene trigger, fallback without quest credit, personal isolation, duplicate prevention, login/death/grid recovery and cleanup.\n";
}
