#include <algorithm>
#define MARKSMANSHIP_INTERROGATION_TEST
#include "ConversationFarewellTest.cpp"

int main(int argc, char** argv)
{
    _set_abort_behavior(0, _CALL_REPORTFAULT);
    assert(argc == 3);
    std::map<uint32, Store> definitions;
    std::ifstream input(argv[1]);
    std::string text;
    while (std::getline(input, text))
    {
        std::istringstream row(text);
        std::string kind; row >> kind;
        if (kind == "creature")
        {
            ConversationCreature actor{};
            assert(row >> actor.entry >> actor.id >> actor.creatureId >> actor.creatureGuid >> actor.unk1 >> actor.unk2 >> actor.duration);
            definitions[actor.entry].creatures.push_back(actor);
        }
        else if (kind == "actor")
        {
            ConversationActor actor{};
            assert(row >> actor.entry >> actor.id >> actor.actorId >> actor.creatureId >> actor.displayId >> actor.unk1 >> actor.unk2 >> actor.unk3 >> actor.duration);
            definitions[actor.entry].actors.push_back(actor);
        }
        else
        {
            assert(kind == "line");
            ConversationData line{};
            assert(row >> line.entry >> line.id >> line.idx >> line.textId >> line.unk1 >> line.unk2);
            definitions[line.entry].lines.push_back(line);
        }
    }
    std::ifstream voices(argv[2]);
    std::map<uint32, std::pair<uint32, uint32>> durations;
    uint32 lineId, voice, gap;
    while (voices >> lineId >> voice >> gap) assert(durations.emplace(lineId, std::make_pair(voice, gap)).second);
    for (auto& context : definitions) std::sort(context.second.lines.begin(), context.second.lines.end(), [](ConversationData const& a, ConversationData const& b) { return a.idx < b.idx; });
    assert(definitions.size()==1&&durations.size()==2);
    store=definitions.at(1890);store.identity=1890;
    assert(store.actors.size()==2&&store.creatures.empty()&&store.lines.size()==2);
    uint32 cases=0;
    for(bool npc:{false,true}){
        Unit caster;caster.player=!npc;caster.guid=npc?99:42;caster.nearby.clear();
        Conversation story;story.entry=1890;story.casterGUID=caster.guid;story.playing.clear();currentTime=100;
        assert(story.Start(&caster)&&story.actors.size()==2&&story.creatures.empty());
        for(auto const& actor:store.actors){auto const& field=story.actors.at(actor.id);assert(field.actorId==actor.actorId&&field.creatureId==actor.creatureId&&field.displayId==actor.displayId&&field.unk1==actor.unk1&&field.unk2==actor.unk2&&field.unk3==actor.unk3);}
        uint32 end=0,shortened=0;
        for(auto const& line:store.lines){auto const& field=story.lines.at(line.idx);assert(field.id==line.id&&field.textId==line.textId&&field.unk1==line.unk1&&field.unk2==line.unk2);shortened+=field.textId!=end;auto lineAudio=durations.at(line.id);end+=lineAudio.first+lineAudio.second;}
        assert(end==15157);
        Unit viewer;viewer.guid=42;assert(!story.CanNeverSee2(&viewer));
#ifdef ZIRUS_VOICE_BASELINE
        assert(shortened==1&&story.deadline==10200&&story.lifetime==10200);
        currentTime=100+10201;story.Update(1);assert(story.removed&&story.CanNeverSee2(&viewer));
        assert(10201<8600+durations.at(4018).first); // Last original voice is still unfinished.
#else
        assert(!shortened&&story.deadline==20157&&story.lifetime==20157);
        for(auto const& line:store.lines){auto d=durations.at(line.id);for(auto age:{line.textId,line.textId+d.first,line.textId+d.first+d.second}){currentTime=100+age;story.Update(1);assert(!story.removed&&!story.CanNeverSee2(&viewer));}}
        currentTime=100+20157;story.Update(1);assert(!story.removed);++currentTime;story.Update(1);assert(story.removed&&story.CanNeverSee2(&viewer));
#endif
        ++cases;
    }
    auto saved=store;store.lines.clear();Conversation refused;refused.entry=1890;Unit caster;assert(!refused.Start(&caster));store=saved;
    Conversation cancelled;cancelled.entry=1890;currentTime=100;assert(cancelled.Start(&caster));cancelled.questExpired=true;cancelled.Update(1);assert(cancelled.removed);
#ifdef ZIRUS_VOICE_BASELINE
    std::cout<<"COUNTERFACTUAL actual1890 producer/player+NPC clocks:10200 removes while final original voice still plays,8600 start shortens native6000 pause; virtual actors/fields unchanged. Controlled clocks, not client playback\n";
#else
    std::cout<<"PASS actual1890 producer/ASAN virtual actor+line payload, native6000/1000 gaps/content15157/padding5000/published20157, both player/NPC clock boundaries, absent actors remain unnecessary, missing data refusal and scope cancellation. No client playback claim\n";
#endif
}
