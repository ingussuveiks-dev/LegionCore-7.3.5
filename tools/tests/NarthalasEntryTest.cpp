#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <map>
#include <list>
#include <set>
#include <string>
#include <tuple>
#include <vector>
using uint16=uint16_t; using uint32=uint32_t; using uint64=uint64_t; using int8=int8_t;
using SpellEffIndex=int; using DamageEffectType=int; using SpellCastResult=int;
enum class GossipOptionNpc {None};
enum { GOSSIP_SENDER_MAIN=1,GOSSIP_ACTION_INFO_DEF=100,DEFAULT_GOSSIP_MESSAGE=0,UNIT_FLAG_IMMUNE_TO_PC=64,PLAYER_FIELD_BYTES_5=5, PLAYER_BYTES_2_OVERRIDE_SPELLS_UINT16_OFFSET=1, QUEST_STATUS_INCOMPLETE=1, QUEST_STATUS_COMPLETE=2, QUEST_STATUS_REWARDED=3,
 TEMPSUMMON_TIMED_DESPAWN=1, REACT_PASSIVE=0, REACT_AGGRESSIVE=1, FOLLOW_MOTION_TYPE=4, POINT_MOTION_TYPE=8,
 UNIT_FIELD_FLAGS=1, UNIT_FLAG_NON_ATTACKABLE=2, SPELL_CAST_OK=0, SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW=1,
 SPELL_FAILED_OUT_OF_RANGE=2, EFFECT_0=0, EFFECT_1=1, SPELL_EFFECT_REMOVE_AURA=164, SPELL_EFFECT_APPLY_AURA=6,
 SPELL_EFFECT_ANY=999, SPELL_EFFECT_DUMMY=3, SPELL_EFFECT_TRIGGER_SPELL=64 };
