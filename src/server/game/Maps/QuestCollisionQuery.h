#ifndef TRINITY_QUEST_COLLISION_QUERY_H
#define TRINITY_QUEST_COLLISION_QUERY_H

#include "DynamicTree.h"
#include "GameObject.h"

// Keep the caller's supporting-object receipt and optional filter while adding
// the requesting actor. No global/thread-local viewer or dynamic-tree recursion.
class QuestCollisionQuery
{
public:
    QuestCollisionQuery(WorldObject const* requester, DynamicTreeCallback* output) : _requester(requester), _output(output),
        _callerFilter(output ? output->filter : std::function<bool(GameObject const*)>())
    {
        // Capture only this stack object. Capturing a whole std::function would
        // allocate on every ordinary height/LOS query, even without a filter.
        Query.filter = [this](GameObject const* object)
        {
            return (!object || object->CanCollideWith(_requester)) && (!_callerFilter || _callerFilter(object));
        };
    }

    ~QuestCollisionQuery()
    {
        if (_output)
            _output->go = Query.go;
    }

    QuestCollisionQuery(QuestCollisionQuery const&) = delete;
    QuestCollisionQuery& operator=(QuestCollisionQuery const&) = delete;

    DynamicTreeCallback Query;

private:
    WorldObject const* _requester;
    DynamicTreeCallback* _output;
    std::function<bool(GameObject const*)> _callerFilter;
};

#endif
