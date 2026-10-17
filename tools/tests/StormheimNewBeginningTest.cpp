#include <array>
#include <sstream>
constexpr unsigned MINUTE=60,IN_MILLISECONDS=1000,UNIT_STATE_CONTROLLED=7,MOVEMENTFLAG_MASK_MOVING_OR_TURN=255;
constexpr float INTERACTION_DISTANCE=5;
#define TC_LOG_INFO(...) (void)0
struct WorldLocation {unsigned map=0;float x=0,y=0,z=0,o=0;WorldLocation()=default;WorldLocation(unsigned m,float a,float b,float c,float d):map(m),x(a),y(b),z(c),o(d){}};
namespace WorldPackets {namespace Misc {struct CompleteMovie{};struct TriggerMovie {unsigned MovieID=0;int Write(){return 0;}};}}
#include "AscendantFixture.inc"
#include "NewBeginningProduction.inc"
bool Player::TeleportTo(uint32 mapid,float x,float y,float z,float orientation,uint32 options){++transferRequests;
#include "NativeNearDelay.inc"
 EndMovieMovement();++transfers;return true;
}
enum {SPELL_EFFECT_HANDLE_HIT_TARGET=1};
struct MovieStore {bool available=true;void* LookupEntry(uint32 id){return id==494&&available?this:nullptr;}} movieStore;auto& sMovieStore=movieStore;
struct NativeMovieSpell {int effectHandleMode=1;Unit* unitTarget=nullptr;uint32 m_diffMode=0;
 struct Info {struct Effect {uint32 MiscValue=494;} effect;Effect* GetEffect(int,uint32){return &effect;}} info;Info* GetSpellInfo(){return &info;}
 void EffectPlayMovie(SpellEffIndex);
};
#include "NativeMovieEffect.inc"
int Player::CastSpell(Player* target,uint32 id,bool triggered){assert(target==this&&id==190073);lastCast=id;lastTriggered=triggered;if(castResult)return castResult;NativeMovieSpell spell;spell.unitTarget=target;spell.EffectPlayMovie(0);if(isWatchingMovie())++moviePackets;return 0;}
struct WorldSession {Player* player;Player* GetPlayer(){return player;}void HandleCompleteMovie(WorldPackets::Misc::CompleteMovie&);};
#include "NativeMovieComplete.inc"
enum {SMART_ACTION_INVOKER_CAST=85,SMARTCAST_INTERRUPT_PREVIOUS=2,SMARTCAST_TRIGGERED=1,SMARTCAST_AURA_NOT_PRESENT=4};
using ObjectList=std::list<WorldObject*>;bool IsUnit(WorldObject* o){return o->ToUnit()!=nullptr;}
namespace ObjectAccessor {Unit* GetUnit(WorldObject&,ObjectGuid id){auto i=players.find(id);return i==players.end()?nullptr:i->second;}}
struct NativeDeferred {Player* p;struct Event {struct {struct {uint32 flags=0,spell=190073;} cast;} action;} e;
 Unit* GetLastInvoker(){return p;}ObjectList* GetTargets(Event&,Unit*){return new ObjectList{p};}
 void Run(){Unit* unit=p;switch(85){
#include "NativeDeferredCast.inc"
 }}
};
void ReadData(char const* path){std::ifstream input(path);assert(input);std::string line;while(std::getline(input,line)){std::istringstream in(line);std::string kind;in>>kind;
 if(kind=="quest"){uint32 id,ex,start,package;in>>id;auto& q=store.quests[id];q.id=id;in>>q.flags>>ex>>q.AllowableRaces>>start>>q.NextQuestIdChain>>q.PrevQuestID>>q.NextQuestID>>q.ExclusiveGroup>>q.special>>package;}
 if(kind=="objective"){uint32 qid,id;QuestObjective o;int index;in>>qid>>id>>o.Type>>o.ObjectID>>o.Amount>>index>>o.Flags;o.StorageIndex=int8(index);store.quests[qid].objectives.push_back(o);}
 if(kind=="starter"){uint32 entry,id;in>>entry>>id;store.starters.emplace(entry,id);}
 }store.quests[39122].special|=QUEST_SPECIAL_FLAGS_CAST|QUEST_SPECIAL_FLAGS_KILL;}