struct ObjectGuid { unsigned value=0; bool operator<(ObjectGuid b)const{return value<b.value;}bool operator==(ObjectGuid b)const{return value==b.value;} };
using GuidList=std::vector<ObjectGuid>;
struct Position { float x=0,y=0,z=0,o=0;float GetPositionX()const{return x;}float GetPositionY()const{return y;}float GetPositionZ()const{return z;} };
struct Player;struct Creature;struct TempSummon;
struct Unit {
 ObjectGuid guid;Position pos;unsigned map=1220;bool alive=true;
 virtual ~Unit()=default;virtual Player* ToPlayer(){return nullptr;}virtual Creature* ToCreature(){return nullptr;}
 virtual Player* GetCharmerOrOwnerPlayerOrPlayerItself(){return ToPlayer();}
 ObjectGuid GetGUID()const{return guid;}bool IsAlive()const{return alive;}
 float GetDistance(Position p){return std::sqrt((pos.x-p.x)*(pos.x-p.x)+(pos.y-p.y)*(pos.y-p.y)+(pos.z-p.z)*(pos.z-p.z));}
 bool IsWithinDistInMap(Unit* u,float r){return u && map==u->map && GetDistance(u->pos)<=r;}
 Position GetPosition(){return pos;}float GetPositionX(){return pos.x;}float GetPositionY(){return pos.y;}float GetPositionZ(){return pos.z;}
};
struct Motion { unsigned type=0,moves=0,id=0;Position destination;
 void MovePoint(unsigned p,float x,float y,float z,bool=true){type=POINT_MOTION_TYPE;id=p;destination={x,y,z,0};++moves;}
 void MovePoint(unsigned p,Position const& where){MovePoint(p,where.x,where.y,where.z);}
 void MoveFollow(Player*,float,float){type=FOLLOW_MOTION_TYPE;++moves;}unsigned GetCurrentMovementGeneratorType(){return type;}
};
struct CreatureAI { virtual ~CreatureAI()=default;virtual void IsSummonedBy(Unit*){}virtual void Reset(){}virtual void DamageTaken(Unit*,uint32&,DamageEffectType){}virtual void UpdateAI(uint32){}virtual void MovementInform(uint32,uint32){}virtual void PassengerBoarded(Unit*,int8,bool){} };
struct Creature:Unit {
 unsigned entry=0,expiry=0,flags=0,react=0;uint64 spawn=0,health=100,maxHealth=100;float multiplier=1;ObjectGuid owner,viewer;
 bool removed=false;Motion motion;Creature* ToCreature()override{return this;}virtual TempSummon* ToTempSummon(){return nullptr;}
 unsigned GetEntry(){return entry;}uint64 GetDBTableGUIDLow(){return spawn;}
 void DespawnOrUnsummon(unsigned delay=0){if(delay)expiry=delay;else removed=true;}
 void AddPlayerInPersonnalVisibilityList(ObjectGuid g){viewer=g;}void SetReactState(int state){react=state;}void SetDisableGravity(bool){}void SetCanFly(bool){}void CombatStop(bool){}
 void SetFlag(unsigned,unsigned value){flags|=value;}void RemoveFlag(unsigned,unsigned value){flags&=~value;}
 uint64 GetHealth(Unit*){return health;}uint64 GetMaxHealth(Unit*){return maxHealth;}float GetHealthMultiplierForTarget(Unit*){return multiplier;}
 Motion* GetMotionMaster(){return &motion;}
};
struct TempSummon:Creature {TempSummon* ToTempSummon()override{return this;}ObjectGuid GetSummonerGUID(){return owner;}Unit* GetSummoner();};
static std::map<ObjectGuid,TempSummon> creatures;static std::map<ObjectGuid,Player*> players;static unsigned serial=100;
struct Menu { unsigned id=0;unsigned GetMenuId(){return id;} };
struct TalkClass { Menu menu;Menu& GetGossipMenu(){return menu;}void SendCloseGossip(){} };
struct QuestObjective { unsigned ID=284778,ObjectID=107995; };
struct Quest {unsigned id=0;unsigned GetQuestId()const{return id;} std::vector<QuestObjective> objectives{{}};auto const& GetObjectives()const{return objectives;} };
struct QuestStore { Quest quest;Quest const* GetQuestTemplate(unsigned id){assert(id==37862);return &quest;} } questStore;
auto sQuestDataStore=&questStore;
struct Player:Unit {
 Player(){guid={++serial};players[guid]=this;}~Player(){players.erase(guid);}Player* ToPlayer()override{return this;}
 uint16 bar=0;std::set<unsigned> completable;std::set<unsigned> temporarySpells,auras; unsigned zone=7334;bool los=true,failedSpawn=false;Creature* vehicle=nullptr;TalkClass talk;TalkClass* PlayerTalkClass=&talk;
 std::map<unsigned,int> quests;std::map<std::pair<unsigned,unsigned>,unsigned> objectives;std::map<unsigned,unsigned> items;
 std::map<unsigned,GuidList> summons;std::vector<unsigned> credits,casts;
 unsigned GetGUIDLow(){return guid.value;}unsigned GetMapId(){return map;}unsigned GetZoneId(){return zone;}
 bool CanCompleteQuest(unsigned q){return completable.count(q);}void CompleteQuest(unsigned q){quests[q]=QUEST_STATUS_COMPLETE;}int GetQuestStatus(unsigned q){return quests[q];}unsigned GetQuestObjectiveData(unsigned q,unsigned id){return objectives[{q,id}];}
 uint16 GetUInt16Value(unsigned,unsigned){return bar;}void SetUInt16Value(unsigned,unsigned,unsigned value){bar=static_cast<uint16>(value);}void AddTemporarySpell(unsigned id){temporarySpells.insert(id);}void RemoveTemporarySpell(unsigned id){temporarySpells.erase(id);}bool HasSpell(unsigned id){return temporarySpells.count(id);}
 bool HasAura(unsigned id){return auras.count(id);}void RemoveAurasDueToSpell(unsigned id){auras.erase(id);}
 void PrepareQuestMenu(ObjectGuid){}void ADD_GOSSIP_ITEM(GossipOptionNpc,char const*,unsigned,unsigned){}void SEND_GOSSIP_MENU(unsigned,ObjectGuid){}
 bool HasItemCount(unsigned id,unsigned n){return items[id]>=n;}void DestroyItemCount(unsigned id,unsigned n,bool){assert(items[id]>=n);items[id]-=n;}
 bool IsWithinLOSInMap(Unit*){return los;}GuidList* GetSummonList(unsigned id){return &summons[id];}
 Creature* GetVehicle(){return vehicle;}Creature* GetVehicleBase(){return vehicle;}void ExitVehicle(Position const* p){vehicle=nullptr;pos=*p;}
 void KilledMonsterCredit(unsigned id,ObjectGuid={}){credits.push_back(id);unsigned q=0;switch(id){case 89655:case 89656:case 89657:q=37729;break;case 107279:q=42370;break;case 117780:q=44784;break;case 88746:q=37467;break;case 90315:q=37853;break;case 90880:q=42271;break;case 90167:q=37859;break;case 90372:case 90479:q=37957;break;case 90406:q=37857;break;case 100386:q=37860;break;case 91155:case 108721:q=42756;break;default:assert(false);}++objectives[{q,id}];}
 void SetQuestObjectiveData(Quest const*,QuestObjective const* o,unsigned n){objectives[{37862,o->ObjectID}]=n;}
 TempSummon* SummonCreature(unsigned id,Position where,int,unsigned duration,int,ObjectGuid viewer){if(failedSpawn)return nullptr;ObjectGuid next{++serial};auto& c=creatures[next];c.guid=next;c.entry=id;c.pos=where;c.map=map;c.owner=guid;c.viewer=viewer;c.expiry=duration;summons[id].push_back(next);return &c;}
 void CastSpell(float,float,float,unsigned id,bool){casts.push_back(id);}void CastSpell(Unit*,unsigned id,bool){casts.push_back(id);if(id>=179151 && id<=179153)auras.insert(id);}
 Creature* FindNearestCreature(unsigned id,float r){for(auto& pair:creatures)if(!pair.second.removed && pair.second.entry==id && IsWithinDistInMap(&pair.second,r))return &pair.second;return nullptr;}
};
Unit* TempSummon::GetSummoner(){auto it=players.find(owner);return it==players.end()?nullptr:it->second;}
struct ObjectAccessor {static Creature* GetCreature(Player& p,ObjectGuid id){auto it=creatures.find(id);return it==creatures.end()||it->second.removed||it->second.map!=p.map?nullptr:&it->second;}};
struct ScriptedAI:CreatureAI {Creature* me;ScriptedAI(Creature* c):me(c){}void Talk(unsigned){}bool UpdateVictim(){return true;}void DoMeleeAttackIfReady(){} };
struct PlayerScript {PlayerScript(char const*){}virtual ~PlayerScript()=default;virtual void OnLogout(Player*){}virtual void OnMapChanged(Player*){}virtual void OnUpdate(Player*,unsigned){} };
struct CreatureScript {CreatureScript(char const*){}virtual ~CreatureScript()=default;virtual bool OnQuestAccept(Player*,Creature*,Quest const*){return false;}virtual bool OnGossipHello(Player*,Creature*){return false;}virtual bool OnGossipSelect(Player*,Creature*,unsigned,unsigned){return false;}virtual CreatureAI* GetAI(Creature*)const{return nullptr;} };
struct SpellInfo {unsigned Id=0;};struct Hook {unsigned count=0;void operator+=(int){++count;} };
struct SpellScript {Unit* caster=nullptr;Unit* target=nullptr;Creature* hit=nullptr;SpellInfo info;std::set<int> prevented;
 virtual ~SpellScript()=default;virtual void Register(){}Unit* GetCaster(){return caster;}Unit* GetExplTargetUnit(){return nullptr;}Unit* GetOriginalTarget(){return target;}Creature* GetHitCreature(){return hit;}SpellInfo const* GetSpellInfo(){return &info;}
 void PreventHitDefaultEffect(int i){prevented.insert(i);}Hook OnCheckCast,OnEffectHitTarget,OnEffectHit,OnEffectLaunch,OnEffectLaunchTarget;
};
#define PrepareSpellScript(x) public:
#define SpellCheckCastFn(...) 0
#define SpellEffectFn(...) 0
#define RegisterSpellScript(x) ((void)0)
#define RegisterCreatureAI(x) ((void)0)

