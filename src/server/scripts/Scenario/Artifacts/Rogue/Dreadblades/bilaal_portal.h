#pragma once

namespace BilaalPortal
{
    struct State
    {
        ObjectGuid portal;
        uint32 timer = 0;
        bool arrived = false, done = false, creating = false, warned = false;

        bool Context(Map* map, BilaalDeath::State const& death)
        {
            Scenario* scenario = BilaalDeath::Progress(map, death.route);
            return death.earned && death.done && scenario && scenario->GetCurrentStep() == 5 &&
                BilaalDeath::Victory(scenario, death.route);
        }

        GameObject* Create(Map* map, BilaalDeath::State const& death)
        {
            // Native248553 placement; captured native phase and summoner, but
            // map ownership keeps an earned portal after its emitter retires.
            GameObject* object = new GameObject;
            if (!object->Create(sObjectMgr->GetGenerator<HighGuid::GameObject>()->Generate(), 248553,
                map, death.phaseMask, Position(4202.22f, 7084.17f, 268.199f, 2.81046f),
                G3D::Quat(0, 0, 0, 0), 100, GO_STATE_READY))
            {
                delete object;
                return nullptr;
            }
            object->SetPhaseId(death.phases, false);
            object->SetSummonerGUID(death.receiver);
            object->SetRespawnTime(0);
            object->SetSpawnedByDefault(false);
            if (!map->AddToMap(object))
            {
                object->CleanupsBeforeDelete();
                delete object;
                return nullptr;
            }
            return object;
        }

        void Try(Map* map, BilaalDeath::State const& death)
        {
            if (!arrived || done || creating || !map) return;
            if (!Context(map, death)) { done = true; return; }
            creating = true;
            struct Release { bool& value; ~Release() { value = false; } } release{creating};
            GameObject* object = Create(map, death);
            if (!object)
            {
                if (!warned)
                {
                    warned = true;
                    TC_LOG_ERROR("scripts", "Bilaal: earned Azuregos portal248553 creation failed; map receipt retained");
                }
                return;
            }
            portal = object->GetGUID();
            done = true;
            if (warned)
                TC_LOG_INFO("scripts", "Bilaal: recovered earned Azuregos portal248553 creation");
        }

        void Arrived(Map* map, BilaalDeath::State const& death, ObjectGuid actorGuid)
        {
            if (arrived || done || creating || !map || actorGuid != death.receiver || !Context(map, death)) return;
            Creature* actor = map->GetCreature(actorGuid);
            if (!actor || !actor->IsInWorld() || !actor->IsAlive() || actor->IsPreDelete() ||
                actor->GetEntry() != 104546 || actor->HasQuestObjectScope() || !actor->ToTempSummon() ||
                actor->ToTempSummon()->GetSummonerGUID() != death.source ||
                !actor->InSamePhase(death.phaseMask) || !actor->InSamePhaseId(death.phases, false)) return;
            WPPath* path = sSmartWaypointMgr->GetPath(104546);
            if (!path || path->size() != 1) return;
            auto endpoint = path->find(1);
            if (endpoint == path->end() || !endpoint->second || actor->isMoving() ||
                actor->GetExactDist(endpoint->second->x, endpoint->second->y, endpoint->second->z) > 1.0f) return;
            // Only the actual endpoint callback of this receipt's receiver
            // arms retry. Neither spawning nor a timer invents arrival.
            arrived = true;
            timer = 2000;
            Try(map, death);
        }

        void Update(Map* map, BilaalDeath::State const& death, uint32 diff)
        {
            if (!arrived || done) return;
            if (timer > diff) { timer -= diff; return; }
            timer = 2000;
            Try(map, death);
        }
    };
}