int main(int argc,char**argv){
 _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);_CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE);_CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);assert(argc==2);ReadData(argv[1]);auto& q=store.quests[39122];assert(q.objectives.size()==2);
 Creature genn;genn.entry=95804;genn.dbGuid=338653;npc_stormheim_new_beginning_genn ai(&genn);
 {Player p,near;p.Accept(39122);near.Accept(39122);players[p.guid]=&p;GroupReference b{&near,nullptr},a{&p,&b};Group g;g.first=&a;p.group=&g;
  NativeDeferred old{&p};old.Run();assert(p.events.size()==1&&!p.isWatchingMovie());p.RewardPlayerAndGroupAtEvent(95269,&p);p.RewardPlayerAndGroupAtEvent(94431,&p);p.TeleportTo(1220,2851.9f,863.39f,77.8f,.3f);assert(p.transfers==1&&p.GetQuestStatus(39122)==QUEST_STATUS_COMPLETE&&near.GetQuestStatus(39122)==QUEST_STATUS_COMPLETE);auto events=std::move(p.events);for(auto& event:events)event();assert(p.isWatchingMovie()&&p.transfers==1);players.clear();
 } // Original action85 actually queues200ms; linked credits/transfer precede PLAY_MOVIE.
 for(unsigned completion=0;completion<3;++completion){Player p,near;p.Accept(39122);near.Accept(39122);GroupReference b{&near,nullptr},a{&p,&b};Group g;g.first=&a;p.group=&g;ai.sQuestAccept(&p,&q);
  assert(p.lastCast==190073&&p.lastTriggered&&p.isWatchingMovie()&&p.delay&&p.pending&&p._movieMovementHeld&&!p.clientControl&&!p.transfers&&p.transferRequests==1&&p.CanRewardQuest(&q,false)&&!near.packets);
  assert(p.m_teleport_dest.map==1220&&std::abs(p.m_teleport_dest.x-2851.9f)<.01&&std::abs(p.m_teleport_dest.y-863.39f)<.01&&std::abs(p.m_teleport_dest.z-77.8f)<.01);
  auto packets=p.packets;ai.sGossipHello(&p);ai.sQuestAccept(&p,&q);assert(p.moviePackets==1&&p.packets==packets&&p.transferRequests==1);p.SetCanDelayTeleport(false);p.TickTransfer();assert(!p.transfers); // Player::Update clears the temporary delay flag; watching movie still holds the queued transfer.
  if(completion<2){WorldSession session{&p};WorldPackets::Misc::CompleteMovie packet;session.HandleCompleteMovie(packet);session.HandleCompleteMovie(packet);}else p.UpdateMovieMovement(600000);
  assert(!p.isWatchingMovie()&&!p.delay&&!p._movieMovementHeld&&p.clientControl);p.TickTransfer();p.TickTransfer();assert(p.transfers==1&&!p.pending&&p.packets==packets);
 }
 for(unsigned reason=0;reason<16;++reason){Player p;p.Accept(39122);switch(reason){case 0:p.world=false;break;case 1:p.alive=false;break;case 2:p.team=HORDE;break;case 3:p.map=1;break;case 4:p.zone=1;break;case 5:p.phase=false;break;case 6:genn.distance=6;break;case 7:genn.dbGuid=1;break;case 8:genn.world=false;break;case 9:p.m_watching_movie=true;break;case 10:p.teleport=true;break;case 11:p.vehicle=true;break;case 12:p.transport=true;break;case 13:p.flight=true;break;case 14:p.quests.clear();break;case 15:p.m_RewardedQuests.insert(39122);p.quests.clear();break;}
  ai.sQuestAccept(&p,&q);assert(!p.lastCast&&!p.transferRequests&&!p.packets);genn.distance=0;genn.dbGuid=338653;genn.world=true;
 }
 {Player p;p.Accept(39122);p.castResult=1;ai.sQuestAccept(&p,&q);assert(!p.pending&&!p.packets&&!p.transfers);p.castResult=0;ai.sGossipHello(&p);assert(p.isWatchingMovie()&&p.pending&&p.CanRewardQuest(&q,false));}
 {Player p;p.Accept(39122);movieStore.available=false;ai.sQuestAccept(&p,&q);assert(!p.pending&&!p.packets);movieStore.available=true;ai.sGossipHello(&p);assert(p.pending&&p.isWatchingMovie());}
 for(unsigned credit=0;credit<3;++credit){Player p;p.Accept(39122);if(credit>0)p.KilledMonsterCredit(95269);if(credit>1)p.KilledMonsterCredit(94431);auto before=p.data;ai.sGossipHello(&p);assert(p.pending&&p.isWatchingMovie()&&p.GetQuestObjectiveData(39122,95269)==1&&p.GetQuestObjectiveData(39122,94431)==1);}
 {Player p;p.Accept(39122);ai.sQuestAccept(&p,&q);auto saved=p.data;Player login;login.Accept(39122);login.data=saved;login.quests[39122].Status=QUEST_STATUS_COMPLETE;ai.sGossipHello(&login);assert(login.pending&&login.isWatchingMovie());} // Loaded completed state at source can recover a lost volatile transfer.
 {Player p;p.Accept(39122);ai.sQuestAccept(&p,&store.quests[39092]);assert(!p.lastCast);ai.sQuestAccept(&p,&q);auto packet=p.packets;p.quests.clear();p.data.clear();WorldSession session{&p};WorldPackets::Misc::CompleteMovie done;session.HandleCompleteMovie(done);p.TickTransfer();assert(p.transfers==1&&p.packets==packet);} // Once movie starts, transfer completes independently of later abandonment; no delayed quest credit leaks into a new run.
 std::cout<<"PASS: original native SmartAI85 race/group completion reproduced; actual quest AI + PLAY_MOVIE/start/complete/timeout + near-delay/update branches verify movie-before-credit/transfer, personal goals, skip/repeat/failure/retry/legacy-login boundaries. Cast dispatch, world services and saved login are fixtures; client movie/pet/camera/arrival remain pending.\n";
}
