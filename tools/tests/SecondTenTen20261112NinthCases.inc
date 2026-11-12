static std::vector<uint32> Drain(EventMap&events){std::vector<uint32>out;events.Update(1200000);while(auto id=events.ExecuteEvent())out.push_back(id);std::sort(out.begin(),out.end());return out;}
template<class AI>static void ResetCase(Creature&me,Unit&victim,uint32 stale,char const*message,size_t roots){AI ai(&me);ai.events.ScheduleEvent(stale,100);ai.Reset();auto after=Drain(ai.events);Require(std::find(after.begin(),after.end(),stale)==after.end(),message);ai.Reset();ai.EnterCombat(&victim);auto first=Drain(ai.events);Require(first.size()==roots,"Authored fresh combat roots remain present exactly once");ai.events.ScheduleEvent(stale,100);ai.Reset();ai.EnterCombat(&victim);auto second=Drain(ai.events);Require(first==second,"Reset and combat reentry reproduce one fresh authored queue");}
static uint32 Count(Creature const&me,uint32 spell){return uint32(std::count(me.casts.begin(),me.casts.end(),spell));}
static void RunNinthCase(int test){Creature me;Unit victim;me.victim=&victim;switch(test){
case 0:ResetCase<NinthClock0>(me,victim,EVENT_2,"Gogduh reset must discard its previous owned combat events",2);break;
case 1:ResetCase<NinthClock1>(me,victim,EVENT_14,"Magmolatus reset must discard the previous combat phase callback",4);break;
case 2:ResetCase<NinthClock2>(me,victim,EVENT_2,"Roltall reset must discard its previous boulder continuation",1);break;
case 3:ResetCase<NinthClock3>(me,victim,EVENT_FLIGHT_SEQUENCE,"Felmyst reset must discard its previous flight continuation",6);break;
case 4:ResetCase<NinthClock4>(me,victim,EVENT_HARSH_WIND,"Fiery Vortex reset must discard an old tornado callback",1);break;
case 5:ResetCase<NinthClock5>(me,victim,EVENT_CONTINUE_METEOR,"Living Meteor reset must discard its old thaw continuation",1);break;
case 6:ResetCase<NinthClock6>(me,victim,EVENT_EXPLODE,"Flarecore reset must discard its previous expiry deadline",2);break;
case 7:me.entry=NPC_SETTHIK_WINDBLADE;ResetCase<NinthClock7>(me,victim,EVENT_TOXIC_SLIME,"Royal sentinel reset must discard its previous attack queue",1);me.entry=NPC_KORTHIK_REAVER;ResetCase<NinthClock7>(me,victim,EVENT_TOXIC_SLIME,"Royal sentinel reset must discard its previous attack queue",2);break;
case 8:{NinthClock8 ai(&me);ai.Reset();for(int i=0;i<8;++i)ai.DoAction(ACTION_INTRO);for(int i=0;i<50;++i)ai.UpdateAI(1000);Require(Count(me,SPELL_HOZEN_KNIFE_FIGHT)>=3,"Unga Knife Fight must repeat under its own event");Require(Count(me,SPELL_RUSTY_SHANK)==8,"Knife Fight cannot add extra Rusty Shank schedules");break;}
case 9:{NinthClock9 ai(&me);ai.events.ScheduleEvent(EVENT_SHADOWBLAZE,0);for(int i=0;i<60;++i){ai.events.Update(1000);while(auto event=ai.events.ExecuteEvent())ai.Dispatch(event);}Require(Count(me,SPELL_SHADOWBLAZE_SPARK)>=3,"Nefarian Shadowblaze must repeat under its own event");Require(!Count(me,SPELL_TAIL_LASH),"Shadowblaze cannot manufacture a Tail Lash timer");break;}
default:throw std::runtime_error("unknown ninth case");}}