struct GameObject:Unit {};
struct GameObjectScript {GameObjectScript(char const*){}virtual ~GameObjectScript()=default;virtual bool OnGossipHello(Player*,GameObject*){return false;} };
struct SpellScene {int MiscValue=0;};
struct SceneTriggerScript {SceneTriggerScript(char const*){}virtual ~SceneTriggerScript()=default;virtual bool OnTrigger(Player*,SpellScene const*,std::string){return false;} };
struct AreaTrigger:Unit {
 Player* caster=nullptr;std::list<Creature*> targets;std::set<ObjectGuid> inside;
 Unit* GetCaster(){return caster;}bool IsInArea(Creature* c){return inside.count(c->guid);}
 void GetCreatureListWithEntryInGrid(std::list<Creature*>& list,unsigned id,float){for(auto c:targets)if(c->entry==id)list.push_back(c);}
 void GetCreatureListWithEntryInGridAppend(std::list<Creature*>& list,unsigned id,float r){GetCreatureListWithEntryInGrid(list,id,r);}
};
struct AreaTriggerAI {AreaTrigger* at;AreaTriggerAI(AreaTrigger* a):at(a){}virtual ~AreaTriggerAI()=default;virtual void OnUpdate(unsigned){} };
#define RegisterAreaTriggerAI(x) ((void)0)
#include "../../src/server/scripts/Legion/narthalas_academy.cpp"
Creature* Static(unsigned entry,Position pos){ObjectGuid id{++serial};auto& c=creatures[id];c.guid=id;c.entry=entry;c.pos=pos;return &c;}
int main()
{
 Player p,other;p.pos={-139.941f,6418.82f,27.4824f,0};other.pos=p.pos;
 auto giver=Static(88867,p.pos);
 spell_narthalas_start_walk start;start.caster=&p;
 assert(start.Check()!=SPELL_CAST_OK);
 p.quests[37467]=QUEST_STATUS_INCOMPLETE;assert(start.Check()==SPELL_CAST_OK);
 p.los=false;assert(start.Check()!=SPELL_CAST_OK);p.los=true;
 giver->pos.x+=10;assert(start.Check()!=SPELL_CAST_OK);giver->pos=p.pos;
 auto escort=p.SummonCreature(88889,p.pos,1,600000,0,p.guid);
 npc_narthalas_farondis_walk ai(escort);ai.IsSummonedBy(&p);
 assert(escort->viewer==p.guid && start.Check()!=SPELL_CAST_OK);
 // Another player can start independently while the first guide exists.
 start.caster=&other;other.quests[37467]=QUEST_STATUS_INCOMPLETE;assert(start.Check()==SPELL_CAST_OK);
 ai.UpdateAI(1000);assert(escort->motion.id==1 && p.credits.empty());
 ai.MovementInform(POINT_MOTION_TYPE,2);assert(ai.point==0);
 for(unsigned i=0;i<10;++i){
  escort->pos=escort->motion.destination;
  ai.MovementInform(POINT_MOTION_TYPE,i+1);
  p.pos.x=10000;unsigned before=escort->motion.moves;
  ai.UpdateAI(1000);assert(escort->motion.moves==before && p.credits.empty());
  p.pos=escort->pos;
  if(i==9){p.los=false;ai.UpdateAI(1000);assert(p.credits.empty());p.los=true;}
  ai.UpdateAI(1000);
 }
 assert(p.credits.size()==1 && p.credits[0]==88746 && other.credits.empty());
 ai.UpdateAI(1000);assert(p.credits.size()==1 && escort->motion.moves==10);
 auto retry=other.SummonCreature(88889,other.pos,1,600000,0,other.guid);
 npc_narthalas_farondis_walk retryAI(retry);retryAI.IsSummonedBy(&other);
 other.alive=false;retryAI.UpdateAI(1000);assert(retry->removed && other.credits.empty());
 other.alive=true;other.quests[37467]=0;
 auto abandoned=other.SummonCreature(88889,other.pos,1,600000,0,other.guid);
 npc_narthalas_farondis_walk abandonedAI(abandoned);abandonedAI.IsSummonedBy(&other);assert(abandoned->removed);
 other.quests[37467]=QUEST_STATUS_INCOMPLETE;
 auto expired=other.SummonCreature(88889,other.pos,1,600000,0,other.guid);
 npc_narthalas_farondis_walk expiredAI(expired);expiredAI.IsSummonedBy(&other);expiredAI.UpdateAI(600000);assert(expired->removed);
 auto logout=other.SummonCreature(88889,other.pos,1,600000,0,other.guid);
 npc_narthalas_farondis_walk logoutAI(logout);players.erase(other.guid);logoutAI.UpdateAI(1000);assert(logout->removed);players[other.guid]=&other;
 // The spell's original clicked student survives explicit-target cleanup.
 p.pos={54.717f,6309,-15.1583f,0};auto student=Static(89669,p.pos);
 spell_narthalas_borrow_robes robes;robes.caster=&p;robes.target=student;
 assert(robes.Check()!=SPELL_CAST_OK);p.quests[37736]=QUEST_STATUS_INCOMPLETE;assert(robes.Check()==SPELL_CAST_OK);
 p.items[120948]=1;assert(robes.Check()!=SPELL_CAST_OK);p.items.clear();
 student->entry=89668;assert(robes.Check()!=SPELL_CAST_OK);student->entry=89669;
 student->alive=false;assert(robes.Check()!=SPELL_CAST_OK);student->alive=true;
 p.los=false;assert(robes.Check()!=SPELL_CAST_OK);p.los=true;
 student->pos.x+=7;assert(robes.Check()!=SPELL_CAST_OK);student->pos=p.pos;
 p.map=1;assert(robes.Check()!=SPELL_CAST_OK);p.map=1220;
 spell_narthalas_dressing_complete reward;reward.Register();
 assert(reward.OnEffectLaunch.count==1 && reward.OnEffectLaunchTarget.count==1);
 reward.ObsoleteTrigger(EFFECT_1);assert(reward.prevented.count(EFFECT_1)==1 && reward.prevented.count(EFFECT_0)==0);
 player_narthalas_wand bar;spell_narthalas_wand wand;wand.caster=&p;
 p.pos={201.533f,6470.87f,-52.8034f,0};bar.OnUpdate(&p,1000);assert(p.bar==0);
 p.quests[42370]=QUEST_STATUS_INCOMPLETE;bar.OnUpdate(&p,1000);assert(p.bar==711 && p.HasSpell(212782) && wand.Check()==SPELL_CAST_OK);
 p.bar=999;bar.OnUpdate(&p,1000);assert(p.bar==999);p.quests[42370]=0;bar.OnUpdate(&p,1000);assert(p.bar==999 && !p.HasSpell(212782));
 p.bar=0;p.quests[42370]=QUEST_STATUS_INCOMPLETE;bar.OnUpdate(&p,1000);bar.OnLogout(&p);assert(!p.bar && !p.HasSpell(212782));
 bar.OnUpdate(&p,1000);p.pos.z+=30;bar.OnUpdate(&p,1000);assert(!p.bar && wand.Check()!=SPELL_CAST_OK);p.pos.z-=30;
 bar.OnUpdate(&p,1000);p.alive=false;bar.OnUpdate(&p,1000);assert(!p.bar);p.alive=true;
 bar.OnUpdate(&p,1000);p.map=1;bar.OnMapChanged(&p);assert(!p.bar);p.map=1220;
 p.quests[44784]=QUEST_STATUS_INCOMPLETE;
 auto dummy=Static(117780,p.pos);auto legacy=Static(107279,p.pos);
 AreaTrigger missile;missile.caster=&p;missile.targets={dummy,legacy};at_narthalas_wand projectile(&missile);
 auto previous=p.credits.size();projectile.OnUpdate(100);assert(p.credits.size()==previous);
 missile.inside.insert(dummy->guid);projectile.OnUpdate(100);assert(p.credits.size()==previous+2);
 projectile.OnUpdate(100);assert(p.credits.size()==previous+2); // one hit per target per projectile
 missile.inside.insert(legacy->guid);p.los=false;projectile.OnUpdate(100);assert(p.credits.size()==previous+2);p.los=true;
 projectile.OnUpdate(100);assert(p.credits.size()==previous+4);
 at_narthalas_wand anotherShot(&missile);anotherShot.OnUpdate(100);assert(p.credits.size()==previous+8); // repeat shots intentionally count
 p.quests[42370]=QUEST_STATUS_COMPLETE;p.quests[44784]=QUEST_STATUS_COMPLETE;
 at_narthalas_wand lateShot(&missile);lateShot.OnUpdate(100);assert(p.credits.size()==previous+8);bar.OnUpdate(&p,1000);assert(!p.bar);
 wand.Register();assert(wand.OnEffectLaunch.count==1 && wand.OnEffectLaunchTarget.count==1);
 wand.ObsoleteTrigger(EFFECT_1);assert(!wand.prevented.count(EFFECT_0));
 // Rune credit is an explicit native success event, never scene cancellation.
 p.pos={200.607635f,6451.277832f,-53.777821f,0};p.quests[37729]=QUEST_STATUS_INCOMPLETE;
 Narthalas::StartRune(&p);assert(p.HasAura(179151));
 scene_narthalas_rune runes;SpellScene runeA{935},runeB{936},runeC{937};previous=p.credits.size();
 runes.OnTrigger(&p,&runeA,"complete");assert(p.credits.size()==previous); // Cancel has no credit.
 runes.OnTrigger(&p,&runeB,"Credit");assert(p.credits.size()==previous); // Wrong active scene.
 runes.OnTrigger(&p,&runeA,"Credit");runes.OnTrigger(&p,&runeA,"Credit");assert(p.credits.size()==previous+1);
 runes.OnTrigger(&p,&runeA,"complete");assert(!p.HasAura(179151) && p.HasAura(179152));
 runes.OnTrigger(&p,&runeB,"Credit");runes.OnTrigger(&p,&runeB,"complete");assert(p.HasAura(179153));
 runes.OnTrigger(&p,&runeC,"Credit");runes.OnTrigger(&p,&runeC,"complete");assert(Narthalas::NextRune(&p)==3 && !p.HasAura(179153));
 p.quests[37729]=0;p.auras.insert(179151);bar.OnUpdate(&p,1000);assert(!p.HasAura(179151));
 // Three books: no encounter until the appropriate book is in inventory;
 // no kill credit on click, duplicate click or spawn failure. Saved deaths
 // determine the next stage and allow retrying the same stage after logout.
 GameObject podium;podium.pos=p.pos;go_narthalas_podium book;p.quests[42371]=QUEST_STATUS_INCOMPLETE;
 previous=p.credits.size();book.OnGossipHello(&p,&podium);assert(!Narthalas::OwnedDrawing(&p,107301));
 for(unsigned stage=0;stage<3;++stage){
  p.items[Narthalas::Books[stage]]=1;p.failedSpawn=true;book.OnGossipHello(&p,&podium);assert(!Narthalas::OwnedDrawing(&p,Narthalas::Drawings[stage]));p.failedSpawn=false;
  book.OnGossipHello(&p,&podium);auto drawing=Narthalas::OwnedDrawing(&p,Narthalas::Drawings[stage]);assert(drawing && drawing->viewer==p.guid && p.casts.back()==212912+stage);
  unsigned casts=unsigned(p.casts.size());book.OnGossipHello(&p,&podium);assert(p.casts.size()==casts && p.credits.size()==previous);
  npc_narthalas_drawing battle(drawing);battle.IsSummonedBy(&p);assert(drawing->react==REACT_PASSIVE && (drawing->flags & UNIT_FLAG_IMMUNE_TO_PC));battle.UpdateAI(6000);assert(drawing->react==REACT_PASSIVE);battle.UpdateAI(500);assert(drawing->react==REACT_AGGRESSIVE && !(drawing->flags & UNIT_FLAG_IMMUNE_TO_PC));p.alive=false;battle.UpdateAI(100);assert(drawing->removed);p.alive=true;
  book.OnGossipHello(&p,&podium);drawing=Narthalas::OwnedDrawing(&p,Narthalas::Drawings[stage]);assert(drawing);
  // Boundary supplied by core's actual monster death handler, not the podium.
  drawing->alive=false;p.objectives[{42371,Narthalas::Drawings[stage]}]=1;
 }
 assert(Narthalas::NextBook(&p)==3 && p.credits.size()==previous);
 p.quests[42370]=QUEST_STATUS_INCOMPLETE;p.completable.insert(42370);bar.OnUpdate(&p,1000);assert(p.quests[42370]==QUEST_STATUS_COMPLETE);
 std::cout << "PASS: actual ten-point escort arrival, owner proximity/LOS, independent players, retries/cleanup and clicked-student robe checks; obsolete triggers suppressed; native wand bar lifecycle, other bars preserved and projectile intersection credits only its caster.\n";
}
