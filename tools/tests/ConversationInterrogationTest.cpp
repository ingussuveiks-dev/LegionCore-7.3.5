#define MARKSMANSHIP_INTERROGATION_TEST
#include "ConversationFarewellTest.cpp"
int main(int argc,char** argv){
    _set_abort_behavior(0,_CALL_REPORTFAULT);assert(argc==2);std::map<uint32,Store> definitions;
    std::ifstream input(argv[1]);std::string text;
    while(std::getline(input,text)){std::istringstream row(text);std::string kind;row>>kind;
        if(kind=="actor"){ConversationActor value{};assert(row>>value.entry>>value.id>>value.actorId>>value.creatureId>>value.displayId>>value.unk1>>value.unk2>>value.unk3>>value.duration);auto& definition=definitions[value.entry];definition.identity=value.entry;definition.actors.push_back(value);}
        else if(kind=="line"){ConversationData value{};assert(row>>value.entry>>value.id>>value.idx>>value.textId>>value.unk1>>value.unk2);auto& definition=definitions[value.entry];definition.identity=value.entry;definition.lines.push_back(value);}
        else assert(false);
    }
    assert(definitions.size()==2);Unit finder;finder.nearby.clear();
    for(uint32 identity:{1033u,1556u}){
        store=definitions.at(identity);bool capture=identity==1033;
        assert(store.actors.size()==(capture?2u:1u)&&store.lines.size()==(capture?3u:1u));
        uint32 repairedDeadline=capture?41000:15000;uint32 baselineDeadline=capture?23000:8450;
        uint32 nativeVoiceEnd=capture?35552:8505;uint32 paddedRequired=capture?40552:14505;
        auto repairedActors=store.actors;store.actors[0].duration=capture?9000:3450;
        {Conversation baseline;baseline.entry=identity;assert(baseline.Start(&finder)&&baseline.deadline==baselineDeadline);
            currentTime=100+nativeVoiceEnd;baseline.Update(1);assert(baseline.removed);
        }
        store.actors=repairedActors;
        {Conversation repaired;repaired.entry=identity;assert(repaired.Start(&finder)&&repaired.deadline==repairedDeadline&&repaired.lifetime==repairedDeadline);
            assert(repaired.creatures.empty()&&repaired.actors.size()==store.actors.size()&&repaired.lines.size()==store.lines.size());
            for(auto const& actor:store.actors){auto const& payload=repaired.actors.at(actor.id);assert(payload.actorId==actor.actorId&&payload.creatureId==actor.creatureId&&payload.displayId==actor.displayId&&payload.unk1==actor.unk1&&payload.unk2==actor.unk2&&payload.unk3==actor.unk3);}
            for(auto const& line:store.lines){auto const& payload=repaired.lines.at(line.idx);assert(payload.id==line.id&&payload.textId==line.textId&&payload.unk1==line.unk1&&payload.unk2==line.unk2);}
            for(uint32 checkpoint:{baselineDeadline+1,nativeVoiceEnd,paddedRequired,repairedDeadline}){currentTime=100+checkpoint;repaired.Update(1);assert(!repaired.removed);}
            currentTime=100+repairedDeadline+1;repaired.Update(1);assert(repaired.removed);
        }
        {Conversation scope;scope.entry=identity;assert(scope.Start(&finder));scope.questExpired=true;currentTime=101;scope.Update(1);assert(scope.removed);}
        {currentTime=999999;Conversation replay;replay.entry=identity;replay.playing.clear();assert(replay.Start(&finder)&&replay.deadline==repairedDeadline);
            assert(!replay.CanNeverSee2(&finder)&&replay.playing.at(finder.GetGUID())==currentTime);
            currentTime+=repairedDeadline;replay.Update(1);assert(!replay.removed);++currentTime;replay.Update(1);assert(replay.removed);
        }
        {Conversation unrelated;unrelated.entry=999999;assert(!unrelated.Start(&finder));}
        store.actors.clear();{Conversation missing;missing.entry=identity;assert(!missing.Start(&finder));}
        store=definitions.at(identity);store.lines.clear();{Conversation missing;missing.entry=identity;assert(!missing.Start(&finder));}
        store=definitions.at(identity);for(auto& actor:store.actors)actor.duration=0;
        {Conversation fallback;fallback.entry=identity;assert(fallback.Start(&finder)&&fallback.deadline==35000);}
    }
    std::cout<<"PASS: native actor payload/lifetime/playback expiry reproduce1033/1556 early expiry at audited voice/gap checkpoints;41000/15000 retain all original actors/lines until normal expiry. Scope cleanup, replay, missing definitions and35s zero-duration fallback retained; no client scheduling/animation claim.\n";
}
