#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>
#include <vector>
using uint32=uint32_t;using int32=int32_t;
using uint8=uint8_t;
enum {QUEST_STATUS_INCOMPLETE=1,DONE=3,DATA_XAVIUS=3,NPC_MALFURION_STORMRAGE=100652,SAY_DEATH=1,ACTION_1=1};
struct GameObject;
struct Player {int quest=1;bool alive=true,combat=false,vehicle=false;unsigned credit=0,teleports=0;float distance=1;
 int GetQuestStatus(unsigned q){assert(q==38147);return quest;}bool IsAlive(){return alive;}bool isInCombat(){return combat;}void* GetVehicle(){return vehicle?this:nullptr;}
 float GetDistance(GameObject*){return distance;}unsigned GetQuestObjectiveData(unsigned q,unsigned c){assert(q==38147&&c==99032);return credit;}
 void KilledMonsterCredit(unsigned c){assert(c==99032);++credit;}void TeleportTo(unsigned map,float x,float y,float z,float o){assert(map==1466&&x==3248.16f&&y==1829.34f&&z==236.84f&&o==0.1f);++teleports;}
};
struct GameObject {unsigned entry=242279,map=1220,zone=7558,opens=0;unsigned GetEntry(){return entry;}unsigned GetMapId(){return map;}unsigned GetZoneId(){return zone;}void UseDoorOrButton(unsigned t,bool b,Player*){assert(t==30&&!b);++opens;}};
struct GameObjectScript {GameObjectScript(char const*){}virtual ~GameObjectScript()=default;virtual bool OnGossipHello(Player*,GameObject*){return false;}};
#include "../../src/server/scripts/Legion/valsharah_quest_support.cpp"
struct Motion {unsigned jumps=0;void MoveJump(float x,float y,float z,float o,float xy,float zz){assert(x==2692.97f&&y==1302.77f&&z==128.36f&&o==0&&xy==10&&zz==10);++jumps;}};
std::vector<unsigned> order;
struct AI {void DoAction(int action){assert(action==1);order.push_back(3);}};
struct Creature {Motion motion;struct AI ai;void ExitVehicle(){order.push_back(1);}Motion* GetMotionMaster(){return &motion;}struct AI* AI(){return &ai;}};
struct Map {Creature* creature=nullptr;Creature* GetCreature(int){return creature;}};
struct Instance {Map* instance;unsigned state=0;int GetGuidData(int id){assert(id==100652);return 1;}unsigned GetBossState(unsigned id){assert(id==3);return state;}};
struct Summons {void DespawnAll(){order.push_back(2);}};
struct Unit{};
struct Boss {Instance* instance;Summons summons;void Talk(int){}void _JustDied(){}
#include "ValBossDeath.inc"
};
struct Malfurion {Instance* instance;Creature* me;
#include "ValMalfurion.inc"
};
enum {GO_GLAIDALIS_FIRE_DOOR=1,GO_DRESARON_FIRE_DOOR=2,GO_OAKHEART_DOOR=3,DATA_GLAIDALIS=0,DATA_DRESARON=2,DATA_OAKHEART=1,DOOR_TYPE_ROOM=0,BOUNDARY_NONE=0};
struct DoorData{unsigned entry,bossId,type,boundary;};
#include "ValDoors.inc"
enum {MAX_SPELL_EFFECTS=11,SPELL_EFFECT_QUEST_COMPLETE=16,QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT=2};
struct SpellEffect {unsigned Effect=0,MiscValue=0;};
struct SpellInfo {unsigned Id;SpellEffect data[11];SpellEffect* Effects[11];SpellInfo(unsigned id):Id(id){for(unsigned i=0;i<11;++i)Effects[i]=&data[i];}};
struct SpellManager {std::vector<SpellInfo*> spells;unsigned GetSpellInfoStoreSize(){return unsigned(spells.size());}SpellInfo const* GetSpellInfo(unsigned i){return spells[i];}};
SpellManager* sSpellMgr;
struct Quest {unsigned flags=0;bool HasSpecialFlag(unsigned f)const{return(flags&f)!=0;}void SetSpecialFlag(unsigned f){flags|=f;}};
#define TC_LOG_ERROR(...) ((void)0)
struct Loader {std::map<unsigned,Quest> quests;Quest const* GetQuestTemplate(unsigned q){auto i=quests.find(q);return i==quests.end()?nullptr:&i->second;}void InferFlags(){
#include "ValQuestLoader.inc"
}};
int main(){
 Player p;GameObject go;go_valsharah_bramble_wall wall;
 assert(wall.OnGossipHello(&p,&go)&&p.credit==1&&go.opens==1);wall.OnGossipHello(&p,&go);assert(p.credit==1);
 p.credit=0;p.quest=0;assert(!wall.OnGossipHello(&p,&go)&&p.credit==0);p.quest=1;p.alive=false;assert(!wall.OnGossipHello(&p,&go));p.alive=true;p.distance=6;assert(!wall.OnGossipHello(&p,&go));p.distance=1;go.map=1;assert(!wall.OnGossipHello(&p,&go));
 Creature npc;Map map{&npc};Instance instance{&map};Boss boss{&instance};boss.JustDied(nullptr);assert((order==std::vector<unsigned>{1,2,3}));map.creature=nullptr;order.clear();boss.JustDied(nullptr);assert((order==std::vector<unsigned>{2}));
 Malfurion mf{&instance,&npc};mf.DoAction(0);assert(npc.motion.jumps==0);mf.DoAction(1);assert(npc.motion.jumps==1);
 mf.sGossipSelect(&p,20530,0);assert(p.teleports==0);instance.state=DONE;mf.sGossipSelect(&p,20530,0);assert(p.teleports==1);
 mf.sGossipSelect(&p,99,0);mf.sGossipSelect(&p,20530,1);p.combat=true;mf.sGossipSelect(&p,20530,0);p.combat=false;p.vehicle=true;mf.sGossipSelect(&p,20530,0);p.vehicle=false;p.alive=false;mf.sGossipSelect(&p,20530,0);assert(p.teleports==1);
 static_assert(sizeof(doorData)/sizeof(DoorData)==4,"door sentinel missing");unsigned count=0;for(auto d=doorData;d->entry;++d){assert(++count<=3);}assert(count==3);
 SpellInfo debug(197654),normal(42);debug.data[0]={16,38384};debug.data[1]={16,38147};normal.data[0]={16,99};normal.data[1]={16,1000};
 SpellManager mgr{{nullptr,&debug,&normal}};sSpellMgr=&mgr;Loader loader;loader.quests[38384]={};loader.quests[38147]={};loader.quests[99]={};loader.InferFlags();
 assert(loader.quests[38384].flags==0&&loader.quests[38147].flags==0&&loader.quests[99].flags==2);assert(debug.data[0].Effect==16);loader.InferFlags();assert(loader.quests[99].flags==2);
 std::cout<<"PASS: production wall interaction, Malfurion detach/despawn/escape ordering, exact jump coordinates, exit gossip guards and terminated dungeon door table.\n";
}
