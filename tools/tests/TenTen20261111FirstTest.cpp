#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <list>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
using uint8=uint8_t;using uint32=uint32_t;using uint64=uint64_t;
enum Labels{LANG_PLAYER_NOT_FOUND=1,STATUS_IN_PROGRESS=2,BG_STARTING_EVENT_FOURTH=3,SPELL_AURA_PERIODIC_DUMMY=4,EFFECT_0=0,SPELL_INDOMITABLE_DMG=90045};
static uint32 m_messageTimer[]={0,0,0,0};
struct ObjectGuid{uint64 value=0;static ObjectGuid Empty;bool operator==(ObjectGuid const&g)const{return value==g.value;}std::string ToString()const{return std::to_string(value);}};ObjectGuid ObjectGuid::Empty;
struct Position{float x=1,y=2,z=3;};
struct Player;using Unit=Player;using UnitSet=std::set<Unit*>;
struct Record{uint32 messages=0,language=0,doors=0,status=0,delay=0,spectators=0,casts=0,actions=0,creations=0,wins=0;bool error=false;uint32 winner=0;Player*target=nullptr;}rec;
struct Battleground{bool skirmish=true,arena=false,rated=false,rbg=false,bg=false;void StartingEventOpenDoors(){++rec.doors;}void SetStatus(uint32 s){rec.status=s;}void SetStartDelayTime(uint32 d){rec.delay=d;}void EndBattleground(uint32 team){++rec.wins;rec.winner=team;}uint32 GetOtherTeam(uint32 team){return team==1?2:1;}void SetSkirmish(bool v){skirmish=v;}void SetArena(bool v){arena=v;}void SetRated(bool v){rated=v;}void SetRBG(bool v){rbg=v;}void SetBG(bool v){bg=v;}};
struct Group{ObjectGuid leader{1};ObjectGuid GetLeaderGUID(){return leader;}};
struct Reference{Player*source;Player*getSource()const{return source;}};
struct Map{struct PlayerList{std::list<Reference>rows;using const_iterator=std::list<Reference>::const_iterator;bool isEmpty()const{return rows.empty();}const_iterator begin()const{return rows.begin();}const_iterator end()const{return rows.end();}}players;PlayerList const&GetPlayers(){return players;}};
struct AreaTrigger;std::vector<AreaTrigger*>spawned;static bool allowCreate=true;
struct Player{Battleground*bg=nullptr;Map*map=nullptr;Group*group=nullptr;ObjectGuid guid{1};UnitSet attackers;bool free=true;uint32 pulse=0;std::list<AreaTrigger*>objects;Battleground*GetBattleground(){return bg;}Map*GetMap(){return map;}Group*GetGroup(){return group;}uint32 GetInstanceId(){return 7;}ObjectGuid GetGUID(){return guid;}uint32 GetBGTeam(){return 1;}Position GetPosition(){return {};}void GetAreaObjectList(std::list<AreaTrigger*>&out,uint32 spell){if(spell==123)out=objects;}UnitSet*getAttackers(){return &attackers;}bool CanFreeMove(){return free;}void CastSpell(Player*target,uint32 spell,bool){if(spell!=90045)throw std::runtime_error("Changed native Indomitable child");++pulse;++rec.casts;rec.target=target;}};
struct Session{Player*player=nullptr;Player*GetPlayer(){return player;}};
struct ChatHandler{Session*session=nullptr;Player*selected=nullptr;bool forceMissingSelection=false;Session*GetSession(){return session;}void SendSysMessage(uint32 label){++rec.messages;rec.language=label;}void SetSentErrorMessage(bool v){rec.error=v;}template<class...T>void PSendSysMessage(char const*,T...){++rec.messages;}Unit*getSelectedUnit(){if(forceMissingSelection)return nullptr;return selected?selected:session?session->GetPlayer():nullptr;}};
struct BattlegroundManager{void AddSpectatorData(uint32,ObjectGuid){++rec.spectators;}}manager;auto sBattlegroundMgr=&manager;
enum class HighGuid{AreaTrigger};struct Generator{uint64 Generate(){return 123;}}generator;
struct Manager{template<HighGuid>Generator*GetGenerator(){return &generator;}}objects;auto sObjectMgr=&objects;
struct AreaTrigger{void CastAction(){++rec.actions;}bool CreateAreaTrigger(uint64,uint32,Unit*,void*,Position,Position,void*,ObjectGuid,uint32){++rec.creations;if(allowCreate)spawned.push_back(this);return allowCreate;}};
struct AuraEffect{};struct Hook{template<class T>void operator+=(T){}};
struct AuraScript{Player*caster=nullptr,*target=nullptr;Hook OnEffectPeriodic;Player*GetCaster(){return caster;}Player*GetTarget(){return target;}};
#define PrepareAuraScript(...) public:
#define AuraEffectPeriodicFn(method,...) (&method)
#include "Selected.inc"
static void Require(bool ok,char const*why){if(!ok)throw std::runtime_error(why);}
static void RunAura(){Player a,b;a.free=false;b.free=true;spell_sinestra_indomitable_AuraScript first,second;first.caster=first.target=&a;second.caster=second.target=&b;
for(uint32 i=0;i<3;++i)first.HandleScript(nullptr);second.HandleScript(nullptr);first.HandleScript(nullptr);Require(!a.pulse,"Four ticks cannot consume five-tick control threshold");first.HandleScript(nullptr);Require(a.pulse==1&&!b.pulse,"Another freely moving aura must not reset this caster's timer");
for(uint32 i=0;i<300;++i)first.HandleScript(nullptr);Require(a.pulse==301,"Sustained control remains saturated instead of wrappinguint8");a.free=true;first.HandleScript(nullptr);a.free=false;for(uint32 i=0;i<4;++i)first.HandleScript(nullptr);Require(a.pulse==301,"Freed caster restarts its own five-tick window");first.HandleScript(nullptr);Require(a.pulse==302,"Recontrolled caster reaches authored threshold again");
second.caster=nullptr;second.HandleScript(nullptr);Require(!b.pulse,"Missing caster keeps original null admission");spell_sinestra_indomitable_AuraScript fresh;fresh.caster=fresh.target=&a;fresh.HandleScript(nullptr);Require(a.pulse==302,"Reapplication/new script starts at zero");std::cout<<"PASS complete production Indomitable handler:independent aura instances,interleaved free/controlled casters,5ticks,reapplication,missing caster and300sustained ticks;controlled cast endpoint\n";}
static void RunConsole(bool(*function)(ChatHandler*,char const*),uint32 which){ChatHandler handler;char args[]="123";rec={};Require(!function(&handler,args),"Console without session must reject instead of dereferencing null");Require(rec.error&&rec.language==LANG_PLAYER_NOT_FOUND&&rec.messages==1&&!rec.casts&&!rec.actions&&!rec.creations&&!rec.wins,"Explicit context diagnostic precedes all side effects");Session session;handler.session=&session;rec={};Require(!function(&handler,args)&&rec.error&&rec.language==LANG_PLAYER_NOT_FOUND,"Session without attached player must reject safely");
Player player,other;session.player=&player;Battleground bg;Map map;Group group;player.bg=&bg;player.map=&map;player.group=&group;map.players.rows.push_back({&player});map.players.rows.push_back({&other});player.attackers.insert(&other);handler.selected=&other;other.attackers.insert(&player);rec={};Require(function(&handler,args),"Existing in-game operation must still work");Require(!rec.error,"Valid context cannot receive a new admission error");
if(which==1)Require(rec.doors==1&&rec.status==STATUS_IN_PROGRESS&&rec.spectators==1,"BG start keeps doors,status and native leader spectator enrollment");
if(which==2){Require(rec.wins==1&&rec.winner==2,"BG finish nonempty argument keeps original opposite-team result");rec={};Require(function(&handler,"")&&rec.winner==1,"BG finish empty argument keeps original own-team result");}
if(which==3)Require(!bg.skirmish&&bg.arena&&bg.rated,"Arena command retains three original flag changes");if(which==4)Require(bg.skirmish,"Skirmish flag retained");if(which==5)Require(bg.rbg,"Rated BG flag retained");if(which==6)Require(bg.bg,"BG flag retained");
if(which<=6){player.bg=nullptr;rec={};Require(function(&handler,args)&&rec.messages==1&&!rec.error,"Original no-BG diagnostic remains separate from console admission");player.bg=&bg;}
if(which==7){AreaTrigger first,second;player.objects={&first,&second};rec={};Require(function(&handler,args)&&rec.actions==2,"CastAction retains each actual owned area object's action");rec={};Require(!function(&handler,"")&&!rec.actions,"Missing spell argument does not dispatch area actions");}
if(which==8){Require(rec.creations==1&&spawned.size()==1,"AreaTrigger creation retains native owned context");for(auto object:spawned)delete object;spawned.clear();allowCreate=false;rec={};Require(function(&handler,args)&&rec.creations==1&&rec.messages==1&&spawned.empty(),"Failed creation preserves original visible diagnostic and cleanup");allowCreate=true;}
if(which==9){Require(rec.messages==1,"Selected-unit attacker enumeration retained");handler.forceMissingSelection=true;rec={};Require(function(&handler,args)&&rec.messages==1,"Missing selected unit falls back to actual player's attackers");}
std::cout<<"PASS complete production console handler "<<which<<":no session,no player,valid context,original arguments/side effects and failure diagnostics;external endpoints controlled\n";}
int main(int argc,char**argv){try{if(argc!=2)throw std::runtime_error("Case required");uint32 which=uint32(std::stoul(argv[1]));switch(which){case 0:RunAura();break;CONSOLE_DISPATCH default:throw std::runtime_error("Unknown case");}return 0;}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
