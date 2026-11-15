#include <atomic>
#include <mutex>
#include <map>
#include <vector>
#include <memory>
#include <iostream>
#include <stdexcept>
#include "FunctionProcessorActual.inc"
using ObjectGuid=uint64;
constexpr uint8 MAX_KILL_CREDIT=2;
struct CreatureTemplate{uint32 Entry=121,KillCredit[2]={500,0};};
struct Player;
struct Creature{ObjectGuid guid=99;uint32 entry=121,rewards=0;CreatureTemplate data;ObjectGuid GetGUID(){return guid;}uint32 GetEntry(){return entry;}CreatureTemplate const*GetCreatureTemplate(){return &data;}Creature*ToCreature(){return this;}void AddRewardPlayer(ObjectGuid){++rewards;}};
struct Player{FunctionProcessor queue;bool alive=true,corpse=false;std::vector<uint32> credits;std::vector<ObjectGuid> guids;ObjectGuid GetGUID(){return 1;}bool IsAlive(){return alive;}bool GetCorpse(){return corpse;}void AddDelayedEvent(uint64 ms,std::function<void()>&&f){queue.AddDelayedEvent(ms,std::move(f));}void KilledMonster(CreatureTemplate const*,ObjectGuid);void KilledMonsterCredit(uint32 n,ObjectGuid guid=0){credits.push_back(n);guids.push_back(guid);}};
struct Catalog{CreatureTemplate data;bool present=true;CreatureTemplate const*GetCreatureTemplate(uint32 entry){return present&&entry==121?&data:nullptr;}}catalog;
auto sObjectMgr=&catalog;
namespace ObjectAccessor{static Creature*present;Creature*GetCreature(Player&,ObjectGuid guid){return present&&present->guid==guid?present:nullptr;}}
struct KillRewarder{Creature*_victim;bool _group=false;void _RewardKillCredit(Player*);};
#include "KilledMonster.inc"
#include "Reward.inc"
static void Require(bool ok,char const*message){if(!ok)throw std::runtime_error(message);}
int main(){try{
 for(uint32 mode=0;mode<7;++mode){Player player;auto victim=std::make_unique<Creature>();ObjectAccessor::present=victim.get();KillRewarder reward{victim.get(),mode>=3&&mode<6};if(mode>=3)player.alive=false;if(mode==4||mode==6)player.corpse=true;reward._RewardKillCredit(&player);
  if(mode==0){victim.reset();ObjectAccessor::present=nullptr;}
  if(mode==2)player.queue.KillAllFunctions();
  player.queue.Update(49);Require(player.credits.empty(),"credit deadline shortened");player.queue.Update(1);
  bool eligible=mode!=2&&mode!=4;Require(player.credits.size()==(eligible?2u:0u),"confirmed kill credit lost when corpse disappeared");if(eligible){Require(player.credits==std::vector<uint32>({121,500})&&player.guids==std::vector<ObjectGuid>({99,0}),"native template/aliases/GUIDs changed");if(victim)Require(victim->rewards==1,"live reward-player mark lost");}
  auto before=player.credits;player.queue.Update(60000);Require(player.credits==before,"queued kill delivered twice");ObjectAccessor::present=nullptr;
 }
 {Player player;Creature victim;KillRewarder reward{&victim};ObjectAccessor::present=nullptr;catalog.present=false;reward._RewardKillCredit(&player);player.queue.Update(50);Require(player.credits.empty(),"missing native template fabricated credit");catalog.present=true;}
 std::cout<<"PASS complete delayed kill credit;actual queue/native KilledMonster alias dispatcher,corpse loss/50ms/live mark/cancel/group dead-corpse/once-only/catalog missing;controlled endpoints\n";return 0;
 }catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
