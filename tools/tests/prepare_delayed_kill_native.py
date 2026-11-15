"""Extracted full helper with real Player::KilledMonster/quest storage and queues."""
from pathlib import Path
import sys,runpy
from prepare_delayed_kill_credit import verify
from prepare_core_delayed_owners import ROOT
baseline=len(sys.argv)>4 and sys.argv[4]=='baseline';old,new=verify();out=Path(sys.argv[2]);runpy.run_path(str(ROOT/'tools/tests/prepare_despawn_native.py'),run_name='__main__')
s=(out/'LayeredNative.cpp').read_text();s=s[:s.index('int main(')]
inc=out/'NativeFollowingGoLoot.inc';text=inc.read_text();assert text.count('30698,47828,48094')==2
inc.write_text(text.replace('30698,47828,48094','14,30698,47828,48094'))
s=s.replace('HashMapHolder<Player>::SetSize(1000)','HashMapHolder<Player>::SetSize(20000)')
s=s.replace('q->LoadQuestObjective(f);','q->LoadQuestObjective(f);if(f[2].GetUInt8()==QUEST_OBJECTIVE_MONSTER)q->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL);')
s+='\nstruct NativeRewarder{Unit*_victim;bool _group=false;void _RewardKillCredit(Player*);};\n'
s+=(old if baseline else new).replace('KillRewarder::','NativeRewarder::')+'\n'
s+=r'''
struct PlayerQueueTag{using Type=FunctionProcessor Unit::*;friend Type Access(PlayerQueueTag);};template struct Accessor<PlayerQueueTag,&Unit::m_Functions>;
int main(int argc,char**argv){SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);SetUnhandledExceptionFilter(CrashTrace);try{Init(argc,argv);sObjectMgr->LoadCreatureModelInfo();sObjectMgr->LoadWDBCreatureTemplates();sObjectMgr->LoadCreatureTemplates();sObjectMgr->LoadCreatureClassLevelStats();sObjectMgr->LoadCreatureScalingData();sObjectMgr->LoadCreatureDifficultyStat();HashMapHolder<Creature>::SetSize(20000);
 auto quest=sQuestDataStore->GetQuestTemplate(14);Require(quest&&quest->HasSpecialFlag(QUEST_SPECIAL_FLAGS_KILL),"Actual quest14 native kill flags missing");QuestObjective const*objective=nullptr;for(auto const&o:quest->GetObjectives())if(o.ID==251611)objective=&o;Require(objective&&objective->Type==QUEST_OBJECTIVE_MONSTER&&objective->ObjectID==121&&objective->Amount==5&&objective->StorageIndex==1,"Actual captured quest objective drift");
 for(uint32 mode=0;mode<4;++mode){auto session=new WorldSession(0,"Readonly death credit native probe",{},SEC_PLAYER,6,0,"Win",LOCALE_enUS,0,false,AT_AUTH_FLAG_NONE,{});
  Map map(1220,300,9999770+mode,DIFFICULTY_NONE);NativePlayer player(session,map,18000+mode,{5478.f,470.f,150.f,0});player.m_mailsLoaded=true;player.SetQuestSlot(0,14,0);player.SetQuestStatus(14,mode==3?QUEST_STATUS_COMPLETE:QUEST_STATUS_INCOMPLETE);player.SetQuestObjectiveData(quest,objective,0,true);auto victim=std::make_unique<Actor>(&map,121,19600+mode,Position{5479.f,470.f,150.f,0});Require(map.AddToMap(static_cast<Creature*>(victim.get())),"Native victim admission");Require(!victim->GetDBTableGUIDLow(),"Nonpersistent native victim required");ObjectGuid guid=victim->GetGUID();Actor owned(&map,299,19700+mode,Position{5477.f,470.f,150.f,0});Require(map.AddToMap(static_cast<Creature*>(&owned)),"Owned actor admission");owned.SetOwnerGUID(player.GetGUID());owned.SetCreatorGUID(player.GetGUID());auto recipient=mode%2?&player:owned.GetCharmerOrOwnerPlayerOrPlayerItself();Require(recipient==&player,"Actual owned-creature player resolution differs");NativeRewarder reward{victim.get()};reward._RewardKillCredit(recipient);
  if(mode==0||mode==3){victim.reset();Require(!ObjectAccessor::GetCreature(player,guid),"Actual removed corpse still resolves");}if(mode==2)player.KillAllDelayedEvents();auto&queue=player.*Access(PlayerQueueTag{});queue.Update(49);Require(player.GetQuestObjectiveData(quest,objective->StorageIndex)==0,"Native kill credit deadline changed");queue.Update(1);Require(player.GetQuestObjectiveData(quest,objective->StorageIndex)==(mode<2?1:0),"Native quest kill credit lost after corpse removal/cancel/complete admission");queue.Update(60000);Require(player.GetQuestObjectiveData(quest,objective->StorageIndex)==(mode<2?1:0),"Native death credit repeated");player.SetQuestSlot(0,0,0);player.SetQuestStatus(14,QUEST_STATUS_NONE);std::cout<<"CASE native credit="<<mode<<" PASS\n";
 }
 LoginDatabase.Close();HotfixDatabase.Close();WorldDatabase.Close();std::cout<<"PASS4 native extracted full death-credit helper,actual Player::KilledMonster/template/quest14 objective251611/native Unit queue;corpse removed/present,cancelled,complete guards,no Character writes;full group/pet/client pipeline separate\n";return 0;}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
'''
if baseline:s=s.replace('(mode<2?1:0)','(mode==1?1:0)').replace('PASS4 native extracted full death-credit helper','PASS4 native expected-broken original full death-credit helper')
(out/'LayeredNative.cpp').write_text(s)
